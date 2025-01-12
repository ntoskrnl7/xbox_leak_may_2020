///////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1999 - 2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:       d3dx8.h
//  Content:    D3DX utility library
//
///////////////////////////////////////////////////////////////////////////

#ifndef __D3DX8_H__
#define __D3DX8_H__

#include <d3d8.h>
#include <limits.h>

#include <xobjbase.h>

#ifndef D3DXINLINE
#ifdef __cplusplus
#define D3DXINLINE inline
#else
#define D3DXINLINE _inline
#endif
#endif

#define D3DX_DEFAULT ULONG_MAX
#define D3DX_DEFAULT_FLOAT FLT_MAX

;begin_external
#include "d3dx8math.h"
#include "d3dx8core.h"
#include "d3dx8tex.h"
#include "d3dx8mesh.h"
#include "d3dx8shape.h"
#include "d3dx8effect.h"
;end_external

;begin_internal
#include "d3dx8math.h"
#include "d3dx8core.h"
#include "d3dx8tex.h"
#include "d3dx8meshp.h"
#include "d3dx8shape.h"
#include "d3dx8effect.h"
;end_internal

#endif //__D3DX8_H__
