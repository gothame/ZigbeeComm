/**************************************************************************************************
    Filename:
    Revised:        $Date: 2007-03-22 20:35:43 -0700 (Thu, 22 Mar 2007) $
    Revision:       $Revision: 13832 $

    Description:

    Describe the purpose and contents of the file.

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
**************************************************************************************************/


/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

/* hal */
#include "hal_defs.h"
#include "hal_types.h"

/* exported low-level */
#include "mac_low_level.h"

/* low-level specific */
#include "mac_rx_onoff.h"
#include "mac_rx.h"
#include "mac_tx.h"

/* target specific */
#include "mac_radio_defs.h"

/* debug */
#include "mac_assert.h"


/* ------------------------------------------------------------------------------------------------
 *                                         Global Variables
 * ------------------------------------------------------------------------------------------------
 */
uint8 macRxOnFlag;
uint8 macRxEnableFlags;


/**************************************************************************************************
 * @fn          macRxOnOffInit
 *
 * @brief       Initialize variables for rx on/off module.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void macRxOnOffInit(void)
{
  macRxEnableFlags = 0;
  macRxOnFlag = 0;
}


/**************************************************************************************************
 * @fn          macRxEnable
 *
 * @brief       Set enable flags and then turn on receiver.
 *
 * @param       flags - byte containing rx enable flags to set
 *
 * @return      none
 **************************************************************************************************
 */
void macRxEnable(uint8 flags)
{
  halIntState_t  s;

  MAC_ASSERT(flags != 0); /* rx flags not affected */

  /* set enable flags and then turn on receiver */
  HAL_ENTER_CRITICAL_SECTION(s);
  macRxEnableFlags |= flags;
  macRxOn();
  HAL_EXIT_CRITICAL_SECTION(s);
}


/**************************************************************************************************
 * @fn          macRxSoftEnable
 *
 * @brief       Set enable flags but don't turn on the receiver.  Useful to leave the receiver
 *              on after a transmit, but without turning it on immediately.
 *
 * @param       flags - byte containing rx enable flags to set
 *
 * @return      none
 **************************************************************************************************
 */
void macRxSoftEnable(uint8 flags)
{
  halIntState_t  s;

  MAC_ASSERT(flags != 0); /* rx flags not affected */

  /* set the enable flags but do not turn on the receiver */
  HAL_ENTER_CRITICAL_SECTION(s);
  macRxEnableFlags |= flags;
  HAL_EXIT_CRITICAL_SECTION(s);
}


/**************************************************************************************************
 * @fn          macRxDisable
 *
 * @brief       Clear indicated rx enable flags.  If all flags are clear, turn off receiver
 *              unless there is an active receive or transmit.
 *
 * @param       flags - byte containg rx enable flags to clear
 *
 * @return      none
 **************************************************************************************************
 */
void macRxDisable(uint8 flags)
{
  halIntState_t  s;

  MAC_ASSERT(flags != 0); /* rx flags not affected */

  /* clear the indicated flags */
  HAL_ENTER_CRITICAL_SECTION(s);
  macRxEnableFlags &= ~flags;
  HAL_EXIT_CRITICAL_SECTION(s);

  /* turn off the radio if it is allowed */
  macRxOffRequest();
}


/**************************************************************************************************
 * @fn          macRxHardDisable
 *
 * @brief       Clear all enable flags and turn off receiver.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void macRxHardDisable(void)
{
  halIntState_t  s;

  HAL_ENTER_CRITICAL_SECTION(s);

  macRxEnableFlags = 0;
  macRxOnFlag = 0;

  /* force receiver off */
  MAC_RADIO_RXTX_OFF();
  MAC_RADIO_FLUSH_RX_FIFO();
  MAC_DEBUG_TURN_OFF_RX_LED();

  HAL_EXIT_CRITICAL_SECTION(s);

  /* clean up after being forced off */
  macRxHaltCleanup();
}


/**************************************************************************************************
 * @fn          macRxOnRequest
 *
 * @brief       Turn on the receiver if any rx enable flag is set.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void macRxOnRequest(void)
{
  halIntState_t  s;

  HAL_ENTER_CRITICAL_SECTION(s);
  if (macRxEnableFlags)
  {
    macRxOn();
  }
  HAL_EXIT_CRITICAL_SECTION(s);
}


/**************************************************************************************************
 * @fn          macRxOffRequest
 *
 * @brief       Turn off receiver if permitted.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void macRxOffRequest(void)
{
  halIntState_t  s;

  HAL_ENTER_CRITICAL_SECTION(s);
  if (!macRxEnableFlags)
  {
    if (!MAC_RX_IS_PHYSICALLY_ACTIVE() && !MAC_TX_IS_PHYSICALLY_ACTIVE())
    {
      macRxOff();
    }
  }
  HAL_EXIT_CRITICAL_SECTION(s);
}


/**************************************************************************************************
 * @fn          macRxOn
 *
 * @brief       Turn on the receiver if it's not already on.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void macRxOn(void)
{
  halIntState_t  s;

  HAL_ENTER_CRITICAL_SECTION(s);
  if (!macRxOnFlag)
  {
    macRxOnFlag = 1;
    MAC_RADIO_RX_ON();
    MAC_DEBUG_TURN_ON_RX_LED();
  }
  HAL_EXIT_CRITICAL_SECTION(s);
}


/**************************************************************************************************
 * @fn          macRxOff
 *
 * @brief       Turn off the receiver if it's not already off.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void macRxOff(void)
{
  halIntState_t  s;

  HAL_ENTER_CRITICAL_SECTION(s);
  if (macRxOnFlag)
  {
    macRxOnFlag = 0;
    MAC_RADIO_RXTX_OFF();
    MAC_DEBUG_TURN_OFF_RX_LED();
    
    /* just in case a receive was about to start, flush the receive FIFO */
    MAC_RADIO_FLUSH_RX_FIFO();

    /* clear any receive interrupt that happened to squeak through */
    MAC_RADIO_CLEAR_RX_THRESHOLD_INTERRUPT_FLAG();

  }
  HAL_EXIT_CRITICAL_SECTION(s);
}


/**************************************************************************************************
*/
