/*
 * example.c: User-space example that uses libvai and communicates with the device via driver ioctl
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2024 Fleming Patel
 */

#include "lib/libvai.h"
#include <sys/mman.h>

/* is there a way to find dynamically (TODO) */
#define PCIEMU_DEVICE_PATH "/dev/pcie_accel_emu/d0b0d3f0_bar0"

/* Function to map kernel buffer into user-space */
void *map_buffer(int fd, uint64_t handle, size_t size)
{
	/*
	 * 4KB Page Size is a standard that balances memory fragmentation and page table sizes
	 * if is not page-aligned, mmap fails
	 * TODO round up or any other mechanism?
	 */
	void *mapped_addr;
	long page_size = sysconf(_SC_PAGESIZE);
	if (page_size == -1) {
		fprintf(stderr, "%s: sysconf failed: %s\n", __func__, strerror(errno));
		return NULL;
	}

	/* shift handle to get a page-aligned offset */
	unsigned int PAGE_SHIFT = __builtin_ctzl(page_size);
	off_t offset = (off_t)handle << PAGE_SHIFT;

	/* perform mmap on the device file with buffer_handle as offset */
	mapped_addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
	if (mapped_addr == MAP_FAILED) {
		fprintf(stderr, "%s: mmap failed for handle=%ld: %s\n", __func__, handle, strerror(errno));
		return NULL;
	}

	return mapped_addr;
}

/* Function to unmap a buffer from user-space */
int unmap_buffer(void *addr, size_t size)
{
	if (munmap(addr, size) < 0) {
		fprintf(stderr, "%s: munmap failed: %s\n", __func__, strerror(errno));
		return errno;
	}
	return 0;
}

int main()
{
	int fd;
	int ret;
	void *mapped_addr;
	char *read_output = NULL;

	/* open PCIEMU device */
	fd = open(PCIEMU_DEVICE_PATH, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "%s: failed to open PCIEMU device: %s\n", __func__, strerror(errno));
		return EXIT_FAILURE;
	}

	/*
	 * --------------------------------------------------------------------------------
	 * allocate buffer for model via IOCTL
	 * --------------------------------------------------------------------------------
	 */
	size_t model_size = 4 * 1024; /* 4KB */
	uint64_t model_buffer_handle;
	ret = vai_alloc_buffer(fd, model_size, &model_buffer_handle);
	if (ret != 0) {
		fprintf(stderr, "%s: failed to allocate buffer for model: %s\n", __func__, strerror(errno));
		close(fd);
		return EXIT_FAILURE;
	}

	/*
	 * --------------------------------------------------------------------------------
	 * write some data (string) to model buffer (kernel buffer) directly using mmap
	 * --------------------------------------------------------------------------------
	 */
	const char *data = "ECEA 5307: Linux Embedded System Topics and Projects";
	size_t data_size = strlen(data) + 1; // include null terminator

	/* map the buffer into user-space */
	mapped_addr = map_buffer(fd, model_buffer_handle, model_size);
	if (!mapped_addr) {
		goto err_mmap;
	}

	/* write data into the buffer */
	memcpy(mapped_addr, data, data_size);

	/* unmap the buffer */
	if (unmap_buffer(mapped_addr, model_size) != 0) {
		goto err_mmap;
	}

	/*
	 * --------------------------------------------------------------------------------
	 * load model via IOCTL
	 * --------------------------------------------------------------------------------
	 */
	uint32_t model_id;
	ret = vai_load_model(fd, model_buffer_handle, model_size, &model_id);
	if (ret != 0) {
		fprintf(stderr, "%s: failed to load model into device: %s\n", __func__, strerror(errno));
		goto err_load_model;
	}

	/*
	 * --------------------------------------------------------------------------------
	 * run inference
	 * allocate input and output buffers
	 * input buffer: for input data
	 * output buffer: for output of inference
	 * --------------------------------------------------------------------------------
	 */
	size_t input_size = 4 * 1024; /* 4KB */
	uint64_t input_buffer_handle;
	ret = vai_alloc_buffer(fd, input_size, &input_buffer_handle);
	if (ret != 0) {
		fprintf(stderr, "%s: failed to allocate input buffer: %s\n", __func__, strerror(errno));
		goto err_input_buffer;
	}

	size_t output_size = 8 * 1024; /* 8KB */
	uint64_t output_buffer_handle;
	ret = vai_alloc_buffer(fd, output_size, &output_buffer_handle);
	if (ret != 0) {
		fprintf(stderr, "%s: failed to allocate out buffer: %s\n", __func__, strerror(errno));
		goto err_out_buffer;
	}

	/* run inference */
	int batch_size = 2;
	ret = vai_run_inference(fd, model_id, input_buffer_handle, input_size, output_buffer_handle,
				output_size, batch_size);
	if (ret != 0) {
		fprintf(stderr, "%s: failed to run inference: %s\n", __func__, strerror(errno));
		goto err_run_inference;
	}

	/*
	 * --------------------------------------------------------------------------------
	 * read output data from output buffer (kernel buffer) directly with device update
	 * --------------------------------------------------------------------------------
	 */
	/* dynamically allocate read_output to avoid VLA issues */
	read_output = malloc(output_size);
	if (!read_output) {
		fprintf(stderr, "%s: failed to allocate memory for output buffer\n", __func__);
		goto err_run_inference;
	}

	/* map the buffer into user-space */
	mapped_addr = map_buffer(fd, output_buffer_handle, output_size);
	if (!mapped_addr) {
		goto err_read_out;
	}

	/* copy data into the output buffer */
	memcpy(read_output, mapped_addr, output_size);

	/* unmap the buffer */
	if (unmap_buffer(mapped_addr, output_size) != 0) {
		goto err_read_out;
	}

	fprintf(stdout, "%s: inference output: %s\n", __func__, read_output);

	/*
	 * --------------------------------------------------------------------------------
	 * unload model
	 * functionality not available from device other than IRQ interrupt (TODO)
	 * --------------------------------------------------------------------------------
	 */

	/* clean up */
	free(read_output);
	vai_free_buffer(fd, output_buffer_handle);
	vai_free_buffer(fd, input_buffer_handle);
	vai_free_buffer(fd, model_buffer_handle);
	close(fd);

	return EXIT_SUCCESS;

err_read_out:
	free(read_output);
err_run_inference:
	vai_free_buffer(fd, output_buffer_handle);
err_out_buffer:
	vai_free_buffer(fd, input_buffer_handle);
err_input_buffer:
err_load_model:
err_mmap:
	vai_free_buffer(fd, model_buffer_handle);
	close(fd);
	return EXIT_FAILURE;
}
