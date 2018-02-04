#include "RNet_App.h"

#include "RApp.h"
#include "RNet_App.h"
#include "RPHY.h"
#include "RStack.h"
#include "Radio.h"

#include "Error.h"

#include "FreeRTOS.h"
#include "task.h"

enum {
    LED_ON,
    LED_OFF,
};

typedef enum {
    RNETA_INITIAL, /* initialization state */
    RNETA_POWERUP, /* powered up the transceiver */
    RNETA_TX_RX    /* ready to send and receive data */
} RNETA_State;

static RNETA_State appState = RNETA_INITIAL;

static uint8_t
RNETA_HandleRxMessage(RAPP_MSG_Type type, uint8_t size, uint8_t *data,
                      RNWK_ShortAddrType srcAddr, bool *handled,
                      RPHY_PacketDesc *packet)
{
    (void)size;
    (void)data;
    (void)srcAddr;
    (void)packet;
    
    switch (type) {
    case RAPP_MSG_TYPE_PING: /* <type><size><data */
        *handled = true;
        LED_G_Write(LED_ON);
        vTaskDelay(pdMS_TO_TICKS(20));
        LED_G_Write(LED_OFF);
        return ERR_OK;
    default:
        break;
    }
    
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
    
    if (xTime < pdMS_TO_TICKS(150)) {
        /* not powered for 100 ms:
        * wait until we can access the radio transceiver */
        xTime = pdMS_TO_TICKS(150) - xTime; /* remaining ticks to wait */
        
        vTaskDelay(xTime);
    }
    
    /* enable the transceiver */
    RADIO_PowerUp();
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
            RADIO_Process();
            break;

        default:
            break;
        }
        break;
    }
}

static portTASK_FUNCTION(RNetTask, pvParameters)
{
    (void)pvParameters;
    
    uint32_t cntr = 0;
    uint8_t msgCntr = 0;
    appState = RNETA_INITIAL; /* initialize state machine state */
    
    /* set a default address */
    // always return ERR_OK so no need to check for it
    RAPP_SetThisNodeAddr(RNWK_ADDR_BROADCAST);
        
    while (1) {
        
        Process(); /* process state machine */
        cntr++;
        
        /* with an RTOS 10 ms/100 Hz tick rate, this is every second */
        if (100 == cntr) {
            //LED3_On(); /* blink blue LED for 20 ms */
            LED_B_Write(LED_ON);
            
            RAPP_SendPayloadDataBlock(&msgCntr, sizeof(msgCntr),
                                      RAPP_MSG_TYPE_PING, RNWK_ADDR_BROADCAST,
                                      RPHY_PACKET_FLAGS_NONE);
            msgCntr++;
            cntr = 0;
            
            //vTaskDelay(20 / portTICK_RATE_MS);
            vTaskDelay(pdMS_TO_TICKS(20));
            //LED3_Off(); /* blink blue LED */
            LED_B_Write(LED_OFF);
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
        //FRTOS1_vTaskDelay(10 / portTICK_RATE_MS);
    }
}

void RNETApp_Init(void)
{
    BaseType_t rnet_task_base;
    
    /* initialize stack */
    RSTACK_Init();
    
    /* SetMessageHandlerTable always returns ERR_OK so no need to check it */
    RAPP_SetMessageHandlerTable(handlerTable);

    rnet_task_base = xTaskCreate(RNetTask,
        "RNet",
        configMINIMAL_STACK_SIZE,
        (void *) NULL,
        tskIDLE_PRIORITY,
        (TaskHandle_t *) NULL);
    
    if (pdPASS != rnet_task_base){
        while (1) {}
    }
    
}

/* [] END OF FILE */
