/*********************************************************************
    Filename:       OSAL_Tasks.c
    Revised:        $Date: 2006-04-06 08:19:08 -0700 (Thu, 06 Apr 2006) $
    Revision:       $Revision: 10396 $

    Description:

       This file contains the OSAL Task definition and manipulation
       functions.

    Notes:

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
*********************************************************************/


/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OSAL_Custom.h"


 /*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */


/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// �������
osalTaskRec_t *tasksHead;

osalTaskRec_t *activeTask;

byte taskIDs;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

 /*********************************************************************
 * LOCAL VARIABLES
 */

/*********************************************************************
 * LOCAL FUNCTION PROTOTYPES
 */

/*********************************************************************
 * FUNCTIONS
 *********************************************************************/

/*********************************************************************
 * @fn      osalTaskInit
 *
 * @brief   ��ʼ������ϵͳ
 *
 * @param   none
 *
 * @return
 */
void osalTaskInit( void )
{
  tasksHead = (osalTaskRec_t *)NULL;
  activeTask = (osalTaskRec_t *)NULL;
  taskIDs = 0;
}

/***************************************************************************
 * @fn      osalTaskAdd
 *
 * @brief   ���һ�����������б�. ����������е����ȼ�.
 *
 * @param   none
 *
 * @return
 */
void osalTaskAdd( pTaskInitFn pfnInit,
                  pTaskEventHandlerFn pfnEventProcessor,
                  byte taskPriority)
{
  osalTaskRec_t *newTask;
  osalTaskRec_t *srchTask;
  osalTaskRec_t **ptr;

  newTask = osal_mem_alloc( sizeof( osalTaskRec_t ) );
  if ( newTask )
  {
      // ������
      newTask->pfnInit           = pfnInit;
      newTask->pfnEventProcessor = pfnEventProcessor;
      newTask->taskID            = taskIDs++;
      newTask->taskPriority      = taskPriority;
      newTask->events            = 0;
      newTask->next              = (osalTaskRec_t *)NULL;

      // ��һ����������Ƕ��ģ���ptr����ָ��һ��������ĵ�ַ������������һ����ַ����taskshead��
      ptr      = &tasksHead;
      srchTask = tasksHead;
      while (srchTask)  {
          if (newTask->taskPriority > srchTask->taskPriority)  {
              // Ƕ�뵽����.��������һ����������������
              newTask->next = srchTask;
              *ptr          = newTask;
              return;
          }
          // set 'ptr' to address of the pointer to 'next' in the current
          // (soon to be previous) task control block
          ptr      = &srchTask->next;
          srchTask = srchTask->next;
      }

	  // We're at the end of the current queue. New task is not higher
	  // priority than any other already in the list. Make it the tail.
      // (It is also the head if the queue was initially empty.)
      *ptr = newTask;
  }
  return;
}

/*********************************************************************
 * @fn      osalInitTasks
 *
 * @brief   ���ø�������ĳ�ʼ������
 *
 * @param   none
 *
 * @return  none
 */
void osalInitTasks( void )
{
  // Start at the beginning
  activeTask = tasksHead;

  // Stop at the end
  while ( activeTask )
  {
    if (  activeTask->pfnInit  )
      activeTask->pfnInit( activeTask->taskID );

    activeTask = activeTask->next;
  }

  activeTask = (osalTaskRec_t *)NULL;
}

/*********************************************************************
 * @fn      osalNextActiveTask
 *
 * @brief   ���������������һ�������������
 *
 *
 * @param   none
 *
 * @return  ָ�������¼�������NULLΪû�з����¼�
 */
osalTaskRec_t *osalNextActiveTask( void )
{
  osalTaskRec_t *srchTask;

  // Start at the beginning
  srchTask = tasksHead;

  // When found or not
  while ( srchTask )  {
      if (srchTask->events)  {
		  // �����ȼ�������׼��
          return srchTask;
      }
      srchTask = srchTask->next;
  }
  return NULL;
}


/*********************************************************************
 * @fn      osalFindTask
 *
 * @brief   �������������һ��ָ������ı�ͨ��������ID
 *
 * @param   taskID - task ID to look for
 *
 * @return  pointer to the found task, NULL if not found
 */
osalTaskRec_t *osalFindTask( byte taskID )
{
  osalTaskRec_t *srchTask;

  // Start at the beginning
  srchTask = tasksHead;

  // When found or not
  while ( srchTask )
  {
    // Look for any activity
    if (  srchTask->taskID == taskID  )
      return ( srchTask );

    srchTask = srchTask->next;
  }

  return ( (osalTaskRec_t *)NULL );
}

/*********************************************************************
*********************************************************************/
