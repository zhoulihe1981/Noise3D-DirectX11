/***********************************************************

					Path Tracer Shader: Minimal

			Noise3D::PathTracer's soft shader "hello world", 
			for demonstration and test usage

***********************************************************/
#include "Noise3D.h"

using namespace Noise3D;

void Noise3D::GI::PathTracerShader_Minimal::ClosestHit(int bounces, float travelledDistance,const N_Ray& ray, const N_RayHitInfoForPathTracer & hitInfo, N_TraceRayPayload & in_out_payload)
{
	//in_out_payload.radiance = GI::Radiance(1.0f, 0, 0);
	//in_out_payload.radiance = GI::Radiance(abs(hitInfo.normal.x), abs(hitInfo.normal.y), abs(hitInfo.normal.z));
	Vec3 rgb = hitInfo.normal;
	if (rgb.x < 0)rgb.x *= -0.5f;
	if (rgb.y < 0)rgb.y *= -0.5f;
	if (rgb.z < 0)rgb.z *= -0.5f;
	in_out_payload.radiance = GI::Radiance(rgb);
}

void Noise3D::GI::PathTracerShader_Minimal::Miss(N_Ray ray, N_TraceRayPayload & in_out_payload)
{
	in_out_payload.radiance = GI::Radiance(0, 0, 0);
}