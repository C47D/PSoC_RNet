#include "RNet_App.h"

#include "RApp.h"
#include "RNet_App.h"
#include "RPHY.h"
#include "RStack.h"
#include "Radio.h"

#include "Error.h"

#include "FreeRTOS.h"
#include "task.h"

typedef enum {
    RNETA_INITIAL, /* initialization state */
    RNETA_POWERUP, /* powered up the transceiver */
    RNETA_TX_RX    /* ready to send and receive data */
} RNETA_State;

static RNETA_State appState = RNETA_INITIAL;

static uint8_t RNETA_HandleRxMessage(RAPP_MSG_Type type, uint8_t size,
                                     uint8_t *data, RNWK_ShortAddrType srcAddr,
                                     bool *handled, RPHY_PacketDesc *packet)
{
    (void)size;
    (void)data;
    (void)srcAddr;
    (void)packet;
    
    switch (type) {
    case RAPP_MSG_TYPE_PING: /* <type><size><data */
        *handled = true;
        /* to be defined: do something with the ping, e.g blink a LED */
        //LED2_On(); /* green LED blink */
        vTaskDelay(20 / portTICK_RATE_MS);
        //LED2_Off();
        return ERR_OK;
    default:
        break;
    } /* switch */
    return ERR_OK;
}

static const RAPP_MsgHandler handlerTable[] = {
    RNETA_HandleRxMessage,
    NULL /* sentinel */
};

static void RadioPowerUp(void)
{
    /* need to ensure that we wait at least 100 ms (I use 150 ms here) after
     * power-on of the transceiver */
    TickType_t xTime;

    xTime = xTaskGetTickCount();
    if (xTime < (150 / portTICK_RATE_MS)) {
        /* not powered for 100 ms: wait until we can access the radio
         * transceiver */
        xTime = (150 / portTICK_RATE_MS) - xTime; /* remaining ticks to wait */
        vTaskDelay(xTime);
    }
    
    /* enable the transceiver */
    (void)RADIO_PowerUp();
}

static void Process(void)
{
    while (1) {
        switch (appState) {
        case RNETA_INITIAL:
            appState = RNETA_POWERUP;
            continue;

        case RNETA_POWERUP:
            RadioPowerUp();
            appState = RNETA_TX_RX;
            break;

        case RNETA_TX_RX:
            (void)RADIO_Process();
            break;

        default:
            break;
        }      /* switch */
        break; /* break for loop */
    }          /* for */
}

static portTASK_FUNCTION(RNetTask, pvParameters)
{
    (void)pvParameters; /* not used */
    
    uint32_t cntr = 0;
    uint8_t msgCntr = 0;
    appState = RNETA_INITIAL; /* initialize state machine state */
    
    /* set a default address */
    if (ERR_OK != RAPP_SetThisNodeAddr(RNWK_ADDR_BROADCAST)) {
        /* "ERR: Failed setting node address" */
        while (1) {
        }
    }
        
    while (1) {
        /* process state machine */
        Process();
        cntr++;
        
        /* with an RTOS 10 ms/100 Hz tick rate, this is every second */
        if (100 == cntr) {
            //LED3_On(); /* blink blue LED for 20 ms */
            RAPP_SendPayloadDataBlock(&msgCntr, sizeof(msgCntr),
                                      RAPP_MSG_TYPE_PING, RNWK_ADDR_BROADCAST,
                                      RPHY_PACKET_FLAGS_NONE);
            msgCntr++;
            cntr = 0;
            //vTaskDelay(20 / portTICK_RATE_MS);
            vTaskDelay(pdMS_TO_TICKS(20));
            //LED3_Off(); /* blink blue LED */
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
        //FRTOS1_vTaskDelay(10 / portTICK_RATE_MS);
    }
}

void RNETA_Init(void)
{
    /* initialize stack */
    RSTACK_Init();
    
    /* assign application message handler */
    if (ERR_OK != RAPP_SetMessageHandlerTable(handlerTable)) {
        /* "ERR: failed setting message handler!" */
        while (1) {
        }
    }
        
    if (xTaskCreate(
            RNetTask, /* pointer to the task */
            "RNet",   /* task name for kernel awareness debugging */
            configMINIMAL_STACK_SIZE, /* task stack size */
            (void *)NULL,             /* optional task startup argument */
            tskIDLE_PRIORITY,         /* initial priority */
            (TaskHandle_t *)NULL       /* optional task handle to create */
            ) != pdPASS) {
                
        while (1) {
        }
    }
}

/* [] END OF FILE */
