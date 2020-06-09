#ifndef UDSLIN_API_H_
#define UDSLIN_API_H_

#include <windows.h>

#define LUDS_MAJOR_VER_NUMBER    0
#define LUDS_MINOR_VER_NUMBER    3
#define LUDS_REVISION_NUMBER     1

/* UDS Service ids defined in ISO 14229-1 */
#define LUDS_SI_DiagnosticSessionControl         0x10 /* see ISO 14229-1 */
#define LUDS_SI_ECUReset                         0x11 /* see ISO 14229-1 */         
#define LUDS_SI_SecurityAccess                   0x27 /* see ISO 14229-1 */
#define LUDS_SI_CommunicationControl             0x28 /* see ISO 14229-1 */
#define LUDS_SI_TesterPresent                    0x3E /* see ISO 14229-1 */
#define LUDS_SI_AccessTimingParameter            0x83 /* see ISO 14229-1 */
#define LUDS_SI_SecuredDataTransmission          0x84 /* see ISO 14229-1 */
#define LUDS_SI_ControlDTCSetting                0x85 /* see ISO 14229-1 */
#define LUDS_SI_ResponseOnEvent                  0x86 /* see ISO 14229-1 */
#define LUDS_SI_LinkControl                      0x87 /* see ISO 14229-1 */
#define LUDS_SI_ReadDataByIdentifier             0x22 /* see ISO 14229-1 */
#define LUDS_SI_ReadMemoryByAddress              0x23 /* see ISO 14229-1 */
#define LUDS_SI_ReadScalingDataByIdentifier      0x24 /* see ISO 14229-1 */
#define LUDS_SI_ReadDataByPeriodicIdentifier     0x2A /* see ISO 14229-1 */
#define LUDS_SI_DynamicallyDefineDataIdentifier  0x2C /* see ISO 14229-1 */
#define LUDS_SI_WriteDataByIdentifier            0x2E /* see ISO 14229-1 */
#define LUDS_SI_WriteMemoryByAddress             0x3D /* see ISO 14229-1 */
#define LUDS_SI_ClearDiagnosticInformation       0x14 /* see ISO 14229-1 */
#define LUDS_SI_ReadDTCInformation               0x19 /* see ISO 14229-1 */
#define LUDS_SI_InputOutputControlByIdentifier   0x2F /* see ISO 14229-1 */
#define LUDS_SI_RoutineControl                   0x31 /* see ISO 14229-1 */
#define LUDS_SI_RequestDownload                  0x34 /* see ISO 14229-1 */
#define LUDS_SI_RequestUpload                    0x35 /* see ISO 14229-1 */
#define LUDS_SI_TransferData                     0x36 /* see ISO 14229-1 */
#define LUDS_SI_RequestTransferExit              0x37 /* see ISO 14229-1 */
#define LUDS_NR_SI                               0x7F /* Negative response service identifier */
#define LUDS_NRC_EXTENDED_TIMING                 0x78 /* server wants more time */
#define LUDS_SI_POSITIVE_RESPONSE                0x40 /* positive response offset */

/* UDS message type */
#define LUDS_MSG_TYPE_REQUEST                    0x00 /* UDS Request Message */
#define LUDS_MSG_TYPE_RESPONSE                   0x01 /* UDS Response Message */
#define LUDS_MSG_TYPE_CONFIRM                    0x02 /* UDS Request/Response confirmation Message */
#define LUDS_MSG_TYPE_INDICATION                 0x03 /* Incoming UDS Message */
#define LUDS_MSG_TYPE_INDICATION_TX              0x04 /* UDS Message transmission started */
#define LUDS_MSG_TYPE_CONFIRM_UUDT               0x05 /* Unacknowledge Unsegmented Data Transfert (RAW data only) */

/* UDS error code */
#define LUDS_ERROR_OK                            0x0000 /* No error */
#define LUDS_ERROR_NOT_INITIALIZED               0x0001 /* Not Initialized */
#define LUDS_ERROR_ALREADY_INITIALIZED           0x0002 /* Already Initialized */
#define LUDS_ERROR_NO_MEMORY                     0x0003 /* Could not obtain memory */
#define LUDS_ERROR_OVERFLOW                      0x0004 /* Input buffer overflow */
#define LUDS_ERROR_TIMEOUT                       0x0006 /* Timeout */
#define LUDS_ERROR_NO_MESSAGE                    0x0007 /* No Message available */
#define LUDS_ERROR_WRONG_PARAM                   0x0008 /* Wrong message parameters */

/* Network error code */
#define LUDS_RESULT_N_OK                         0x00 /* No network error */
#define LUDS_RESULT_N_TIMEOUT_A                  0x01 /* timeout occured between 2 frames transmission (sender and receiver side) */
#define LUDS_RESULT_N_TIMEOUT_Bs                 0x02 /* sender side timeout while waiting for flow control frame */
#define LUDS_RESULT_N_TIMEOUT_Cr                 0x03 /* receiver side timeout while waiting for consecutive frame */
#define LUDS_RESULT_N_WRONG_SN                   0x04 /* unexpected sequence number */
#define LUDS_RESULT_N_INVALID_FS                 0x05 /* invalid or unknown FlowStatus */
#define LUDS_RESULT_N_UNEXP_PDU                  0x06 /* unexpected protocol data unit */
#define LUDS_RESULT_N_WFT_OVRN                   0x07 /* reception of flow control WAIT frame that exceeds the maximum counter defined by LUDS_RESULT_PARAM_WFT_MAX */
#define LUDS_RESULT_N_BUFFER_OVFLW               0x08 /* buffer on the receiver side cannot store the data length (server side only) */
#define LUDS_RESULT_N_ERROR                      0x09 /* general error */
#define LUDS_RESULT_N_IGNORED                    0x0A /* message was invalid and ignored */

/* LUDS parameters */
#define LUDS_PARAM_TIMING_REQUEST                0xA0
#define LUDS_PARAM_TIMING_RESPONSE               0xA1
#define LUDS_PARAM_TIMING_P2                     0xA2
#define LUDS_PARAM_TIMING_P2_EXT                 0xA3
#define LUDS_PARAM_TP_PADDING_VALUE              0xF0 /* padding value, default is 0xFF */
#define LUDS_PARAM_TP_TIMING_RESPONSE            0xF1
#define LUDS_PARAM_TP_TIMING_As                  0xF2
#define LUDS_PARAM_TP_TIMING_Cr                  0xF3

#define LUDS_MAX_DATA                            4095

typedef DWORD   LUdsResult;
typedef BYTE    LUdsMsgType;
typedef BYTE    LUdsParam;

typedef struct _LUdsMsg {
    BYTE nad;

    /* Result status of the network communication */
    BYTE n_result;

    /* States wether Positive Response Message should be suppressed */
    BOOL no_positive_response;

    /* Data Length Code of the message */
    WORD length;

    /* Type of UDS Message (request, response, pending) */
    LUdsMsgType msg_type;

    union {
        BYTE raw[LUDS_MAX_DATA];
        
        struct {
            BYTE si;
            BYTE param[LUDS_MAX_DATA-1];
        } request;

        struct {
            BYTE si;
            BYTE param[LUDS_MAX_DATA-1];
        } positive;

        struct {
            BYTE nr_si;
            BYTE si;
            BYTE nrc;
        } negative;
    } data;
} LUdsMsg;

typedef void (*PF_LUDS_MSG_HOOK)(LUdsMsg *);

#ifdef __cplusplus
#define EXTERNC    extern "C"
#else
#define EXTERNC
#endif

EXTERNC LUdsResult __stdcall LUDS_SetMsgHook(PF_LUDS_MSG_HOOK hook);

EXTERNC LUdsResult __stdcall LUDS_Init(void);

EXTERNC LUdsResult __stdcall LUDS_Uninit(void);

EXTERNC LUdsResult __stdcall LUDS_WaitForDiagRequestSent(
    LUdsMsg *req,
    LUdsMsg *rsp,
    DWORD timeout);

EXTERNC LUdsResult __stdcall LUDS_WaitForDiagResponse(
    LUdsMsg *req,
    LUdsMsg *rsp,
    DWORD timeout);

EXTERNC LUdsResult __stdcall LUDS_WaitForService(
    LUdsMsg *req,
    LUdsMsg *rsp);

EXTERNC LUdsResult __stdcall LUDS_DiagnosticSessionControl(
    LUdsMsg *req,
    BYTE session_type);

EXTERNC LUdsResult __stdcall LUDS_EcuReset(
    LUdsMsg *req,
    BYTE reset_type);

EXTERNC LUdsResult __stdcall LUDS_SecurityAccess(
    LUdsMsg *req,
    BYTE level,
    BYTE *buffer,
    WORD buffer_len);

EXTERNC LUdsResult __stdcall LUDS_CommunicationControl(
    LUdsMsg *req,
    BYTE ctrl_type,
    BYTE comm_type);

EXTERNC LUdsResult __stdcall LUDS_TestPresent(
    LUdsMsg *req);

EXTERNC LUdsResult __stdcall LUDS_ReadDataByIdentifier(
    LUdsMsg *req,
    WORD did);

EXTERNC LUdsResult __stdcall LUDS_WriteDataByIdentifier(
    LUdsMsg *req,
    WORD did,
    BYTE *buffer,
    WORD buffer_len);

EXTERNC LUdsResult __stdcall LUDS_ClearDiagnosticInformation(
    LUdsMsg *req,
    DWORD group);

EXTERNC LUdsResult __stdcall LUDS_ReadDTCInformation(
    LUdsMsg *req,
    BYTE report_type,
    BYTE *buffer,
    WORD buffer_len);

EXTERNC LUdsResult __stdcall LUDS_InputOutputControlByIdentifier(
    LUdsMsg *req,
    WORD did,
    BYTE *ctrl_option_record,
    WORD ctrl_option_record_len,
    BYTE *ctrl_enable_mask_record,
    WORD ctrl_enable_mask_record_len);

EXTERNC LUdsResult __stdcall LUDS_RoutineControl(
    LUdsMsg *req,
    BYTE ctrl_type,
    WORD id,
    BYTE *buffer,
    WORD buffer_len);

EXTERNC LUdsResult __stdcall LUDS_RequestDownload(
    LUdsMsg *req,
    BYTE *mem_address,
    BYTE mem_address_len,
    BYTE *mem_size,
    BYTE mem_size_len);

EXTERNC LUdsResult __stdcall LUDS_TransferData(
    LUdsMsg *req,
    BYTE block_seq_counter,
    BYTE *buffer,
    WORD buffer_len);

EXTERNC LUdsResult __stdcall LUDS_RequestTransferExit(
    LUdsMsg *req,
    BYTE *buffer,
    WORD buffer_len);

EXTERNC LUdsResult __stdcall LUDS_SetParam(
    LUdsParam param,
    void *data,
    DWORD size);
EXTERNC LUdsResult __stdcall LUDS_GetParam(
    LUdsParam param,
    void *data,
    DWORD size);

/*============================================================================*/
#define LINDRV_ERROR_OK                      0x0000 /* No error */
#define LINDRV_ERROR_NO_MESSAGE              0x0001 /* No message */
#define LINDRV_ERROR_NULL_PTR                0x0002 /* Null ptr */

#define LINDRV_MSG_DIR_PUBLISHER             0x01
#define LINDRV_MSG_DIR_SUBSCRIBER            0x02

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

EXTERNC LUdsResult __stdcall LUDS_SetLinDriver(LinDrvOps *ops);

#endif /* UDSLIN_API_H_ */
