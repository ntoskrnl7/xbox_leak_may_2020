//#define D3D_OVERLOADS
#include "d3dlocus.h"
#include "cd3dtest.h"
#include "ZCmp.h"

//************************************************************************
// Internal API:    ComboTests
//
// Purpose:         
//
// Return:          None.
//************************************************************************

void CZCmpTest::ComboTests(float Zval)
{
	Zval = (!Zval ? 0.01f : Zval);

	// Red Triangle
	VertexList[dwVertexCount++] = cD3DTLVERTEX(cD3DVECTOR((float)m_vpTest.X + 110.0f, 10.0f + (float)m_vpTest.Y,0.9999999f),(1.0f / 0.9999999f), RGBA_MAKE(255,0,0,255),0, 0.5f,0.0f);
	VertexList[dwVertexCount++] = cD3DTLVERTEX(cD3DVECTOR((float)m_vpTest.X + 210.0f,210.0f + (float)m_vpTest.Y,0.01f),     (1.0f / 0.01f),      RGBA_MAKE(255,0,0,255),0, 1.0f,1.0f);
	VertexList[dwVertexCount++] = cD3DTLVERTEX(cD3DVECTOR((float)m_vpTest.X +  10.0f,210.0f + (float)m_vpTest.Y,0.01f),     (1.0f / 0.01f),      RGBA_MAKE(255,0,0,255),0, 0.0f,1.0f);
	// Green Triangle
	VertexList[dwVertexCount++] = cD3DTLVERTEX(cD3DVECTOR((float)m_vpTest.X + 110.0f, 10.0f + (float)m_vpTest.Y,Zval),(1.0f / Zval), RGBA_MAKE(0,255,0,255),0, 0.5f,0.0f);
	VertexList[dwVertexCount++] = cD3DTLVERTEX(cD3DVECTOR((float)m_vpTest.X + 210.0f,210.0f + (float)m_vpTest.Y,Zval),(1.0f / Zval), RGBA_MAKE(0,255,0,255),0, 1.0f,1.0f);
	VertexList[dwVertexCount++] = cD3DTLVERTEX(cD3DVECTOR((float)m_vpTest.X +  10.0f,210.0f + (float)m_vpTest.Y,Zval),(1.0f / Zval), RGBA_MAKE(0,255,0,255),0, 0.0f,1.0f);
}

