#pragma once
#include <mesh.h>
#include <polyline.h>

namespace nap
{
	// Forward declares
	class RenderService;
	class Core;

	class NAPAPI LineMesh : public IMesh
	{
		RTTI_ENABLE(IMesh)
	public:
		LineMesh(Core& core);

		bool init(utility::ErrorState& errorState) override;

		MeshInstance& getMeshInstance() override					{ return *mMeshInstance; }

		const MeshInstance& getMeshInstance() const override		{ return *mMeshInstance; }

		ResourcePtr<PolyLine> mPolyLine;						///<
		EMemoryUsage mUsage = EMemoryUsage::Static;				///< Property: 'Usage' If the line is created once or frequently updated.
		uint mCount = 2;										///< Property: 'Count' The vertex attribute element count.

		// Buffers
		VertexBufferVec4& getPositionBuffer(uint index) const	{ assert(index<mPositionBuffer.size()); return *mPositionBuffer[index]; }
		VertexBufferVec4& getNormalBuffer() const				{ return *mNormalBuffer; }
		VertexBufferVec4& getUVBuffer() const					{ return *mUVBuffer; }
		VertexBufferVec4& getColorBuffer() const				{ return *mColorBuffer; }

	private:
		using VertexDoubleBufferVec4 = std::array<std::unique_ptr<VertexBufferVec4>, 2>;

		VertexDoubleBufferVec4 mPositionBuffer;
		std::unique_ptr<VertexBufferVec4> mNormalBuffer;
		std::unique_ptr<VertexBufferVec4> mUVBuffer;
		std::unique_ptr<VertexBufferVec4> mColorBuffer;

		std::unique_ptr<MeshInstance> mMeshInstance = nullptr;	///< The mesh instance to construct
		RenderService& mRenderService;							///< Handle to the render service
	};
}
