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
		enum class EBufferRank
		{
			Read = 0,
			Write = 1,
			Original = 2,
			Readback = 3
		};

		LineMesh(Core& core);

		bool init(utility::ErrorState& errorState) override;

		MeshInstance& getMeshInstance() override						{ return *mMeshInstance; }

		const MeshInstance& getMeshInstance() const override			{ return *mMeshInstance; }

		ResourcePtr<PolyLine> mPolyLine;						///<
		EMemoryUsage mUsage = EMemoryUsage::Static;				///< Property: 'Usage' If the line is created once or frequently updated.
		uint mCount = 2;										///< Property: 'Count' The vertex attribute element count.

		/**
		 * Double-buffer
		 **/
		VertexBufferVec4& getPositionBuffer(EBufferRank rank) const;

		/**
		 * Double-buffer
		 **/
		VertexBufferVec4& getNormalBuffer(EBufferRank rank) const;

		/**
		 * Single-buffer
		 **/
		VertexBufferVec4& getUVBuffer(EBufferRank rank) const;

		/**
		 * Single-buffer
		 **/
		VertexBufferVec4& getColorBuffer(EBufferRank rank) const;

		/**
		 *
		 * @return
		 */
		const std::vector<glm::vec4>& getPositionsLocal() const { return mPositionsLocal; }

		/**
		 *
		 * @return
		 */
		const std::vector<glm::vec4>& getColorsLocal() const { return mColorsLocal; }

		/**
		 * Swaps
		 */
		void swapPositionBuffer();
		void swapNormalBuffer();
		void swapUVBuffer();
		void swapColorBuffer();

		/**
		 * Resets storage to original buffer.
		 */
		void reset();

		/**
		 * Readback buffer.
		 */
		void readback();

	private:
		constexpr static uint QUAD_BUFFER_COUNT = 4;
		constexpr static uint ORIGINAL_BUFFER_INDEX = 2;
		constexpr static uint READBACK_BUFFER_INDEX = 3;

		using VertexQuadrupleBufferVec4 = std::array<std::unique_ptr<VertexBufferVec4>, QUAD_BUFFER_COUNT>;
		uint getBufferIndex(EBufferRank rank, uint index, bool reset) const;

		VertexQuadrupleBufferVec4 mPositionBuffer;
		VertexQuadrupleBufferVec4 mNormalBuffer;
		VertexQuadrupleBufferVec4 mUVBuffer;
		VertexQuadrupleBufferVec4 mColorBuffer;

		std::vector<glm::vec4> mPositionsLocal;
		std::vector<glm::vec4> mColorsLocal;

		std::unique_ptr<MeshInstance> mMeshInstance = nullptr;	///< The mesh instance to construct
		RenderService& mRenderService;							///< Handle to the render service

		uint mPositionBufferIndex = 0;							///< Buffer index
		uint mNormalBufferIndex = 0;							///< Buffer index
		uint mUVBufferIndex = 0;								///< Buffer index
		uint mColorBufferIndex = 0;								///< Buffer index

		bool mResetPositions = false;							///< Whether reset is enabled
		bool mResetNormals = false;								///< Whether reset is enabled
		bool mResetUVs = false;									///< Whether reset is enabled
		bool mResetColors = false;								///< Whether reset is enabled
	};
}
