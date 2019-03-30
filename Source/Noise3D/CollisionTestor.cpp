﻿
/***********************************************************************

								cpp ：Collision Testor
				provide all kinds of collision detection
				to all kinds of objects

************************************************************************/

#include "Noise3D.h"

using namespace Noise3D;
using namespace Noise3D::D3D;

CollisionTestor::CollisionTestor():
	m_pRefShaderVarMgr(nullptr),
	m_pSOGpuWriteableBuffer(nullptr),
	m_pSOCpuReadableBuffer(nullptr),
	m_pSOQuery(nullptr),
	m_pFX_Tech_Picking(nullptr),
	m_pDSS_DisableDepthTest(nullptr)
{

}

CollisionTestor::~CollisionTestor()
{
	ReleaseCOM(m_pSOCpuReadableBuffer);
	ReleaseCOM(m_pSOGpuWriteableBuffer);
	ReleaseCOM(m_pSOQuery);
	ReleaseCOM(m_pDSS_DisableDepthTest);
}

void CollisionTestor::Picking_GpuBased(Mesh * pMesh, const Vec2 & mouseNormalizedCoord, std::vector<Vec3>& outCollidedPointList)
{
	g_pImmediateContext->IASetInputLayout(g_pVertexLayout_Default);
	g_pImmediateContext->IASetVertexBuffers(0, 1, &pMesh->m_pVB_Gpu, &g_cVBstride_Default, &g_cVBoffset);
	g_pImmediateContext->IASetIndexBuffer(pMesh->m_pIB_Gpu, DXGI_FORMAT_R32_UINT, 0);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	g_pImmediateContext->OMSetBlendState(nullptr, NULL, 0x00000000);//disable color drawing??
	g_pImmediateContext->OMSetDepthStencilState(m_pDSS_DisableDepthTest, 0x00000000);

	//picking info
	m_pRefShaderVarMgr->SetVector2(IShaderVariableManager::NOISE_SHADER_VAR_VECTOR::PICKING_RAY_NORMALIZED_DIR_XY, mouseNormalizedCoord);

	//update camera Info
	Camera* pCamera = GetScene()->GetCamera();
	Matrix projMatrix, viewMatrix, invProjMatrix, invViewMatrix;
	pCamera->GetProjMatrix(projMatrix);
	pCamera->GetViewMatrix(viewMatrix);
	pCamera->GetViewInvMatrix(invViewMatrix);
	Vec3 camPos = pCamera->GetWorldTransform().GetPosition();
	m_pRefShaderVarMgr->SetMatrix(IShaderVariableManager::NOISE_SHADER_VAR_MATRIX::PROJECTION, projMatrix);
	m_pRefShaderVarMgr->SetMatrix(IShaderVariableManager::NOISE_SHADER_VAR_MATRIX::VIEW, viewMatrix);
	m_pRefShaderVarMgr->SetMatrix(IShaderVariableManager::NOISE_SHADER_VAR_MATRIX::VIEW_INV, invViewMatrix);
	m_pRefShaderVarMgr->SetVector3(IShaderVariableManager::NOISE_SHADER_VAR_VECTOR::CAMERA_POS3, camPos);


	//update target tested mesh world Matrix
	Matrix worldMat, worldInvTransMat;
	pMesh->ISceneObject::GetAttachedSceneNode()->EvalWorldTransform().GetAffineTransformMatrix(worldMat, worldInvTransMat);
	m_pRefShaderVarMgr->SetMatrix(IShaderVariableManager::NOISE_SHADER_VAR_MATRIX::WORLD, worldMat);
	m_pRefShaderVarMgr->SetMatrix(IShaderVariableManager::NOISE_SHADER_VAR_MATRIX::WORLD_INV_TRANSPOSE, worldInvTransMat);


	UINT offset = 0;
	ID3D11Buffer* pNullBuff = nullptr;
	//apply technique first !!!!!!!!!!!!!!!!!!!!!!!!then set SO Buffer
	m_pFX_Tech_Picking->GetPassByIndex(0)->Apply(0, g_pImmediateContext);
	g_pImmediateContext->SOSetTargets(1, &m_pSOGpuWriteableBuffer, &offset);

	//issue a draw call (with Begin() and End(), between which SO statistic will be recorded
	UINT indexCount = pMesh->GetIndexBuffer()->size();
	g_pImmediateContext->Begin(m_pSOQuery);
	g_pImmediateContext->DrawIndexed(indexCount, 0, 0);
	g_pImmediateContext->End(m_pSOQuery);
	g_pImmediateContext->SOSetTargets(1, &pNullBuff, &offset);

	//!!!!HERE's the problem : how many primitives had been Streamed Output???
	//Use ID3D11Query !!
	D3D11_QUERY_DATA_SO_STATISTICS  queriedStat;

	//this while seems to wait for Gpu's availability
	while (S_OK != g_pImmediateContext->GetData(
		m_pSOQuery,
		&queriedStat,	//different query types yield different return type. in this case, D3D11_QUERY_DATA_SO_STATISTICS
		m_pSOQuery->GetDataSize(),
		0));

	//get number of primitives written to SO buffer
	UINT returnedPrimCount = UINT(queriedStat.NumPrimitivesWritten);
	//PrimitivesStorageNeeded :: actually that initialized SO buffer 
	//might be not big enough hold all SO data from shaders


	//RETRIEVE RESULT!!
	//but first, we need to copy data from 'default' buffer to 'staging' buffer
	g_pImmediateContext->CopyResource(m_pSOCpuReadableBuffer, m_pSOGpuWriteableBuffer);
	//use device context->map
	D3D11_MAPPED_SUBRESOURCE mappedSR;
	HRESULT hr = S_OK;
	hr = g_pImmediateContext->Map(m_pSOCpuReadableBuffer, 0, D3D11_MAP_READ, NULL, &mappedSR);

	if (FAILED(hr))
	{
		ERROR_MSG("Collosion Testor : failed to retrieve collision data (DeviceContext::Map)");
		return;
	}

	//WARNING: please match the primitive format of 'shader SO' and 'here' 
	//SO primitive vertex format is defined in Effect source file.
	//(2017.1.28)currently POSITION.xyz <--> Vec3
	Vec3* pVecList = reinterpret_cast<Vec3*>(mappedSR.pData);
	for (UINT i = 0; i < returnedPrimCount; ++i)
	{
		outCollidedPointList.push_back(*(pVecList + i));
	}

	g_pImmediateContext->Unmap(m_pSOCpuReadableBuffer, 0);

}

UINT CollisionTestor::Picking_GpuBased(Mesh * pMesh, const Vec2 & mouseNormalizedCoord)
{
	//preparation is similar to another PICKING

	g_pImmediateContext->IASetInputLayout(g_pVertexLayout_Default);
	g_pImmediateContext->IASetVertexBuffers(0, 1, &pMesh->m_pVB_Gpu, &g_cVBstride_Default, &g_cVBoffset);
	g_pImmediateContext->IASetIndexBuffer(pMesh->m_pIB_Gpu, DXGI_FORMAT_R32_UINT, 0);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	g_pImmediateContext->OMSetBlendState(nullptr, NULL, 0x00000000);//disable color drawing??
	g_pImmediateContext->OMSetDepthStencilState(m_pDSS_DisableDepthTest, 0x00000000);

	//picking info
	m_pRefShaderVarMgr->SetVector2(IShaderVariableManager::NOISE_SHADER_VAR_VECTOR::PICKING_RAY_NORMALIZED_DIR_XY, mouseNormalizedCoord);

	//update camera Info
	Camera* pCamera = GetScene()->GetCamera();
	Matrix projMatrix, viewMatrix, invProjMatrix, invViewMatrix;
	pCamera->GetProjMatrix(projMatrix);
	pCamera->GetViewMatrix(viewMatrix);
	pCamera->GetViewInvMatrix(invViewMatrix);
	Vec3 camPos = pCamera->GetWorldTransform().GetPosition();
	m_pRefShaderVarMgr->SetMatrix(IShaderVariableManager::NOISE_SHADER_VAR_MATRIX::PROJECTION, projMatrix);
	m_pRefShaderVarMgr->SetMatrix(IShaderVariableManager::NOISE_SHADER_VAR_MATRIX::VIEW, viewMatrix);
	m_pRefShaderVarMgr->SetMatrix(IShaderVariableManager::NOISE_SHADER_VAR_MATRIX::VIEW_INV, invViewMatrix);
	m_pRefShaderVarMgr->SetVector3(IShaderVariableManager::NOISE_SHADER_VAR_VECTOR::CAMERA_POS3, camPos);

	//update target tested mesh world Matrix
	Matrix worldMat, worldInvTransMat;
	pMesh->ISceneObject::GetAttachedSceneNode()->EvalWorldTransform().GetAffineTransformMatrix(worldMat, worldInvTransMat);
	m_pRefShaderVarMgr->SetMatrix(IShaderVariableManager::NOISE_SHADER_VAR_MATRIX::WORLD, worldMat);
	m_pRefShaderVarMgr->SetMatrix(IShaderVariableManager::NOISE_SHADER_VAR_MATRIX::WORLD_INV_TRANSPOSE, worldInvTransMat);


	UINT offset = 0;
	ID3D11Buffer* pNullBuff = nullptr;
	//apply technique first !!!!!!!!!!!!!!!!!!!!!!!!then set SO Buffer
	m_pFX_Tech_Picking->GetPassByIndex(0)->Apply(0, g_pImmediateContext);
	g_pImmediateContext->SOSetTargets(1, &m_pSOGpuWriteableBuffer, &offset);

	//issue a draw call (with Begin() and End(), between which SO statistic will be recorded
	UINT indexCount = pMesh->GetIndexBuffer()->size();
	g_pImmediateContext->Begin(m_pSOQuery);
	g_pImmediateContext->DrawIndexed(indexCount, 0, 0);
	g_pImmediateContext->End(m_pSOQuery);
	g_pImmediateContext->SOSetTargets(1, &pNullBuff, &offset);

	//!!!!HERE's the problem : how many primitives had been Streamed Output???
	//Use ID3D11Query !!
	D3D11_QUERY_DATA_SO_STATISTICS  queriedStat;

	//this while seems to wait for Gpu's availability
	while (S_OK != g_pImmediateContext->GetData(
		m_pSOQuery,
		&queriedStat,	//different query types yield different return type. in this case, D3D11_QUERY_DATA_SO_STATISTICS
		m_pSOQuery->GetDataSize(),
		0));

	//get number of primitives written to SO buffer
	return UINT(queriedStat.NumPrimitivesWritten);
	//PrimitivesStorageNeeded :: actually that initialized SO buffer 
	//might be not big enough hold all SO data from shaders

}

bool Noise3D::CollisionTestor::IntersectRayAabb(const N_Ray & ray, const N_AABB & aabb)
{
	//if a ray hit AABB, then the ray param t of x/y/z will overlap
	//the intersection of those 3 t-value interval is the final intersection ray interval.
	//if the final interval is invalid (say left is greater than right),
	//then ray-AABB intersection failed/missed.
	//for more info, refer to Peter Shirley's 《Ray Tracing : The Next Week》
	//or pbrt-v3: https://github.com/mmp/pbrt-v3 /src/core/geometry.h Bounds3<T>::IntersectP
	float t_resultMin = 0;
	float t_resultMax = ray.t_max;

	//put aabb's component into an array to use a 'for'
	float rayOrigin[3] = { ray.origin.x, ray.origin.y, ray.origin.z };
	float rayDir[3] = { ray.dir.x, ray.dir.y, ray.dir.z };
	float slab_min[3] = { aabb.min.x, aabb.min.y, aabb.min.z };
	float slab_max[3] = { aabb.max.x, aabb.max.y, aabb.max.z };

	//mark to test near and far's hit state
	bool isNearHit = false;
	bool isFarHit = false;

	//calculate a pair of slabs for an interval in each loop
	for (int i = 0; i < 3; ++i)
	{
		//note the corner case ray[i]==0, division result will be infinity, which is still well-defined by IEEE
		//and fortunately, the [t_near, t_far] result is still correct
		float rayDirReciprocal = 1.0f / rayDir[i];
		float t_near = (slab_min[i] - rayOrigin[i]) * rayDirReciprocal;
		float t_far = (slab_max[i] - rayOrigin[i]) * rayDirReciprocal;

		// Update ray's parameter t's interval from slab intersection
		if (t_near > t_far) std::swap(t_near, t_far);

		// Update _tFar_ to ensure robust ray--bounds intersection(pbrt-v3, 'rounding errors')
		t_far *= (1 + 2 * mFunc_Gamma(3));

		//try to narrow the t value interval
		if (t_near > t_resultMin) { t_resultMin = t_near; isNearHit = true; }
		if (t_far < t_resultMax) { t_resultMax = t_far;  isFarHit = true; }

		//validate t interval. 
		if (t_resultMin >= t_resultMax)
		{
			//if not valid, then the ray miss the AABB
			return false;
		}
	}

	//t_min& t_max rejection
	if (t_resultMin<ray.t_min || t_resultMin > ray.t_max)isNearHit = false;
	if (t_resultMax<ray.t_min || t_resultMax > ray.t_max)isFarHit = false;

	//2 intersection points at most, but if there is none:
	if (!isNearHit && !isFarHit)return false;

	return true;
}

bool Noise3D::CollisionTestor::IntersectRayAabb(const N_Ray & ray, const N_AABB & aabb, N_RayHitResult & outHitRes)
{
	//similar to another overloaded version of IntersectRayAABB
	//except that this version keep track of possible hit's corresponding facet.
	float t_resultMin = 0;
	float t_resultMax = ray.t_max;
	NOISE_BOX_FACET result_minHitFacetId, result_maxHitFacetId;//convert to NOISE_BOX_FACET later

	//put aabb's component into an array to use a 'for'
	float rayOrigin[3] = { ray.origin.x, ray.origin.y, ray.origin.z };
	float rayDir[3] = { ray.dir.x, ray.dir.y, ray.dir.z };
	float slab_min[3] = { aabb.min.x, aabb.min.y, aabb.min.z };
	float slab_max[3] = { aabb.max.x, aabb.max.y, aabb.max.z };

	//mark to test near and far's hit state
	bool isNearHit = false;
	bool isFarHit = false;

	//calculate a pair of slabs for an interval in each loop
	for (int i = 0; i < 3; ++i)
	{
		//note the corner case ray[i]==0, division result will be infinity, which is still well-defined by IEEE
		//and fortunately, the [t_near, t_far] result is still correct
		float rayDirReciprocal = 1.0f / rayDir[i];
		float t_near = (slab_min[i] - rayOrigin[i]) * rayDirReciprocal;
		float t_far = (slab_max[i] - rayOrigin[i]) * rayDirReciprocal;

		// Update ray's parameter t's interval from slab intersection
		if (t_near > t_far) std::swap(t_near, t_far);

		// Update _tFar_ to ensure robust ray--bounds intersection(pbrt-v3, 'rounding errors')
		t_far *= (1 + 2 * mFunc_Gamma(3));

		//get two possible hits' facet id
		NOISE_BOX_FACET nearFacetId, farFacetId;
		mFunction_AabbFacet(i, rayDir[i], nearFacetId, farFacetId);//+x:0, -x:1, +y:2, -y:3, +z:4, -z:5, 

		//try to narrow the t value interval(and update hit facet accordingly)
		if (t_near > t_resultMin)
			{t_resultMin = t_near;	isNearHit = true;	result_minHitFacetId = nearFacetId;}
		if (t_far < t_resultMax)
			{t_resultMax = t_far;	isFarHit = true;	result_maxHitFacetId = farFacetId;}

		//validate t interval. 
		if (t_resultMin >= t_resultMax)
		{
			//if not valid, then the ray miss the AABB
			return false;
		}
	}

	//t_min& t_max rejection
	if (t_resultMin<ray.t_min || t_resultMin > ray.t_max)isNearHit = false;
	if (t_resultMax<ray.t_min || t_resultMax > ray.t_max)isFarHit = false;

	//2 intersection points at most, but if there is none then quit
	if (!isNearHit && !isFarHit)return false;

	//after evaluating the intersection of 3 t-value intervals
	//[t_resultMin, t_resultMax] is still valid, then output the result
	if (isNearHit)
	{
		N_RayHitInfo hitInfo;
		hitInfo.t = t_resultMin;
		hitInfo.pos = ray.Eval(t_resultMin);
		hitInfo.normal = LogicalBox::ComputeNormal(result_minHitFacetId);//static function to compute normal of an AABB
		outHitRes.hitList.push_back(hitInfo);
	}

	if (isFarHit)
	{
		N_RayHitInfo hitInfo;
		hitInfo.t = t_resultMax;
		hitInfo.pos = ray.Eval(t_resultMax);
		hitInfo.normal = LogicalBox::ComputeNormal(result_maxHitFacetId);//static function to compute normal of an AABB
		outHitRes.hitList.push_back(hitInfo);
	}

	return true;

}

bool Noise3D::CollisionTestor::IntersectRayBox(const N_Ray & ray, LogicalBox * pBox)
{
	if (pBox == nullptr)
	{
		ERROR_MSG("CollisionTestor: object is nullptr.");
		return false;
	}

	if (!pBox->Collidable::IsCollidable())return false;

	//convert ray to model space (but first scene object must attach to scene node)
	N_Ray localRay;
	RayIntersectionTransformHelper helper;
	if (!helper.Ray_WorldToModel(ray, false, pBox, localRay))return false;

	//perform Ray-AABB intersection in local space
	N_AABB localBox = pBox->GetLocalBox();
	bool anyHit = CollisionTestor::IntersectRayAabb(localRay, localBox);

	return anyHit;
}

bool Noise3D::CollisionTestor::IntersectRayBox(const N_Ray & ray, LogicalBox* pBox, N_RayHitResult & outHitRes)
{
	if (pBox == nullptr)
	{
		ERROR_MSG("CollisionTestor: object is nullptr.");
		return false;
	}

	if (!pBox->Collidable::IsCollidable())return false;

	//convert ray to model space (but first scene object must attach to scene node)
	N_Ray localRay;
	RayIntersectionTransformHelper helper;
	if (!helper.Ray_WorldToModel(ray, false, pBox, localRay))return false;

	//perform Ray-AABB intersection in local space
	N_AABB localBox = pBox->GetLocalBox();
	bool anyHit = CollisionTestor::IntersectRayAabb(localRay, localBox, outHitRes);

	//convert result point back to world space
	helper.HitResult_ModelToWorld(outHitRes);

	return anyHit;
}

bool Noise3D::CollisionTestor::IntersectRaySphere(const N_Ray & ray, LogicalSphere* pSphere, N_RayHitResult & outHitRes)
{
	if (pSphere == nullptr)
	{
		ERROR_MSG("CollisionTestor: object is nullptr.");
		return false;
	}

	if (!pSphere->Collidable::IsCollidable())return false;

	//convert ray to model space (but first scene object must attach to scene node)
	N_Ray localRay;
	RayIntersectionTransformHelper helper;
	if (!helper.Ray_WorldToModel(ray, true, pSphere, localRay))return false;

	//Ray-Sphere intersection can be easily evaluated by solving a quadratic equation
	//*** Ray P=O+tD
	//*** sphere:  x^2+y^2+z^2=r^2
	//combine this two and re-write the equation into quadratic equation At^2+Bt+C=0
	// this gives us the coefficients of the equation:
	// A = D.x^2 + D.y^2 +  D.z^2  = D dot D
	// B = 2(D.x*O.x + D.y*O.y + D.z*O.z）= 2(D dot O)
	// C = O.x^2 + O.y^2 + O.z^2 - r^2 = O dot O -r^2
	float r = pSphere->GetRadius();//no scale
	Vec3& D = localRay.dir;
	Vec3& O = localRay.origin;
	float A = D.Dot(D);
	float B = 2.0f * D.Dot(O);
	float C = O.Dot(O) - r*r;

	//determinant of quadratic equation
	float det = B * B - 4.0f * A*C;
	if (det < 0.0001f || D == Vec3(0.0f,0.0f,0.0f))
	{
		//0(miss) or 1(tangent) solution, or A==0(then the equation even not exist)
		return false;
	}
	else
	{
		float sqrtDet = std::sqrtf(det);

		float t1 = (-B + sqrtDet) / (2.0f * A);
		if (t1 >= ray.t_min && t1 <= ray.t_max)//t_min && t_max rejection
		{
			//local space hit info 1
			N_RayHitInfo hitInfo1;
			hitInfo1.t = t1;
			hitInfo1.pos = localRay.Eval(t1);
			hitInfo1.normal = hitInfo1.pos;
			hitInfo1.normal.Normalize();
			outHitRes.hitList.push_back(hitInfo1);
		}

		float t2 = (-B - sqrtDet) / (2.0f * A);
		if (t2 >= ray.t_min && t2 <= ray.t_max)//t_min && t_max rejection
		{
			//local space hit info 2
			N_RayHitInfo hitInfo2;
			hitInfo2.t = t2;
			hitInfo2.pos = localRay.Eval(t2);
			hitInfo2.normal = hitInfo2.pos;
			hitInfo2.normal.Normalize();
			outHitRes.hitList.push_back(hitInfo2);
		}
	}

	//transform the result hit back to world space
	helper.HitResult_ModelToWorld(outHitRes);

	return outHitRes.HasAnyHit();
}

bool Noise3D::CollisionTestor::IntersectRayTriangle(const N_Ray & ray, Vec3 v0, Vec3 v1, Vec3 v2, N_RayHitInfo & outHitInfo)
{
	//[Reference for the implementation]
	//Tomas Möller, Trumbore B . Fast, Minimum Storage Ray-Triangle Intersection[J]. 2005.
	//the method can be describe as follow:
	//**Triangle(V0, V1, V2)
	//**Ray p=O+tD (O for origin, D for direction)
	//**Point in Triangle R(u,v) = (1-u-v)V0 + uV1 + vV2
	//then use CRAMER's RULE to solve the linear equation system for the unknown (t,u,v):
	//		O+tD = (1-u-v)V0 + uV1 + vV2		******(u >0, v>0, u+v<=1)
	// for more info, plz refer to document [Notes]Intersection between Ray and Shapes.docx
	//the code here won't be intuitive, because it's mainly algebraic inductions.
	Vec3 E1 = v1 - v0;
	Vec3 E2 = v2 - v0;

	Vec3 P = ray.dir.Cross(E2);//P = D x E2
	float det = P.Dot(E1);//3x3 matrix's determinant= (d x e2) dot e1 = P dot e1
	if (std::abs(det) <= std::numeric_limits<float>::epsilon())return false;

	float invDet = 1.0f / det;//calculate once
	Vec3 M = ray.origin - v0;// M = O - V0
	float u = invDet * M.Dot(P);// invDet * (M dot P)
	if (u < 0.0f || u> 1.0f)return false;//early return to avoid further computation

	Vec3 Q = M.Cross(E1);//Q = M x E1
	float v = invDet * ray.dir.Dot(Q);//v = invDet * (D dot Q)
	if (v < 0.0f || u + v>1.0f)return false;//early return to avoid further computation

	float result_t = invDet * E2.Dot(Q);//t = invDet * (E2 dot Q)

	//t_min && t_max rejection
	if (result_t < ray.t_min || result_t > ray.t_max)return false;

	outHitInfo.t = result_t;
	outHitInfo.pos = ray.Eval(result_t);
	outHitInfo.normal = E1.Cross(E2);
	return true;
}

bool Noise3D::CollisionTestor::IntersectRayTriangle(const N_Ray & ray, const N_DefaultVertex & v0, const N_DefaultVertex & v1, const N_DefaultVertex & v2, N_RayHitInfo & outHitInfo)
{
	//same as another overload of IntersectTriangle(), with with Mesh's vertex format as input
	//(to make full use of the normal/Tangent vector of Mesh*)
	//Normal interpolation of hit point is implemented.
	Vec3 E1 = v1.Pos - v0.Pos;
	Vec3 E2 = v2.Pos - v0.Pos;

	Vec3 P = ray.dir.Cross(E2);//P = D x E2
	float det = P.Dot(E1);//3x3 matrix's determinant= (d x e2) dot e1 = P dot e1
	if (std::abs(det) <= std::numeric_limits<float>::epsilon())return false;//early return to avoid further computation

	float invDet = 1.0f / det;//calculate once
	Vec3 M = ray.origin - v0.Pos;// M = O - V0
	float u = invDet * M.Dot(P);// invDet * (M dot P)
	if (u < 0.0f || u> 1.0f)return false;//early return to avoid further computation

	Vec3 Q = M.Cross(E1);//Q = M x E1
	float v = invDet * ray.dir.Dot(Q);//v = invDet * (D dot Q)
	if (v < 0.0f || u + v>1.0f)return false;//early return to avoid further computation

	float result_t = invDet * E2.Dot(Q);//t = invDet * (E2 dot Q)

	//t_min && t_max rejection
	if (result_t < ray.t_min || result_t > ray.t_max)return false;

	outHitInfo.t = result_t;
	outHitInfo.pos = ray.Eval(result_t);
	outHitInfo.normal = ((1.0f - u - v)*v0.Normal + u*v1.Normal + v*v2.Normal);//normal's interpolation
	outHitInfo.normal.Normalize();
	return true;
}

bool Noise3D::CollisionTestor::IntersectRayMesh(const N_Ray & ray, Mesh * pMesh, N_RayHitResult & outHitRes)
{
	if (pMesh == nullptr)
	{
		ERROR_MSG("CollisionTestor: object is nullptr.");
		return false;
	}

	if (!pMesh->Collidable::IsCollidable())return false;

	//convert ray to model space (but first scene object must attach to scene node)
	N_Ray localRay;
	RayIntersectionTransformHelper helper;
	if (!helper.Ray_WorldToModel(ray,false, pMesh, localRay))return false;

	//get vertex data (be noted that the vertex is in MODEL SPACE
	const std::vector<N_DefaultVertex>* pVB = pMesh->GetVertexBuffer();
	const std::vector<uint32_t>* pIB = pMesh->GetIndexBuffer();

	//apply ray-triangle intersection for each triangle in model space
	for (uint32_t i = 0; i < pMesh->GetTriangleCount(); ++i)
	{
		N_RayHitInfo hitInfo;
		const N_DefaultVertex& v0 = pVB->at(pIB->at(3 * i + 0));
		const N_DefaultVertex& v1 = pVB->at(pIB->at(3 * i + 1));
		const N_DefaultVertex& v2 = pVB->at(pIB->at(3 * i + 2));

		//!!!!!!!!!!!!!!!
		//(2019.3.23)there might be a problem, if ray hit the triangle edge, 
		//then it might generate 2 hit points from 2 ray-triangle tests.

		//delegate each ray-tri intersection task to another function
		if (CollisionTestor::IntersectRayTriangle(localRay, v0, v1, v2, hitInfo))
		{
			outHitRes.hitList.push_back(hitInfo);
		}
	}

	//convert result point back to world space
	helper.HitResult_ModelToWorld(outHitRes);

	return outHitRes.HasAnyHit();
}

bool Noise3D::CollisionTestor::IntersectRayScene(const N_Ray & ray, N_RayHitResult & outHitRes)
{
	//warn the user if BVH is not rebuilt
	BvhNode* pBvhRoot = mBvhTree.GetRoot();
	if (pBvhRoot->IsLeafNode() && pBvhRoot->GetSceneObject() == nullptr)
	{
		WARNING_MSG("IntersectRayScene: BVH tree seems to be empty. Forgot to rebuild BVH? or there is no collidable object in the scene?");
		return false;
	}

	//start the recursion and BVH branch pruning
	mFunction_IntersectRayBvhNode(ray, pBvhRoot, outHitRes);
	return outHitRes.HasAnyHit();
}

bool Noise3D::CollisionTestor::RebuildBvhTree(const SceneGraph & graph)
{
	mBvhTree.Reset();
	return mBvhTree.Construct(graph);
}


/*************************************************

							P R I V A T E

************************************************/

bool CollisionTestor::mFunction_Init()
{
	m_pRefShaderVarMgr = IShaderVariableManager::GetSingleton();
	if (m_pRefShaderVarMgr == nullptr)
	{
		ERROR_MSG("Collision Testor: Initialization failed! (Shader var manager)");
		return false;
	}

	//Create Id3d11query to get SO info
	//so this page for more info
	//https://www.gamedev.net/blog/272/entry-1913400-using-d3d11-stream-out-for-debugging/
	D3D11_QUERY_DESC queryDesc;
	queryDesc.Query = D3D11_QUERY_SO_STATISTICS;//this enum for usage, D3d11_query_xxx
	queryDesc.MiscFlags = NULL;


	HRESULT hr = g_pd3dDevice11->CreateQuery(&queryDesc, &m_pSOQuery);
	HR_DEBUG(hr, "Collision Testor : Initialization failed! (ID3D11Query for SO Info)");


	//SO Buffer initial data
	N_MinimizedVertex initArr[c_maxSOVertexCount];
	ZeroMemory(initArr, sizeof(N_MinimizedVertex) * c_maxSOVertexCount);
	D3D11_SUBRESOURCE_DATA initData;
	ZeroMemory(&initData, sizeof(initData));
	initData.pSysMem = initArr;


	//create 2 Stream Output buffer (one for gpu write(default) , one for cpu read(staging))
	// 1. GPU writeable SO buffer
	D3D11_BUFFER_DESC desc;
	desc.BindFlags = D3D11_BIND_STREAM_OUTPUT;
	desc.ByteWidth = c_maxSOVertexCount * sizeof(N_MinimizedVertex);//only float4 position
	desc.CPUAccessFlags = NULL;
	desc.Usage = D3D11_USAGE_DEFAULT;//the slowest usage but read from VideoMem is allowed
	desc.MiscFlags = NULL;
	desc.StructureByteStride = NULL;
	hr = g_pd3dDevice11->CreateBuffer(&desc, &initData, &m_pSOGpuWriteableBuffer);
	
	HR_DEBUG(hr, "Collision Testor : failed to create Stream Out Buffer! #1" + std::to_string(hr));

	// 2. CPU readable SO data buffer , Simply re-use the above struct
	desc.BindFlags =NULL;//if we 
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.Usage = D3D11_USAGE_STAGING;
	hr = g_pd3dDevice11->CreateBuffer(&desc, &initData, &m_pSOCpuReadableBuffer);

	HR_DEBUG(hr, "Collision Testor : failed to create Stream Out Buffer #2!" + std::to_string(hr));

	//Create DSS
	if (!mFunction_InitDSS())return false;

	//Create Techiniques from FX
	m_pFX_Tech_Picking = g_pFX->GetTechniqueByName("PickingIntersection");

	return true;
}

bool CollisionTestor::mFunction_InitDSS()
{
	HRESULT hr = S_OK;
	//depth stencil state
	D3D11_DEPTH_STENCIL_DESC dssDesc;
	ZeroMemory(&dssDesc, sizeof(dssDesc));
	dssDesc.DepthEnable = TRUE;
	dssDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dssDesc.DepthEnable = FALSE;
	dssDesc.StencilEnable = FALSE;
	hr = g_pd3dDevice11->CreateDepthStencilState(&dssDesc, &m_pDSS_DisableDepthTest);
	HR_DEBUG(hr, "Collision Testor : Create Depth Stencil State Failed!!!");
	return true;
}

inline float Noise3D::CollisionTestor::mFunc_Gamma(int n)
{
	//'gamma' for floating error in pbrt-v3 /scr/core/pbrt.h
	return (n * std::numeric_limits<float>::epsilon()) / (1 - n * std::numeric_limits<float>::epsilon());
}

inline void  Noise3D::CollisionTestor::mFunction_AabbFacet(uint32_t slabsPairId, float dirComponent, NOISE_BOX_FACET& nearHit, NOISE_BOX_FACET& farHit)
{
	if(slabsPairId>2)
	{
		WARNING_MSG("Collision Testor: slabs id invalid.");
		nearHit = NOISE_BOX_FACET::_INVALID_FACET;
		farHit = NOISE_BOX_FACET::_INVALID_FACET;
		return;
	}

	//get facet ID in Ray-Slabs intersection(assume current dir component is positive)
	nearHit = static_cast<NOISE_BOX_FACET>(2 * slabsPairId + 1);//NOISE_BOX_FACET::NEG_???
	farHit = static_cast<NOISE_BOX_FACET>(2 * slabsPairId + 0);//NOISE_BOX_FACET::POS_???

	//if dir component is negative, reverse the result
	bool isDirNeg = dirComponent<0.0f;
	if (isDirNeg)std::swap(nearHit, farHit);
}

void Noise3D::CollisionTestor::mFunction_IntersectRayBvhNode(const N_Ray & ray, BvhNode * bvhNode, N_RayHitResult& outHitRes)
{
	//test ray against current bvh AABB
	//(2019.3.30)WARNING: in some cases, the ray is inside AABB, but there won't be intersection on the surface
	//however, the ray might still hit something inside the AABB
	N_AABB aabb = bvhNode->GetAABB();
	bool isRayEndPointInside = aabb.IsPointInside(ray.origin) || aabb.IsPointInside(ray.Eval(1.0f));
	bool isHit = CollisionTestor::IntersectRayAabb(ray, aabb);//ray's start,end are outside the AABB but still possible to hit
	if (isRayEndPointInside || isHit )
	{
		//extract concrete object to intersect
		if (bvhNode->IsLeafNode())
		{
			ISceneObject* pObj = bvhNode->GetSceneObject();
			N_RayHitResult tmpResult;
			
			//(2019.3.30)well, it shouldn't run into the 'return' line if the BVH construction is correct.
			//but i'm not totally confident about that orz= =
			if (pObj == nullptr)return;

			switch (pObj->GetObjectType())
			{
			case NOISE_SCENE_OBJECT_TYPE::LOGICAL_BOX:
				CollisionTestor::IntersectRayBox(ray, static_cast<LogicalBox*>(pObj), tmpResult);
				break;
			case NOISE_SCENE_OBJECT_TYPE::LOGICAL_SPHERE:
				CollisionTestor::IntersectRaySphere(ray, static_cast<LogicalSphere*>(pObj), tmpResult);
				break;
			case NOISE_SCENE_OBJECT_TYPE::MESH:
				CollisionTestor::IntersectRayMesh(ray, static_cast<Mesh*>(pObj), tmpResult);
				break;
			default:
				ERROR_MSG("Error: Bug!! I forgot to include some collidable object.");
				break;
			}
			outHitRes.Union(tmpResult);

		}//isLeafNode
		else
		{
			//gather results from
			for (uint32_t i = 0; i < bvhNode->GetChildNodeCount(); ++i)
			{
				N_RayHitResult tmpResult;
				BvhNode* pChildNode = bvhNode->GetChildNode(i);
				CollisionTestor::mFunction_IntersectRayBvhNode(ray, pChildNode, tmpResult);
				outHitRes.Union(tmpResult);//push to back
			}

		}
	}//isHit
	//else, BVH branch pruned. thus accelerated.
}

bool Noise3D::CollisionTestor::RayIntersectionTransformHelper::Ray_WorldToModel(
	const N_Ray& in_ray_world, bool isRigidTransform, ISceneObject * pObj, N_Ray& out_ray_local)
{
	//convert the box to local space to use Ray-AABB intersection
	SceneNode* pNode = pObj->GetAttachedSceneNode();
	if (pNode == nullptr)
	{
		WARNING_MSG("CollisionTestor: Box is not bound to a scene node.");
		return false;
	}

	//get 'World' related transform matrix (all are useful)
	if (isRigidTransform)
	{
		pNode->EvalWorldTransform_Rigid().GetAffineTransformMatrix(worldMat, worldInvMat, worldInvTransposeMat);
	}
	else
	{
		pNode->EvalWorldTransform().GetAffineTransformMatrix(worldMat, worldInvMat, worldInvTransposeMat);
	}

	//transform the ray into local space
	//note that dir(vector) has no translation, we can't just apply affine matrix worldInv to it
	N_Ray localRay;
	localRay.origin = AffineTransform::TransformVector_MatrixMul(in_ray_world.origin, worldInvMat);
	Vec3 localRayEnd = AffineTransform::TransformVector_MatrixMul(in_ray_world.Eval(1.0f), worldInvMat);
	localRay.dir = localRayEnd - localRay.origin;
	localRay.t_max = in_ray_world.t_max;

	out_ray_local = localRay;
	return true;
}

void Noise3D::CollisionTestor::RayIntersectionTransformHelper::HitResult_ModelToWorld(N_RayHitResult & hitResult)
{
	//transform only the hit results back to world space(minimize the count of inv transform)
	for (auto& refHitInfo : hitResult.hitList)
	{
		//hitInfo.t = hitInfo.t
		refHitInfo.normal = AffineTransform::TransformVector_MatrixMul(refHitInfo.normal, worldInvTransposeMat);
		refHitInfo.normal.Normalize();
		refHitInfo.pos = AffineTransform::TransformVector_MatrixMul(refHitInfo.pos, worldMat);
	}
}
