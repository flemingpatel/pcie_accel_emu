/*
 * model.h
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2024 Fleming Patel
 */

#ifndef PCIEMU_MODEL_H
#define PCIEMU_MODEL_H

#include "pciemu_hw.h"
#include "dma.h"

/* model control command */
typedef uint64_t model_cmd_t;

typedef struct ModelEngine {
	uint64_t model_load_addr;
	uint64_t model_load_size;
	uint64_t input_addr;
	uint64_t input_size;
	uint64_t output_addr;
	uint64_t output_size;
	uint32_t batch_size;
} ModelEngine;

void pciemu_model_load_addr(PCIEMUDevice *dev, dma_addr_t addr);

void pciemu_model_load_size(PCIEMUDevice *dev, dma_size_t size);

void pciemu_model_output_addr(PCIEMUDevice *dev, dma_addr_t addr);

void pciemu_model_output_size(PCIEMUDevice *dev, dma_size_t size);

void pciemu_model_control(PCIEMUDevice *dev, model_cmd_t cmd);

void pciemu_model_reset(PCIEMUDevice *dev);

void pciemu_model_init(PCIEMUDevice *dev, Error **errp);

void pciemu_model_fini(PCIEMUDevice *dev);

#endif /* PCIEMU_MODEL_H */
