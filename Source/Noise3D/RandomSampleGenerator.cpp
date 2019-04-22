/***********************************************************************

								Random Sample Generator

************************************************************************/

#include "Noise3D.h"
#include "Noise3D_InDevHeader.h"

using namespace Noise3D;

std::default_random_engine Noise3D::GI::RandomSampleGenerator::mRandomEngine = std::default_random_engine();

std::uniform_real_distribution<float> Noise3D::GI::RandomSampleGenerator::mCanonicalDist = std::uniform_real_distribution<float>(0.0, 1.0f);

std::uniform_real_distribution<float> Noise3D::GI::RandomSampleGenerator::mNormalizedDist = std::uniform_real_distribution<float>(-1.0f, 1.0f);

Noise3D::GI::RandomSampleGenerator::RandomSampleGenerator()
{
}

float Noise3D::GI::RandomSampleGenerator::CanonicalReal()
{
	return mCanonicalDist(mRandomEngine);
}

float Noise3D::GI::RandomSampleGenerator::NormalizedReal()
{
	return mNormalizedDist(mRandomEngine);
}

Vec3 Noise3D::GI::RandomSampleGenerator::UniformSphericalVec()
{
	float var1 = mCanonicalDist(mRandomEngine);
	float var2 = mCanonicalDist(mRandomEngine);

	//according to <gritty detail> and my own paper note
	//we can generate random points that uniformly distribute on a sphere 
	//by mapping a pair of canonical variable (x,y) to azimuthal angle (theta, phi) using formula:
	//(theta,phi) <-- (2arccos(sqrt(1-x)) , 2\pi * y)
	//or
	//(theta,phi)<-- (arccos(1-2x),2\pi*y)
	//but obviously, arccos(1-2x) is a alittle more computationally efficient
	float theta = acosf(1 - 2.0f * var1);
	float phi = 2 * Ut::PI * var2;

	//NOTE: this parameterization is different from the common one
	//the 'theta' start from the top of the Y-axis
	Vec3 out = mFunc_UniformSphericalVecGen_AzimuthalToDir(theta, phi);

	return out;
}

Vec2 Noise3D::GI::RandomSampleGenerator::UniformSphericalAzimuthal()
{
	float var1 = mCanonicalDist(mRandomEngine);
	float var2 = mCanonicalDist(mRandomEngine);

	//according to <gritty detail> and my own paper note
	//we can generate random points that uniformly distribute on a sphere 
	//by mapping a pair of canonical variable (x,y) to azimuthal angle (theta, phi) using formula:
	//(theta,phi) <-- (2arccos(sqrt(1-x)) , 2\pi * y)
	//or
	//(theta,phi)<-- (arccos(1-2x),2\pi*y)
	//but obviously, arccos(1-2x) is a alittle more computationally efficient
	float theta = acosf(1 - 2.0f * var1);
	float phi = 2 * Ut::PI * var2;
	return Vec2(theta, phi);
}

Vec3 Noise3D::GI::RandomSampleGenerator::UniformSphericalVec_Cone(Vec3 normal, float maxAngle)
{
	//(2019.4.12)perahps there is no need to clamp to [0, pi]

	//(2019.4.12) similar to the original UniformSphericalVec()
	float var1 = mCanonicalDist(mRandomEngine);
	float var2 = mCanonicalDist(mRandomEngine);

	//(2019.4.18)derived with partial sphere's pdf and Inverse Transform Sampling
	//float theta = acosf(1 - 2.0f * var1);
	//float phi = 2 * Ut::PI * var2;
	float theta = acosf(1.0f - (1.0f - cosf(maxAngle))*var1);
	float phi = 2.0f * Ut::PI * var2;

	//(2019.4.12)and they lie within a cone with angle 'maxAngle'
	Vec3 vec = mFunc_UniformSphericalVecGen_AzimuthalToDir(theta, phi);

	//no need to transform
	if (normal.x==0.0f && normal.z ==0.0f && normal.y!=0.0f)return vec;

	//transform local sample to world space
	//original samples are centered vec is Y axis (0,1,0) if theta = 0;
	//now need to transform to new basis whose new Y axis is 'normal'
	normal.Normalize();
	Vec3 matRow1, matRow2, matRow3;
	mFunc_ConstructTransformMatrixFromYtoNormal(normal, matRow1, matRow2, matRow3);

	//flattened matrix mul/basis transformation
	Vec3 out;
	out.x = matRow1.Dot(vec);
	out.y = matRow2.Dot(vec);
	out.z = matRow3.Dot(vec);

	//(tried quaternion rot. 
	//but it has overhead of more arccos() and cos() sin() op)
	/*Vec3 y_axis = Vec3(0, 1.0f, 0);
	Vec3 rotAxis = y_axis.Cross(normal);
	float rotAngle = acosf(y_axis.Dot(normal));
	Quaternion q = XMQuaternionRotationAxis(rotAxis, rotAngle);*/

	return out;
}

void Noise3D::GI::RandomSampleGenerator::UniformSphericalVec_Cone(Vec3 normal, float maxAngle, int sampleCount, std::vector<Vec3>& outVecList, float& outPdf)
{
	//transform matrix/basis only construct once
	normal.Normalize();
	Vec3 matRow1, matRow2, matRow3;
	mFunc_ConstructTransformMatrixFromYtoNormal(normal, matRow1, matRow2, matRow3);

	//S=2 *pi * r^2( 1-cos theta)
	outPdf = 1.0f / (2.0f * Ut::PI * 1.0f * 1.0f * (1.0f - cosf(maxAngle)));

	for (int i = 0; i < sampleCount; ++i)
	{
		float var1 = mCanonicalDist(mRandomEngine);
		float var2 = mCanonicalDist(mRandomEngine);
		//float theta = acosf(1 - 2.0f * var1) * maxAngle / Ut::PI;
		//float phi = 2 * Ut::PI * var2;
		//F(theta)=(1-costheta)/(r^2(1-cosMaxAngle))
		float oneMinusCos = 1.0f - cosf(maxAngle);
		float theta = acosf(1.0f - oneMinusCos*var1);
		float phi = Ut::PI * 2.0f * var2;
		Vec3 vec = mFunc_UniformSphericalVecGen_AzimuthalToDir(theta, phi);

		if (normal.x == 0.0f && normal.z == 0.0f && normal.y != 0.0f)
		{
			outVecList.push_back(vec);
		}
		else
		{
			//flattened matrix mul/basis transformation
			Vec3 out;
			out.x = matRow1.Dot(vec);
			out.y = matRow2.Dot(vec);
			out.z = matRow3.Dot(vec);
			outVecList.push_back(out);
		}
	}
}

Vec3 Noise3D::GI::RandomSampleGenerator::UniformSphericalVec_Hemisphere(Vec3 normal)
{
	//can be optimized using UniformSphereVec (inverse those in the other side of hemisphere)
	return GI::RandomSampleGenerator::UniformSphericalVec_Cone(normal, Ut::PI / 2.0f);
}

void Noise3D::GI::RandomSampleGenerator::UniformSphericalVec_Hemisphere(Vec3 normal, int sampleCount, std::vector<Vec3>& outVecList)
{
	float tmpS;
	RandomSampleGenerator::UniformSphericalVec_Cone(normal, Ut::PI / 2.0f, sampleCount, outVecList,tmpS);
}

void Noise3D::GI::RandomSampleGenerator::UniformSphericalVec_ShadowRays(Vec3 pos, ISceneObject * pObj, int sampleCount, std::vector<Vec3>& outVecList, float& outPdf)
{
	//cast rays to objects to sample local lighting
	if (pObj == nullptr)return;
	Vec3 centerDirVec;
	float coneAngle = 0.0f;
	mFunc_ComputeConeAngleForShadowRay(pObj, pos, centerDirVec, coneAngle);

	RandomSampleGenerator::UniformSphericalVec_Cone(centerDirVec, coneAngle,sampleCount,outVecList, outPdf);
}

void Noise3D::GI::RandomSampleGenerator::CosinePdfSphericalVec_Cone(Vec3 normal, float maxAngle, int sampleCount, std::vector<Vec3>& outVecList, std::vector<float>& outPdfList)
{
	//maxAngle : gamma
	if (maxAngle >= Ut::PI / 2.0f) maxAngle = Ut::PI / 2.0f - 0.0001f;

	//transform matrix/basis only construct once
	normal.Normalize();
	Vec3 matRow1, matRow2, matRow3;
	mFunc_ConstructTransformMatrixFromYtoNormal(normal, matRow1, matRow2, matRow3);
	//float pdfNormalizeFactor = 1.0f / (2.0f * Ut::PI * sinf(maxAngle));//denominator of cosine-weighted pdf
	float pdfNormalizeFactor = 2.0f / (Ut::PI * (1.0f-cosf(2.0f*maxAngle)) );//denominator of cosine-weighted pdf

	for (int i = 0; i < sampleCount; ++i)
	{
		float var1 = mCanonicalDist(mRandomEngine);
		float var2 = mCanonicalDist(mRandomEngine);
		//more info, plz refer to my zhihu article about 'SH lighting and Uniform Spherical sampling'
		//learn about the Inverse Transform Sampling(ITS)

		// assume pdf p(theta,phi)= (k costheta)/ S_cone, then
		//F_theta = sintheta / singamma
		//float theta =asinf(sinf(maxAngle) * var1);
		//float phi = 2.0f * Ut::PI *  var2;

		float theta = 0.5f * acosf(1.0f - (1.0f - cosf(2.0f * maxAngle))* var1);
		float phi =2.0f * Ut::PI *  var2;

		Vec3 vec = mFunc_UniformSphericalVecGen_AzimuthalToDir(theta, phi);

		if (normal.x == 0.0f && normal.z == 0.0f && normal.y != 0.0f)
		{
			outVecList.push_back(vec);
		}
		else
		{
			//flattened matrix mul/basis transformation
			Vec3 out;
			out.x = matRow1.Dot(vec);
			out.y = matRow2.Dot(vec);
			out.z = matRow3.Dot(vec);
			outVecList.push_back(out);
		}

		outPdfList.push_back(cosf(theta)*pdfNormalizeFactor);
	}
}

void Noise3D::GI::RandomSampleGenerator::CosinePdfSphericalVec_ShadowRays(Vec3 pos, ISceneObject * pObj, int sampleCount, std::vector<Vec3>& outVecList, std::vector<float>& outPdfList)
{
	//cast rays to objects to sample local lighting
	if (pObj == nullptr)return;
	Vec3 centerDirVec;
	float coneAngle = 0.0f;
	mFunc_ComputeConeAngleForShadowRay(pObj, pos, centerDirVec, coneAngle);

	RandomSampleGenerator::CosinePdfSphericalVec_Cone(centerDirVec, coneAngle,sampleCount,outVecList,outPdfList);

}

void Noise3D::GI::RandomSampleGenerator::RectShadowRays(Vec3 pos, LogicalRect * pRect, int sampleCount, std::vector<Vec3>& outVecList, std::vector<float>& outPdfList)
{
	if (pRect == nullptr)return;

	//get rect's world transform
	AffineTransform t = pRect->GetAttachedSceneNode()->EvalWorldTransform();
	NOISE_RECT_ORIENTATION ori = pRect->GetOrientation();
	outVecList.reserve(sampleCount);
	outPdfList.reserve(sampleCount);

	Vec3 localNormal = pRect->ComputeNormal();//used to compute pdf
	for (int i = 0; i < sampleCount; ++i)
	{
		Vec3 localPointOnRect = pRect->GenLocalRandomPoint();
		Vec3 worldPointOnRect = t.TransformVector_Affine(localPointOnRect);
		Vec3 dir = worldPointOnRect - pos;
		float dirLength = dir.Length();
		dir.Normalize();
		outVecList.push_back(dir);

		Vec3 worldNormal = t.TransformVector_Rigid(localNormal);
		worldNormal -= t.GetPosition();
		//d \omega / dA = <n,dir>/r^2
		float cos = abs(worldNormal.Dot(dir));
		outPdfList.push_back((dirLength*dirLength) /(cos * pRect->ComputeArea()) );

	}

	//now the sample parameter uses (u,v) on rect instead of (theta,phi) of tracing position
	//outArea_InvPdf = pRect->ComputeArea();
}

/*****************************************

							PRIVATE

*****************************************/

inline Vec3 Noise3D::GI::RandomSampleGenerator::mFunc_UniformSphericalVecGen_AzimuthalToDir(float theta, float phi)
{
	//NOTE: this parameterization is different from the common one
	//the 'theta' start from the top of the Y-axis
	Vec3 vec;
	vec.x = sinf(theta)*cosf(phi);
	vec.y = cosf(theta);
	vec.z = sinf(theta)*sinf(phi);
	return vec;
}

void Noise3D::GI::RandomSampleGenerator::mFunc_ConstructTransformMatrixFromYtoNormal(Vec3 n, Vec3 & outMatRow1, Vec3 & outMatRow2, Vec3 & outMatRow3)
{
	Vec3 y_axis = Vec3(0, 1.0f, 0);

	//new basis
	Vec3 new_x_axis = y_axis.Cross(n);
	Vec3 new_y_axis = n;
	Vec3 new_z_axis = new_x_axis.Cross(new_y_axis);
	new_x_axis.Normalize();
	new_z_axis.Normalize();

	//construct transform matrix
	/*
	[ (Y x N)   (N)   ((YxN)xN)] * vec
	*/
	outMatRow1 = { new_x_axis.x, new_y_axis.x ,new_z_axis.x };
	outMatRow2 = { new_x_axis.y, new_y_axis.y ,new_z_axis.y };
	outMatRow3 = { new_x_axis.z, new_y_axis.z ,new_z_axis.z };
}

inline void Noise3D::GI::RandomSampleGenerator::mFunc_ComputeConeAngleForShadowRay(ISceneObject * pObj, Vec3 pos, Vec3 & outCenterDirVec, float & outConeAngle)
{
	N_BoundingSphere sphere = pObj->ComputeWorldBoundingSphere_Accurate();
	outCenterDirVec =  sphere.pos - pos;//trace a cone of rays from pos to light source (pObj)

	//calculate tangent cone  (start from pos) of bounding sphere
	float a = sphere.radius;
	//float b, the tangent line
	float c = (outCenterDirVec == Vec3(0,0,0)? 0: outCenterDirVec.Length());//hypotenuse
	outConeAngle = 0.0f;
	if (a != 0)
	{
		outConeAngle = a > c ? (Ut::PI /2.0f) : (Ut::PI / 2.0f - std::acosf(a / c));
	}
}
