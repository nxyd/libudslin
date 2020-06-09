#ifndef LINDRV_H_
#define LINDRV_H_

#include <windows.h>

/* LIN driver error code */
#define LINDRV_ERROR_OK                      0x0000 /* No error */
#define LINDRV_ERROR_NO_MESSAGE              0x0001 /* No message */
#define LINDRV_ERROR_NULL_PTR                0x0002 /* Null pointer */

/* LIN message direction */
#define LINDRV_MSG_DIR_PUBLISHER             0x01
#define LINDRV_MSG_DIR_SUBSCRIBER            0x02

/* LIN Schedule table type */
#define LINDRV_SCHED_TYPE_STOP               0x01
#define LINDRV_SCHED_TYPE_REQUEST            0x02
#define LINDRV_SCHED_TYPE_RESPONSE           0x03

typedef DWORD    LinDrvResult;
typedef BYTE     LinDrvMsgDir;
typedef BYTE     LinDrvSchedType;

typedef struct _LinMsg {
    BYTE data[8];
    BYTE length;
    LinDrvMsgDir dir;
} LinMsg;

LinDrvResult LinDrv_Init(void);
LinDrvResult LinDrv_Uninit(void);
LinDrvResult LinDrv_Write(LinMsg *msg);
LinDrvResult LinDrv_Read(LinMsg *msg);
LinDrvResult LinDrv_SetSchedule(LinDrvSchedType type);

typedef LinDrvResult (*PF_LinDrv_Init)(void);
typedef LinDrvResult (*PF_LinDrv_Uninit)(void);
typedef LinDrvResult (*PF_LinDrv_Write)(LinMsg *);
typedef LinDrvResult (*PF_LinDrv_Read)(LinMsg *);
typedef LinDrvResult (*PF_LinDrv_SetSchedule)(LinDrvSchedType);

typedef struct _LinDrvOps {
    PF_LinDrv_Init init;
    PF_LinDrv_Uninit uninit;
    PF_LinDrv_Write write;
    PF_LinDrv_Read read;
    PF_LinDrv_SetSchedule set_schedule;
} LinDrvOps;

void LinDrv_SetInterface(LinDrvOps *ops);

#endif /* LINDRV_H_ */
