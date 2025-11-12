/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#pragma once

#include <component.h>
#include <componentptr.h>
#include <smoothdamp.h>
#include <parameternumeric.h>
#include <computecomponent.h>
#include <linemesh.h>

namespace nap
{
	class ComputeLineComponentInstance;

	/**
	 * Properties associated with the line noise modulation component
	 */
	struct NAPAPI NoiseProperties
	{
		ResourcePtr<ParameterFloat> mWavelength;			// Parameter that controls frequency
		ResourcePtr<ParameterFloat> mClockSpeed;			// Parameter that controls speed in seconds to move the waveform
		ResourcePtr<ParameterFloat> mOffset;				// Parameter that controls offset along the line
		ResourcePtr<ParameterFloat> mAmplitude;				// Parameter that controls amplitude of the modulation
		ResourcePtr<ParameterFloat> mShift;					// Parameter that controls shift
		ResourcePtr<ParameterFloat> mPeak;					// Parameter that controls peak height
		float mSmoothTime = 0.1f;							// Smooth time
	};


	/**
	 * Resource of the LineNoiseComponent
	 */
	class NAPAPI ComputeLineComponent : public ComputeComponent
	{
		RTTI_ENABLE(ComputeComponent)
		DECLARE_COMPONENT(LineNoiseComponent, ComputeLineComponentInstance)
	public:
		ResourcePtr<LineMesh> mLineMesh;				//< Property 'LineMesh':
		NoiseProperties mProperties;					//< Property 'Properties': all modulation settings
		double mClockSpeed = 1.0;						//< Property 'ClockSpeed': speed multiplier
		bool mResetStorage = false;						//< Property 'ResetStorage': resets storage buffer to original
	};


	/**
	 * Displaces the vertices of a line based on the line normals and a noise pattern.
	 * the noise is applied in the line's uv space
	 */
	class NAPAPI ComputeLineComponentInstance : public ComputeComponentInstance
	{
		RTTI_ENABLE(ComputeComponentInstance)
	public:
		ComputeLineComponentInstance(EntityInstance& entity, Component& resource);

		/**
		* Initializes this component
		*/
		bool init(utility::ErrorState& errorState) override;

		/**
		* Updates the line color
		* @param deltaTime the time in between frames in seconds
		*/
		void update(double deltaTime) override;

		/**
		 * @param commandBuffer
		 * @param numInvocations
		 */
		void onCompute(VkCommandBuffer commandBuffer, uint numInvocations) override;

		/**
		 * @return
		 */
		LineMesh& getLineMesh() const { return *mLineMesh; }

	protected:
		LineMesh* mLineMesh = nullptr;
		NoiseProperties mProperties;

		double mClockSpeed = 1.0;
		double mElapsedClockTime = 0.0;
		glm::vec3 mRandomSeed;
		uint mBufferIndex = 0;
		bool mResetStorage = false;

		math::SmoothOperator<double> mWavelengthSmoother			{ 1.0, 0.1 };
		math::SmoothOperator<double> mLinePosFreqSmoother	{ 1.0, 0.1 };
		math::SmoothOperator<double> mAmplitudeSmoother			{ 1.0, 0.1 };
		math::SmoothOperator<double> mSpeedSmoother			{ 0.0, 0.1 };
		math::SmoothOperator<double> mOffsetSmoother		{ 0.0, 0.1 };
		math::SmoothOperator<double> mShiftSmoother			{ 0.0, 0.1 };
		math::SmoothOperator<double> mPeakSmoother			{ 0.0, 0.1 };
	};
}
