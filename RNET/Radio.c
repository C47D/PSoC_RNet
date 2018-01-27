/**
 * \file
 * \brief This is the implementation of the Nordic Semiconductor nRF24L01+ low
 * level driver. \author (c) 2013-2014 Erich Styger, http://mcuoneclipse.com/
 * \note MIT License (http://opensource.org/licenses/mit-license.html), see
 * 'RNet_License.txt'
 *
 * This module deals with the low level functions of the transceiver.
 */

#include "FreeRTOS.h"
#include "task.h"

#include "Radio.h"
#include "RNetConf.h"
#include "RadioNRF24.h"

#include "RMSG.h"
#include "RPHY.h"
#include "RStdIO.h"
//#include "UTIL1.h"
//#include "Events.h" /* for event handler interface */
#include "Error.h"
#include <stdbool.h>
#include "UART.h"

/* if set to one, use dynamic payload size */
#define NRF24_DYNAMIC_PAYLOAD   1
/* if set to one, the transceiver is configured to use auto acknowledge */
#define NRF24_AUTO_ACKNOWLEDGE  1
/* default communication channel */
#define RADIO_CHANNEL_DEFAULT   RNET_CONFIG_TRANSCEIVER_CHANNEL
/* timeout value in milliseconds, used for RADIO_WAITING_DATA_SENT */
#define RADIO_WAITNG_TIMEOUT_MS 200

/* macros to configure device either for RX or TX operation */
#define RF1_CONFIG_SETTINGS (NRF_CONFIG_EN_CRC | NRF_CONFIG_CRCO)
/* enable 2 byte CRC, power up and set as PTX */
#define TX_POWERUP()    RF1_write_register(NRF_CONFIG_REG, RF1_CONFIG_SETTINGS | NRF_CONFIG_PWR_UP /* | NRF_CONFIG_PRIM_RX, this should be NRF_CONFIG_PRIM_TX */)
/* enable 1 byte CRC, power up and set as PRX */
#define RX_POWERUP()    RF1_write_register(NRF_CONFIG_REG, RF1_CONFIG_SETTINGS | NRF_CONFIG_PWR_UP | NRF_CONFIG_PRIM_RX)
/* power down */
#define POWERDOWN()     RF1_write_register(NRF_CONFIG_REG, RF1_CONFIG_SETTINGS)

static bool RADIO_isSniffing = false;
/* we are using 5 address bytes */
#define RADIO_NOF_ADDR_BYTES 5

/* device address for pipe 0 */
static const uint8_t RADIO_TADDR_P0[RADIO_NOF_ADDR_BYTES] = {
    0x11, 0x22, 0x33, 0x44, 0x55};

/* device address for pipe 1 */
static const uint8_t RADIO_TADDR_P1[RADIO_NOF_ADDR_BYTES] = {
    0xB3, 0xB4, 0xB5, 0xB6, 0xF1};

/* device address for pipe 2 */
static const uint8_t RADIO_TADDR_P2[1] = {0xF2};

/* device address for pipe 3 */
static const uint8_t RADIO_TADDR_P3[1] = {0xF3};

/* device address for pipe 4 */
static const uint8_t RADIO_TADDR_P4[1] = {0xF4};

/* device address for pipe 5 */
static const uint8_t RADIO_TADDR_P5[1] = {0xF5};

#if RNET_CONFIG_SEND_RETRY_CNT > 0
static uint8_t RADIO_RetryCnt;
/*!< global buffer if using retries */
static uint8_t TxDataBuffer[RPHY_BUFFER_SIZE];
#endif

/* Radio state definitions */
typedef enum RADIO_AppStatusKind {
    /* initial state of the state machine */
    RADIO_INITIAL_STATE,
    /* receiver is in RX mode */
    RADIO_RECEIVER_ALWAYS_ON,
    /* send data */
    RADIO_TRANSMIT_DATA,
    /* wait until data is sent */
    RADIO_WAITING_DATA_SENT,
    RADIO_TIMEOUT,
    RADIO_READY_FOR_TX_RX_DATA,
    /* send data if any */
    RADIO_CHECK_TX,
    /* transceiver powered down */
    RADIO_POWER_DOWN,
} RADIO_AppStatusKind;

static RADIO_AppStatusKind RADIO_AppStatus = RADIO_INITIAL_STATE;
static RPHY_PacketDesc radioRx;
static uint8_t radioRxBuf[RPHY_BUFFER_SIZE];
static uint8_t RADIO_CurrChannel = RADIO_CHANNEL_DEFAULT;

/* need to have this in case RF device is still added to project */
static volatile bool RADIO_isrFlag; /* flag set by ISR */

static void Err(unsigned char *msg)
{
    /* not used, as no Shell used */
    //(void)msg;
    UART_PutString((const char *) msg);
}

/* callback called from radio driver */
void RADIO_OnInterrupt(void)
{
    RADIO_isrFlag = true;
}

uint8_t RADIO_FlushQueues(void)
{
    uint8_t res = ERR_OK;

    if (RPHY_FlushRxQueue() != ERR_OK) {
        res = ERR_FAILED;
    }
    
    if (RPHY_FlushTxQueue() != ERR_OK) {
        res = ERR_FAILED;
    }
    
    return res;
}

static uint8_t RADIO_Flush(void)
{
    // RF1_Write(RF1_FLUSH_RX); /* flush old data */
    // RF1_Write(RF1_FLUSH_TX); /* flush old data */
    RF1_flush_rx();
    RF1_flush_rx();
    
    return ERR_OK;
}

bool RADIO_CanDoPowerDown(void)
{
    /* interrupt pending */
    if (RADIO_isrFlag) {
        return false;
    }
    
    switch (RADIO_AppStatus) {
    
    /* sending/receiving data, cannot power down */
    case RADIO_TRANSMIT_DATA:
    case RADIO_WAITING_DATA_SENT:
    case RADIO_TIMEOUT:
        return false;

    /* check other conditions */
    case RADIO_INITIAL_STATE:
    case RADIO_RECEIVER_ALWAYS_ON:
    case RADIO_READY_FOR_TX_RX_DATA:
    case RADIO_CHECK_TX:
    case RADIO_POWER_DOWN:
        break;
    default:
        break;
    }
    
    /* items received, cannot power down */
    if (RMSG_RxQueueNofItems() != 0) {
        return false;
    }
    
    /* items to be sent, cannot power down */
    if (RMSG_TxQueueNofItems() != 0) {
        return false;
    }
    
    /* ok to power down */
    return true;
}

uint8_t RADIO_PowerDown(void)
{
    uint8_t res;

    res = RADIO_Flush();
    POWERDOWN();
    
    return res;
}

static uint8_t CheckTx(void)
{
    RPHY_PacketDesc packet;
    
#if RNET_CONFIG_SEND_RETRY_CNT == 0
    /* local tx buffer if not using retries */
    uint8_t TxDataBuffer[RPHY_BUFFER_SIZE];
#endif

    RPHY_FlagsType flags;

    if (RMSG_GetTxMsg(TxDataBuffer, sizeof(TxDataBuffer)) == ERR_OK) {
        flags = RPHY_BUF_FLAGS(TxDataBuffer);
        if (flags & RPHY_PACKET_FLAGS_POWER_DOWN) {
            /* special request */
            (void)RADIO_PowerDown();
            
            /* no more data, pipes flushed */
            return ERR_DISABLED;
        }
        
        //RF1_StopRxTx(); /* CE low */
        RF1_stop_listening();
        
        TX_POWERUP();
        
        /* set up packet structure */
        packet.phyData = &TxDataBuffer[0];
        packet.flags = flags;
        packet.phySize = sizeof(TxDataBuffer);

#if NRF24_DYNAMIC_PAYLOAD
        packet.rxtx = RPHY_BUF_PAYLOAD_START(packet.phyData);
#else
        /* we transmit the data size too */
        packet.rxtx = &RPHY_BUF_SIZE(packet.phyData);
#endif

        /* sniff outgoing packet */
        if (RADIO_isSniffing) {
            RPHY_SniffPacket(&packet, true);
        }
        
#if NRF24_DYNAMIC_PAYLOAD
        /* send data, using dynamic payload size */
        //RF1_TxPayload(packet.rxtx, RPHY_BUF_SIZE(packet.phyData));
        RF1_transmit(packet.rxtx, RPHY_BUF_SIZE(packet.phyData));
#else
        /* send data, using fixed payload size */
        // RF1_TxPayload(packet.rxtx, RPHY_PAYLOAD_SIZE);
        RF1_transmit(packet.rxtx, RPHY_PAYLOAD_SIZE);
#endif

        return ERR_OK;
    }
    
    /* no data to send? */
    return ERR_NOTAVAIL;
}

/* called to check if we have something in the RX queue. If so, we queue it */
static uint8_t CheckRx(void)
{
    
#if NRF24_DYNAMIC_PAYLOAD
    uint8_t payloadSize;
#endif

    uint8_t res = ERR_OK;
    uint8_t RxDataBuffer[RPHY_BUFFER_SIZE];
    uint8_t status;
    RPHY_PacketDesc packet;
    bool hasRxData, hasRx;

    hasRxData = false;
    packet.flags = RPHY_PACKET_FLAGS_NONE;
    packet.phyData = &RxDataBuffer[0];
    packet.phySize = sizeof(RxDataBuffer);

#if NRF24_DYNAMIC_PAYLOAD
    packet.rxtx = RPHY_BUF_PAYLOAD_START(packet.phyData);
#else
    /* we transmit the data size too */
    packet.rxtx = &RPHY_BUF_SIZE(packet.phyData);
#endif

    //status = RF1_GetStatusClrIRQ();
    status = RF1_get_status_clear_irq();
    hasRx = (status & RF1_STATUS_RX_DR) != 0;
    
#if !RF1_CONFIG_IRQ_PIN_ENABLED
#if 1             /* experimental */
    if (!hasRx) { /* interrupt flag not set, check if we have otherwise data */
        (void)RF1_GetFifoStatus(&status);
        
        if (!(status & RF1_FIFO_STATUS_RX_EMPTY) ||
            (status & RF1_FIFO_STATUS_RX_FULL)) { /* Rx not empty? */
            hasRx = true;
        }
    }
#endif
#endif

    if (hasRx) { /* data received interrupt */
        hasRxData = true;
#if NRF24_DYNAMIC_PAYLOAD
        //(void)RF1_ReadNofRxPayload(&payloadSize);
        payloadSize = RF1_read_payload_width_cmd();
        
        if (payloadSize == 0 || payloadSize > 32) { /* packet with error? */
            //RF1_Write(RF1_FLUSH_RX);                /* flush old data */
            RF1_flush_rx();
            
            return ERR_FAILED;
        } else {
            /* get payload: note that we transmit <size> as payload! */
            //RF1_RxPayload(packet.rxtx, payloadSize);
            RF1_get_rx_payload(packet.rxtx, payloadSize);
            
            RPHY_BUF_SIZE(packet.phyData) = payloadSize;
        }
#else
        /* get payload: note that we transmit <size> as payload! */
        RF1_RxPayload(packet.rxtx, RPHY_PAYLOAD_SIZE);
#endif
    }
    
    /* put message into Rx queue */
    if (hasRxData) {
#if RNET1_CREATE_EVENTS
        RNET1_OnRadioEvent(RNET1_RADIO_MSG_RECEIVED);
#endif
        res = RMSG_QueueRxMsg(packet.phyData, packet.phySize,
                              RPHY_BUF_SIZE(packet.phyData), packet.flags);
        if (res != ERR_OK) {
            if (res == ERR_OVERFLOW) {
                Err((unsigned char *)"ERR: Rx queue overflow!\r\n");
            } else {
                Err((unsigned char *)"ERR: Rx Queue full?\r\n");
            }
        }
    } else {
        res = ERR_RXEMPTY; /* no data */
    }
    
    return res;
}

static void WaitRandomTime(void)
{
    if (configTICK_RATE_HZ <= 100) { /* slower tick rate */
        vTaskDelay(10 + (xTaskGetTickCount() % 16));
    } else { /* higher tick rate: wait between 10 and 10+32 ticks */
        vTaskDelay(10 + (xTaskGetTickCount() % 32));
    }
}

static void RADIO_HandleStateMachine(void)
{
    
#if RADIO_WAITNG_TIMEOUT_MS > 0
    static TickType_t sentTimeTickCntr = 0; /* used for timeout */
#endif
    uint8_t status = 0;
    uint8_t res = 0;

    /* will break/return */
    for (;;) {
        switch (RADIO_AppStatus) {
        case RADIO_INITIAL_STATE:
            //RF1_StopRxTx(); /* will send/receive data later */
            RF1_stop_listening();
            RADIO_AppStatus = RADIO_RECEIVER_ALWAYS_ON; /* turn receive on */
            break; /* process switch again */

        case RADIO_RECEIVER_ALWAYS_ON:
            RX_POWERUP(); /* turn receive on */
            //RF1_StartRxTx(); /* Listening for packets */
            RF1_start_listening();
            RADIO_AppStatus = RADIO_READY_FOR_TX_RX_DATA;
            break; /* process switch again */

        /* we are ready to receive/send data */
        case RADIO_READY_FOR_TX_RX_DATA:
#if !RF1_CONFIG_IRQ_PIN_ENABLED
            //RF1_PollInterrupt();
            RF1_poll_interrupt();
#if 1   /* experimental */
            /* interrupt flag not set, check if we have otherwise data */
            if (!RADIO_isrFlag) {
                (void)RF1_GetFifoStatus(&status);
                
                /* Rx not empty? */
                if (!(status & RF1_FIFO_STATUS_RX_EMPTY) || // RF1_FIFO_STATUS_RX_EMPTY  (1<<0)
                    (status & RF1_FIFO_STATUS_RX_FULL)) {
                    RADIO_isrFlag = true;
                }
            }
#endif
#endif
            if (RADIO_isrFlag) {         /* Rx interrupt? */
                RADIO_isrFlag = false;   /* reset interrupt flag */
                res = CheckRx();         /* get message */
#if 1                                    /* experimental */
                if (res == ERR_FAILED) { /* failed reading from device */
                    RADIO_AppStatus =
                        RADIO_RECEIVER_ALWAYS_ON; /* continue listening */
                    return;                       /* get out of loop */
                }
                (void)RF1_GetFifoStatus(&status);
                
                /* no data, but still flag set? */
                // RF1_FIFO_STATUS_RX_EMPTY  (1<<0)
                if (ERR_RXEMPTY == res && !(status & RF1_FIFO_STATUS_RX_EMPTY)) {
                    //RF1_Write(RF1_FLUSH_RX); /* flush old data */
                    RF1_flush_rx();
                    
                    /* continue listening */
                    RADIO_AppStatus = RADIO_RECEIVER_ALWAYS_ON;
                    
                /* Rx not empty? */
                } else if (!(status & RF1_FIFO_STATUS_RX_EMPTY) || // RF1_FIFO_STATUS_RX_EMPTY  (1<<0)
                           (status & RF1_FIFO_STATUS_RX_FULL)) { // RF1_FIFO_STATUS_RX_FULL  (1<<1)
                    RADIO_isrFlag = true; /* stay in current state */
                } else {
                    RADIO_AppStatus =
                        RADIO_RECEIVER_ALWAYS_ON; /* continue listening */
                }
#else
                /* continue listening */
                RADIO_AppStatus = RADIO_RECEIVER_ALWAYS_ON;
#endif
                break; /* process switch again */
            }
            
#if RNET_CONFIG_SEND_RETRY_CNT > 0
            RADIO_RetryCnt = 0;
#endif

            /* check if we can send something */
            RADIO_AppStatus = RADIO_CHECK_TX;
            break;

        case RADIO_CHECK_TX:
            res = CheckTx();
            
            /* there was data and it has been sent */
            if (res == ERR_OK) {
#if RADIO_WAITNG_TIMEOUT_MS > 0
                /* remember time when it was sent, used for timeout */
                sentTimeTickCntr = xTaskGetTickCount();
#endif
                RADIO_AppStatus = RADIO_WAITING_DATA_SENT;
                break;                        /* process switch again */

            /* powered down transceiver */
            } else if (res == ERR_DISABLED) {
                RADIO_AppStatus = RADIO_POWER_DOWN;
            } else {
                RADIO_AppStatus = RADIO_READY_FOR_TX_RX_DATA;
            }
            return;

        case RADIO_POWER_DOWN:
            return;

        case RADIO_WAITING_DATA_SENT:
#if !RF1_CONFIG_IRQ_PIN_ENABLED
            //RF1_PollInterrupt();
            RF1_poll_interrupt();
#else /* experimental */
            if (!RADIO_isrFlag) { /* check if we missed an interrupt? */
                //RF1_PollInterrupt();
                RF1_poll_interrupt();
            }
#endif

/* check if we have received an interrupt: this is either timeout or low level ack */
            if (RADIO_isrFlag) {
                RADIO_isrFlag = false; /* reset interrupt flag */
                //status = RF1_GetStatusClrIRQ();
                status = RF1_get_status_clear_irq();
                
                /* retry timeout interrupt */
                if (status & RF1_STATUS_MAX_RT) { // RF1_STATUS_MAX_RT = 0x10
                    //RF1_Write(RF1_FLUSH_TX);      /* flush old data */
                    RF1_flush_rx();
                    RADIO_AppStatus = RADIO_TIMEOUT; /* timeout */
                    WaitRandomTime();
                } else {
#if RNET1_CREATE_EVENTS
                    RNET1_OnRadioEvent(RNET1_RADIO_MSG_SENT);
#endif
                    /* turn receive on */
                    RADIO_AppStatus = RADIO_RECEIVER_ALWAYS_ON;
                }
                break; /* process switch again */
            }
            
#if RADIO_WAITNG_TIMEOUT_MS > 0
            if (pdMS_TO_TICKS((xTaskGetTickCount() - sentTimeTickCntr)) >
                RADIO_WAITNG_TIMEOUT_MS) {
                RADIO_AppStatus = RADIO_TIMEOUT; /* timeout */
            }
#endif

            return;

        case RADIO_TIMEOUT:
#if RNET_CONFIG_SEND_RETRY_CNT > 0
            if (RADIO_RetryCnt < RNET_CONFIG_SEND_RETRY_CNT) {
                Err((unsigned char *)"ERR: Retry\r\n");
#if RNET1_CREATE_EVENTS
                RNET1_OnRadioEvent(RNET1_RADIO_RETRY);
#endif
                RADIO_RetryCnt++;
                if (RMSG_PutRetryTxMsg(TxDataBuffer, sizeof(TxDataBuffer)) ==
                    ERR_OK) {
                    RADIO_AppStatus = RADIO_CHECK_TX; /* resend packet */
                    return; /* iterate state machine next time */
                } else {
                    Err((unsigned char *)"ERR: PutRetryTxMsg failed!\r\n");
#if RNET1_CREATE_EVENTS
                    /*lint -save -e522 function lacks side effect  */
                    RNET1_OnRadioEvent(RNET1_RADIO_RETRY_MSG_FAILED);
                    /*lint -restore */
#endif
                }
            }
#endif

            Err((unsigned char *)"ERR: Timeout\r\n");

#if RNET1_CREATE_EVENTS
            RNET1_OnRadioEvent(RNET1_RADIO_TIMEOUT);
#endif
            RADIO_AppStatus = RADIO_RECEIVER_ALWAYS_ON; /* turn receive on */
            break; /* process switch again */

        default: /* should not happen! */
            return;
        } /* switch */
    }     /* for */
}

uint8_t RADIO_SetChannel(uint8_t channel)
{
    RADIO_CurrChannel = channel;
    //return RF1_SetChannel(channel);
    RF1_set_channel(channel);
    // TODO: return value from set_channel
    return ERR_OK;
}

/*!
 * \brief Radio power-on initialization.
 * \return Error code, ERR_OK if everything is ok.
 */
uint8_t RADIO_PowerUp(void)
{
    //uint8_t addr[RADIO_NOF_ADDR_BYTES];

    //RF1_Init(); /* set CE and CSN to initialization value */
    RF1_start();

    //RF1_WriteRegister(RF1_RF_SETUP, RF1_RF_SETUP_RF_PWR_0 | RNET_CONFIG_NRF24_DATA_RATE);
    RF1_write_register(NRF_RF_SETUP_REG, NRF_RF_SETUP_RF_PWR_0 | RNET_CONFIG_NRF24_DATA_RATE);
    
#if NRF24_DYNAMIC_PAYLOAD
    /* enable dynamic payload, set EN_DPL for dynamic payload */
    //(void)RF1_WriteFeature(
    //    RF1_FEATURE_EN_DPL | RF1_FEATURE_EN_ACK_PAY |
    //    RF1_FEATURE_EN_DYN_PAY);
    RF1_write_register(NRF_FEATURE_REG, NRF_FEATURE_EN_DPL | NRF_FEATURE_EN_ACK_PAY | NRF_FEATURE_EN_DYN_ACK);
    //  (void)RF1_EnableDynamicPayloadLength(RF1_DYNPD_DPL_P0); /* set DYNPD
    //  register for dynamic payload for pipe0 */
    /* set DYNPD register for dynamic payload for all pipes */
    //(void)RF1_EnableDynamicPayloadLength(RF1_DYNPD_DPL_ALL);
    RF1_enable_dynamic_payload(NRF_PIPE0);
    RF1_enable_dynamic_payload(NRF_PIPE1);
    RF1_enable_dynamic_payload(NRF_PIPE2);
    RF1_enable_dynamic_payload(NRF_PIPE3);
    RF1_enable_dynamic_payload(NRF_PIPE4);
    RF1_enable_dynamic_payload(NRF_PIPE5);
#else
    /* static number of payload bytes we want to send and receive */
    (void)RF1_SetStaticPipePayload(0, RPHY_PAYLOAD_SIZE);
#endif
    (void)RADIO_SetChannel(RADIO_CurrChannel);

    /* set address width to 5 bytes (default) */
    //(void)RF1_SetAddressWidth(RADIO_NOF_ADDR_BYTES);
    RF1_set_address_width(NRF_SETUP_AW_5BYTES);

    /* Set RADDR and TADDR as the transmit address since we also enable auto
     * acknowledgment */
    //(void)RF1_SetTxAddress((uint8_t *)RADIO_TADDR_P0, sizeof(RADIO_TADDR_P0));
    /* Pipes 0 to 5 */
    //(void)RF1_SetRxAddress(0, (uint8_t *)RADIO_TADDR_P0, RADIO_NOF_ADDR_BYTES);
    //(void)RF1_SetRxAddress(1, (uint8_t *)RADIO_TADDR_P1, RADIO_NOF_ADDR_BYTES);
    RF1_set_rx_pipe_0_address(RADIO_TADDR_P0, RADIO_NOF_ADDR_BYTES);
    
    RF1_set_rx_pipe_1_address(RADIO_TADDR_P1, RADIO_NOF_ADDR_BYTES);
    
    /* for the following pipes, use P1 as base. Only need to write single byte
     * to transceiver */
    //for (i = 0; i < RADIO_NOF_ADDR_BYTES; i++) {
    //    addr[i] = RADIO_TADDR_P1[i]; /* use P1 as base */
    //}
    //addr[RADIO_NOF_ADDR_BYTES - 1] = RADIO_TADDR_P2[0];
    //(void)RF1_SetRxAddress(2, addr, RADIO_NOF_ADDR_BYTES);
    RF1_set_rx_pipe_2_address(RADIO_TADDR_P2[0]);

    //addr[RADIO_NOF_ADDR_BYTES - 1] = RADIO_TADDR_P3[0];
    //(void)RF1_SetRxAddress(3, addr, RADIO_NOF_ADDR_BYTES);
    RF1_set_rx_pipe_3_address(RADIO_TADDR_P3[0]);
    
    //addr[RADIO_NOF_ADDR_BYTES - 1] = RADIO_TADDR_P4[0];
    //(void)RF1_SetRxAddress(4, addr, RADIO_NOF_ADDR_BYTES);
    RF1_set_rx_pipe_4_address(RADIO_TADDR_P4[0]);
    
    //addr[RADIO_NOF_ADDR_BYTES - 1] = RADIO_TADDR_P5[0];
    //(void)RF1_SetRxAddress(5, addr, RADIO_NOF_ADDR_BYTES);
    RF1_set_rx_pipe_5_address(RADIO_TADDR_P5[0]);
    
    /* Enable RX_ADDR address matching */
    /* enable all data pipes */
    // RF1_EN_RXADDR_ERX_ALL = 0x37
    //(void)RF1_WriteRegister(RF1_EN_RXADDR, RF1_EN_RXADDR_ERX_ALL);
    RF1_write_register(NRF_EN_RXADDR_ERX_P0, 0x37);

    /* clear interrupt flags */
    //RF1_ResetStatusIRQ(RF1_STATUS_RX_DR | RF1_STATUS_TX_DS | RF1_STATUS_MAX_RT);
    RF1_clear_all_irqs();

    /* rx/tx mode */
#if NRF24_AUTO_ACKNOWLEDGE
#if 0
    /* enable auto acknowledge on pipe 0. RX_ADDR_P0 needs to be equal to TX_ADDR! */
    (void)RF1_EnableAutoAck(RF1_EN_AA_ENAA_P0);
#else
    /* enable auto acknowledge on all pipes. RX_ADDR_P0 needs to be equal to TX_ADDR! */
    //(void)RF1_EnableAutoAck(RF1_EN_AA_ENAA_ALL);
    // RF1_EN_AA_ENAA_ALL = 0x3F
    RF1_enable_auto_ack(0x3F);
#endif
#endif
    /* Important: need 750 us delay between every retry */
    //RF1_WriteRegister(RF1_SETUP_RETR, RF1_SETUP_RETR_ARD_750 | RF1_SETUP_RETR_ARC_15);
    RF1_write_register(NRF_SETUP_RETR_REG, NRF_SETUP_RETR_ARD_750_US | NRF_SETUP_RETR_ARC_15);

    RX_POWERUP();        /* Power up in receiving mode */
    (void)RADIO_Flush(); /* flush possible old data */
    //RF1_StartRxTx();     /* Listening for packets */
    RF1_start_listening();

    RADIO_AppStatus = RADIO_INITIAL_STATE;
    /* init Rx descriptor */
    radioRx.phyData = &radioRxBuf[0];
    radioRx.phySize = sizeof(radioRxBuf);
    /* we transmit the size too */
    radioRx.rxtx = &RPHY_BUF_SIZE(radioRx.phyData);
    
    return ERR_OK;
}

uint8_t RADIO_Process(void)
{
    uint8_t res = 0;

    /* process state machine */
    RADIO_HandleStateMachine();
    
    /* breaks, tries to handle multiple incoming messages */
    for (uint8_t i = 0; i < 10; i++) {
        /* process received packets */
        /* get message */
        res = RPHY_GetPayload(&radioRx);
        /* packet received */
        if (res == ERR_OK) {
            /* sniff incoming packet */
            if (RADIO_isSniffing) {
                RPHY_SniffPacket(&radioRx, false);
            }
            /* process incoming packets */
            if (ERR_OK == RPHY_OnPacketRx(&radioRx)) {
#if RNET1_CREATE_EVENTS
                /* it was an ack! */
                if (radioRx.flags & RPHY_PACKET_FLAGS_IS_ACK) {
                    RNET1_OnRadioEvent(RNET1_RADIO_ACK_RECEIVED);
                }
#endif
            }
        } else {
            break; /* no message, get out of for loop */
        }
    }
    
    return ERR_OK;
}

void RADIO_Deinit(void)
{
    /* nothing to do */
}

void RADIO_Init(void)
{
    RADIO_isSniffing = false;
    RADIO_CurrChannel = RADIO_CHANNEL_DEFAULT;
}
