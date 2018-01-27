/**
 * \file
 * \brief This is a configuration file for the RNet stack
 * \author (c) 2013 Erich Styger, http://mcuoneclipse.com/
 * \note MIT License (http://opensource.org/licenses/mit-license.html)
 *
 * Here the stack can be configured using macros.
 */

#ifndef __RNET_APP_CONFIG__
#define __RNET_APP_CONFIG__

// #include "Platform.h"

// #if PL_CONFIG_HAS_RADIO
/*! type ID's for application messages */
#define ADDRESS_SIGNALS	(0x12)
typedef enum {
  RAPP_MSG_TYPE_STDIN = 0x00,
  RAPP_MSG_TYPE_STDOUT = 0x01,
  RAPP_MSG_TYPE_STDERR = 0x02,
  RAPP_MSG_TYPE_ACCEL = 0x03,
  RAPP_MSG_TYPE_DATA = 0x04,
  RAPP_MSG_TYPE_JOYSTICK_XY = 0x05,
  RAPP_MSG_TYPE_PLAY_TUNE = 0x06,
  RAPP_MSG_TYPE_INC_SPD = 0x07,
  RAPP_MSG_TYPE_DEC_SPD = 0x08,
  RAPP_MSG_TYPE_ZERO_DIR = 0x09,
  RAPP_MSG_TYPE_INC_LEFT = 0x10,
  RAPP_MSG_TYPE_INC_RIGHT = 0x11,
  RAPP_MSG_TYPE_TURN_LEFT = 0x12,
  RAPP_MSG_TYPE_TURN_RIGHT = 0x13,
  RAPP_MSG_TYPE_STOP = 0x14,
  RAPP_MSG_TYPE_EXIT_STOP = 0x15,
  RAPP_MSG_TYPE_START_DRIVE = 0x16,
  RAPP_MSG_TYPE_SEARCH_START = 0x17,
  RAPP_MSG_TYPE_SEND_SPEED = 0x18,
  RAPP_MSG_TYPE_GET_SPEED = 0x19,
  RAPP_MSG_TYPE_CALIB_LINE = 0x20,
  RAPP_MSG_TYPE_SIGNALS = 0xAC,
  RAPP_MSG_TYPE_JOYSTICK_BTN = 0x54, /* Joystick button message (data is one byte: 'A', 'B', ... 'F' and 'K') */
  /* \todo extend with your own messages */
} RAPP_MSG_Type;


// #endif /* PL_CONFIG_HAS_RADIO */

#endif /* __RNET_APP_CONFIG__ */

/* [] END OF FILE */
