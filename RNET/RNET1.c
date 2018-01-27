#include "RNET1.h"
#include "RStack.h"
#include "Radio.h"
#include "RNWK.h"
#include "RMSG.h"
#include "RStdIO.h"


/*
** ===================================================================
**     Method      :  RNET1_OnInterrupt (component RNet)
**
**     Description :
**         This method is internal. It is used by Processor Expert only.
** ===================================================================
*/
void RF1_OnInterrupt(void)
{
  /* Write your code here ... */
}

/*
** ===================================================================
**     Method      :  RNET1_Init (component RNet)
**     Description :
**         Initializes the RNet Stack
**     Parameters  : None
**     Returns     : Nothing
** ===================================================================
*/
void RNET1_Init(void)
{
  RSTACK_Init();
}

/*
** ===================================================================
**     Method      :  RNET1_Deinit (component RNet)
**     Description :
**         Deinitializes the RNet Stack
**     Parameters  : None
**     Returns     : Nothing
** ===================================================================
*/
void RNET1_Deinit(void)
{
  RSTACK_Deinit();
}

/*
** ===================================================================
**     Method      :  RNET1_Process (component RNet)
**     Description :
**         Processes the Radio Rx and Tx messages
**     Parameters  : None
**     Returns     :
**         ---             - Error code
** ===================================================================
*/
uint8_t RNET1_Process(void)
{
  return RADIO_Process();
}

/*
** ===================================================================
**     Method      :  RNET1_PowerUp (component RNet)
**     Description :
**         Initializes and powers the radio up.
**     Parameters  : None
**     Returns     :
**         ---             - Error code
** ===================================================================
*/
uint8_t RNET1_PowerUp(void)
{
  return RADIO_PowerUp();
}

/*
** ===================================================================
**     Method      :  RNET1_SetChannel (component RNet)
**     Description :
**         Sets the radio channel
**     Parameters  :
**         NAME            - DESCRIPTION
**         channel         - Channel number
**     Returns     :
**         ---             - Error code
** ===================================================================
*/
uint8_t RNET1_SetChannel(uint8_t channel)
{
  return RADIO_SetChannel(channel);
}

/* END RNET1. */
