/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// Local Includes
#include "renderlinecomponent.h"

// External Includes
#include <entity.h>
#include <nap/core.h>
#include <renderservice.h>
#include <renderglobals.h>
#include <transformcomponent.h>

RTTI_BEGIN_CLASS(nap::RenderLineComponent)
	RTTI_PROPERTY("MaterialInstance",	&nap::RenderLineComponent::mMaterialInstance,	nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("Color",				&nap::RenderLineComponent::mColor,				nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("Opacity",			&nap::RenderLineComponent::mOpacity,			nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("LineWidth",			&nap::RenderLineComponent::mLineWidth,			nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("PointSize",			&nap::RenderLineComponent::mPointSize,			nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("ComputeLine",		&nap::RenderLineComponent::mComputeLine,		nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::RenderLineComponentInstance)
	RTTI_CONSTRUCTOR(nap::EntityInstance&, nap::Component&)
	RTTI_FUNCTION(nap::material::instance::getOrCreateMaterial, &nap::RenderLineComponentInstance::getOrCreateMaterial)
RTTI_END_CLASS

namespace nap
{
	namespace uniform
	{
		static constexpr const char* UBO = "UBO";
		static constexpr const char* color = "color";
		static constexpr const char* alpha = "alpha";
	}


	/**
	 * Creates the uniform with the given name, logs an error when not available.
	 * @return the uniform, nullptr if not available.
	 */
	template<typename T>
	static T* getUniform(const std::string& uniformName, UniformStructInstance& uniformStruct, utility::ErrorState& error)
	{
		T* found_uniform = uniformStruct.getOrCreateUniform<T>(uniformName);
		return error.check(found_uniform != nullptr,
			"Unable to get or create uniform with name: %s in struct: %s", uniformName.c_str(), uniformStruct.getDeclaration().mName.c_str()) ?
			found_uniform : nullptr;
	}


	//////////////////////////////////////////////////////////////////////////
	// RenderLineComponent
	//////////////////////////////////////////////////////////////////////////

	void RenderLineComponent::getDependentComponents(std::vector<rtti::TypeInfo>& components) const
	{
		components.push_back(RTTI_OF(ComputeLineComponent));
		components.push_back(RTTI_OF(TransformComponent));
	}


	//////////////////////////////////////////////////////////////////////////
	// RenderLineComponentInstance
	//////////////////////////////////////////////////////////////////////////

	RenderLineComponentInstance::RenderLineComponentInstance(EntityInstance& entity, Component& resource) :
		RenderableComponentInstance(entity, resource),
		mRenderService(entity.getCore()->getService<RenderService>())
	{ }


	bool RenderLineComponentInstance::init(utility::ErrorState& errorState)
	{
		// Cache resource
		mResource = getComponent<RenderLineComponent>();

		// Fetch mesh
		mMesh = &mComputeLine->getLineMesh();
		assert(mMesh != nullptr);

		// Ensure there is a transform component
		mTransform = getEntityInstance()->findComponent<TransformComponentInstance>();
		if (!errorState.check(mTransform != nullptr, "%s: missing transform component", mID.c_str()))
			return false;

		// Initialize base class
		if (!RenderableComponentInstance::init(errorState))
			return false;

		// Create constant material instance
		if (!mMaterialInstance.init(*mRenderService, mResource->mMaterialInstance, errorState))
			return false;

		// Since the material can't be changed at run-time, cache the matrices to set on draw
		// If the struct is found, we expect the matrices with those names to be there
		// Ensure the mvp struct is available
		mMVPStruct = mMaterialInstance.getOrCreateUniform(uniform::mvpStruct);
		if (!errorState.check(mMVPStruct != nullptr, "%s: Unable to find uniform MVP struct: %s in shader: %s",
			mID.c_str(), uniform::mvpStruct, mMaterialInstance.getMaterial().getShader().getDisplayName().c_str()))
			return false;

		// Get all matrices
		mModelMatUniform = getUniform<UniformMat4Instance>(uniform::modelMatrix, *mMVPStruct, errorState);
		mViewMatUniform = getUniform<UniformMat4Instance>(uniform::viewMatrix, *mMVPStruct, errorState);
		mProjectMatUniform = getUniform<UniformMat4Instance>(uniform::projectionMatrix, *mMVPStruct, errorState);
		// mNormalMatrixUniform = getUniform<UniformMat4Instance>(uniform::normalMatrix, *mMVPStruct, errorState);
		// mCameraWorldPosUniform = getUniform<UniformVec3Instance>(uniform::cameraPosition,  *mMVPStruct, errorState);
		if (mModelMatUniform == nullptr || mViewMatUniform == nullptr || mProjectMatUniform == nullptr)
			return false;

		// Get all constant uniforms
		mUBOStruct = mMaterialInstance.getOrCreateUniform(uniform::UBO);
		if (!errorState.check(mUBOStruct != nullptr, "%s: Unable to find uniform struct: %s in shader: %s",
			mID.c_str(), uniform::UBO, mMaterialInstance.getMaterial().getShader().getDisplayName().c_str()))
			return false;

		mColorUniform = getUniform<UniformVec3Instance>(uniform::color, *mUBOStruct, errorState);	assert(mColorUniform != nullptr);
		mAlphaUniform = getUniform<UniformFloatInstance>(uniform::alpha, *mUBOStruct, errorState); assert(mAlphaUniform != nullptr);

		// Create mesh / material combo that can be rendered to target
		mRenderableMesh = mRenderService->createRenderableMesh(*mMesh, mMaterialInstance, errorState);
		if (!mRenderableMesh.isValid())
			return false;

		return true;
	}


	void RenderLineComponentInstance::update(double deltaTime)
	{
		mColorUniform->setValue(mResource->mColor->mValue.toVec3());
		mAlphaUniform->setValue(mResource->mOpacity->mValue);
	}


	void RenderLineComponentInstance::onDraw(IRenderTarget& renderTarget, VkCommandBuffer commandBuffer, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
	{
		// Skip rendering if opacity is zero
		if (mResource->mOpacity->mValue <= math::epsilon<float>())
			return;

		// Get material to work with
		if (!mRenderableMesh.isValid())
		{
			assert(false);
			return;
		}

		// Set mvp matrices if present in material
		if (mProjectMatUniform != nullptr)
			mProjectMatUniform->setValue(projectionMatrix);

		if (mViewMatUniform != nullptr)
			mViewMatUniform->setValue(viewMatrix);

		if (mModelMatUniform != nullptr)
			mModelMatUniform->setValue(mTransform->getGlobalTransform());

		// if (mNormalMatrixUniform != nullptr)
		// 	mNormalMatrixUniform->setValue(glm::transpose(glm::inverse(mTransform->getGlobalTransform())));
		//
		// if (mCameraWorldPosUniform != nullptr)
		// 	mCameraWorldPosUniform->setValue(math::extractPosition(glm::inverse(viewMatrix)));

		// Acquire new / unique descriptor set before rendering
		auto& mat_instance = mMaterialInstance;
		const auto& descriptor_set = mat_instance.update();

		// Fetch and bind pipeline
		utility::ErrorState error_state;
		auto pipeline = mRenderService->getOrCreatePipeline(renderTarget, mRenderableMesh.getMesh(), mat_instance, error_state);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.mPipeline);

		// Bind shader descriptors
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.mLayout, 0, 1, &descriptor_set.mSet, 0, nullptr);

		// Copy the ordered vector of VkBuffers from the renderable mesh
		auto vertex_buffers = mRenderableMesh.getVertexBuffers();

		// Override position vertex attribute buffer with storage buffer.
		// We do this by first fetching the internal buffer binding index of the position vertex attribute.
		// Overwrite the VkBuffer under the previously fetched position vertex attribute index.
		int position_attr_binding_idx = mRenderableMesh.getVertexBufferBindingIndex(vertexid::position);
		if (position_attr_binding_idx >= 0)
			vertex_buffers[position_attr_binding_idx] = mMesh->getPositionBuffer(LineMesh::EBufferRank::Read).getBuffer();

		int normal_attr_binding_idx = mRenderableMesh.getVertexBufferBindingIndex(vertexid::normal);
		if (normal_attr_binding_idx >= 0)
			vertex_buffers[normal_attr_binding_idx] = mMesh->getNormalBuffer(LineMesh::EBufferRank::Read).getBuffer();

		int uv_attr_binding_idx = mRenderableMesh.getVertexBufferBindingIndex(vertexid::uv);
		if (uv_attr_binding_idx >= 0)
			vertex_buffers[uv_attr_binding_idx] = mMesh->getUVBuffer(LineMesh::EBufferRank::Read).getBuffer();

		int color_attr_binding_idx = mRenderableMesh.getVertexBufferBindingIndex(vertexid::getColorName(0));
		if (color_attr_binding_idx >= 0)
			vertex_buffers[color_attr_binding_idx] = mMesh->getColorBuffer(LineMesh::EBufferRank::Read).getBuffer();

		// Get offsets
		const auto& offsets = mRenderableMesh.getVertexBufferOffsets();

		// Bind buffers
		// The shader will now use the storage buffer updated by the compute shader as a vertex buffer when rendering the current mesh.
		vkCmdBindVertexBuffers(commandBuffer, 0, vertex_buffers.size(), vertex_buffers.data(), offsets.data());

		// Draw points or lines
		vkCmdSetLineWidth(commandBuffer, mResource->mLineWidth);

		const auto& index_buffer = mMesh->getMeshInstance().getGPUMesh().getIndexBuffer(0);
		vkCmdBindIndexBuffer(commandBuffer, index_buffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(commandBuffer, index_buffer.getCount(), 1, 0, 0, 0);

		vkCmdSetLineWidth(commandBuffer, 1.0f);
	}
}
