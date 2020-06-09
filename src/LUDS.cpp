#include "LUDS.h"
#include <string.h>
#include "LinTp.h"

static BOOL init = FALSE;
static DWORD timing_p2 = 500;
static DWORD timing_p2_ext = 5000;
static DWORD timing_request = 1000;
static DWORD timing_response = 0;

static PF_LUDS_MSG_HOOK msg_hook = NULL;

#define LUDS_MSG_DEBUG(msg) \
    do { \
    if (msg_hook != NULL) \
        msg_hook(msg); \
    } while (0)

LUdsResult __stdcall LUDS_SetMsgHook(PF_LUDS_MSG_HOOK hook)
{
    if (hook == NULL)
        return LUDS_ERROR_WRONG_PARAM;
    msg_hook = hook;
    return LUDS_ERROR_OK;
}

LUdsResult __stdcall LUDS_Init(void)
{
    if (init)
        return LUDS_ERROR_ALREADY_INITIALIZED;

    LinTp_Init();
    init = TRUE;
    return LUDS_ERROR_OK;
}

LUdsResult __stdcall LUDS_Uninit(void)
{
    if (!init)
        return LUDS_ERROR_NOT_INITIALIZED;

    LinTp_Uninit();
    init = FALSE;
    return LUDS_ERROR_OK;
}

LUdsResult __stdcall LUDS_WaitForDiagRequestSent(
    LUdsMsg *req,
    LUdsMsg *rsp,
    DWORD timeout)
{
    DWORD start_time = GetTickCount();
    
    while (1)
    {
        LinTpMsg tpmsg;
        LinTpTimestamp timestamp;
        LinTpResult result = LinTp_Read(&tpmsg, &timestamp);
        if (result == LINTP_ERROR_OK)
        {
            if (tpmsg.msg_type == LINTP_MSG_TYPE_INDICATION_TX)
            {
                rsp->nad = tpmsg.nad;
                rsp->msg_type = LUDS_MSG_TYPE_INDICATION_TX;
                rsp->n_result = tpmsg.result;
                rsp->length = tpmsg.length;
                memcpy(rsp->data.raw, tpmsg.data, tpmsg.length);
                LUDS_MSG_DEBUG(rsp);

                if (rsp->n_result != LINTP_N_OK)
                    break;

                start_time = GetTickCount();
            }
            else if (tpmsg.msg_type == LINTP_MSG_TYPE_CONFIRM)
            {
                rsp->nad = tpmsg.nad;
                rsp->msg_type = LUDS_MSG_TYPE_CONFIRM;
                rsp->n_result = tpmsg.result;
                rsp->length = tpmsg.length;
                memcpy(rsp->data.raw, tpmsg.data, tpmsg.length);
                LUDS_MSG_DEBUG(rsp);
                break;
            }
        }

        DWORD cur_time = GetTickCount();
        if (timeout != 0)
        {
            if (cur_time - start_time > timeout)
                return LUDS_ERROR_TIMEOUT;
        }
    }

    return LUDS_ERROR_OK;
}

LUdsResult __stdcall LUDS_WaitForDiagResponse(
    LUdsMsg *req,
    LUdsMsg *rsp,
    DWORD timeout)
{
    DWORD start_time = GetTickCount();
    BOOL responsed = FALSE;
    WORD p2_time = timing_p2;

    while (1)
    {
        LinTpMsg tpmsg;
        LinTpTimestamp timestamp;
        LinTpResult result = LinTp_Read(&tpmsg, &timestamp);
        if (result == LINTP_ERROR_OK)
        {
            if (tpmsg.msg_type == LINTP_MSG_TYPE_INDICATION)
            {
                rsp->nad = tpmsg.nad;
                rsp->msg_type = LUDS_MSG_TYPE_INDICATION;
                rsp->n_result = tpmsg.result;
                rsp->length = tpmsg.length;
                memcpy(rsp->data.raw, tpmsg.data, tpmsg.length);
                LUDS_MSG_DEBUG(rsp);

                if (rsp->n_result != LINTP_N_OK)
                    break;

                responsed = TRUE;
            }
            else if (tpmsg.msg_type == LINTP_MSG_TYPE_DIAGNOSTIC)
            {
                rsp->nad = tpmsg.nad;
                rsp->msg_type = LUDS_MSG_TYPE_RESPONSE;
                rsp->n_result = tpmsg.result;
                rsp->length = tpmsg.length;
                memcpy(rsp->data.raw, tpmsg.data, tpmsg.length);
                LUDS_MSG_DEBUG(rsp);

                if ((rsp->data.negative.nr_si == 0x7F) &&
                    (rsp->data.negative.nrc == 0x78))
                {
                    LinTp_KeepWaitForResponse();
                    start_time = GetTickCount();
                    p2_time = timing_p2_ext;
                }
                else
                {
                    break;
                }
            }
        }

        DWORD cur_time = GetTickCount();
        if (timeout != 0)
        {
            if (cur_time - start_time > timeout)
                return LUDS_ERROR_TIMEOUT;
        }

        if (!responsed)
        {
            if (cur_time - start_time > p2_time)
                return LUDS_ERROR_TIMEOUT;
        }
    }

    return LUDS_ERROR_OK;
}

LUdsResult __stdcall LUDS_WaitForService(
    LUdsMsg *req,
    LUdsMsg *rsp)
{
    if (req == NULL)
        return LUDS_ERROR_WRONG_PARAM;
    if (rsp == NULL)
        return LUDS_ERROR_WRONG_PARAM;

    LUdsResult result = LUDS_WaitForDiagRequestSent(req, rsp, timing_request);
    if (req->no_positive_response || (result != LUDS_ERROR_OK))
        return result;

    result = LUDS_WaitForDiagResponse(req, rsp, timing_response);
    if (result != LUDS_ERROR_OK)
        return result;

    return result;
}

LUdsResult __stdcall LUDS_DiagnosticSessionControl(
    LUdsMsg *req,
    BYTE session_type)
{
    if (req == NULL)
        return LUDS_ERROR_WRONG_PARAM;

    req->msg_type = LUDS_MSG_TYPE_REQUEST;
    req->data.request.si = LUDS_SI_DiagnosticSessionControl;
    req->data.request.param[0] = session_type;
    if (req->no_positive_response)
        req->data.request.param[0] |= 0x80;
    req->length = 2;

    LinTpMsg lintp_req;
    lintp_req.nad = req->nad;
    memcpy(lintp_req.data, req->data.raw, req->length);
    lintp_req.length = req->length;
    LinTp_Write(&lintp_req);

    return LUDS_ERROR_OK;
}

LUdsResult __stdcall LUDS_EcuReset(
    LUdsMsg *req,
    BYTE reset_type)
{
    if (req == NULL)
        return LUDS_ERROR_WRONG_PARAM;

    req->msg_type = LUDS_MSG_TYPE_REQUEST;
    req->data.request.si = LUDS_SI_ECUReset;
    req->data.request.param[0] = reset_type;
    if (req->no_positive_response)
        req->data.request.param[0] |= 0x80;
    req->length = 2;

    LinTpMsg lintp_req;
    lintp_req.nad = req->nad;
    memcpy(lintp_req.data, req->data.raw, req->length);
    lintp_req.length = req->length;
    LinTp_Write(&lintp_req);

    return LUDS_ERROR_OK;
}

LUdsResult __stdcall LUDS_SecurityAccess(
    LUdsMsg *req,
    BYTE level,
    BYTE *buffer,
    WORD buffer_len)
{
    if (req == NULL)
        return LUDS_ERROR_WRONG_PARAM;

    req->msg_type = LUDS_MSG_TYPE_REQUEST;
    req->no_positive_response = FALSE;
    req->data.request.si = LUDS_SI_SecurityAccess;
    req->data.request.param[0] = level;
    for (WORD i = 0; i < buffer_len; i++)
        req->data.request.param[i+1] = buffer[i];
    req->length = 2+buffer_len;

    LinTpMsg lintp_req;
    lintp_req.nad = req->nad;
    memcpy(lintp_req.data, req->data.raw, req->length);
    lintp_req.length = req->length;
    LinTp_Write(&lintp_req);

    return LUDS_ERROR_OK;
}

LUdsResult __stdcall LUDS_CommunicationControl(
    LUdsMsg *req,
    BYTE ctrl_type,
    BYTE comm_type)
{
    if (req == NULL)
        return LUDS_ERROR_WRONG_PARAM;

    req->msg_type = LUDS_MSG_TYPE_REQUEST;
    req->data.request.si = LUDS_SI_CommunicationControl;
    req->data.request.param[0] = ctrl_type;
    if (req->no_positive_response)
        req->data.request.param[0] |= 0x80;
    req->data.request.param[1] = comm_type;
    req->length = 3;

    LinTpMsg lintp_req;
    lintp_req.nad = req->nad;
    memcpy(lintp_req.data, req->data.raw, req->length);
    lintp_req.length = req->length;
    LinTp_Write(&lintp_req);

    return LUDS_ERROR_OK;
}

LUdsResult __stdcall LUDS_TestPresent(
    LUdsMsg *req)
{
    if (req == NULL)
        return LUDS_ERROR_WRONG_PARAM;

    req->msg_type = LUDS_MSG_TYPE_REQUEST;
    req->data.request.si = LUDS_SI_TesterPresent;
    req->data.request.param[0] = 0x00;
    if (req->no_positive_response)
        req->data.request.param[0] |= 0x80;
    req->length = 2;

    LinTpMsg lintp_req;
    lintp_req.nad = req->nad;
    memcpy(lintp_req.data, req->data.raw, req->length);
    lintp_req.length = req->length;
    LinTp_Write(&lintp_req);

    return LUDS_ERROR_OK;
}

LUdsResult __stdcall LUDS_ReadDataByIdentifier(
    LUdsMsg *req,
    WORD did)
{
    if (req == NULL)
        return LUDS_ERROR_WRONG_PARAM;

    req->msg_type = LUDS_MSG_TYPE_REQUEST;
    req->no_positive_response = FALSE;
    req->data.request.si = LUDS_SI_ReadDataByIdentifier;
    req->data.request.param[0] = (BYTE)((did >> 8) & 0xFF);
    req->data.request.param[1] = (BYTE)(did & 0xFF);
    req->length = 3;

    LinTpMsg lintp_req;
    lintp_req.nad = req->nad;
    memcpy(lintp_req.data, req->data.raw, req->length);
    lintp_req.length = req->length;
    LinTp_Write(&lintp_req);

    return LUDS_ERROR_OK;
}

LUdsResult __stdcall LUDS_WriteDataByIdentifier(
    LUdsMsg *req,
    WORD did,
    BYTE *buffer,
    WORD buffer_len)
{
    if (req == NULL)
        return LUDS_ERROR_WRONG_PARAM;

    req->msg_type = LUDS_MSG_TYPE_REQUEST;
    req->no_positive_response = FALSE;
    req->data.request.si = LUDS_SI_WriteDataByIdentifier;
    req->data.request.param[0] = (BYTE)((did >> 8) & 0xFF);
    req->data.request.param[1] = (BYTE)(did & 0xFF);
    for (WORD i = 0; i < buffer_len; i++)
        req->data.request.param[i+2] = buffer[i];
    req->length = 3+buffer_len;

    LinTpMsg lintp_req;
    lintp_req.nad = req->nad;
    memcpy(lintp_req.data, req->data.raw, req->length);
    lintp_req.length = req->length;
    LinTp_Write(&lintp_req);

    return LUDS_ERROR_OK;
}

LUdsResult __stdcall LUDS_ClearDiagnosticInformation(
    LUdsMsg *req,
    DWORD group)
{
    if (req == NULL)
        return LUDS_ERROR_WRONG_PARAM;

    req->msg_type = LUDS_MSG_TYPE_REQUEST;
    req->no_positive_response = FALSE;
    req->data.request.si = LUDS_SI_ClearDiagnosticInformation;
    req->data.request.param[0] = (BYTE)((group >> 16) & 0xFF);
    req->data.request.param[1] = (BYTE)((group >> 8) & 0xFF);
    req->data.request.param[2] = (BYTE)(group & 0xFF);
    req->length = 4;

    LinTpMsg lintp_req;
    lintp_req.nad = req->nad;
    memcpy(lintp_req.data, req->data.raw, req->length);
    lintp_req.length = req->length;
    LinTp_Write(&lintp_req);

    return LUDS_ERROR_OK;
}

LUdsResult __stdcall LUDS_ReadDTCInformation(
    LUdsMsg *req,
    BYTE report_type,
    BYTE *buffer,
    WORD buffer_len)
{
    if (req == NULL)
        return LUDS_ERROR_WRONG_PARAM;

    req->msg_type = LUDS_MSG_TYPE_REQUEST;
    req->no_positive_response = FALSE;
    req->data.request.si = LUDS_SI_ReadDTCInformation;
    req->data.request.param[0] = report_type;
    for (WORD i = 0; i < buffer_len; i++)
        req->data.request.param[i+1] = buffer[i];
    req->length = 2+buffer_len;

    LinTpMsg lintp_req;
    lintp_req.nad = req->nad;
    memcpy(lintp_req.data, req->data.raw, req->length);
    lintp_req.length = req->length;
    LinTp_Write(&lintp_req);

    return LUDS_ERROR_OK;
}

LUdsResult __stdcall LUDS_InputOutputControlByIdentifier(
    LUdsMsg *req,
    WORD did,
    BYTE *ctrl_option_record,
    WORD ctrl_option_record_len,
    BYTE *ctrl_enable_mask_record,
    WORD ctrl_enable_mask_record_len)
{
    if (req == NULL)
        return LUDS_ERROR_WRONG_PARAM;

    req->msg_type = LUDS_MSG_TYPE_REQUEST;
    req->no_positive_response = FALSE;
    req->data.request.si = LUDS_SI_InputOutputControlByIdentifier;
    req->data.request.param[0] = (BYTE)((did >> 8) & 0xFF);
    req->data.request.param[1] = (BYTE)(did & 0xFF);
    WORD index = 2;
    for (WORD i = 0; i < ctrl_option_record_len; i++)
    {
        req->data.request.param[index] = ctrl_option_record[i];
        index++;
    }
    for (WORD i = 0; i < ctrl_enable_mask_record_len; i++)
    {
        req->data.request.param[index] = ctrl_enable_mask_record[i];
        index++;
    }
    req->length = 3+ctrl_option_record_len+ctrl_enable_mask_record_len;

    LinTpMsg lintp_req;
    lintp_req.nad = req->nad;
    memcpy(lintp_req.data, req->data.raw, req->length);
    lintp_req.length = req->length;
    LinTp_Write(&lintp_req);

    return LUDS_ERROR_OK;
}

LUdsResult __stdcall LUDS_RoutineControl(
    LUdsMsg *req,
    BYTE ctrl_type,
    WORD id,
    BYTE *buffer,
    WORD buffer_len)
{
    if (req == NULL)
        return LUDS_ERROR_WRONG_PARAM;

    req->msg_type = LUDS_MSG_TYPE_REQUEST;
    req->no_positive_response = FALSE;
    req->data.request.si = LUDS_SI_RoutineControl;
    req->data.request.param[0] = ctrl_type;
    req->data.request.param[1] = (BYTE)((id >> 8) & 0xFF);
    req->data.request.param[2] = (BYTE)(id & 0xFF);
    for (WORD i = 0; i < buffer_len; i++)
        req->data.request.param[i+3] = buffer[i];
    req->length = 4+buffer_len;

    LinTpMsg lintp_req;
    lintp_req.nad = req->nad;
    memcpy(lintp_req.data, req->data.raw, req->length);
    lintp_req.length = req->length;
    LinTp_Write(&lintp_req);

    return LUDS_ERROR_OK;
}

LUdsResult __stdcall LUDS_RequestDownload(
    LUdsMsg *req,
    BYTE *mem_address,
    BYTE mem_address_len,
    BYTE *mem_size,
    BYTE mem_size_len)
{
    if (req == NULL)
        return LUDS_ERROR_WRONG_PARAM;

    req->msg_type = LUDS_MSG_TYPE_REQUEST;
    req->no_positive_response = FALSE;
    req->data.request.si = LUDS_SI_RequestDownload;
    req->data.request.param[0] = 0x00;
    req->data.request.param[1] = (BYTE)((mem_address_len << 4) | mem_size_len);
    WORD index = 2;
    for (WORD i = 0; i < mem_address_len; i++)
    {
        req->data.request.param[index] = mem_address[i];
        index++;
    }
    for (WORD i = 0; i < mem_size_len; i++)
    {
        req->data.request.param[index] = mem_size[i];
        index++;
    }
    req->length = 3+ mem_address_len+ mem_size_len;

    LinTpMsg lintp_req;
    lintp_req.nad = req->nad;
    memcpy(lintp_req.data, req->data.raw, req->length);
    lintp_req.length = req->length;
    LinTp_Write(&lintp_req);

    return LUDS_ERROR_OK;
}

LUdsResult __stdcall LUDS_TransferData(
    LUdsMsg *req,
    BYTE block_seq_counter,
    BYTE *buffer,
    WORD buffer_len)
{
    if (req == NULL)
        return LUDS_ERROR_WRONG_PARAM;

    req->msg_type = LUDS_MSG_TYPE_REQUEST;
    req->no_positive_response = FALSE;
    req->data.request.si = LUDS_SI_TransferData;
    req->data.request.param[0] = block_seq_counter;
    for (WORD i = 0; i < buffer_len; i++)
        req->data.request.param[i+1] = buffer[i];
    req->length = 2+buffer_len;

    LinTpMsg lintp_req;
    lintp_req.nad = req->nad;
    memcpy(lintp_req.data, req->data.raw, req->length);
    lintp_req.length = req->length;
    LinTp_Write(&lintp_req);

    return LUDS_ERROR_OK;
}

LUdsResult __stdcall LUDS_RequestTransferExit(
    LUdsMsg *req,
    BYTE *buffer,
    WORD buffer_len)
{
    if (req == NULL)
        return LUDS_ERROR_WRONG_PARAM;

    req->msg_type = LUDS_MSG_TYPE_REQUEST;
    req->no_positive_response = FALSE;
    req->data.request.si = LUDS_SI_RequestTransferExit;
    for (WORD i = 0; i < buffer_len; i++)
        req->data.request.param[i] = buffer[i];
    req->length = 1+buffer_len;

    LinTpMsg lintp_req;
    lintp_req.nad = req->nad;
    memcpy(lintp_req.data, req->data.raw, req->length);
    lintp_req.length = req->length;
    LinTp_Write(&lintp_req);

    return LUDS_ERROR_OK;
}

EXTERNC LUdsResult __stdcall LUDS_SetParam(
    LUdsParam param,
    void *data,
    DWORD size)
{
    switch (param)
    {
    case LUDS_PARAM_TIMING_REQUEST:
        if (size != 4)
            return LINTP_ERROR_WRONG_PARAM;
        memcpy(&timing_request, data, size);
        break;

    case LUDS_PARAM_TIMING_RESPONSE:
        if (size != 4)
            return LINTP_ERROR_WRONG_PARAM;
        memcpy(&timing_response, data, size);
        break;

    case LUDS_PARAM_TIMING_P2:
        if (size != 4)
            return LINTP_ERROR_WRONG_PARAM;
        memcpy(&timing_p2, data, size);
        break;

    case LUDS_PARAM_TIMING_P2_EXT:
        if (size != 4)
            return LINTP_ERROR_WRONG_PARAM;
        memcpy(&timing_p2_ext, data, size);
        break;

    case LUDS_PARAM_TP_PADDING_VALUE:
    {
        LinTpResult result = LinTp_SetParam(LINTP_PARAM_PADDING_VALUE, data, size);
        if (result != LINTP_ERROR_OK)
            return LUDS_ERROR_WRONG_PARAM;
    }
    break;

    case LUDS_PARAM_TP_TIMING_RESPONSE:
    {
        LinTpResult result = LinTp_SetParam(LINTP_PARAM_TIMING_RESPONSE, data, size);
        if (result != LINTP_ERROR_OK)
            return LUDS_ERROR_WRONG_PARAM;
    }
    break;

    case LUDS_PARAM_TP_TIMING_As:
    {
        LinTpResult result = LinTp_SetParam(LINTP_PARAM_TIMING_As, data, size);
        if (result != LINTP_ERROR_OK)
            return LUDS_ERROR_WRONG_PARAM;
    }
    break;

    case LUDS_PARAM_TP_TIMING_Cr:
    {
        LinTpResult result = LinTp_SetParam(LINTP_PARAM_TIMING_Cr, data, size);
        if (result != LINTP_ERROR_OK)
            return LUDS_ERROR_WRONG_PARAM;
    }
    break;
    
    default:
        return LUDS_ERROR_WRONG_PARAM;
    }

    return LUDS_ERROR_OK;
}

EXTERNC LUdsResult __stdcall LUDS_GetParam(
    LUdsParam param,
    void *data,
    DWORD size)
{
    switch (param)
    {
    case LUDS_PARAM_TIMING_REQUEST:
        if (size != 4)
            return LINTP_ERROR_WRONG_PARAM;
        memcpy(data, &timing_request, size);
        break;

    case LUDS_PARAM_TIMING_RESPONSE:
        if (size != 4)
            return LINTP_ERROR_WRONG_PARAM;
        memcpy(data, &timing_response, size);
        break;

    case LUDS_PARAM_TIMING_P2:
        if (size != 4)
            return LINTP_ERROR_WRONG_PARAM;
        memcpy(data, &timing_p2, size);
        break;

    case LUDS_PARAM_TIMING_P2_EXT:
        if (size != 4)
            return LINTP_ERROR_WRONG_PARAM;
        memcpy(data, &timing_p2_ext, size);
        break;

    case LUDS_PARAM_TP_PADDING_VALUE:
    {
        LinTpResult result = LinTp_GetParam(LINTP_PARAM_PADDING_VALUE, data, size);
        if (result != LINTP_ERROR_OK)
            return LUDS_ERROR_WRONG_PARAM;
    }
    break;

        case LUDS_PARAM_TP_TIMING_RESPONSE:
    {
        LinTpResult result = LinTp_GetParam(LINTP_PARAM_TIMING_RESPONSE, data, size);
        if (result != LINTP_ERROR_OK)
            return LUDS_ERROR_WRONG_PARAM;
    }
    break;

    case LUDS_PARAM_TP_TIMING_As:
    {
        LinTpResult result = LinTp_GetParam(LINTP_PARAM_TIMING_As, data, size);
        if (result != LINTP_ERROR_OK)
            return LUDS_ERROR_WRONG_PARAM;
    }
    break;

    case LUDS_PARAM_TP_TIMING_Cr:
    {
        LinTpResult result = LinTp_GetParam(LINTP_PARAM_TIMING_Cr, data, size);
        if (result != LINTP_ERROR_OK)
            return LUDS_ERROR_WRONG_PARAM;
    }
    break;
    
    default:
        return LUDS_ERROR_WRONG_PARAM;
    }

    return LUDS_ERROR_OK;
}

/*============================================================================*/
LUdsResult __stdcall LUDS_SetLinDriver(LinDrvOps *ops)
{
    LinDrv_SetInterface(ops);
    return LUDS_ERROR_OK;
}
