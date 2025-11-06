/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "linenoisecomponent.h"

#include <component.h>
#include <componentptr.h>
#include <smoothdamp.h>
#include <parameternumeric.h>

namespace nap
{
	class LineNoiseFFTComponentInstance;

	/**
	 * Resource of the LineNoiseFFTComponent
	 */
	class NAPAPI LineNoiseFFTComponent : public LineNoiseComponent
	{
		RTTI_ENABLE(LineNoiseComponent)
		DECLARE_COMPONENT(LineNoiseFFTComponent, LineNoiseFFTComponentInstance)
	public:
		// level meter parameters
		ResourcePtr<ParameterFloat> mLowLevel;
		ResourcePtr<ParameterFloat> mMidLevel;
		ResourcePtr<ParameterFloat> mHighLevel;

		ResourcePtr<ParameterFloat> mLowLevelIntensity;
		ResourcePtr<ParameterFloat> mMidLevelIntensity;
		ResourcePtr<ParameterFloat> mHighLevelIntensity;

		ResourcePtr<ParameterFloat> mLowLevelAccumulation;
		ResourcePtr<ParameterFloat> mMidLevelAccumulation;
		ResourcePtr<ParameterFloat> mHighLevelAccumulation;
	};


	/**
	 * Displaces the vertices of a line based on the line normals and a noise pattern.
	 * the noise is applied in the line's uv space
	 */
	class NAPAPI LineNoiseFFTComponentInstance : public LineNoiseComponentInstance
	{
		RTTI_ENABLE(LineNoiseComponentInstance)
	public:
		LineNoiseFFTComponentInstance(EntityInstance& entity, Component& resource) :
			LineNoiseComponentInstance(entity, resource) {}

		/**
		* Initializes this component
		*/
		bool init(utility::ErrorState& errorState) override;

		/**
		* Updates the line color
		* @param deltaTime the time in between frames in seconds
		*/
		void update(double deltaTime) override;

	private:
		LineNoiseFFTComponent* mResource = nullptr;

		float mLowLevelAccumulator = 0.0f;
		float mMidLevelAccumulator = 0.0f;
		float mHighLevelAccumulator = 0.0f;
	};
}
