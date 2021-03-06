/**************************************************************************************************
    Filename:
    Revised:        $Date: 2007-03-07 16:52:39 -0800 (Wed, 07 Mar 2007) $
    Revision:       $Revision: 13715 $

    Description:

    Describe the purpose and contents of the file.

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
**************************************************************************************************/

#ifndef MAC_RX_ONOFF_H
#define MAC_RX_ONOFF_H


/* ------------------------------------------------------------------------------------------------
 *                                             Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "hal_defs.h"
#include "hal_types.h"

/* ------------------------------------------------------------------------------------------------
 *                                          Prototypes
 * ------------------------------------------------------------------------------------------------
 */
void macRxOnOffInit(void);
void macRxOnRequest(void);
void macRxOffRequest(void);
void macRxOn(void);
void macRxOff(void);


/* ------------------------------------------------------------------------------------------------
 *                                         Global Variables
 * ------------------------------------------------------------------------------------------------
 */
extern uint8 macRxOnFlag;
extern uint8 macRxEnableFlags;


/* ------------------------------------------------------------------------------------------------
 *                                            Macros
 * ------------------------------------------------------------------------------------------------
 */

/* debug macros */
#ifdef MAC_RX_ONOFF_DEBUG_LED
#include "hal_board.h"
#define MAC_DEBUG_TURN_ON_RX_LED()    HAL_TURN_ON_LED2()
#define MAC_DEBUG_TURN_OFF_RX_LED()   HAL_TURN_OFF_LED2()
#else
#define MAC_DEBUG_TURN_ON_RX_LED()
#define MAC_DEBUG_TURN_OFF_RX_LED()
#endif

/* interface macros */
#define MAC_RX_WAS_FORCED_OFF()     st( macRxOnFlag = 0; MAC_DEBUG_TURN_OFF_RX_LED(); )
#define MAC_RX_WAS_FORCED_ON()      st( macRxOnFlag = 1;  MAC_DEBUG_TURN_ON_RX_LED(); )


/**************************************************************************************************
 */
#endif
