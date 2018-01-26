/**
 * \file
 * \brief Configuration header file for RNet
 *
 * This header file is used to configure settings RNet module.
 */
#ifndef __RNET1_CONFIG_H
#define __RNET1_CONFIG_H

/* remote standard I/O destination address */
#ifndef RSTDIO_CONFIG_SETTING_RSTDIO_DEFAULT_DESTINATION_ADDRESS
  #define RSTDIO_CONFIG_SETTING_RSTDIO_DEFAULT_DESTINATION_ADDRESS   RNWK_ADDR_BROADCAST
#endif

#endif /* __RNET1_CONFIG_H */
