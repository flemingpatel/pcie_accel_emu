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
	// TODO
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
