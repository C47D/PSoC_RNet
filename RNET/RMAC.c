/**
 * \file
 * \brief This is the implementation of the Radio MAC part.
 * \author (c) 2013-2014 Erich Styger, http://mcuoneclipse.com/
 * \note MIT License (http://opensource.org/licenses/mit-license.html), see
 * 'RNet_License.txt'
 *
 * This module encapsulates the Media Access of network stack.
 */

#include "RMAC.h"
#include "RNWK.h"
#include "RNetConf.h"
#include "RPHY.h"

static uint8_t RMAC_SeqNr = 0;
static uint8_t RMAC_ExpectedAckSeqNr;

static void strcat(uint8_t *dst, size_t dstSize, const unsigned char *src)
{
    dstSize--; /* for zero byte */
    /* point to the end of the source */
    while (dstSize > 0 && *dst != '\0') {
        dst++;
        dstSize--;
    }
    /* copy the src in the destination */
    while (dstSize > 0 && *src != '\0') {
        *dst++ = *src++;
        dstSize--;
    }
    /* terminate the string */
    *dst = '\0';
}

static void chcat(uint8_t *dst, size_t dstSize, uint8_t ch)
{
    dstSize--; /* for zero byte */
    /* point to the end of the source */
    while (dstSize > 0 && *dst != '\0') {
        dst++;
        dstSize--;
    }
    /* copy the ch in the destination */
    if (dstSize > 0) {
        *dst++ = ch;
    }
    /* terminate the string */
    *dst = '\0';
}

uint8_t RMAC_PutPayload(uint8_t *buf, size_t bufSize, uint8_t payloadSize,
                        RPHY_FlagsType flags)
{
    if (flags & RPHY_PACKET_FLAGS_REQ_ACK) {
        RMAC_BUF_TYPE(buf) = (uint8_t)((uint8_t)RMAC_MSG_TYPE_DATA |
                                       (uint8_t)RMAC_MSG_TYPE_REQ_ACK);
    } else {
        RMAC_BUF_TYPE(buf) = (uint8_t)RMAC_MSG_TYPE_DATA;
    }
    
    RMAC_ExpectedAckSeqNr = RMAC_SeqNr;
    RMAC_BUF_SEQN(buf) = RMAC_SeqNr++;
    
    return RPHY_PutPayload(buf, bufSize, payloadSize + RMAC_HEADER_SIZE, flags);
}

uint8_t RMAC_OnPacketRx(RPHY_PacketDesc *packet)
{
    /* pass data packet up the stack */
    return RNWK_OnPacketRx(packet);
}

uint8_t RMAC_SendACK(RPHY_PacketDesc *rxPacket, RPHY_PacketDesc *ackPacket)
{
    /* set type to ack */
    RMAC_BUF_TYPE(ackPacket->phyData) = (uint8_t)RMAC_MSG_TYPE_ACK;
    RMAC_BUF_SEQN(ackPacket->phyData) = RMAC_BUF_SEQN(rxPacket->phyData);
    
    /* use same sequence number as in the received package, so no change */
    return RPHY_PutPayload(ackPacket->phyData, ackPacket->phySize,
                           RMAC_HEADER_SIZE + RNWK_HEADER_SIZE,
                           RPHY_PACKET_FLAGS_NONE);
}

RMAC_MsgType RMAC_GetType(uint8_t *buf, size_t bufSize)
{
    (void)bufSize; /* not used */
    
    return (RMAC_MsgType)RMAC_BUF_TYPE(buf);
}

bool RMAC_IsExpectedACK(uint8_t *buf, size_t bufSize)
{
    (void)bufSize; /* not used */
    
    return RMAC_BUF_SEQN(buf) == RMAC_ExpectedAckSeqNr;
}

void RMAC_SniffPacket(RPHY_PacketDesc *packet, bool isTx)
{
    RNWK_SniffPacket(packet, isTx);
}

void RMAC_DecodeType(uint8_t *buf, size_t bufSize, RPHY_PacketDesc *packet)
{
    RMAC_MsgType type;
    bool first = true;

    type = (RMAC_MsgType)RMAC_BUF_TYPE(packet->phyData);
    buf[0] = '\0';
    //UTIL1_chcat(buf, bufSize, '(');
    chcat(buf, bufSize, '(');
    
    if ((uint8_t)type & (uint8_t)RMAC_MSG_TYPE_REQ_ACK) {
        //UTIL1_strcat(buf, bufSize, (unsigned char *)"RACK");
        strcat(buf, bufSize, (unsigned char *)"RACK");
        first = true;
    } else {
        //UTIL1_strcat(buf, bufSize, (unsigned char *)"NACK");
        strcat(buf, bufSize, (unsigned char *)"NACK");
        first = false;
    }
    if ((uint8_t)type & (uint8_t)RMAC_MSG_TYPE_DATA) {
        if (!first) {
            //UTIL1_chcat(buf, bufSize, '|');
            chcat(buf, bufSize, '|');
        }
        //UTIL1_strcat(buf, bufSize, (unsigned char *)"DATA");
        strcat(buf, bufSize, (unsigned char *)"DATA");
        first = false;
    }
    if ((uint8_t)type & (uint8_t)RMAC_MSG_TYPE_ACK) {
        if (!first) {
            //UTIL1_chcat(buf, bufSize, '|');
            chcat(buf, bufSize, '|');
        }
        //UTIL1_strcat(buf, bufSize, (unsigned char *)"ACK");
        strcat(buf, bufSize, (unsigned char *)"ACK");
    }
    if ((uint8_t)type & (uint8_t)RMAC_MSG_TYPE_CMD) {
        //UTIL1_strcat(buf, bufSize, (unsigned char *)"CMD");
        strcat(buf, bufSize, (unsigned char *)"CMD");
    }
    //UTIL1_chcat(buf, bufSize, ')');
    chcat(buf, bufSize, ')');
}

void RMAC_Deinit(void)
{
    /* nothing needed */
}

void RMAC_Init(void)
{
    RMAC_SeqNr = 0;
    RMAC_ExpectedAckSeqNr = 0;
}
