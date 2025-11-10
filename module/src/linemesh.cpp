#include "linemesh.h"

#include <nap/core.h>
#include <mesh.h>
#include <renderglobals.h>
#include <renderservice.h>
#include <glm/gtc/constants.hpp>

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::LineMesh)
	RTTI_CONSTRUCTOR(nap::Core&)
	RTTI_PROPERTY("PolyLine", &nap::LineMesh::mPolyLine, nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("Usage", &nap::LineMesh::mUsage, nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("Count", &nap::LineMesh::mCount, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS


namespace nap
{
	static bool uploadVec3ToVec4(const std::vector<glm::vec3>& src, VertexBufferVec4& dst, float w, utility::ErrorState& errorState)
	{
		std::vector<glm::vec4> tmp;
		tmp.reserve(src.size());
		for (const auto& v : src)
			tmp.emplace_back(v.x, v.y, v.z, w);
		return dst.setData(tmp, errorState);
	}


	LineMesh::LineMesh(Core& core) :
		mRenderService(*core.getService<RenderService>()),
		mPositionBuffer({std::make_unique<VertexBufferVec4>(core), std::make_unique<VertexBufferVec4>(core)}),
		mNormalBuffer(std::make_unique<VertexBufferVec4>(core)),
		mUVBuffer(std::make_unique<VertexBufferVec4>(core)),
		mColorBuffer(std::make_unique<VertexBufferVec4>(core))
	{ }


	bool LineMesh::init(utility::ErrorState& errorState)
	{
		// Create resources
		const auto buffers = { mPositionBuffer[0].get(), mPositionBuffer[1].get(), mNormalBuffer.get(), mUVBuffer.get(), mColorBuffer.get() };
		for (auto* buffer : buffers)
		{
			buffer->mCount = mPolyLine->getMeshInstance().getNumVertices();
			buffer->mMemoryUsage = EMemoryUsage::Static;
			if (!buffer->init(errorState))
				return false;
		}

		// Create mesh instance
		mMeshInstance = std::make_unique<MeshInstance>(mRenderService);
		mMeshInstance->setNumVertices(std::max<int>(mCount, 2));
		mMeshInstance->setUsage(mUsage);
		mMeshInstance->setDrawMode(EDrawMode::LineStrip);
		mMeshInstance->setPolygonMode(EPolygonMode::Line);
		mMeshInstance->setCullMode(ECullMode::None);

		// Upload initial data for all vertex buffers from the line attributes
		auto& poly = mPolyLine->getMeshInstance();
		const auto& pos_attr = poly.getOrCreateAttribute<glm::vec3>(vertexid::position);
		for (auto& buf : mPositionBuffer)
			if (!uploadVec3ToVec4(pos_attr.getData(), *buf, 1.0f, errorState))
				return false;

		const auto& nor_attr = poly.getOrCreateAttribute<glm::vec3>(vertexid::normal);
		if (!uploadVec3ToVec4(nor_attr.getData(), *mNormalBuffer, 0.0f, errorState))
			return false;

		const auto& uv_attr = poly.getOrCreateAttribute<glm::vec3>(vertexid::uv);
		if (!uploadVec3ToVec4(uv_attr.getData(), *mUVBuffer, 0.0f, errorState))
			return false;

		const auto& col_attr = poly.getOrCreateAttribute<glm::vec4>(vertexid::color);
		if (!mColorBuffer->setData(col_attr.getData(), errorState))
			return false;

		// Attributes
		const std::vector position_buffer(mCount, glm::zero<glm::vec4>());
		mMeshInstance->getOrCreateAttribute<glm::vec4>(vertexid::position).setData(position_buffer);

		const std::vector normal_buffer(mCount, glm::zero<glm::vec4>());
		mMeshInstance->getOrCreateAttribute<glm::vec4>(vertexid::normal).setData(normal_buffer);

		const std::vector uv_buffer(mCount, glm::zero<glm::vec4>());
		mMeshInstance->getOrCreateAttribute<glm::vec4>(vertexid::getUVName(0)).setData(uv_buffer);

		const std::vector color_buffer(mCount, glm::one<glm::vec4>());
		mMeshInstance->getOrCreateAttribute<glm::vec4>(vertexid::getColorName(0)).setData(color_buffer);

		auto& shape = mMeshInstance->createShape();
		shape.setIndices(poly.getShape(0).getIndices().data(), poly.getShape(0).getNumIndices());

		return mMeshInstance->init(errorState);
	}
}
