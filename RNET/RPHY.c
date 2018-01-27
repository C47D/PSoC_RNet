/**
 * \file
 * \brief This implements the PHY layer of the network stack.
 * \author (c) 2013-2014 Erich Styger, http://mcuoneclipse.com/
 * \note MIT License (http://opensource.org/licenses/mit-license.html), see
 * 'RNet_License.txt'
 *
 * This module is used to process the raw payload packets.
 */

#include "RPHY.h"
#include "RMAC.h"
#include "RMSG.h"
#include "RNetConf.h"

uint8_t RPHY_FlushRxQueue(void)
{
    return RMSG_FlushRxQueue();
}

uint8_t RPHY_FlushTxQueue(void)
{
    return RMSG_FlushTxQueue();
}

uint8_t RPHY_GetPayload(RPHY_PacketDesc *packet)
{
    packet->flags = RPHY_PACKET_FLAGS_NONE;
    
    /* ERR_OK, ERR_OVERFLOW or ERR_RXEMPTY */
    return RMSG_GetRxMsg(packet->phyData, packet->phySize);
}

uint8_t RPHY_OnPacketRx(RPHY_PacketDesc *packet)
{
    /* pass message up the stack */
    return RMAC_OnPacketRx(packet);
}

uint8_t RPHY_PutPayload(uint8_t *buf, size_t bufSize, uint8_t payloadSize,
                        RPHY_FlagsType flags)
{
    return RMSG_QueueTxMsg(buf, bufSize, payloadSize, flags);
}

void RPHY_SniffPacket(RPHY_PacketDesc *packet, bool isTx)
{
    RMAC_SniffPacket(packet, isTx);
}

void RPHY_Deinit(void)
{
    /* nothing needed */
}

void RPHY_Init(void)
{
    /* nothing needed */
}
