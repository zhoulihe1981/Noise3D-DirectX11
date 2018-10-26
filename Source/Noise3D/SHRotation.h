
/***********************************************************************

					 Spherical Harmonic Rotation

		(2018.10.26)implementation of SH Rotation(cpu-end): 
		in actually render application of SH-based shading, 
		SH-expanded transfer function should rotated/transformed
		to interact with light function under the same coordinate frame/space.
		There are a few way to rotate a SH function by directly manipulating
		SH coefficients, and not by projecting rotated monte-carlo vectors:
		1. Wigner Matrix (ZYZ, ZXZXZ)
		2. Zonal Harmonic Factorization

************************************************************************/

#pragma once

namespace Noise3D
{
	namespace GI
	{
		//(2018.10.26)Zonal Harmonics factorization might be implemented in the future <Sparse Zonal Harmonic Factorization for Efficient SH Rotation>

		//simple encapsulation of a series of Wigner Matrix for SH Rotation
		class SHRotationWignerMatrix
		{
		public:

			//(2018.10.26) signed integer m & n are the notation in Lisle's <Algorithms for Spherical Harmonic Lighting>
			//i and the papers adopt the convention that Top-Left is (+L,+L), Bottom-Right is (-L,-L)
			// l for band index, m & n for matrix element index
			//band L Wigner sub-matrix has (2L+1)*(2L+1) elements (-L<=m<=L)
			SHRotationWignerMatrix(uint32_t highestBandIndex);

			float At(uint32_t l, uint32_t row, uint32_t col);

			float At(uint32_t l, int m, int n);

			void	Set(uint32_t l, uint32_t row, uint32_t col,float val);

			void Set(uint32_t l, int m, int n,float val);
			
			void	Rotate(RigidTransform t, std::vector <NColor4f>& in_out_SHVector);

		private:
			//perform SH z-rotation without explicitly constructing Wigner Matrix
			void mFunction_RotateZ(float angle, std::vector<NColor4f>& in_out_SHVector);

			uint32_t mHighestBandIndex;//inclusive SH band index
			std::vector<std::vector<float>> mMat;//a series of wigner matrix (in different band)

		};
	}
}