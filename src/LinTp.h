#ifndef LINTP_H_
#define LINTP_H_

#include <windows.h>

/* LINTP error code */
#define LINTP_ERROR_OK                            0x0000 /* No error */
#define LINTP_ERROR_NOT_INITIALIZED               0x0001 /* Not Initialized */
#define LINTP_ERROR_ALREADY_INITIALIZED           0x0002 /* Already Initialized */
#define LINTP_ERROR_NO_MEMORY                     0x0003 /* Could not obtain memory */
#define LINTP_ERROR_OVERFLOW                      0x0004 /* Input buffer overflow */
#define LINTP_ERROR_NO_MESSAGE                    0x0007 /* No Message available */
#define LINTP_ERROR_WRONG_PARAM                   0x0008 /* Wrong message parameters */

/* LINTP message type */
#define LINTP_MSG_TYPE_TYPE_UNKNOWN               0x00 /* Unknown (non-ISO-TP) message */
#define LINTP_MSG_TYPE_DIAGNOSTIC                 0x01 /* Diagnostic Request/Confirmation */
#define LINTP_MSG_TYPE_CONFIRM                    0x02 /* Confirms that a message has been sent successfully/ not successfully */
#define LINTP_MSG_TYPE_INDICATION                 0x03 /* Multi-Frame Message is being received or transmitted */
#define LINTP_MSG_TYPE_INDICATION_TX              0x04 /* Multi-Frame Message is being transmitted */

/* Network error code */
#define LINTP_N_OK                                0x00 /* No network error */
#define LINTP_N_TIMEOUT_A                         0x01 /* timeout occured between 2 frames transmission (sender and receiver side) */
#define LINTP_N_TIMEOUT_Bs                        0x02 /* sender side timeout while waiting for flow control frame */
#define LINTP_N_TIMEOUT_Cr                        0x03 /* receiver side timeout while waiting for consecutive frame */
#define LINTP_N_WRONG_SN                          0x04 /* unexpected sequence number */
#define LINTP_N_INVALID_FS                        0x05 /* invalid or unknown FlowStatus */
#define LINTP_N_UNEXP_PDU                         0x06 /* unexpected protocol data unit */
#define LINTP_N_WFT_OVRN                          0x07 /* reception of flow control WAIT frame that exceeds the maximum counter defined by LINTP_PARAM_WFT_MAX */
#define LINTP_N_BUFFER_OVFLW                      0x08 /* buffer on the receiver side cannot store the data length (server side only) */
#define LINTP_N_ERROR                             0x09 /* general error */
#define LINTP_N_IGNORED                           0x0A /* message was invalid and ignored */

/* LINTP parameters */
#define LINTP_PARAM_PADDING_VALUE                 0x10 /* padding value, default is 0xFF */
#define LINTP_PARAM_TIMING_RESPONSE               0x20
#define LINTP_PARAM_TIMING_As                     0x21
#define LINTP_PARAM_TIMING_Cr                     0x22

#define LINTP_MESSAGE_MAX_LENGTH                  4095

typedef DWORD   LinTpResult;
typedef BYTE    LinTpMsgType;
typedef BYTE    LinTpParam;

typedef struct _LinTpMsg {
    BYTE nad;

    /* LIN message type, see LINTP_MSG_TYPE_XXX */
    LinTpMsgType msg_type;		

    BYTE data[LINTP_MESSAGE_MAX_LENGTH];
    WORD length;

    /* network result, see LINTP_N_XXX */
    BYTE result;
} LinTpMsg;

typedef struct _LinTpTimestamp {
    DWORD millis;         /* Base-value: milliseconds: 0.. 2^32-1 */
    WORD millis_overflow; /* Roll-arounds of millis */
    WORD micros;          /* Microseconds: 0..999 */
} LinTpTimestamp;

LinTpResult LinTp_Init(void);
LinTpResult LinTp_Uninit(void);

LinTpResult LinTp_Write(LinTpMsg *msg);
LinTpResult LinTp_Read(LinTpMsg *msg, LinTpTimestamp *timestamp);

LinTpResult LinTp_KeepWaitForResponse(void);

LinTpResult LinTp_SetParam(LinTpParam param, void *data, DWORD size);
LinTpResult LinTp_GetParam(LinTpParam param, void *data, DWORD size);

#endif /* LINTP_H_ */
