/*******************************************************************************

Copyright (c) 2000 Microsoft Corporation.  All rights reserved.

File Name:

    snow.bmp.cpp

Description

    Data file for the resource snow.bmp.

*******************************************************************************/

#include "d3dlocus.h"

static BYTE g_snow[] = {

   0x42, 0x4D, 0xC8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86, 0x00, 0x00, 0x00, 0x28, 0x00, 
   0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x0B, 0x00, 0x00, 0x12, 0x0B, 0x00, 0x00, 0x14, 0x00, 
   0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xF7, 0xF7, 0xF7, 0x00, 0xF0, 0xF0, 
   0xF0, 0x00, 0xEA, 0xEA, 0xEA, 0x00, 0xE6, 0xE6, 0xE6, 0x00, 0xE2, 0xE2, 0xE2, 0x00, 0xDC, 0xDC, 
   0xDC, 0x00, 0xDB, 0xDB, 0xDB, 0x00, 0xD8, 0xD8, 0xD8, 0x00, 0xD4, 0xD4, 0xD4, 0x00, 0xD2, 0xD2, 
   0xD2, 0x00, 0xD1, 0xD1, 0xD1, 0x00, 0xCE, 0xCE, 0xCE, 0x00, 0xCD, 0xCD, 0xCD, 0x00, 0xCC, 0xCC, 
   0xCC, 0x00, 0xCB, 0xCB, 0xCB, 0x00, 0xCA, 0xCA, 0xCA, 0x00, 0xC9, 0xC9, 0xC9, 0x00, 0xC8, 0xC8, 
   0xC8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x12, 0x12, 0x11, 0x11, 0x12, 0x12, 0x12, 0x12, 0x12, 
   0x0F, 0x0B, 0x0A, 0x0D, 0x11, 0x12, 0x12, 0x10, 0x09, 0x05, 0x04, 0x06, 0x0D, 0x12, 0x12, 0x0E, 
   0x07, 0x02, 0x01, 0x04, 0x0A, 0x11, 0x12, 0x0F, 0x08, 0x03, 0x02, 0x05, 0x0B, 0x11, 0x12, 0x11, 
   0x0C, 0x08, 0x07, 0x09, 0x0F, 0x12, 0x12, 0x12, 0x11, 0x0F, 0x0E, 0x10, 0x12, 0x12, 0x12, 0x12, 
   0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x00, 0x00, 
};

static RESOURCEDATA g_rd_snow(TEXT(MODULE_STRING), TEXT("snow.bmp"), g_snow, sizeof(g_snow));

extern "C" LPVOID PREPEND_MODULE(_snow) = (LPVOID)&g_rd_snow;
