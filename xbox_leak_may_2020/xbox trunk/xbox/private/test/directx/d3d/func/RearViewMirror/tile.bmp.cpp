/*******************************************************************************

Copyright (c) 2000 Microsoft Corporation.  All rights reserved.

File Name:

    tile.bmp.cpp

Description

    Data file for the resource tile.bmp.

*******************************************************************************/

#include "d3dlocus.h"

static BYTE g_tile[] = {

   0x42, 0x4D, 0x38, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00, 
   0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC3, 0x0E, 0x00, 0x00, 0xC3, 0x0E, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xBB, 0xBC, 0xBC, 0xA9, 0xA9, 0xA9, 0xA4, 0xA4, 0xA4, 0xA4, 
   0xA4, 0xA4, 0xA5, 0xA5, 0xA5, 0xA7, 0xA7, 0xA7, 0xA5, 0xA5, 0xA5, 0xA4, 0xA4, 0xA4, 0xA5, 0xA5, 
   0xA5, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0x9F, 0x9F, 0x9F, 0x9E, 0x9E, 0x9E, 0x9D, 0x9D, 0x9D, 
   0x9D, 0x9D, 0x9D, 0x9C, 0x9C, 0x9C, 0x9D, 0x9D, 0x9D, 0x9C, 0x9C, 0x9C, 0x9F, 0x9F, 0x9F, 0x9E, 
   0x9E, 0x9E, 0x9E, 0x9E, 0x9E, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0xA0, 0xA0, 
   0xA0, 0x9D, 0x9D, 0x9D, 0x9B, 0x9A, 0x9A, 0x9C, 0x9C, 0x9C, 0x9E, 0x9E, 0x9E, 0xA1, 0xA0, 0xA0, 
   0xB3, 0xB4, 0xB4, 0xBD, 0xBF, 0xBF, 0xB5, 0xBC, 0xBC, 0xB2, 0xB2, 0xB2, 0xB6, 0xB6, 0xB6, 0xB4, 
   0xB4, 0xB4, 0xB6, 0xB6, 0xB6, 0xBB, 0xBB, 0xBB, 0xBE, 0xBE, 0xBE, 0xC0, 0xC0, 0xC0, 0xC3, 0xC3, 
   0xC3, 0xBD, 0xBD, 0xBD, 0xBB, 0xBB, 0xBB, 0xC3, 0xC3, 0xC3, 0xC4, 0xC4, 0xC4, 0xC5, 0xC5, 0xC5, 
   0xC5, 0xC5, 0xC5, 0xC7, 0xC7, 0xC7, 0xC5, 0xC5, 0xC5, 0xC6, 0xC6, 0xC6, 0xC9, 0xC9, 0xC9, 0xC3, 
   0xC3, 0xC3, 0xC4, 0xC4, 0xC4, 0xC2, 0xC2, 0xC2, 0xC4, 0xC4, 0xC4, 0xC7, 0xC7, 0xC7, 0xCB, 0xCB, 
   0xCB, 0xC9, 0xC9, 0xC9, 0xCA, 0xCA, 0xCA, 0xCB, 0xCB, 0xCB, 0xCF, 0xCF, 0xCF, 0xCE, 0xCF, 0xCF, 
   0xC8, 0xC8, 0xC8, 0xC2, 0xC3, 0xC3, 0xBB, 0xBD, 0xBD, 0xD6, 0xD7, 0xD7, 0xF6, 0xF6, 0xF6, 0xF6, 
   0xF6, 0xF6, 0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xFA, 0xFA, 
   0xFA, 0xF8, 0xF8, 0xF8, 0xF5, 0xF5, 0xF5, 0xFA, 0xFA, 0xFA, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 
   0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xF9, 0xF9, 0xF9, 0xF5, 
   0xF5, 0xF5, 0xF5, 0xF5, 0xF5, 0xF5, 0xF5, 0xF5, 0xF5, 0xF5, 0xF5, 0xF5, 0xF5, 0xF5, 0xF5, 0xF5, 
   0xF5, 0xF6, 0xF6, 0xF6, 0xEE, 0xEE, 0xEE, 0xE2, 0xE2, 0xE2, 0xF3, 0xF3, 0xF3, 0xED, 0xED, 0xED, 
   0xCA, 0xCD, 0xCD, 0xBE, 0xC1, 0xC1, 0xBE, 0xBE, 0xBE, 0xD7, 0xD6, 0xD6, 0xF9, 0xF9, 0xF9, 0xFA, 
   0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 
   0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xFA, 0xFA, 0xFA, 0xF5, 0xF5, 0xF5, 0xF4, 
   0xF4, 0xF4, 0xF4, 0xF4, 0xF4, 0xF3, 0xF3, 0xF3, 0xF2, 0xF2, 0xF2, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 
   0xF1, 0xEE, 0xEE, 0xEE, 0xDA, 0xDA, 0xDA, 0xD6, 0xD6, 0xD6, 0xDA, 0xDA, 0xDA, 0xD7, 0xD7, 0xD7, 
   0xC5, 0xC5, 0xC5, 0xBF, 0xBF, 0xBF, 0xBB, 0xBB, 0xBB, 0xD9, 0xD9, 0xD9, 0xFC, 0xFC, 0xFC, 0xF6, 
   0xF6, 0xF6, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF3, 0xF3, 0xF3, 0xF6, 
   0xF6, 0xF6, 0xF6, 0xF6, 0xF6, 0xF7, 0xF7, 0xF7, 0xF5, 0xF5, 0xF5, 0xF0, 0xF0, 0xF0, 0xF2, 0xF2, 
   0xF2, 0xEC, 0xEC, 0xEC, 0xE4, 0xE4, 0xE4, 0xE8, 0xE8, 0xE8, 0xE9, 0xE9, 0xE9, 0xD9, 0xD9, 0xD9, 
   0xC6, 0xC6, 0xC6, 0xBD, 0xBF, 0xBF, 0xBE, 0xBD, 0xBD, 0xD7, 0xD7, 0xD7, 0xFB, 0xFB, 0xFB, 0xF8, 
   0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF8, 0xF8, 0xF8, 0xF4, 0xF4, 0xF4, 0xF3, 0xF3, 0xF3, 0xF7, 
   0xF7, 0xF7, 0xF6, 0xF6, 0xF6, 0xF6, 0xF6, 0xF6, 0xF4, 0xF4, 0xF4, 0xF1, 0xF1, 0xF1, 0xF2, 0xF2, 
   0xF2, 0xF2, 0xF2, 0xF2, 0xEC, 0xEC, 0xEC, 0xED, 0xED, 0xED, 0xF5, 0xF5, 0xF5, 0xE8, 0xE7, 0xE7, 
   0xC7, 0xC7, 0xC7, 0xBC, 0xC2, 0xC2, 0xBB, 0xBD, 0xBD, 0xD3, 0xD3, 0xD3, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF8, 0xF8, 0xF8, 0xF5, 0xF5, 0xF5, 0xF0, 
   0xF0, 0xF0, 0xF2, 0xF2, 0xF2, 0xF1, 0xF1, 0xF1, 0xF3, 0xF3, 0xF3, 0xF6, 0xF6, 0xF6, 0xF7, 0xF7, 
   0xF7, 0xF3, 0xF3, 0xF3, 0xF1, 0xF1, 0xF1, 0xF0, 0xF0, 0xF0, 0xF3, 0xF3, 0xF3, 0xEB, 0xEB, 0xEB, 
   0xC5, 0xC5, 0xC5, 0xBB, 0xC1, 0xC1, 0xBA, 0xBA, 0xBA, 0xD3, 0xD3, 0xD3, 0xFC, 0xFC, 0xFC, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xFA, 0xFA, 0xFA, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xFA, 0xFA, 0xFA, 0xF7, 0xF7, 0xF7, 0xF0, 
   0xF0, 0xF0, 0xEF, 0xEF, 0xEF, 0xF2, 0xF2, 0xF2, 0xF6, 0xF6, 0xF6, 0xF4, 0xF4, 0xF4, 0xF7, 0xF7, 
   0xF7, 0xF3, 0xF3, 0xF3, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF2, 0xF2, 0xF2, 0xE9, 0xE9, 0xE9, 
   0xC5, 0xC7, 0xC8, 0xBB, 0xC1, 0xC1, 0xBA, 0xBA, 0xBA, 0xD8, 0xD8, 0xD8, 0xFD, 0xFD, 0xFD, 0xF8, 
   0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xF9, 0xF9, 0xF9, 
   0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF6, 0xF6, 0xF6, 0xF1, 
   0xF1, 0xF1, 0xF2, 0xF2, 0xF2, 0xF4, 0xF4, 0xF4, 0xF6, 0xF6, 0xF6, 0xF3, 0xF3, 0xF3, 0xF7, 0xF7, 
   0xF7, 0xF2, 0xF2, 0xF2, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF4, 0xF4, 0xF4, 0xEA, 0xEA, 0xE9, 
   0xC6, 0xC7, 0xC9, 0xC1, 0xC3, 0xC3, 0xBA, 0xBA, 0xBA, 0xD7, 0xD7, 0xD7, 0xFB, 0xFB, 0xFB, 0xF5, 
   0xF5, 0xF5, 0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF7, 0xF7, 0xF7, 0xF5, 
   0xF5, 0xF5, 0xF1, 0xF1, 0xF1, 0xF2, 0xF2, 0xF2, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF5, 0xF5, 
   0xF5, 0xF3, 0xF3, 0xF3, 0xF3, 0xF3, 0xF3, 0xF0, 0xF0, 0xF0, 0xF7, 0xF7, 0xF7, 0xEE, 0xEE, 0xEE, 
   0xC8, 0xC9, 0xC9, 0xC4, 0xC4, 0xC4, 0xB9, 0xB9, 0xB9, 0xD4, 0xD4, 0xD4, 0xF7, 0xF7, 0xF7, 0xF8, 
   0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF8, 
   0xF8, 0xF8, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF6, 0xF6, 0xF6, 0xF7, 0xF7, 
   0xF7, 0xF3, 0xF3, 0xF3, 0xF3, 0xF3, 0xF3, 0xF1, 0xF1, 0xF1, 0xF8, 0xF8, 0xF8, 0xEF, 0xEF, 0xEF, 
   0xC8, 0xCA, 0xCA, 0xBF, 0xC0, 0xC0, 0xB8, 0xB8, 0xB8, 0xD3, 0xD3, 0xD3, 0xFB, 0xFB, 0xFB, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF8, 
   0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF6, 0xF6, 
   0xF6, 0xF4, 0xF4, 0xF4, 0xF3, 0xF3, 0xF3, 0xF1, 0xF1, 0xF1, 0xF6, 0xF6, 0xF6, 0xEB, 0xEB, 0xEB, 
   0xC4, 0xC3, 0xC3, 0xBB, 0xBC, 0xBC, 0xBD, 0xBD, 0xBD, 0xD9, 0xD9, 0xD9, 0xFD, 0xFD, 0xFD, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF8, 0xF8, 0xF8, 0xF6, 0xF6, 0xF6, 0xF7, 0xF7, 0xF7, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF8, 0xF8, 
   0xF8, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF7, 0xF7, 0xF6, 0xEB, 0xEA, 0xEA, 
   0xC2, 0xC3, 0xC3, 0xB5, 0xBB, 0xBB, 0xBC, 0xBD, 0xBD, 0xDA, 0xDA, 0xDA, 0xFC, 0xFC, 0xFC, 0xFA, 
   0xFA, 0xFA, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFB, 0xFB, 0xFB, 0xFA, 0xFA, 0xFA, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xF8, 0xF8, 0xF8, 0xFB, 0xFB, 0xFB, 0xED, 0xEC, 0xEC, 
   0xC5, 0xC7, 0xC7, 0xB7, 0xBC, 0xBC, 0xB8, 0xBA, 0xBA, 0xC9, 0xCB, 0xCB, 0xF8, 0xF8, 0xF8, 0xFA, 
   0xFA, 0xFA, 0xF9, 0xF9, 0xF9, 0xF9, 0xFA, 0xFA, 0xF8, 0xF9, 0xF9, 0xF8, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xFA, 0xFA, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF8, 
   0xF9, 0xF9, 0xF8, 0xF9, 0xF9, 0xF8, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xFA, 0xFB, 0xFB, 0xFC, 0xEB, 0xEA, 0xEA, 
   0xC4, 0xC5, 0xC5, 0xB7, 0xBD, 0xBD, 0xBB, 0xBB, 0xBB, 0xC4, 0xC5, 0xC5, 0xF7, 0xF7, 0xF7, 0xFA, 
   0xFA, 0xFA, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF8, 0xF8, 
   0xF8, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xFB, 0xFB, 0xFB, 0xEB, 0xEA, 0xEA, 
   0xC2, 0xC3, 0xC3, 0xB6, 0xBB, 0xBB, 0xB7, 0xBA, 0xBA, 0xC7, 0xC6, 0xC6, 0xF7, 0xF7, 0xF7, 0xFA, 
   0xFA, 0xFA, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 
   0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF7, 0xF7, 0xF7, 0xF8, 0xF8, 0xF8, 0xE9, 0xE9, 0xE9, 
   0xC5, 0xC5, 0xC5, 0xB6, 0xBB, 0xBB, 0xBC, 0xBC, 0xBC, 0xCD, 0xCC, 0xCC, 0xF4, 0xF4, 0xF4, 0xF7, 
   0xF7, 0xF7, 0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 
   0xF8, 0xF8, 0xF8, 0xF8, 0xF5, 0xF5, 0xF5, 0xF3, 0xF3, 0xF3, 0xF4, 0xF4, 0xF4, 0xE5, 0xE5, 0xE5, 
   0xC5, 0xC6, 0xC6, 0xB8, 0xBD, 0xBD, 0xB9, 0xBB, 0xBB, 0xC3, 0xC5, 0xC5, 0xED, 0xEE, 0xEE, 0xF4, 
   0xF4, 0xF4, 0xF7, 0xF7, 0xF7, 0xF9, 0xF9, 0xF9, 0xF6, 0xF6, 0xF6, 0xF5, 0xF5, 0xF5, 0xF8, 0xF8, 
   0xF8, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF8, 0xF8, 0xF8, 0xF8, 
   0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF7, 0xF7, 0xF7, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF4, 0xF4, 
   0xF4, 0xF2, 0xF2, 0xF2, 0xF3, 0xF3, 0xF3, 0xF3, 0xF3, 0xF3, 0xF5, 0xF5, 0xF5, 0xE9, 0xE8, 0xE8, 
   0xC7, 0xC7, 0xC7, 0xB8, 0xBC, 0xBC, 0xBD, 0xBE, 0xBE, 0xBB, 0xBC, 0xBC, 0xEC, 0xEC, 0xEC, 0xF8, 
   0xF8, 0xF8, 0xF5, 0xF5, 0xF5, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF2, 0xF2, 0xF2, 0xF5, 0xF5, 
   0xF5, 0xF6, 0xF6, 0xF6, 0xF6, 0xF6, 0xF6, 0xF6, 0xF6, 0xF6, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF4, 0xF4, 0xF4, 0xF6, 
   0xF6, 0xF6, 0xF6, 0xF6, 0xF6, 0xF5, 0xF5, 0xF5, 0xF4, 0xF4, 0xF4, 0xF0, 0xF0, 0xF0, 0xEA, 0xEA, 
   0xEA, 0xEE, 0xEE, 0xEE, 0xF3, 0xF3, 0xF3, 0xE5, 0xE5, 0xE5, 0xEC, 0xEC, 0xEC, 0xE9, 0xE9, 0xE9, 
   0xC6, 0xC6, 0xC6, 0xBA, 0xBB, 0xBB, 0xBA, 0xBE, 0xBE, 0xBD, 0xBD, 0xBD, 0xF0, 0xF0, 0xF0, 0xFB, 
   0xFB, 0xFB, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF5, 0xF5, 0xF5, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 
   0xF2, 0xF3, 0xF3, 0xF3, 0xF6, 0xF6, 0xF6, 0xF3, 0xF3, 0xF3, 0xF6, 0xF6, 0xF6, 0xFA, 0xFA, 0xFA, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF6, 0xF6, 0xF6, 0xF1, 0xF1, 0xF1, 0xF6, 
   0xF6, 0xF6, 0xF5, 0xF5, 0xF5, 0xF3, 0xF3, 0xF3, 0xF0, 0xF0, 0xF0, 0xEE, 0xEE, 0xEE, 0xF0, 0xF0, 
   0xF0, 0xF5, 0xF5, 0xF5, 0xF5, 0xF5, 0xF5, 0xE2, 0xE2, 0xE2, 0xDB, 0xDB, 0xDB, 0xDA, 0xDA, 0xDA, 
   0xC2, 0xC3, 0xC3, 0xB9, 0xBD, 0xBD, 0xBB, 0xBB, 0xBB, 0xBE, 0xBE, 0xBE, 0xF1, 0xF1, 0xF1, 0xFB, 
   0xFB, 0xFB, 0xF8, 0xF8, 0xF8, 0xF6, 0xF6, 0xF6, 0xF2, 0xF2, 0xF2, 0xF3, 0xF3, 0xF3, 0xF6, 0xF6, 
   0xF6, 0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xF5, 0xF5, 0xF5, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF8, 0xF8, 0xF8, 0xF4, 0xF4, 0xF4, 0xF0, 0xF0, 0xF0, 0xF6, 
   0xF6, 0xF6, 0xF3, 0xF3, 0xF3, 0xEB, 0xEB, 0xEB, 0xEE, 0xEE, 0xEE, 0xF2, 0xF2, 0xF2, 0xEE, 0xEE, 
   0xEE, 0xF2, 0xF2, 0xF2, 0xF4, 0xF4, 0xF4, 0xEE, 0xEE, 0xEE, 0xE4, 0xE4, 0xE4, 0xDE, 0xDE, 0xDE, 
   0xC2, 0xC3, 0xC3, 0xB5, 0xBC, 0xBC, 0xBD, 0xBD, 0xBD, 0xBB, 0xBE, 0xBE, 0xEF, 0xF0, 0xF0, 0xFA, 
   0xFA, 0xFA, 0xF9, 0xF9, 0xF9, 0xF6, 0xF6, 0xF6, 0xF4, 0xF4, 0xF4, 0xF7, 0xF7, 0xF7, 0xF6, 0xF6, 
   0xF6, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF8, 0xF8, 0xF8, 0xF6, 0xF6, 0xF6, 0xF6, 0xF6, 0xF6, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF8, 0xF8, 0xF8, 0xF7, 0xF7, 0xF7, 0xF3, 0xF3, 0xF3, 0xF5, 
   0xF5, 0xF5, 0xEF, 0xEF, 0xEF, 0xE7, 0xE7, 0xE7, 0xF0, 0xF0, 0xF0, 0xF1, 0xF1, 0xF1, 0xEF, 0xEF, 
   0xEF, 0xF1, 0xF1, 0xF1, 0xF2, 0xF2, 0xF2, 0xEB, 0xEB, 0xEB, 0xE3, 0xE3, 0xE3, 0xE2, 0xE2, 0xE2, 
   0xC6, 0xC7, 0xC7, 0xB8, 0xBC, 0xBC, 0xBA, 0xBD, 0xBD, 0xBB, 0xBC, 0xBC, 0xEB, 0xEB, 0xEB, 0xF4, 
   0xF4, 0xF4, 0xF6, 0xF6, 0xF6, 0xFA, 0xFA, 0xFA, 0xF9, 0xF9, 0xF9, 0xF6, 0xF5, 0xF5, 0xF1, 0xF1, 
   0xF1, 0xF5, 0xF5, 0xF5, 0xFA, 0xFA, 0xFA, 0xF9, 0xF9, 0xF9, 0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xFA, 0xFA, 0xFA, 0xF3, 0xF3, 0xF3, 0xF0, 
   0xF0, 0xF0, 0xEB, 0xEB, 0xEB, 0xEA, 0xEA, 0xEA, 0xF1, 0xF1, 0xF1, 0xF2, 0xF2, 0xF2, 0xF1, 0xF1, 
   0xF1, 0xF1, 0xF1, 0xF1, 0xEF, 0xEF, 0xEF, 0xEB, 0xEB, 0xEB, 0xE4, 0xE4, 0xE4, 0xD8, 0xD8, 0xD8, 
   0xC5, 0xC4, 0xC4, 0xB9, 0xBD, 0xBD, 0xB9, 0xBF, 0xBF, 0xB8, 0xB9, 0xB9, 0xE9, 0xE8, 0xE8, 0xF5, 
   0xF5, 0xF5, 0xF6, 0xF6, 0xF6, 0xF8, 0xF8, 0xF8, 0xF4, 0xF4, 0xF4, 0xEF, 0xF1, 0xF1, 0xF0, 0xF1, 
   0xF1, 0xF2, 0xF2, 0xF2, 0xF5, 0xF5, 0xF5, 0xF8, 0xF8, 0xF8, 0xFA, 0xFA, 0xFA, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xFA, 0xFA, 0xFA, 0xF3, 0xF3, 0xF3, 0xEA, 
   0xEA, 0xEA, 0xED, 0xED, 0xED, 0xEF, 0xEF, 0xEF, 0xEE, 0xEE, 0xEE, 0xED, 0xED, 0xED, 0xEF, 0xEF, 
   0xEF, 0xF1, 0xF1, 0xF1, 0xEF, 0xEF, 0xEF, 0xEF, 0xEF, 0xEF, 0xF0, 0xF0, 0xF0, 0xDA, 0xDA, 0xDA, 
   0xC3, 0xC2, 0xC2, 0xB8, 0xBC, 0xBC, 0xB8, 0xBC, 0xBC, 0xB7, 0xB7, 0xB7, 0xEB, 0xEA, 0xEA, 0xF6, 
   0xF6, 0xF6, 0xF3, 0xF3, 0xF3, 0xF4, 0xF4, 0xF4, 0xEF, 0xF0, 0xF0, 0xE9, 0xF1, 0xF1, 0xF3, 0xF3, 
   0xF3, 0xF6, 0xF6, 0xF6, 0xF7, 0xF7, 0xF7, 0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xF8, 0xF8, 0xF8, 
   0xF7, 0xF7, 0xF7, 0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF5, 0xF5, 0xF5, 0xEE, 
   0xEE, 0xEE, 0xEF, 0xEF, 0xEF, 0xEE, 0xEE, 0xEE, 0xF0, 0xF0, 0xF0, 0xEF, 0xEF, 0xEF, 0xEF, 0xEF, 
   0xEF, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF2, 0xF1, 0xF1, 0xF5, 0xF5, 0xF5, 0xE3, 0xE4, 0xE4, 
   0xBF, 0xC2, 0xC2, 0xB5, 0xBD, 0xBD, 0xBA, 0xBD, 0xBD, 0xB8, 0xBA, 0xBA, 0xEF, 0xEF, 0xEF, 0xFB, 
   0xFB, 0xFB, 0xF6, 0xF6, 0xF6, 0xF3, 0xF3, 0xF3, 0xF4, 0xF4, 0xF4, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF6, 0xF6, 0xF6, 
   0xF7, 0xF7, 0xF7, 0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xF8, 0xF8, 0xF8, 0xF6, 0xF6, 0xF6, 0xF4, 
   0xF4, 0xF4, 0xEF, 0xEF, 0xEF, 0xED, 0xED, 0xED, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF2, 0xF2, 
   0xF2, 0xF1, 0xF1, 0xF1, 0xF0, 0xF1, 0xF1, 0xED, 0xEE, 0xEE, 0xF4, 0xF4, 0xF4, 0xE3, 0xE3, 0xE3, 
   0xC1, 0xC2, 0xC2, 0xBD, 0xBC, 0xBC, 0xB7, 0xB8, 0xB8, 0xBB, 0xBB, 0xBB, 0xEC, 0xEC, 0xEC, 0xF6, 
   0xF6, 0xF6, 0xF6, 0xF6, 0xF6, 0xF9, 0xF9, 0xF9, 0xF8, 0xF8, 0xF8, 0xF6, 0xF6, 0xF6, 0xF8, 0xF8, 
   0xF8, 0xF9, 0xF9, 0xF9, 0xF8, 0xF8, 0xF8, 0xF3, 0xF3, 0xF3, 0xF6, 0xF6, 0xF6, 0xF9, 0xF9, 0xF9, 
   0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF8, 0xF8, 0xF8, 0xF7, 0xF7, 0xF7, 0xF4, 
   0xF4, 0xF4, 0xF3, 0xF3, 0xF3, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xEF, 0xEF, 
   0xEF, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xEA, 0xEA, 0xEA, 0xE8, 0xE8, 0xE8, 0xD1, 0xD1, 0xD1, 
   0xBE, 0xBE, 0xBE, 0xBD, 0xC0, 0xC0, 0xB2, 0xBD, 0xBD, 0xC1, 0xC0, 0xC0, 0xEB, 0xEB, 0xEB, 0xF9, 
   0xF8, 0xF8, 0xFA, 0xFA, 0xFA, 0xF9, 0xF9, 0xF9, 0xF7, 0xF6, 0xF6, 0xF7, 0xF7, 0xF6, 0xF6, 0xF6, 
   0xF6, 0xF8, 0xF8, 0xF7, 0xF5, 0xF5, 0xF5, 0xF9, 0xF9, 0xF8, 0xF9, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 
   0xFA, 0xFA, 0xF9, 0xFA, 0xFA, 0xF9, 0xFA, 0xFA, 0xF9, 0xFA, 0xF9, 0xF9, 0xF7, 0xF6, 0xF6, 0xF2, 
   0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF1, 0xF3, 0xF3, 0xF2, 0xF2, 0xF2, 0xF2, 0xF1, 0xF0, 
   0xF0, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xEE, 0xED, 0xED, 0xE8, 0xE8, 0xE8, 0xD3, 0xD4, 0xD4, 
   0xBE, 0xC0, 0xC0, 0xBF, 0xBF, 0xBF, 0xB5, 0xB6, 0xB6, 0xB8, 0xBB, 0xBB, 0xED, 0xEF, 0xEF, 0xF8, 
   0xFA, 0xFA, 0xF7, 0xF7, 0xF8, 0xF4, 0xF4, 0xF6, 0xF5, 0xF6, 0xF6, 0xF6, 0xF8, 0xF8, 0xF7, 0xF7, 
   0xF8, 0xF7, 0xF6, 0xF8, 0xF6, 0xF8, 0xF8, 0xF7, 0xF7, 0xF8, 0xF5, 0xF8, 0xF7, 0xF5, 0xF7, 0xF7, 
   0xF7, 0xF6, 0xF8, 0xF6, 0xF7, 0xF8, 0xF7, 0xF7, 0xF8, 0xF5, 0xF8, 0xF8, 0xF3, 0xF5, 0xF5, 0xF2, 
   0xF4, 0xF4, 0xF3, 0xF3, 0xF4, 0xF0, 0xEF, 0xF2, 0xF5, 0xF6, 0xF7, 0xF4, 0xF6, 0xF6, 0xF1, 0xF3, 
   0xF4, 0xF4, 0xF4, 0xF5, 0xF2, 0xF3, 0xF3, 0xF4, 0xF5, 0xF5, 0xFC, 0xFC, 0xFC, 0xE6, 0xE5, 0xE6, 
   0xC4, 0xC3, 0xC3, 0xBB, 0xBF, 0xBF, 0xC2, 0xC2, 0xC6, 0xC2, 0xCC, 0xD2, 0xCB, 0xDE, 0xE4, 0xD0, 
   0xE0, 0xE8, 0xD2, 0xDE, 0xE4, 0xD3, 0xDA, 0xE6, 0xD3, 0xE0, 0xE7, 0xD7, 0xDD, 0xE9, 0xD0, 0xDC, 
   0xE8, 0xCF, 0xD9, 0xE2, 0xD4, 0xDE, 0xE6, 0xD1, 0xDA, 0xE7, 0xD6, 0xE0, 0xE8, 0xD7, 0xE0, 0xE6, 
   0xD0, 0xDA, 0xE3, 0xD3, 0xDD, 0xE7, 0xD0, 0xDA, 0xE8, 0xD8, 0xE2, 0xE8, 0xD4, 0xE4, 0xE4, 0xD2, 
   0xE1, 0xE2, 0xD2, 0xDD, 0xE5, 0xD0, 0xD8, 0xE1, 0xD8, 0xDE, 0xE2, 0xD7, 0xE1, 0xE2, 0xD8, 0xDF, 
   0xE1, 0xCF, 0xDC, 0xE0, 0xCB, 0xE0, 0xE2, 0xCD, 0xE2, 0xE2, 0xD1, 0xE2, 0xE3, 0xD0, 0xE0, 0xE5, 
   0xC6, 0xCB, 0xCC, 0xBC, 0xBD, 0xBD, 0xBA, 0xBF, 0xC0, 0xBA, 0xBC, 0xC1, 0xAB, 0xB2, 0xB8, 0xB2, 
   0xB6, 0xBC, 0xAF, 0xB6, 0xB9, 0xB2, 0xB7, 0xBA, 0xB2, 0xB8, 0xBA, 0xB4, 0xB7, 0xBA, 0xAE, 0xB5, 
   0xB5, 0xB2, 0xB8, 0xB8, 0xB3, 0xB8, 0xB9, 0xAE, 0xB3, 0xB6, 0xB3, 0xB3, 0xB6, 0xB7, 0xB8, 0xB8, 
   0xB2, 0xB9, 0xB9, 0xB2, 0xB7, 0xB8, 0xAC, 0xB2, 0xB6, 0xB5, 0xB5, 0xB7, 0xB6, 0xBC, 0xBC, 0xB8, 
   0xBB, 0xBB, 0xB7, 0xBC, 0xBE, 0xB4, 0xBB, 0xBB, 0xBD, 0xBC, 0xBC, 0xBA, 0xBA, 0xBA, 0xB9, 0xB9, 
   0xB8, 0xB2, 0xBA, 0xBA, 0xB4, 0xBC, 0xBC, 0xB3, 0xBA, 0xBA, 0xB3, 0xBA, 0xBA, 0xB6, 0xBE, 0xBF, 
   0xBB, 0xC2, 0xC3, 0xBA, 0xBC, 0xBC, 0x00, 0x00, 
};

static RESOURCEDATA g_rd_tile(TEXT(MODULE_STRING), TEXT("tile.bmp"), g_tile, sizeof(g_tile));

extern "C" LPVOID PREPEND_MODULE(_tile) = (LPVOID)&g_rd_tile;
