/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// External includes
#include <rendercomponent.h>
#include <materialinstance.h>
#include <renderablemesh.h>
#include <mesh.h>
#include <nap/resourceptr.h>
#include <componentptr.h>
#include <parameternumeric.h>
#include <parametercolor.h>

// Local includes
#include "computelinecomponent.h"

namespace nap
{
	// Forward declares
	class RenderLineComponentInstance;
	class TransformComponentInstance;
	class PolyLine;

	/**
	 * RenderLineComponent
	 */
	class NAPAPI RenderLineComponent : public RenderableComponent
	{
		RTTI_ENABLE(RenderableComponent)
		DECLARE_COMPONENT(RenderLineComponent, RenderLineComponentInstance)

	public:
		void getDependentComponents(std::vector<rtti::TypeInfo>& components) const;

		MaterialInstanceResource mMaterialInstance;								///< Property: 'Material' The material instance resource
		float mLineWidth = 1.0f;												///< Property: 'LineWidth' line stroke width
		float mPointSize = 32.0f;												///< Property: 'PointSize' point size

		ComponentPtr<ComputeLineComponent> mComputeLine;
	};


	/**
	 * RenderLineComponentInstance
	 */
	class NAPAPI RenderLineComponentInstance : public RenderableComponentInstance
	{
		RTTI_ENABLE(RenderableComponentInstance)
	public:
		// Default constructor
		RenderLineComponentInstance(EntityInstance& entity, Component& resource);

		/**
		 * Checks whether a transform component is available.
		 */
		bool init(utility::ErrorState& errorState) override;

		/**
		 * onDraw() override
		 */
		void onDraw(IRenderTarget& renderTarget, VkCommandBuffer commandBuffer, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) override;

		/**
		 * Returns the program used to render the mesh.
		 *
		 * TODO: This should be private, but our current RTTI implementation 'mangles' class name-spaces,
		 * causing the RTTR_REGISTRATION_FRIEND macro to fail -> needs to be fixed.
		 * It is therefore not recommended to use this function at runtime, use 'getMaterialInstance' instead!
		 *
		 * @return material handle
		 */
		MaterialInstance* getOrCreateMaterial() { return &mMaterialInstance; }

	private:
		ComponentInstancePtr<ComputeLineComponent> mComputeLine = { this, &RenderLineComponent::mComputeLine };

		RenderLineComponent*				mResource = nullptr;				///< Reference to resource
		RenderService*						mRenderService = nullptr;			///< Reference to render service
		TransformComponentInstance*			mTransform = nullptr;				///< Component transform

		RenderableMesh						mRenderableMesh;					///< The renderable box mesh
		MaterialInstance					mMaterialInstance;					///< The MaterialInstance as created from the resource.

		UniformStructInstance*				mMVPStruct = nullptr;				///< model view projection struct
		UniformMat4Instance*				mModelMatUniform = nullptr;			///< Pointer to the model matrix uniform
		UniformMat4Instance*				mViewMatUniform = nullptr;			///< Pointer to the view matrix uniform
		UniformMat4Instance*				mProjectMatUniform = nullptr;		///< Pointer to the projection matrix uniform
		UniformMat4Instance*				mNormalMatrixUniform = nullptr;		///< Pointer to the normal matrix uniform
		UniformVec3Instance*				mCameraWorldPosUniform = nullptr;	///< Pointer to the camera world position uniform

		LineMesh* 							mMesh = nullptr;
	};
}
