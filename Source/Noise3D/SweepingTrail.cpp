
/***********************************************************************

								h��SweepingTrail

	A Sweeping trail several main characteristic:
	1. a trail path is driven by a sequence of spatial line segments
	2. adjacent line segments pairs can build up 2 triangles ( or a spatial 'quad')
	3. the header of the trail (the head line segment) must be updated in every frame
	4. the header of the trail should be "cooled down" when reached a given time duration limit.
			A new "free" header line trail shall be generated by then.
	5. the tail of the trail(the last line segment) must be updated in every frame, and
			approaching to the second last line segment over time to reduce the area 
			of the last quad (Meanwhile, the u coord of the tail vertices is maintained to 1)
	6. (2018.7.23)the headers' vertices u coord should be 0, while the tail should be 1.

************************************************************************/

#include "Noise3D.h"

using namespace Noise3D;

Noise3D::ISweepingTrail::ISweepingTrail() :
	mIsHeaderActive(false),
	mIsTailActive(false),
	mHeaderCoolDownTimeThreshold(20.0f),
	mHeaderCoolDownTimer(0.0f),
	mTailQuadCollapseDuration(20.0f),
	mTailQuadCollapsingTimer(0.0f)
{
}

Noise3D::ISweepingTrail::~ISweepingTrail()
{
	ReleaseCOM(m_pVB_Gpu);
}

void Noise3D::ISweepingTrail::SetHeaderLineSegment(N_LineSegment lineSeg)
{
	mFreeHeader = lineSeg;
}

void Noise3D::ISweepingTrail::SetHeaderCoolDownTime(float duration)
{
	mHeaderCoolDownTimeThreshold = duration;
}

void Noise3D::ISweepingTrail::SetTailCollapsedTime(float duration)
{
	mTailQuadCollapseDuration = duration;
}

void Noise3D::ISweepingTrail::Update(float deltaTime)
{
	mFunction_UpdateHeaderPos();
	mFunction_UpdateTailPos();
	mFunction_UpdateUV();
}

/*****************************************************************


*****************************************************************/
bool NOISE_MACRO_FUNCTION_EXTERN_CALL Noise3D::ISweepingTrail::mFunction_InitGpuBuffer(UINT maxVertexCount)
{
	D3D11_SUBRESOURCE_DATA tmpInitData_Vertex;
	ZeroMemory(&tmpInitData_Vertex, sizeof(tmpInitData_Vertex));
	tmpInitData_Vertex.pSysMem = &mVB_Mem.at(0);
	UINT vertexCount = mVB_Mem.size();

	//Simple Vertex!
	D3D11_BUFFER_DESC vbd;
	vbd.ByteWidth = sizeof(N_SweepingTrailVertexType)* vertexCount;
	vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	//create gpu vertex buffer
	int hr = 0;
	hr = D3D::g_pd3dDevice11->CreateBuffer(&vbd, &tmpInitData_Vertex, &m_pVB_Gpu);
	HR_DEBUG(hr, "SweepingTrail : Failed to create vertex pool ! ");
}

void Noise3D::ISweepingTrail::mFunction_UpdateHeaderPos()
{
}

void Noise3D::ISweepingTrail::mFunction_UpdateTailPos()
{
}

void Noise3D::ISweepingTrail::mFunction_UpdateUV()
{
}
