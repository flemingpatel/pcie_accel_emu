/*
 * model.c
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2024 Fleming Patel
 */

#include "model.h"
#include "pciemu.h"
#include "qemu/log.h"

/**
 * @brief Stores model host dma address
 *
 * @param dev Instance of PCIEMUDevice object
 * @param addr host dma address where model is stored
 */
void pciemu_model_load_addr(PCIEMUDevice *dev, dma_addr_t addr)
{
	dev->model_engine.model_load_addr = addr;
}

/**
 * @brief Stores the size of model
 *
 * @param dev Instance of PCIEMUDevice object
 * @param size model size
 */
void pciemu_model_load_size(PCIEMUDevice *dev, dma_size_t size)
{
	dev->model_engine.model_load_size = size;
}

/**
 * @brief Stores host output buffer dma address
 *
 * @param dev Instance of PCIEMUDevice object
 * @param addr host dma address where model output will be stored
 */
void pciemu_model_output_addr(PCIEMUDevice *dev, dma_addr_t addr)
{
	dev->model_engine.output_addr = addr;
}

/**
 * @brief Stores the size of output buffer
 *
 * @param dev Instance of PCIEMUDevice object
 * @param size output buffer size
 */
void pciemu_model_output_size(PCIEMUDevice *dev, dma_size_t size)
{
	dev->model_engine.output_size = size;
}

/**
 * @brief Runs inference
 *
 * @param dev Instance of PCIEMUDevice object
 */
static void pciemu_run_inference(PCIEMUDevice *dev)
{
	ModelEngine *model_engine = &dev->model_engine;

	if (!model_engine->model_load_addr || !model_engine->model_load_size) {
		qemu_log_mask(LOG_GUEST_ERROR, "model load registers missing \n");
		return;
	}

	if (!model_engine->output_addr || !model_engine->output_size) {
		qemu_log_mask(LOG_GUEST_ERROR, "model inference output registers missing \n");
		return;
	}

	uint64_t model_load_size = model_engine->model_load_size;
	uint64_t model_load_addr = model_engine->model_load_addr;

	/* strings to append */
	const char *append_str = " Fleming Patel (from custom pci qemu device)!";
	size_t append_str_len = strlen(append_str);

	/* ensure the output buffer can accommodate the original model data + appended string */
	uint64_t model_output_size = model_engine->output_size;
	uint64_t model_output_addr = model_engine->output_addr;

	size_t total_output_size = model_load_size + append_str_len;
	if (model_output_size < total_output_size) {
		qemu_log_mask(LOG_GUEST_ERROR, "output buffer too small for result\n");
		return;
	}

	/* allocate buffer for combined output */
	uint8_t *output_data = g_malloc(total_output_size);

	/* read the model data directly into the host dma output buffer */
	pci_dma_read(&dev->pci_dev, model_load_addr, output_data, model_load_size);

	/* append the new string directly to the output buffer
	 * we use append_offset because model_load_size is buffer size not the bytes written to it
	 */
	size_t append_offset = strnlen((char *)output_data, total_output_size);
	memcpy(output_data + append_offset, append_str, append_str_len);

	/* write the combined output back to the host dma output buffer */
	pci_dma_write(&dev->pci_dev, model_output_addr, output_data, total_output_size);

	/* clean up */
	g_free(output_data);
}

/**
 * pciemu_model_control: Runs model related ops based on control command
 *
 * @param dev Instance of PCIEMUDevice object
 * @param cmd control command to indicate model ops
 */
void pciemu_model_control(PCIEMUDevice *dev, model_cmd_t cmd)
{
	switch (cmd) {
	case PCIEMU_HW_MODEL_CMD_LOAD_MODEL:
		pciemu_irq_raise(dev, PCIEMU_HW_IRQ_MODEL_LOADED_VECTOR);
		break;

	case PCIEMU_HW_MODEL_CMD_UNLOAD_MODEL:
		pciemu_irq_raise(dev, PCIEMU_HW_IRQ_MODEL_UNLOADED_VECTOR);
		break;

	case PCIEMU_HW_MODEL_CMD_RUN_INFERENCE:
		pciemu_run_inference(dev);
		pciemu_irq_raise(dev, PCIEMU_HW_IRQ_MODEL_INFERENCE_DONE_VECTOR);
		break;
	}
}

/**
 * pciemu_model_reset: Reset ModelEngine
 *
 * @dev: Instance of PCIEMUDevice object being initialized
 */
void pciemu_model_reset(PCIEMUDevice *dev)
{
	ModelEngine *model_engine = &dev->model_engine;
	model_engine->model_load_addr = 0;
	model_engine->model_load_size = 0;
	model_engine->input_addr = 0;
	model_engine->input_size = 0;
	model_engine->output_addr = 0;
	model_engine->output_size = 0;
	model_engine->batch_size = 0;
}

/**
 * pciemu_model_init: ModelEngine initialization
 *
 * @dev: Instance of PCIEMUDevice object being initialized
 * @errp: pointer to indicate errors
 */
void pciemu_model_init(PCIEMUDevice *dev, Error **errp)
{
	/* basically reset the ModelEngine */
	pciemu_model_reset(dev);
}

/**
 * pciemu_model_fini: Finalize ModelEngine (destructor)
 *
 * @dev: Instance of PCIEMUDevice object being initialized
 */
void pciemu_model_fini(PCIEMUDevice *dev)
{
	/* basically reset the ModelEngine */
	pciemu_model_reset(dev);
}
