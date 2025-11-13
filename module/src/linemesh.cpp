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


	static uint swapBufferIndex(uint index)
	{
		return (index + 1) % 2;
	}


	/**
	 * LineMesh
	 */
	LineMesh::LineMesh(Core& core) :
		mRenderService(*core.getService<RenderService>()),
		mPositionBuffer({std::make_unique<VertexBufferVec4>(core), std::make_unique<VertexBufferVec4>(core), std::make_unique<VertexBufferVec4>(core), std::make_unique<VertexBufferVec4>(core)}),
		mNormalBuffer({std::make_unique<VertexBufferVec4>(core), std::make_unique<VertexBufferVec4>(core), std::make_unique<VertexBufferVec4>(core), std::make_unique<VertexBufferVec4>(core)}),
		mUVBuffer({std::make_unique<VertexBufferVec4>(core), std::make_unique<VertexBufferVec4>(core), std::make_unique<VertexBufferVec4>(core), std::make_unique<VertexBufferVec4>(core)}),
		mColorBuffer({std::make_unique<VertexBufferVec4>(core), std::make_unique<VertexBufferVec4>(core), std::make_unique<VertexBufferVec4>(core), std::make_unique<VertexBufferVec4>(core)})
	{ }


	bool LineMesh::init(utility::ErrorState& errorState)
	{
		const uint count = mPolyLine->getMeshInstance().getNumVertices();

		// Create resources
		const std::vector buffers = { &mPositionBuffer, &mNormalBuffer, &mUVBuffer, &mColorBuffer };
		for (const auto* quad_buffer : buffers)
		{
			for (uint i = 0; i < QUAD_BUFFER_COUNT; i++)
			{
				auto& buf = (*quad_buffer)[i];
				switch (static_cast<EBufferRank>(i))
				{
					case EBufferRank::Original:
						buf->mMemoryUsage = EMemoryUsage::Static;
						buf->mClear = false;
						break;
					case EBufferRank::Readback:
						buf->mMemoryUsage = EMemoryUsage::DynamicRead;
						buf->mClear = false;
						break;
					default:
						buf->mMemoryUsage = EMemoryUsage::Static;
						buf->mClear = true;
				}
				buf->mCount = count;
				if (!buf->init(errorState))
					return false;
			}
		}

		// Create mesh instance
		mMeshInstance = std::make_unique<MeshInstance>(mRenderService);
		mMeshInstance->setNumVertices(std::max<int>(count, 2));
		mMeshInstance->setUsage(mUsage);
		mMeshInstance->setDrawMode(EDrawMode::LineStrip);
		mMeshInstance->setPolygonMode(EPolygonMode::Line);
		mMeshInstance->setCullMode(ECullMode::None);

		// Upload initial data for all vertex buffers from the line attributes
		auto& poly = mPolyLine->getMeshInstance();
		const auto& pos_attr = poly.getOrCreateAttribute<glm::vec3>(vertexid::position);
		const auto& nor_attr = poly.getOrCreateAttribute<glm::vec3>(vertexid::normal);
		const auto& uv_attr = poly.getOrCreateAttribute<glm::vec3>(vertexid::uv);
		const auto& col_attr = poly.getOrCreateAttribute<glm::vec4>(vertexid::color);

		const std::vector<uint> copy_indices = { 0, 1, ORIGINAL_BUFFER_INDEX };
		for (const uint i : copy_indices)
		{
			if (!uploadVec3ToVec4(pos_attr.getData(), *mPositionBuffer[i], 1.0f, errorState))
				return false;

			if (!uploadVec3ToVec4(nor_attr.getData(), *mNormalBuffer[i], 0.0f, errorState))
				return false;

			if (!uploadVec3ToVec4(uv_attr.getData(), *mUVBuffer[i], 0.0f, errorState))
				return false;

			if (!mColorBuffer[i]->setData(col_attr.getData(), errorState))
				return false;
		}

		// Attributes
		const std::vector position_buffer(count, glm::zero<glm::vec4>());
		mMeshInstance->getOrCreateAttribute<glm::vec4>(vertexid::position).setData(position_buffer);

		const std::vector normal_buffer(count, glm::zero<glm::vec4>());
		mMeshInstance->getOrCreateAttribute<glm::vec4>(vertexid::normal).setData(normal_buffer);

		const std::vector uv_buffer(count, glm::zero<glm::vec4>());
		mMeshInstance->getOrCreateAttribute<glm::vec4>(vertexid::getUVName(0)).setData(uv_buffer);

		const std::vector color_buffer(count, glm::one<glm::vec4>());
		mMeshInstance->getOrCreateAttribute<glm::vec4>(vertexid::getColorName(0)).setData(color_buffer);

		auto& shape = mMeshInstance->createShape();
		shape.setIndices(poly.getShape(0).getIndices().data(), poly.getShape(0).getNumIndices());

		return mMeshInstance->init(errorState);
	}


	void LineMesh::readback()
	{
		// Copy to readback
		assert(mRenderService.getCurrentCommandBuffer() != VK_NULL_HANDLE);
		const VkBufferCopy region = { 0, 0, mCount*sizeof(glm::vec4) };
		vkCmdCopyBuffer(mRenderService.getCurrentCommandBuffer(), getPositionBuffer(EBufferRank::Read).getBuffer(), getPositionBuffer(EBufferRank::Readback).getBuffer(), 1, &region);
		vkCmdCopyBuffer(mRenderService.getCurrentCommandBuffer(), getColorBuffer(EBufferRank::Read).getBuffer(), getColorBuffer(EBufferRank::Readback).getBuffer(), 1, &region);

		// Queue download
		getPositionBuffer(EBufferRank::Readback).asyncGetData([this, count = mCount](const void *data, size_t size)
		{
			const size_t copy_bytes = count*sizeof(glm::vec4); assert(copy_bytes <= size);
			mPositionsLocal.resize(count);
			std::memcpy(mPositionsLocal.data(), data, copy_bytes);
		});

		// Queue download
		getColorBuffer(EBufferRank::Readback).asyncGetData([this, count = mCount](const void *data, size_t size)
		{
			const size_t copy_bytes = count*sizeof(glm::vec4); assert(copy_bytes <= size);
			mColorsLocal.resize(count);
			std::memcpy(mColorsLocal.data(), data, copy_bytes);
		});
	}


	void LineMesh::reset()
	{
		mResetPositions = true;
		mResetNormals = true;
		mResetUVs = true;
		mResetColors = true;
	}


	VertexBufferVec4& LineMesh::getPositionBuffer(EBufferRank rank) const
	{
		return *mPositionBuffer[getBufferIndex(rank, mPositionBufferIndex, mResetPositions)];
	}


	VertexBufferVec4& LineMesh::getNormalBuffer(EBufferRank rank) const
	{
		return *mNormalBuffer[getBufferIndex(rank, mNormalBufferIndex, mResetNormals)];
	}


	VertexBufferVec4& LineMesh::getUVBuffer(EBufferRank rank) const
	{
		return *mUVBuffer[getBufferIndex(rank, mUVBufferIndex, mResetUVs)];
	}


	VertexBufferVec4& LineMesh::getColorBuffer(EBufferRank rank) const
	{
		return *mColorBuffer[getBufferIndex(rank, mColorBufferIndex, mResetColors)];
	}


	void LineMesh::swapPositionBuffer()
	{
		mResetPositions = false;
		mPositionBufferIndex = swapBufferIndex(mPositionBufferIndex);
	}


	void LineMesh::swapNormalBuffer()
	{
		mResetNormals = false;
		mNormalBufferIndex = swapBufferIndex(mNormalBufferIndex);
	}


	void LineMesh::swapUVBuffer()
	{
		mResetUVs = false;
		mUVBufferIndex = swapBufferIndex(mUVBufferIndex);
	}


	void LineMesh::swapColorBuffer()
	{
		mResetColors = false;
		mColorBufferIndex = swapBufferIndex(mColorBufferIndex);
	}


	uint LineMesh::getBufferIndex(EBufferRank rank, uint index, bool reset) const
	{
		switch(rank)
		{
			case EBufferRank::Read:
				return !reset ? swapBufferIndex(index) : ORIGINAL_BUFFER_INDEX;
			case EBufferRank::Write:
				return index;
			case EBufferRank::Original:
				return ORIGINAL_BUFFER_INDEX;
			case EBufferRank::Readback:
				return READBACK_BUFFER_INDEX;
			default:
				assert(false);
				return ORIGINAL_BUFFER_INDEX;
		}
	}
}
