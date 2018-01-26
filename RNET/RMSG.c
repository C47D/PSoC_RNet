/**
 * \file
 * \brief This implements a queue/buffer for radio messages
 * \author (c) 2013-2014 Erich Styger, http://mcuoneclipse.com/
 * \note MIT License (http://opensource.org/licenses/mit-license.html), see 'RNet_License.txt'
 *
 * This module uses queues to retrieve and store radio messages.
 */

#include "RNetConf.h"
#include "RMSG.h"
#include "FRTOS1.h"
#include "RPHY.h"

/* Configuration for tx and rx queues */
#define RMSG_QUEUE_RX_NOF_ITEMS   (RNET_CONFIG_MSG_QUEUE_NOF_RX_ITEMS) /* number of items in the queue */
#define RMSG_QUEUE_TX_NOF_ITEMS   (RNET_CONFIG_MSG_QUEUE_NOF_TX_ITEMS) /* number of items in the queue */
#define RMSG_QUEUE_PUT_WAIT       (RNET_CONFIG_MSG_QUEUE_PUT_BLOCK_TIME_MS) /* blocking time for putting messages into queue */

static xQueueHandle RMSG_MsgRxQueue, RMSG_MsgTxQueue; /* queue for messages,  format is: kind(8bit) dataSize(8bit) data */

unsigned int RMSG_RxQueueNofItems(void) {
  return (unsigned int)FRTOS1_uxQueueMessagesWaiting(RMSG_MsgRxQueue);
}

unsigned int RMSG_TxQueueNofItems(void) {
  return (unsigned int)FRTOS1_uxQueueMessagesWaiting(RMSG_MsgTxQueue);
}

uint8_t RMSG_FlushRxQueue(void) {
  if (FRTOS1_xQueueReset(RMSG_MsgRxQueue)!=pdPASS) {
    return ERR_FAILED;
  }
  return ERR_OK;
}

uint8_t RMSG_FlushTxQueue(void) {
  if (FRTOS1_xQueueReset(RMSG_MsgTxQueue)!=pdPASS) {
    return ERR_FAILED;
  }
  return ERR_OK;
}

uint8_t RMSG_QueuePut(uint8_t *buf, size_t bufSize, uint8_t payloadSize, bool fromISR, bool isTx, bool toBack, RPHY_FlagsType flags) {
  /* data format is: dataSize(8bit) data */
  uint8_t res = ERR_OK;
  xQueueHandle queue;
  BaseType_t qRes;

  if (payloadSize>RPHY_PAYLOAD_SIZE) {
    return ERR_OVERFLOW; /* more data than can fit into payload! */
  }
  if (bufSize!=RPHY_BUFFER_SIZE) {
    return ERR_FAILED; /* must be exactly this buffer size!!! */
  }
  if (isTx) {
    queue = RMSG_MsgTxQueue;
  } else {
    queue = RMSG_MsgRxQueue;
  }
  RPHY_BUF_FLAGS(buf) = flags;
  RPHY_BUF_SIZE(buf) = payloadSize;
  if (fromISR) {
    signed portBASE_TYPE pxHigherPriorityTaskWoken;
    
    if (toBack) {
      qRes = FRTOS1_xQueueSendToBackFromISR(queue, buf, &pxHigherPriorityTaskWoken);
    } else {
      qRes = FRTOS1_xQueueSendToFrontFromISR(queue, buf, &pxHigherPriorityTaskWoken);
    }
    if (qRes!=pdTRUE) {
      /* was not able to send to the queue. Well, not much we can do here... */
      res = ERR_BUSY;
    }
  } else {
    if (toBack) {
      qRes = FRTOS1_xQueueSendToBack(queue, buf, RMSG_QUEUE_PUT_WAIT);
    } else {
      qRes = FRTOS1_xQueueSendToFront(queue, buf, RMSG_QUEUE_PUT_WAIT);
    }
    if (qRes!=pdTRUE) {
      res = ERR_BUSY;
    }
  }
  return res;
}

uint8_t RMSG_PutRetryTxMsg(uint8_t *buf, size_t bufSize) {
  if (bufSize<RPHY_BUFFER_SIZE) {
    return ERR_OVERFLOW; /* not enough space in buffer */
  }
  if (FRTOS1_xQueueSendToFront(RMSG_MsgTxQueue, buf, 0)==pdPASS) {
    /* received message from queue */
    return ERR_OK;
  }
  return ERR_RXEMPTY;
}

uint8_t RMSG_GetTxMsg(uint8_t *buf, size_t bufSize) {
  if (bufSize<RPHY_BUFFER_SIZE) {
    return ERR_OVERFLOW; /* not enough space in buffer */
  }
  if (FRTOS1_xQueueReceive(RMSG_MsgTxQueue, buf, 0)==pdPASS) {
    /* received message from queue */
    return ERR_OK;
  }
  return ERR_RXEMPTY;
}

uint8_t RMSG_GetRxMsg(uint8_t *buf, size_t bufSize) {
  /* first byte in the queue is the size of the item */
  if (bufSize<RPHY_BUFFER_SIZE) {
    return ERR_OVERFLOW; /* not enough space in buffer */
  }
  if (FRTOS1_xQueueReceive(RMSG_MsgRxQueue, buf, 0)==pdPASS) { /* immediately returns if queue is empty */
    /* received message from queue */
    return ERR_OK;
  }
  return ERR_RXEMPTY;
}

uint8_t RMSG_QueueTxMsg(uint8_t *buf, size_t bufSize, uint8_t payloadSize, RPHY_FlagsType flags) {
  return RMSG_QueuePut(buf, bufSize, payloadSize, FALSE, TRUE, TRUE, flags);
}

uint8_t RMSG_QueueRxMsg(uint8_t *buf, size_t bufSize, uint8_t payloadSize, RPHY_FlagsType flags) {
  return RMSG_QueuePut(buf, bufSize, payloadSize, FALSE, FALSE, TRUE, flags);
}


void RMSG_Deinit(void) {
  FRTOS1_vQueueUnregisterQueue(RMSG_MsgRxQueue);
  FRTOS1_vQueueDelete(RMSG_MsgRxQueue);
  RMSG_MsgRxQueue = NULL;

  FRTOS1_vQueueUnregisterQueue(RMSG_MsgTxQueue);
  FRTOS1_vQueueDelete(RMSG_MsgTxQueue);
  RMSG_MsgTxQueue = NULL;
}

void RMSG_Init(void) {
  RMSG_MsgRxQueue = FRTOS1_xQueueCreate(RMSG_QUEUE_RX_NOF_ITEMS, RPHY_BUFFER_SIZE);
  if (RMSG_MsgRxQueue==NULL) { /* queue creation failed! */
    for(;;) {} /* not enough memory? */
  }
  FRTOS1_vQueueAddToRegistry(RMSG_MsgRxQueue, "RadioRxMsg");
#if configUSE_TRACE_HOOKS
  vTraceSetQueueName(RMSG_MsgRxQueue, "RadioRxMsg");
#endif

  RMSG_MsgTxQueue = FRTOS1_xQueueCreate(RMSG_QUEUE_TX_NOF_ITEMS, RPHY_BUFFER_SIZE);
  if (RMSG_MsgTxQueue==NULL) { /* queue creation failed! */
    for(;;) {} /* not enough memory? */
  }
  FRTOS1_vQueueAddToRegistry(RMSG_MsgTxQueue, "RadioTxMsg");
#if configUSE_TRACE_HOOKS
  vTraceSetQueueName(RMSG_MsgTxQueue, "RadioTxMsg");
#endif
}

