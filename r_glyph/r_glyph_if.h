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
* File Name	   : r_glyph_if.h
* Version      : 1.10 
* Description  : Implements the IO functions needed for the Glyph code. These functions are 'tied' to the Glyph code
*                in r_glyph_register.c.
***********************************************************************************************************************/
/**********************************************************************************************************************
* History : DD.MM.YYYY Version  Description
*         : 01.11.2011 1.00     First Release
*         : 08.03.2012 1.10     Updated code to be compliant with FIT spec v0.7. Moved 2 GLYPH_RESET* macros from 
*                               public header file to r_glyph.c since global scope was not needed. Renamed r_glyph.h to
*                               r_glyph_if.h.
***********************************************************************************************************************/

/***********************************************************************************************************************
Includes   <System Includes> , "Project Includes"
***********************************************************************************************************************/
/* Fixed width integer support. */
#include <stdint.h>
/* bool support */
#include <stdbool.h>
/* Glyph typdefs. */
#include "./src/glyph/glyph.h"
/* Configuration options for r_glpyh package. */
#include "r_glyph_config.h"

/***********************************************************************************************************************
Macro definitions
***********************************************************************************************************************/
/* Version Number of API. */
#define GLYPH_VERSION_MAJOR           (1)
#define GLYPH_VERSION_MINOR           (10)
/* The process of getting the version number is done through the macro below. The version number is encoded where the 
   top 2 bytes are the major version number and the bottom 2 bytes are the minor version number. For example, 
   Version 4.25 would be returned as 0x00040019. */
#define R_GLYPH_GetVersion()  ((((uint32_t)GLYPH_VERSION_MAJOR) << 16) | (uint32_t)GLYPH_VERSION_MINOR)

/***********************************************************************************************************************
Typedef definitions
***********************************************************************************************************************/

/***********************************************************************************************************************
Exported global variables
***********************************************************************************************************************/

/***********************************************************************************************************************
Exported global functions (to be accessed by other files)
***********************************************************************************************************************/
T_glyphError R_GLYPH_Open(T_glyphHandle aHandle);
void R_GLYPH_CommandSend(int8_t c_command);
void R_GLYPH_DataSend(int8_t c_data);



