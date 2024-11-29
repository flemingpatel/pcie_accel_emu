/* model.c
 *
 * Author: Fleming Patel
 *
 */

#include "model.h"
#include "pciemu.h"


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
    model_engine->output_addr = 0;
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

/**
 * pciemu_model_control: Runs model related ops based on control command
 *
 * @dev: Instance of PCIEMUDevice object being initialized
 * @model_cmd_t: control command to indicate model ops
 */
void pciemu_model_control(PCIEMUDevice *dev, model_cmd_t cmd)
{
    ModelEngine *model_engine = &dev->model_engine;
    switch (cmd)
    {
        case PCIEMU_HW_MODEL_CMD_LOAD_MODEL:
            pciemu_irq_raise(dev, PCIEMU_HW_IRQ_MODEL_LOADED_VECTOR);
            break;

        case PCIEMU_HW_MODEL_CMD_UNLOAD_MODEL:
            pciemu_irq_raise(dev, PCIEMU_HW_IRQ_MODEL_UNLOADED_VECTOR);
            break;

        case PCIEMU_HW_MODEL_CMD_RUN_INFERENCE:
            pciemu_irq_raise(dev, PCIEMU_HW_IRQ_MODEL_INFERENCE_DONE_VECTOR);
            break;
    }
}
