
/*********************************************************

								Logical Box
		geometry representation that is not based on polygon.
		instead, it's based on analytic description

************************************************************/

#pragma once

namespace Noise3D
{

	//side's of AABB (or box in local space)
	enum NOISE_BOX_FACET
	{
		POS_X=0, 
		NEG_X=1, 
		POS_Y=2, 
		NEG_Y=3, 
		POS_Z=4, 
		NEG_Z=5,
		_INVALID_FACET=0xffffffff
	};

	class /*_declspec(dllexport)*/ LogicalBox :
		public ILogicalShape,
		public Collidable
	{
	public:

		//compute normal on given surface position
		//('facet' is used to explicit decide  face, avoid the rounding error on box's corner, might round to adjacent facet)
		static NVECTOR3 ComputeNormal(NOISE_BOX_FACET facet);

		//box is AABB in local space
		//void SetLocalBox(N_AABB aabb);
		void SetSizeXYZ(NVECTOR3 size);

		//box is AABB in local space
		N_AABB GetLocalBox();

		//ISceneObject::
		virtual NOISE_SCENE_OBJECT_TYPE GetObjectType()const override;

		//ISceneObject::
		virtual N_AABB GetLocalAABB() override;

		//ISceneObject::
		virtual N_AABB ComputeWorldAABB_Accurate() override;

		//ILogicalShape::
		virtual float ComputeArea() override;

		//ILogicalShape::
		//virtual bool IsPointInside(NVECTOR3 p) override;

	private:

		friend LogicalBox* LogicalShapeManager::CreateBox(SceneNode*, N_UID);
		
		friend class IFactory<LogicalBox>;

		LogicalBox();

		~LogicalBox();

		//N_AABB mLocalBox;
		NVECTOR3 mSize;

	};

}