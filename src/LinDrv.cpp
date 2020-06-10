#include "LinDrv.h"

static PF_LinDrv_Init lindrv_init = NULL;
static PF_LinDrv_Uninit lindrv_uninit = NULL;
static PF_LinDrv_Write lindrv_write = NULL;
static PF_LinDrv_Read lindrv_read = NULL;
static PF_LinDrv_SetSchedule lindrv_set_schedule = NULL;

LinDrvResult LinDrv_Init(void)
{
    if (lindrv_init == NULL)
        return LINDRV_ERROR_NULL_PTR;
    return lindrv_init();
}

LinDrvResult LinDrv_Uninit(void)
{
    if (lindrv_uninit == NULL)
        return LINDRV_ERROR_NULL_PTR;
    return lindrv_uninit();
}

LinDrvResult LinDrv_Write(LinMsg *msg)
{
    if (lindrv_write == NULL)
        return LINDRV_ERROR_NULL_PTR;
    return lindrv_write(msg);
}

LinDrvResult LinDrv_Read(LinMsg *msg)
{
    if (lindrv_read == NULL)
        return LINDRV_ERROR_NULL_PTR;
    return lindrv_read(msg);
}

LinDrvResult LinDrv_SetSchedule(LinDrvSchedType type)
{
    if (lindrv_set_schedule == NULL)
        return LINDRV_ERROR_NULL_PTR;
    return lindrv_set_schedule(type);
}

void LinDrv_SetInterface(LinDrvOps *ops)
{
    lindrv_init = ops->init;
    lindrv_uninit = ops->uninit;
    lindrv_write = ops->write;
    lindrv_read = ops->read;
    lindrv_set_schedule = ops->set_schedule;
}
