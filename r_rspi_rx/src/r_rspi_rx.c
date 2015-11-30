/***********************************************************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No 
* other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all 
* applicable laws, including copyright laws. 
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, 
* FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM 
* EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES 
* SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS 
* SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of 
* this software. By using this software, you agree to the additional terms and conditions found by accessing the 
* following link:
* http://www.renesas.com/disclaimer 
*
* Copyright (C) 2011 Renesas Electronics Corporation. All rights reserved.    
***********************************************************************************************************************/
/***********************************************************************************************************************
* File Name    : r_rspi_rx.c
* Device(s)    : RX62x, RX63x, RX210
* Tool-Chain   : Renesas RX Standard Toolchain 1.02
* OS           : None
* H/W Platform : YRDKRX62N, YRDKRX63N, RSK+RX63N, RSK+RX62N, RSKRX210
* Description  : Functions for using RSPI on RX devices. Note that the pin setups done in R_RSPI_Init() are done for 
*                specific boards. Make sure that the initialization code is using the same pins as your own board, or 
*                change it to meet your needs.
************************************************************************************************************************
* History : DD.MM.YYYY Version Description           
*         : 03.08.2011 1.00    First Release            
*         : 30.11.2011 1.10    Added RX63x support
*         : 27.02.2012 1.20    Moved pin setup out of here and into the r_bsp package. Added preprocessor guards for 
*                              boards that do not use all slave selects.
*         : 08.03.2012 1.30    Renamed r_rspi_rx.h to r_rspi_rx_if.h to be compliant with updated FIT spec. Also
*                              added GetVersion() function (though it's really a macro). Updated Select() and Deselect()
*                              functions to take 'device_selected_t' as type for chip select since these are the 
*                              devices supported. Updated Lock() function so that it uses the locking mechanism in 
*                              the r_bsp package.
*          : 20.04.2012 1.40   Added RX210 support. Changed name of module from r_rspi_rx to r_rspi_rx since RX2xx is
*                              now supported.
***********************************************************************************************************************/
/***********************************************************************************************************************
Includes   <System Includes> , "Project Includes"
***********************************************************************************************************************/
/* Fixed width integers */
#include <stdint.h>
/* Boolean defines */
#include <stdbool.h>
/* Used for xchg() intrinsic */
#include <machine.h>
/* Access to peripherals and board defines. */
#include "platform.h"
/* Defines for RSPI support */
#include "r_rspi_rx_if.h"

/***********************************************************************************************************************
Typedef definitions
***********************************************************************************************************************/

/***********************************************************************************************************************
Macro definitions
***********************************************************************************************************************/
/* Number of channels of RSPI this MCU has. */
#if defined(MCU_RX62N) || defined(MCU_RX621)
#define RSPI_NUM_CHANNELS   (2)
#elif defined(MCU_RX63N) || defined(MCU_RX630) || defined(MCU_RX631) || defined(BSP_MCU_RX63N)
#define RSPI_NUM_CHANNELS   (3)
#elif defined(MCU_RX210)
#define RSPI_NUM_CHANNELS   (1)
#else
#error  "ERROR in r_rspi_rx package! Either no MCU chosen or MCU is not supported!"
#endif

#ifndef NULL
#define NULL	0
#endif

/***********************************************************************************************************************
Private global variables and functions
***********************************************************************************************************************/
/* These variables are only needed if locking is enabled in r_rspi_rx_config.h */
#if defined(RSPI_REQUIRE_LOCK)
/* Holds the process currently using the RSPI peripheral */
static uint32_t g_rspi_locked_id[RSPI_NUM_CHANNELS] = 
{
    /* Initialize the array for up to 2 channels. Add more as needed. */
#if   RSPI_NUM_CHANNELS == 1
    NO_DEVICE_SELECTED
#elif RSPI_NUM_CHANNELS == 2
    NO_DEVICE_SELECTED, NO_DEVICE_SELECTED
#elif RSPI_NUM_CHANNELS == 3
    NO_DEVICE_SELECTED, NO_DEVICE_SELECTED, NO_DEVICE_SELECTED
#endif
};

/* Locks for RSPI channels. Having one lock per channel allows operations on one channel to not interfere with 
   operations on another channel. */
bsp_lock_t  g_rspi_locks[RSPI_NUM_CHANNELS];
#endif

/* This variable determines whether the peripheral has already been initialized. */
static bool rspi_initialized[RSPI_NUM_CHANNELS] = 
{
    /* Initialize the array for up to 2 channels. Add more as needed. */
#if   RSPI_NUM_CHANNELS == 1
    false
#elif RSPI_NUM_CHANNELS == 2
    false, false
#elif RSPI_NUM_CHANNELS == 3
    false, false, false
#endif
};

/* Used to prevent having duplicate code for each channel. This only works if the channels are identical (just at 
   different locations in memory). This is easy to tell by looking in iodefine.h and seeing if the same structure
   was used for all channels. */
volatile struct st_rspi __evenaccess * g_rspi_channels[RSPI_NUM_CHANNELS] =
{
/* Initialize the array for up to 2 channels. Add more as needed. */
#if   RSPI_NUM_CHANNELS == 1
    &RSPI0,
#elif RSPI_NUM_CHANNELS == 2
    &RSPI0, &RSPI1
#elif RSPI_NUM_CHANNELS == 3
    &RSPI0, &RSPI1, &RSPI2
#endif
};

/***********************************************************************************************************************
* Function Name: R_RSPI_Init
* Description  : Initializes SPI channel
* Arguments    : channel -
*                    Which channel to initialize
* Return Value : true -
*                    RSPI peripheral initialized correctly
*                false -
*                    Error in initialization
***********************************************************************************************************************/
bool R_RSPI_Init(uint8_t channel)
{
    /* Check channel number. */
    if (channel >= RSPI_NUM_CHANNELS)
    {
        /* Invalid channel. */
        return false;
    }

    /* Initialize SPI channel in SPI master mode, on the 'A' pins, keep all SSLs disabled */       

    /* Check to see if the peripheral has already been initialized. */
    if (true == rspi_initialized[channel])
    {
        /* This channel has already been initialized. */
        return true;
    }

    /* All initialization that is channel specific and cannot be done through the g_rspi_channels[] array should be 
       placed here. Examples include port pins, MSTP, and interrupts. */
    if (0 == channel)
    {
#if defined(MCU_RX62N) || defined(MCU_RX621)

        /* Channel 0 - A pins */
        /* Turn on RSPI0 */
        MSTP(RSPI0) = 0;

        /* Setup ICU for RSPI */
        /* Set shared IPL for RSPI0 */
        IPR(RSPI0, SPRI0) = 3;
        /* Disable receive buffer full interrupt */
        IEN(RSPI0, SPRI0) = 0;
        /* Disable transmit buffer empty interrupt */
        IEN(RSPI0, SPTI0) = 0;
        /* Clear pending receive buffer full interrupts */
        IR(RSPI0, SPRI0) = 0 ;

#elif defined(MCU_RX63N) || defined(MCU_RX630) || defined(MCU_RX631) || defined(MCU_RX210) || defined(BSP_MCU_RX63N)

        /* Channel 0 */

        /* Enable writing to MSTP registers. */
        SYSTEM.PRCR.WORD = 0xA502;

        /* Turn on RSPI0 */
        MSTP(RSPI0) = 0;

        /* Disable writing to MSTP registers. */
        SYSTEM.PRCR.WORD = 0xA500;

        /* Setup ICU for RSPI */
        /* Set shared IPL for RSPI0 */
        IPR(RSPI0, SPRI0) = 3;
        /* Disable receive buffer full interrupt */
        IEN(RSPI0, SPRI0) = 0;
        /* Disable transmit buffer empty interrupt */
        IEN(RSPI0, SPTI0) = 0;
        /* Clear pending receive buffer full interrupts */
        IR(RSPI0, SPRI0) = 0 ;

#endif

    }
#if RSPI_NUM_CHANNELS > 1
    else if (1 == channel)        
    {
        /* Channel 1 */

    #if defined(MCU_RX62N) || defined(MCU_RX621)

        /* Turn on RSPI1 */
        MSTP(RSPI1) = 0;

        /* Setup ICU for RSPI */
        /* Set shared IPL for RSPI1 */
        IPR(RSPI1, SPRI1) = 3;
        /* Disable receive buffer full interrupt */
        IEN(RSPI1, SPRI1) = 0;
        /* Disable transmit buffer empty interrupt */
        IEN(RSPI1, SPTI1) = 0;
        /* Clear pending receive buffer full interrupts */
        IR(RSPI1, SPRI1) = 0 ;

    #elif defined(MCU_RX63N) || defined(MCU_RX630) || defined(MCU_RX631) || defined(MCU_RX210) || defined(BSP_MCU_RX63N)

        /* Enable writing to MSTP registers. */
        SYSTEM.PRCR.WORD = 0xA502;

        /* Turn on RSPI0 */
        MSTP(RSPI1) = 0;

        /* Disable writing to MSTP registers. */
        SYSTEM.PRCR.WORD = 0xA500;

        /* Setup ICU for RSPI */
        /* Set shared IPL for RSPI1 */
        IPR(RSPI1, SPRI1) = 3;
        /* Disable receive buffer full interrupt */
        IEN(RSPI1, SPRI1) = 0;
        /* Disable transmit buffer empty interrupt */
        IEN(RSPI1, SPTI1) = 0;
        /* Clear pending receive buffer full interrupts */
        IR(RSPI1, SPRI1) = 0 ;

    #endif
    }
#endif
#if RSPI_NUM_CHANNELS > 2
    else if (2 == channel)
    {

    #if defined(MCU_RX63N) || defined(MCU_RX630) || defined(MCU_RX631) || defined(MCU_RX210) || defined(BSP_MCU_RX63N)

        /* Enable writing to MSTP registers. */
        SYSTEM.PRCR.WORD = 0xA502;

        /* Turn on RSPI2 */
        MSTP(RSPI2) = 0;

        /* Disable writing to MSTP registers. */
        SYSTEM.PRCR.WORD = 0xA500;

        /* Setup ICU for RSPI */
        /* Set shared IPL for RSPI2 */
        IPR(RSPI2, SPRI2) = 3;
        /* Disable receive buffer full interrupt */
        IEN(RSPI2, SPRI2) = 0;
        /* Disable transmit buffer empty interrupt */
        IEN(RSPI2, SPTI2) = 0;
        /* Clear pending receive buffer full interrupts */
        IR(RSPI2, SPRI2) = 0 ;

    #endif
    }
#endif
    
    /* Set pin control register (SPPCR) */
    (*g_rspi_channels[channel]).SPPCR.BYTE = 0x00;

    /* Set RSPI bit rate (SPBR) */
    /* -Set baud rate to 8Mbps (48MHz / (2 * (2 + 1) * 2^0) ) = 8Mbps */
    (*g_rspi_channels[channel]).SPBR = 2;

    /* Set RSPI data control register (SPDCR) */
    /* -SPDR is accessed in longwords (32 bits) 
       -Transfer 1 frame at a time */
    (*g_rspi_channels[channel]).SPDCR.BYTE = 0x20;

    /* Can Set if needed */
#if 0
    /* Set RSPI clock delay registers (SPCKD) */
    (*g_rspi_channels[channel]).SPCKD.BYTE = 7;

    /* Set RSPI slave select negation delay register (SSLND) */
    (*g_rspi_channels[channel]).SSLND.BYTE = 7;

    /* Set RSPI next-access dealy register (SPND) */
    (*g_rspi_channels[channel]).SPND.BYTE = 7;
#endif

    /* Set RSPI control register 2 (SPCR2) */
    /* -Disable Idle interrupt */
    (*g_rspi_channels[channel]).SPCR2.BYTE = 0x00;

    /* Set RSPI command register 0 (SPCMD0) */
    /* -MSB first
       -8 bits data length
       -SSL0 (handled manually)
       -Use bit rate % 1       
        */       
    (*g_rspi_channels[channel]).SPCMD0.WORD = 0x0400;    
    
    /* Set RSPI control register (SPCR) */        
    /* -Clock synchronous operation (3-wire)
       -Full duplex operation
       -Master mode
       -SPTI and SPRI enabled in RSPI (have to check ICU also)
       -Enable RSPI function */
    (*g_rspi_channels[channel]).SPCR.BYTE = 0xE9;          

    /* Peripheral Initialized */
    rspi_initialized[channel] = true;

    return true;
}

/***********************************************************************************************************************
* Function Name: R_RSPI_Select
* Description  : Enable device chip select on SPI channel
* Arguments    : channel -
*                    Which channel to use
*                chip_select -
*                    Which device to enable (choose from device_selected_t enum)
*                pid -
*                    Unique task ID. Used to make sure tasks don't step on each other.
* Return Value : true -
*                    Operation completed.
*                false -
*                    This task did lock the RSPI fist.
***********************************************************************************************************************/
bool R_RSPI_Select(uint8_t channel, device_selected_t chip_select, uint32_t pid)
{
#if defined(RSPI_REQUIRE_LOCK)    
    /* Verify that this task has the lock */
    if (false == R_RSPI_CheckLock(channel, pid)) 
    {
        /* This task does not have the RSPI lock and therefore cannot perform this operation. */
        return false;
    }
#endif    

    switch ( chip_select)
    {
#if defined(SDMICRO_CS)
        case SDMICRO_SELECTED:          /* Enable the SD card */
            SDMICRO_CS = 0;
        break;
#endif
#if defined(FLASH_CS)
        case FLASH_SELECTED:            /* Enable Micron flash */
            FLASH_CS = 0;    
        break;
#endif
#if defined(WIFI_CS)
        case WIFI_SELECTED:             /* Enable Redpine WiFi card */
            WIFI_CS = 0;
        break;
#endif
#if defined(LCD_CS)
        case LCD_SELECTED:              /* Enable Okaya display */
            LCD_CS = 0;
        break;
#endif
        default:
        break;           
    }     

    return true;
}

/***********************************************************************************************************************
* Function Name: R_RSPI_Deselect
* Description  : Disables device chip select
* Arguments    : channel -
*                    Which channel to use
*                chip_select -
*                    Which device to disable (choose from device_selected_t enum)
*                pid -
*                    Unique task ID. Used to make sure tasks don't step on
*                    each other.
* Return Value : true -
*                    Operation completed.
*                false -
*                    This task did lock the RSPI fist.
***********************************************************************************************************************/
bool R_RSPI_Deselect(uint8_t channel, device_selected_t chip_select, uint32_t pid)
{
#if defined(RSPI_REQUIRE_LOCK)    
    /* Verify that this task has the lock */
    if (false == R_RSPI_CheckLock(channel, pid)) 
    {
        /* This task does not have the RSPI lock and therefore cannot perform this operation. */
        return false;
    }
#endif    

    switch ( chip_select)
    {
#if defined(SDMICRO_CS)
        case SDMICRO_SELECTED:          /* Disable the SD card */
            SDMICRO_CS = 1;
        break;
#endif
#if defined(FLASH_CS)
        case FLASH_SELECTED:            /* Disable Micron flash */
            FLASH_CS = 1;    
        break;
#endif
#if defined(WIFI_CS)
        case WIFI_SELECTED:             /* Disable Redpine WiFi card */
            WIFI_CS = 1;
        break;
#endif
#if defined(LCD_CS)
        case LCD_SELECTED:              /* Disable Okaya display */
            LCD_CS = 1;
        break;
#endif
        default:
        break;
    }     

    return true;
}

/***********************************************************************************************************************
* Function Name: R_RSPI_BaudRateSet
* Description  : Set the baud rate of the RSPI peripheral
* Arguments    : channel -
*                    Which channel to initialize
*                divisor -
*                    What divisor to use for the baud rate. The formula is:
*                    baud rate divisor = (divisor+1) * 2
*                    e.g. 0 = /2, 1 = /4, 2 = /6, 3 = /8, etc.                        
*                NOTE: Be careful with low divisors. You can set the divisor low but you still have to hit the 
*                      timing requirements.
*                pid -
*                    Unique task ID. Used to make sure tasks don't step on each other.
* Return Value : true -
*                    Operation completed.
*                false -
*                    This task did lock the RSPI fist.
***********************************************************************************************************************/
bool R_RSPI_BaudRateSet(uint8_t channel, uint8_t divisor, uint32_t pid)
{
#if defined(RSPI_REQUIRE_LOCK)    
    /* Verify that this task has the lock */
    if (false == R_RSPI_CheckLock(channel, pid)) 
    {
        /* This task does not have the RSPI lock and therefore cannot perform this operation. */
        return false;
    }
#endif    

    /* Set RSPI bit rate (SPBR) */
    /* -Set baud rate to (48MHz / (2 * (divisor + 1) * 2^0) ) */
    (*g_rspi_channels[channel]).SPBR = divisor;

    return true;
}

/***********************************************************************************************************************
* Function Name: R_RSPI_SendReceive
* Description  : Performs SPI transfers. Can read and write at the same time.
* Arguments    : channel -
*                    Which channel to use
*                pSrc -  
*                    pointer to data buffer with data to be transmitted.
*                    If NULL, const 0xFF as source.
*                pDest - 
*                    pointer to location to put the received data (can be same as pSrc if desired). 
*                    If NULL, receive data discarded.
*                usBytes - 
*                    number of bytes to be sent/received
*                pid -
*                    Unique task ID. Used to make sure tasks don't step on each other.
* Return Value : true -
*                    Operation completed.
*                false -
*                    This task did lock the RSPI fist.
***********************************************************************************************************************/
bool R_RSPI_SendReceive(uint8_t channel, 
                        uint8_t const *pSrc, 
                        uint8_t *pDest, 
                        uint16_t usBytes, 
                        uint32_t pid)
{    	
    uint16_t byte_count;  
    volatile uint32_t temp;

#if defined(RSPI_REQUIRE_LOCK)    
    /* Verify that this task has the lock */
    if (false == R_RSPI_CheckLock(channel, pid)) 
    {
        /* This task does not have the RSPI lock and therefore cannot perform this operation. */
        return false;
    }
#endif    

    for (byte_count = 0; byte_count < usBytes; byte_count++)
    {
        /* Ensure transmit register is empty */
        while ((*g_rspi_channels[channel]).SPSR.BIT.IDLNF) ;
        
        /* If just reading then transmit 0xFF */
        (*g_rspi_channels[channel]).SPDR.LONG = (pSrc == NULL) ? 0xFF : pSrc[byte_count];
        
        /* Transfer is complete when a byte has been shifted in (full duplex) */
        if (0 == channel)
        {
            while (IR(RSPI0, SPRI0) == 0) ;
        }
#if RSPI_NUM_CHANNELS > 1
        else if (1 == channel)
        {
            while (IR(RSPI1, SPRI1) == 0) ;
        }
#endif
#if RSPI_NUM_CHANNELS > 2
        else 
        {
            while (IR(RSPI2, SPRI2) == 0) ;
        }
#endif
        
        /* Read received data.  If transmit only, then ignore it */
        if (pDest == NULL)
        {
            temp = (*g_rspi_channels[channel]).SPDR.LONG;
        }
        else
        {
            pDest[byte_count] = (uint8_t) ((*g_rspi_channels[channel]).SPDR.LONG & 0xFF);
        }
        
        /* Clear pending read interrupts */
        if (0 == channel)
        {
            IR(RSPI0, SPRI0) = 0;
        }
#if RSPI_NUM_CHANNELS > 1
        else if (1 == channel)
        {
            IR(RSPI1, SPRI1) = 0;
        }
#endif
#if RSPI_NUM_CHANNELS > 2
        else 
        {
            IR(RSPI2, SPRI2) = 0;
        }
#endif
    }

    return true;
}

/***********************************************************************************************************************
* Function Name: R_RSPI_Read
* Description  : Reads data using RSPI
* Arguments    : channel -
*                    Which channel to use
*                pDest - 
*                    Pointer to location to put the received data. 
*                    Returned value will be incremented by number of bytes received.
*                usBytes - 
*                    number of bytes to be received
*                pid -
*                    Unique task ID. Used to make sure tasks don't step on each other.
* Return Value : true -
*                    Operation completed.
*                false -
*                    This task did lock the RSPI fist.
***********************************************************************************************************************/
bool R_RSPI_Read(uint8_t channel, 
                 uint8_t *pDest, 
                 uint16_t usBytes, 
                 uint32_t pid)
{
    uint16_t byte_count;  
    volatile uint32_t temp;

#if defined(RSPI_REQUIRE_LOCK)    
    /* Verify that this task has the lock */
    if (false == R_RSPI_CheckLock(channel, pid)) 
    {
        /* This task does not have the RSPI lock and therefore cannot perform this operation. */
        return false;
    }
#endif    

    for (byte_count = 0; byte_count < usBytes; byte_count++)
    {
        /* Ensure transmit register is empty */
        while ((*g_rspi_channels[channel]).SPSR.BIT.IDLNF) ;
        
        /* If just reading then transmit 0xFF */
        (*g_rspi_channels[channel]).SPDR.LONG = 0xFFFFFFFF ;
        
        /* Transfer is complete when a byte has been shifted in (full duplex) */
        if (0 == channel)
        {
            while (IR(RSPI0, SPRI0) == 0) ;
        }
#if RSPI_NUM_CHANNELS > 1
        else if (1 == channel)
        {
            while (IR(RSPI1, SPRI1) == 0) ;
        }
#endif
#if RSPI_NUM_CHANNELS > 2
        else 
        {
            while (IR(RSPI2, SPRI2) == 0) ;
        }
#endif
        
        /* Read received data.  If transmit only, then ignore it */
        pDest[byte_count] = (uint8_t) ((*g_rspi_channels[channel]).SPDR.LONG & 0xFF);
        
        /* Clear any pending read interrupts */
        if (0 == channel)
        {
            IR(RSPI0, SPRI0) = 0;
        }
#if RSPI_NUM_CHANNELS > 1
        else if (1 == channel)
        {
            IR(RSPI1, SPRI1) = 0;
        }
#endif
#if RSPI_NUM_CHANNELS > 2
        else 
        {
            IR(RSPI2, SPRI2) = 0;
        }
#endif
    }  

    return true;
}

/***********************************************************************************************************************
* Function Name: R_RSPI_Write
* Description  : Write to a SPI device
* Arguments    : channel -
*                    Which channel to use
*                pSrc -  
*                    Pointer to data buffer with data to be transmitted.
*                    Returned value will be incremented by number of attempted writes.
*                usBytes - 
*                    Number of bytes to be sent
*                pid -
*                    Unique task ID. Used to make sure tasks don't step on each other.
* Return Value : true -
*                    Operation completed.
*                false -
*                    This task did lock the RSPI fist.
***********************************************************************************************************************/
bool R_RSPI_Write(uint8_t channel, 
                  const uint8_t *pSrc, 
                  uint16_t usBytes, 
                  uint32_t pid)
{
    uint16_t byte_count;  
    volatile uint32_t temp;

#if defined(RSPI_REQUIRE_LOCK)    
    /* Verify that this task has the lock */
    if (false == R_RSPI_CheckLock(channel, pid)) 
    {
        /* This task does not have the RSPI lock and therefore cannot perform this operation. */
        return false;
    }
#endif    

    for (byte_count = 0; byte_count < usBytes; byte_count++)
    {
        /* Ensure transmit register is empty */
        while ((*g_rspi_channels[channel]).SPSR.BIT.IDLNF) ;
        
        /* If just reading then transmit 0xFF */
        (*g_rspi_channels[channel]).SPDR.LONG = pSrc[byte_count];
        
        /* Transfer is complete when a byte has been shifted in (full duplex) */
        if (0 == channel)
        {
            while (IR(RSPI0, SPRI0) == 0) ;
        }
#if RSPI_NUM_CHANNELS > 1
        else if (1 == channel)
        {
            while (IR(RSPI1, SPRI1) == 0) ;
        }
#endif
#if RSPI_NUM_CHANNELS > 2
        else 
        {
            while (IR(RSPI2, SPRI2) == 0) ;
        }
#endif
        
        /* Read received data.  If transmit only, then ignore it */
        temp = (*g_rspi_channels[channel]).SPDR.LONG;

        /* Clear pending interrupts */
        if (0 == channel)
        {
            IR(RSPI0, SPRI0) = 0;
        }
#if RSPI_NUM_CHANNELS > 1
        else if (1 == channel)
        {
            IR(RSPI1, SPRI1) = 0;
        }
#endif
#if RSPI_NUM_CHANNELS > 2
        else 
        {
            IR(RSPI2, SPRI2) = 0;
        }
#endif
    }

    return true;
}

/* These functions are only needed if locking is enabled in r_rspi_rx_config.h */

/***********************************************************************************************************************
* Function Name: R_RSPI_Lock
* Description  : Get lock on RSPI access.
* Arguments    : channel -
*                    Which channel to use
*                pid - 
*                    Unique program ID to attempt to lock RSPI with
* Return Value : true - 
*                    lock acquired
*                false - 
*                    lock not acquired
***********************************************************************************************************************/
bool R_RSPI_Lock(uint8_t channel, uint32_t pid)
{
#if defined(RSPI_REQUIRE_LOCK)    
    /* Return variable */
    bool ret;

    /* Attempt to acquire lock for this channel. */
    ret = R_BSP_SoftwareLock(&g_rspi_locks[channel]);

    /* Check to see if lock was successfully taken */
    if (ret == true)
    {
        /* Only give this pid access if no program has the reservation already */
        if (g_rspi_locked_id[channel] == NO_DEVICE_SELECTED)
        {
            /* This pid now has reserved this channel. */
            g_rspi_locked_id[channel] = pid;
        }
        else
        {
            /* Channel has already been reserved by another process. */
            ret = false;
        }
        /* Release lock. */
        R_BSP_SoftwareUnlock(&g_rspi_locks[channel]);
    }
    /* Else, lock already taken. Try again later. */

    return ret;
#else
    return true;
#endif
}

/***********************************************************************************************************************
* Function Name: R_RSPI_Unlock
* Description  : Release RSPI lock so another task can use it
* Arguments    : channel -
*                    Which channel to use
*                pid - 
*                    Unique program ID to attempt to unlock RSPI with
* Return Value : true - 
*                    lock was relinquished
*                false - 
*                    lock was not given to this ID previously
***********************************************************************************************************************/
bool R_RSPI_Unlock(uint8_t channel, uint32_t pid)
{
#if defined(RSPI_REQUIRE_LOCK)    
    bool ret = true;

    /* Can only release lock if this program previously acquired it */
    if ( g_rspi_locked_id[channel] == pid )
    {
        /* Lock given back successfully */
        g_rspi_locked_id[channel] = NO_DEVICE_SELECTED;
    }
    else
    {
        /* This program did not have this lock */
        ret = false;
    }

    return ret;
#else
    return true;
#endif
}


/***********************************************************************************************************************
* Function Name: R_RSPI_CheckLock
* Description  : Checks to see if PID matches the one that took the lock
* Arguments    : channel -
*                    Which channel to use
*                pid -
*                    Process ID to check against.
* Return Value : true - 
*                    This task has the lock
*                false - 
*                    This task does not have the lock
***********************************************************************************************************************/
bool R_RSPI_CheckLock(uint8_t channel, uint32_t pid)
{    
#if defined(RSPI_REQUIRE_LOCK)    
    /* Check to see if this task has the lock */
    if (g_rspi_locked_id[channel] != pid) 
    {        
        /* This task does not have the lock */
        return false;
    }
    else
    {
        /* This task has the lock */
        return true;
    }
#else
    return true;
#endif
}


