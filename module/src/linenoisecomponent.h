/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <component.h>
#include <componentptr.h>
#include <polyline.h>
#include <smoothdamp.h>
#include <parameternumeric.h>

namespace nap
{
	class LineNoiseComponentInstance;

	/**
	 * Properties associated with the line noise modulation component
	 */
	struct NAPAPI NoiseProperties
	{
		ResourcePtr<ParameterFloat> mFrequency;				// Parameter that controls frequency
		ResourcePtr<ParameterFloat> mLinePosFrequency;		// Parameter that controls line position frequency blend
		ResourcePtr<ParameterFloat> mSpeed;					// Parameter that controls speed in seconds to move the waveform
		ResourcePtr<ParameterFloat> mOffset;				// Parameter that controls offset along the line
		ResourcePtr<ParameterFloat> mAmplitude;				// Parameter that controls amplitude of the modulation
		ResourcePtr<ParameterFloat> mShift;					// Parameter that controls shift
		float mSmoothTime = 0.1f;							// Smooth time
	};


	/**
	 * Resource of the LineNoiseComponent
	 */
	class NAPAPI LineNoiseComponent : public Component
	{
		RTTI_ENABLE(Component)
		DECLARE_COMPONENT(LineNoiseComponent, LineNoiseComponentInstance)
	public:
		ResourcePtr<PolyLine> mLineIn;					//< Property 'LineIn':
		ResourcePtr<PolyLine> mLineOut;					//< Property 'LineOut':
		bool mUseLineBlender = false;					//< Property 'UseLineBlender':

		NoiseProperties mProperties;					//< Property 'Properties': all modulation settings
		float mClockSpeed = 1.0f;						//< Property 'ClockSpeed': speed multiplier
	};


	/**
	 * Displaces the vertices of a line based on the line normals and a noise pattern.
	 * the noise is applied in the line's uv space
	 */
	class NAPAPI LineNoiseComponentInstance : public ComponentInstance
	{
		RTTI_ENABLE(ComponentInstance)
	public:
		LineNoiseComponentInstance(EntityInstance& entity, Component& resource) :
			ComponentInstance(entity, resource) {}

		/**
		* Initializes this component
		*/
		bool init(utility::ErrorState& errorState) override;

		/**
		* Updates the line color
		* @param deltaTime the time in between frames in seconds
		*/
		void update(double deltaTime) override;

	protected:
		/**
		 * Updates the normals based on displaced positions
		 */
		static void updateNormals(std::vector<glm::vec3>& normals, const std::vector<glm::vec3>& vertices);

		PolyLine* mLineIn = nullptr;
		PolyLine* mLineOut = nullptr;

		// Smooth over time
		math::SmoothOperator<float> mFreqSmoother					{ 1.0f, 0.1f };
		math::SmoothOperator<float> mLinePosFreqSmoother			{ 1.0f, 0.1f };
		math::SmoothOperator<float> mAmpSmoother					{ 1.0f, 0.1f };
		math::SmoothOperator<float> mSpeedSmoother					{ 0.0f, 0.1f };
		math::SmoothOperator<float> mOffsetSmoother					{ 0.0f, 0.1f };
		math::SmoothOperator<float> mShiftSmoother					{ 0.0f, 0.1f };

		NoiseProperties mProperties;
		bool mUseLineBlender = false;
		float mClockSpeed = 1.0f;
		float mCurrentTime = 0.0f;									// Current update time associated with this component
		glm::vec3 mRandomSeed;
	};
}
