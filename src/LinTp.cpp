#include "LinTp.h"
#include <string.h>
#include "LinDrv.h"
#include "RingBuffer.h"

/* PCI types */
#define N_PCI_TYPE_SF            0
#define N_PCI_TYPE_FF            1
#define N_PCI_TYPE_CF            2

#define ST_IDLE                  0x00
#define ST_RECEIVING             0x01
#define ST_WAIT_IDLE             0x02
#define ST_WAIT_SENDBLOCK        0x03
#define ST_SENDBLOCK             0x04

typedef struct _LinTp {
    BYTE tx_state;
    BYTE rx_state;

    BYTE padding_value;

    DWORD timing_rsp;
    DWORD timing_As;
    DWORD timing_Cr;

    BOOL rsp_timer_start;
    DWORD rsp_timestamp;

    WORD cf_offset;
    BYTE cf_cur_seq;
    DWORD cf_tx_timestamp;
    DWORD cf_rx_timestamp;
} LinTp;

static BOOL init = FALSE;
static LinTp lintp;

static MMRESULT timer_id;
static UINT timer_accuracy;

static RingBuffer<LinTpMsg> tx_buffer;
static RingBuffer<LinTpMsg> rx_buffer;

static LinTpMsg rx_msg;
static LinTpMsg tx_msg;

static LinTpResult SendSingleFrame(LinTpMsg *msg)
{
    LinMsg ldmsg;
    ldmsg.data[0] = msg->nad;
    ldmsg.data[1] = (BYTE)msg->length;
    int i = 0;
    for (i = 0; i < msg->length; i++)
        ldmsg.data[i+2] = msg->data[i];
    /* Padding */
    for (i += 2; i < 8; i++)
        ldmsg.data[i] = lintp.padding_value;

    LinTpMsg imsg;
    imsg.nad = msg->nad;
    imsg.length = 8;
    for (i = 0; i < 8; i++)
        imsg.data[i] = ldmsg.data[i];
    imsg.msg_type = LINTP_MSG_TYPE_INDICATION_TX;
    imsg.result = LINTP_N_OK;
    rx_buffer.Put(imsg);

    LinDrv_SetSchedule(LINDRV_SCHED_TYPE_REQUEST);
    LinDrv_Write(&ldmsg);
    lintp.tx_state = ST_WAIT_IDLE;
    return LINTP_ERROR_OK;
}

static LinTpResult SendFirstFrame(LinTpMsg *msg)
{
    LinMsg ldmsg;
    ldmsg.data[0] = msg->nad;
    ldmsg.data[1] = 0x10 | (BYTE)((msg->length >> 8) & 0x0F);
    ldmsg.data[2] = (BYTE)(msg->length & 0xFF);
    int i = 0;
    for (i = 0; i < 5; i++)
        ldmsg.data[i+3] = msg->data[i];
    
    LinTpMsg imsg;
    imsg.nad = msg->nad;
    imsg.length = 8;
    for (i = 0; i < 8; i++)
        imsg.data[i] = ldmsg.data[i];
    imsg.msg_type = LINTP_MSG_TYPE_INDICATION_TX;
    imsg.result = LINTP_N_OK;
    rx_buffer.Put(imsg);

    LinDrv_SetSchedule(LINDRV_SCHED_TYPE_REQUEST);
    LinDrv_Write(&ldmsg);
    lintp.cf_offset = 5;
    lintp.tx_state = ST_WAIT_SENDBLOCK;
    return LINTP_ERROR_OK;
}

static LinTpResult SendConsecutiveFrame(LinTpMsg *msg)
{
    int i = 0;
    LinMsg ldmsg;
    ldmsg.data[0] = msg->nad;
    ldmsg.data[1] = (BYTE)((0x02 << 4) + ((lintp.cf_offset/6+1) & 0x0F));
    for (i = 0; (i < 6) && (lintp.cf_offset < msg->length); i++)
    {
        ldmsg.data[i + 2] = msg->data[lintp.cf_offset];
        lintp.cf_offset++;
    }
    /* Padding */
    for (i += 2; i < 8; i++)
        ldmsg.data[i] = lintp.padding_value;
    
    LinTpMsg imsg;
    imsg.nad = msg->nad;
    imsg.length = 8;
    for (i = 0; i < 8; i++)
        imsg.data[i] = ldmsg.data[i];
    imsg.msg_type = LINTP_MSG_TYPE_INDICATION_TX;
    imsg.result = LINTP_N_OK;
    rx_buffer.Put(imsg);

    LinDrv_Write(&ldmsg);
    /* Check if the transmission is completed */
    if (lintp.cf_offset >= msg->length)
        lintp.tx_state = ST_WAIT_IDLE;
    else
        lintp.tx_state = ST_WAIT_SENDBLOCK;
    return LINTP_ERROR_OK;
}

static void TxConfirmation(LinTpMsg *msg, LinMsg *ldmsg)
{
    switch (lintp.tx_state)
    {
    case ST_WAIT_IDLE:
        LinDrv_SetSchedule(LINDRV_SCHED_TYPE_RESPONSE);

        msg->msg_type = LINTP_MSG_TYPE_CONFIRM;
        msg->result = LINTP_N_OK;
        rx_buffer.Put(*msg);
        lintp.tx_state = ST_IDLE;
        lintp.rsp_timestamp = GetTickCount();
        lintp.rsp_timer_start = TRUE;
        break;

    case ST_WAIT_SENDBLOCK:
        lintp.tx_state = ST_SENDBLOCK;
        break;

    default:
        break;
    }
}

static void RxIndication(LinTpMsg *msg, LinMsg *ldmsg)
{
    if (lintp.tx_state != ST_IDLE)
        return;

    BYTE frametype = (ldmsg->data[1] >> 4) & 0x0F;
    switch (frametype)
    {
    case N_PCI_TYPE_SF: /* Single Frame */
    {
        LinDrv_SetSchedule(LINDRV_SCHED_TYPE_STOP);

        if (lintp.rx_state == ST_RECEIVING)
        {
            LinTpMsg imsg;
            imsg.nad = ldmsg->data[0];
            imsg.length = 8;
            for (int i = 0; i < 8; i++)
                imsg.data[i] = ldmsg->data[i];
            imsg.msg_type = LINTP_MSG_TYPE_INDICATION;
            imsg.result = LINTP_N_UNEXP_PDU;
            rx_buffer.Put(imsg);
        }

        WORD data_len = ldmsg->data[1] & 0x0F;
        if ((data_len == 0) || (data_len > 6))
        {
            LinTpMsg imsg;
            imsg.nad = ldmsg->data[0];
            imsg.length = 8;
            for (int i = 0; i < 8; i++)
                imsg.data[i] = ldmsg->data[i];
            imsg.msg_type = LINTP_MSG_TYPE_INDICATION;
            imsg.result = LINTP_N_ERROR;
            rx_buffer.Put(imsg);
            return;
        }

        LinTpMsg imsg;
        imsg.nad = ldmsg->data[0];
        imsg.length = 8;
        for (int i = 0; i < 8; i++)
            imsg.data[i] = ldmsg->data[i];
        imsg.msg_type = LINTP_MSG_TYPE_INDICATION;
        imsg.result = LINTP_N_OK;
        rx_buffer.Put(imsg);

        msg->length = data_len;
        for (int i = 0; i < data_len; i++)
            msg->data[i] = ldmsg->data[i+2];
        msg->msg_type = LINTP_MSG_TYPE_DIAGNOSTIC;
        msg->result = LINTP_N_OK;
        rx_buffer.Put(*msg);

        lintp.rx_state = ST_IDLE;
    }
    break;

    case N_PCI_TYPE_FF: /* First Frame */
    {
        if (lintp.rx_state == ST_RECEIVING)
        {
            LinTpMsg imsg;
            imsg.nad = ldmsg->data[0];
            imsg.length = 8;
            for (int i = 0; i < 8; i++)
                imsg.data[i] = ldmsg->data[i];
            imsg.msg_type = LINTP_MSG_TYPE_INDICATION;
            imsg.result = LINTP_N_UNEXP_PDU;
            rx_buffer.Put(imsg);
        }

        WORD data_len = (WORD)(((WORD)(ldmsg->data[1] & 0x0F) << 8) | ldmsg->data[2]);
        if (data_len < 7)
        {
            LinDrv_SetSchedule(LINDRV_SCHED_TYPE_STOP);
            LinTpMsg imsg;
            imsg.nad = ldmsg->data[0];
            imsg.length = 8;
            for (int i = 0; i < 8; i++)
                imsg.data[i] = ldmsg->data[i];
            imsg.msg_type = LINTP_MSG_TYPE_INDICATION;
            imsg.result = LINTP_N_ERROR;
            rx_buffer.Put(imsg);
            return;
        }

        LinTpMsg imsg;
        imsg.nad = ldmsg->data[0];
        imsg.length = 8;
        for (int i = 0; i < 8; i++)
            imsg.data[i] = ldmsg->data[i];
        imsg.msg_type = LINTP_MSG_TYPE_INDICATION;
        imsg.result = LINTP_N_OK;
        rx_buffer.Put(imsg);

        msg->nad = ldmsg->data[0];
        msg->length = data_len;
        for (int i = 0; i < 5; i++)
            msg->data[i] = ldmsg->data[i+3];
        lintp.cf_offset = 5;
        lintp.cf_cur_seq = 0;
        lintp.cf_rx_timestamp = GetTickCount();
        lintp.rx_state = ST_RECEIVING;
    }
    break;

    case N_PCI_TYPE_CF: /* Consecutive Frame */
    {
        if (lintp.rx_state != ST_RECEIVING)
            return;

        /* Check if the sequence number is correct */
        BYTE seq = ldmsg->data[1] & 0x0F;
        if (((lintp.cf_cur_seq + 1) & 0xF) != seq)
        {
            LinDrv_SetSchedule(LINDRV_SCHED_TYPE_STOP);
            LinTpMsg imsg;
            imsg.nad = ldmsg->data[0];
            imsg.length = 8;
            for (int i = 0; i < 8; i++)
                imsg.data[i] = ldmsg->data[i];
            imsg.msg_type = LINTP_MSG_TYPE_INDICATION;
            imsg.result = LINTP_N_WRONG_SN;
            rx_buffer.Put(imsg);
            lintp.rx_state = ST_IDLE;
            return;
        }

        lintp.cf_rx_timestamp = GetTickCount();
        lintp.cf_cur_seq = seq;
        if (lintp.cf_cur_seq == ((lintp.cf_offset/6+1) & 0x0F))
        {
            for (int i = 2; (i < ldmsg->length) && (lintp.cf_offset < msg->length); i++)
            {
                msg->data[lintp.cf_offset] = ldmsg->data[i];
                lintp.cf_offset++;
            }
            
            LinTpMsg imsg;
            imsg.nad = ldmsg->data[0];
            imsg.length = 8;
            for (int i = 0; i < 8; i++)
                imsg.data[i] = ldmsg->data[i];
            imsg.msg_type = LINTP_MSG_TYPE_INDICATION;
            imsg.result = LINTP_N_OK;
            rx_buffer.Put(imsg);

            /* Check if the reception is completed */
            if (lintp.cf_offset >= msg->length)
            {
                LinDrv_SetSchedule(LINDRV_SCHED_TYPE_STOP);
                msg->msg_type = LINTP_MSG_TYPE_DIAGNOSTIC;
                msg->result = LINTP_N_OK;
                rx_buffer.Put(*msg);
                lintp.rx_state = ST_IDLE;
            }
        }
    }
    break;

    default: /* Invalid frame type, ignore it */
        break;
    }
}

static void CALLBACK TimerProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, 
                               DWORD_PTR dw1, DWORD_PTR d2)
{
    /* Read LIN message from LIN driver and process */
    LinMsg ldmsg;
    LinDrvResult result = LinDrv_Read(&ldmsg);
    if (result == LINDRV_ERROR_OK)
    {
        if (ldmsg.dir == LINDRV_MSG_DIR_PUBLISHER)
        {
            TxConfirmation(&tx_msg, &ldmsg);
        }
        else
        {
            lintp.rsp_timer_start = FALSE;
            RxIndication(&rx_msg, &ldmsg);
        }
    }

    /* If LINTP tx state is IDLE, then read message from the transmit queue
     * and start sending */
    if ((lintp.tx_state == ST_IDLE) && (lintp.rx_state == ST_IDLE))
    {
        if (!tx_buffer.IsEmpty())
        {
            tx_buffer.Get(tx_msg);
            if (tx_msg.length < 7)
                SendSingleFrame(&tx_msg);
            else
                SendFirstFrame(&tx_msg);
            lintp.cf_tx_timestamp = GetTickCount();
        }
    }
    /* Send consecutive frame */
    else if (lintp.tx_state == ST_SENDBLOCK)
    {
        SendConsecutiveFrame(&tx_msg);
        lintp.cf_tx_timestamp = GetTickCount();
    }

    /* Check Cr timer */
    if (lintp.rx_state == ST_RECEIVING)
    {    
        if (GetTickCount() - lintp.cf_rx_timestamp > lintp.timing_Cr)
        {
            rx_msg.msg_type = LINTP_MSG_TYPE_DIAGNOSTIC;
            rx_msg.result = LINTP_N_TIMEOUT_Cr;
            rx_buffer.Put(rx_msg);
            lintp.rx_state = ST_IDLE;
            LinDrv_SetSchedule(LINDRV_SCHED_TYPE_STOP);
        }
    }

    /* Check As timer */
    if ((lintp.tx_state == ST_WAIT_IDLE) ||
        (lintp.tx_state == ST_SENDBLOCK) ||
        (lintp.tx_state == ST_WAIT_SENDBLOCK))
    {
        if (GetTickCount() - lintp.cf_tx_timestamp >= lintp.timing_As)
        {
            tx_msg.msg_type = LINTP_MSG_TYPE_CONFIRM;
            tx_msg.result = LINTP_N_TIMEOUT_A;
            rx_buffer.Put(tx_msg);
            lintp.tx_state = ST_IDLE;
        }
    }

    /* Check if response is timeout */
    if (lintp.rsp_timer_start)
    {
        if (GetTickCount() - lintp.rsp_timestamp >= lintp.timing_rsp)
        {
            lintp.rsp_timer_start = FALSE;
            LinDrv_SetSchedule(LINDRV_SCHED_TYPE_STOP);
        }
    }
}

LinTpResult LinTp_Init(void)
{
    if (init)
        return LINTP_ERROR_ALREADY_INITIALIZED;

    LinDrv_Init();

    lintp.tx_state = ST_IDLE;
    lintp.rx_state = ST_IDLE;
    lintp.padding_value = 0xFF;
    lintp.timing_rsp = 1000;
    lintp.timing_As = 1000;
    lintp.timing_Cr = 1000;
    lintp.rsp_timer_start = FALSE;

    TIMECAPS tc;
    if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) == TIMERR_NOERROR)
    {
        UINT tmp = tc.wPeriodMin > 1 ? tc.wPeriodMin : 1;
        timer_accuracy = tmp < tc.wPeriodMax ? tmp : tc.wPeriodMax;
        timeBeginPeriod(timer_accuracy);
        timer_id = timeSetEvent(1, 0, TimerProc, NULL, TIME_PERIODIC);
    }

    init = TRUE;
    return LINTP_ERROR_OK;
}

LinTpResult LinTp_Uninit(void)
{
    if (!init)
        return LINTP_ERROR_NOT_INITIALIZED;

    LinDrv_Uninit();

    timeKillEvent(timer_id);
    timeEndPeriod(timer_accuracy);

    init = FALSE;
    return LINTP_ERROR_OK;
}

LinTpResult LinTp_Write(LinTpMsg *msg)
{
    if (msg == NULL)
        return LINTP_ERROR_WRONG_PARAM;

    if (tx_buffer.IsFull())
        return LINTP_ERROR_NO_MEMORY;
    tx_buffer.Put(*msg);
    return LINTP_ERROR_OK;
}

LinTpResult LinTp_Read(LinTpMsg *msg, LinTpTimestamp *timestamp)
{
    if (msg == NULL)
        return LINTP_ERROR_WRONG_PARAM;

    if (rx_buffer.IsEmpty())
        return LINTP_ERROR_NO_MESSAGE;
    rx_buffer.Get(*msg);
    return LINTP_ERROR_OK;
}

LinTpResult LinTp_KeepWaitForResponse(void)
{
    LinDrv_SetSchedule(LINDRV_SCHED_TYPE_RESPONSE);
    lintp.rsp_timestamp = GetTickCount();
    lintp.rsp_timer_start = TRUE;
    return LINTP_ERROR_OK;
}

LinTpResult LinTp_SetParam(LinTpParam param, void *data, DWORD size)
{
    switch (param)
    {
    case LINTP_PARAM_PADDING_VALUE:
        if (size != 1)
            return LINTP_ERROR_WRONG_PARAM;
        memcpy(&lintp.padding_value, data, size);
        break;

    case LINTP_PARAM_TIMING_RESPONSE:
        if (size != 4)
            return LINTP_ERROR_WRONG_PARAM;
        memcpy(&lintp.timing_rsp, data, size);
        break;
    
    case LINTP_PARAM_TIMING_As:
        if (size != 4)
            return LINTP_ERROR_WRONG_PARAM;
        memcpy(&lintp.timing_As, data, size);
        break;

    case LINTP_PARAM_TIMING_Cr:
        if (size != 4)
            return LINTP_ERROR_WRONG_PARAM;
        memcpy(&lintp.timing_Cr, data, size);
        break;

    default:
        return LINTP_ERROR_WRONG_PARAM;
    }

    return LINTP_ERROR_OK;
}

LinTpResult LinTp_GetParam(LinTpParam param, void *data, DWORD size)
{
    switch (param)
    {
    case LINTP_PARAM_PADDING_VALUE:
        if (size != 1)
            return LINTP_ERROR_WRONG_PARAM;
        memcpy(data, &lintp.padding_value, size);
        break;

    case LINTP_PARAM_TIMING_RESPONSE:
        if (size != 4)
            return LINTP_ERROR_WRONG_PARAM;
        memcpy(data, &lintp.timing_rsp, size);
        break;
    
    case LINTP_PARAM_TIMING_As:
        if (size != 4)
            return LINTP_ERROR_WRONG_PARAM;
        memcpy(data, &lintp.timing_As, size);
        break;

    case LINTP_PARAM_TIMING_Cr:
        if (size != 4)
            return LINTP_ERROR_WRONG_PARAM;
        memcpy(data, &lintp.timing_Cr, size);
        break;
    
    default:
        return LINTP_ERROR_WRONG_PARAM;
    }

    return LINTP_ERROR_OK;
}
