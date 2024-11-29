/* model.h
 *
 * Author: Fleming Patel
 *
 */


#ifndef PCIEMU_MODEL_H
#define PCIEMU_MODEL_H

#include "qemu/osdep.h"
#include "pciemu_hw.h"


/* forward declaration */
typedef struct PCIEMUDevice PCIEMUDevice;

/* model control command */
typedef uint64_t model_cmd_t;

typedef struct ModelEngine
{
    uint64_t model_load_addr;
    uint64_t model_load_size;
    uint64_t input_addr;
    uint64_t output_addr;
    uint32_t batch_size;
} ModelEngine;

void pciemu_model_reset(PCIEMUDevice *dev);

void pciemu_model_init(PCIEMUDevice *dev, Error **errp);

void pciemu_model_fini(PCIEMUDevice *dev);

void pciemu_model_control(PCIEMUDevice *dev, model_cmd_t cmd);

#endif /* PCIEMU_MODEL_H */
