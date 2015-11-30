/*
 * keyboard.c
 *
 *  Created on: 07/08/2015
 *      Author: LAfonso01
 */

/******************************************************************************
Includes   <System Includes> , "Project Includes"
 ******************************************************************************/
#include "keyboard.h"
#include "platform.h"
#include "lcd_menu.h"

#ifdef FREE_RTOS_PP
#include "FreeRTOS.h"
#include "task.h"
#endif
/*****************************************************************************
Macro definitions
 ******************************************************************************/


/******************************************************************************
Section    <Section Definition> , "Data Sections"
 ******************************************************************************/

/******************************************************************************
Private global variables and functions
 ******************************************************************************/


/******************************************************************************
External variables and functions
 ******************************************************************************/


/*****************************************************************************
Enumerated Types
 ******************************************************************************/

/******************************************************************************
Section    <Section Definition> , "Project Sections"
 ******************************************************************************/

/******************************************************************************
Function Name   : keyboard_task
Description     : keyboard scan Task process
Arguments       : none
Return value    : none
 ******************************************************************************/
void keyboard_task(void)
{
	uint8_t key_buf = 0;
	uint8_t keyBounce = 0;

	while(1)
	{
		vTaskDelay(30 / portTICK_RATE_MS);
		key_buf = (uint8_t)(SW1|0xFE);
		key_buf &= (uint8_t)(SW2<<1)|0xFD;
		key_buf &= (uint8_t)(SW3<<2)|0xFB;
		key_buf = ~key_buf;
		if (key_buf > 0)
		{
			keyBounce++;
		}
		else
		{
			keyBounce = 0;
		}
		if (keyBounce == 5)
		{
			keyBounce = 0;
            xQueueSend( qKeyboard, &key_buf, 0 );
		}

	}
}
