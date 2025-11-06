/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// Local Includes
#include "linenoisefftcomponent.h"

// External Includes
#include <entity.h>
#include <glm/gtc/noise.hpp>
#include <nap/logger.h>
#include <glm/gtc/noise.hpp>
#include <glm/gtc/random.hpp>

RTTI_BEGIN_CLASS(nap::LineNoiseFFTComponent)
	RTTI_PROPERTY("LowInput",			&nap::LineNoiseFFTComponent::mLowLevel,		nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("MidInput",			&nap::LineNoiseFFTComponent::mMidLevel,		nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("HighInput",			&nap::LineNoiseFFTComponent::mHighLevel,		nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("LowInputIntensity", &nap::LineNoiseFFTComponent::mLowLevelIntensity, nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("MidInputIntensity", &nap::LineNoiseFFTComponent::mMidLevelIntensity, nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("HighInputIntensity", &nap::LineNoiseFFTComponent::mHighLevelIntensity, nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("LowInputAccumulation",	&nap::LineNoiseFFTComponent::mLowLevelAccumulation,	nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("MidInputAccumulation",	&nap::LineNoiseFFTComponent::mMidLevelAccumulation,	nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("HighInputAccumulation",	&nap::LineNoiseFFTComponent::mHighLevelAccumulation,	nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::LineNoiseFFTComponentInstance)
	RTTI_CONSTRUCTOR(nap::EntityInstance&, nap::Component&)
RTTI_END_CLASS

namespace nap
{
	bool LineNoiseFFTComponentInstance::init(utility::ErrorState& errorState)
	{
		if (!LineNoiseComponentInstance::init(errorState))
			return false;

		mResource = getComponent<LineNoiseFFTComponent>();
		return true;
	}


	void LineNoiseFFTComponentInstance::update(double deltaTime)
	{
		// Update accumulators
		float delta_time = static_cast<float>(deltaTime);

		float low = mResource->mLowLevel->mValue * mResource->mLowLevelIntensity->mValue;
		float mid = mResource->mMidLevel->mValue * mResource->mMidLevelIntensity->mValue;
		float high = mResource->mHighLevel->mValue * mResource->mHighLevelIntensity->mValue;

		mLowLevelAccumulator += mResource->mLowLevel->mValue * mResource->mLowLevelAccumulation->mValue * delta_time;
		mMidLevelAccumulator += mResource->mMidLevel->mValue * mResource->mMidLevelAccumulation->mValue * delta_time;
		mHighLevelAccumulator += mResource->mHighLevel->mValue * mResource->mHighLevelAccumulation->mValue * delta_time;

		low += glm::simplex(glm::vec2(mLowLevelAccumulator, mRandomSeed.x)) * mResource->mLowLevelAccumulation->mValue;
		mid += glm::simplex(glm::vec2(mMidLevelAccumulator, mRandomSeed.y)) * mResource->mMidLevelAccumulation->mValue;
		high += glm::simplex(glm::vec2(mHighLevelAccumulator, mRandomSeed.z)) * mResource->mHighLevelAccumulation->mValue;

		// Update smoothers
		mSpeedSmoother.update(mProperties.mSpeed->mValue, deltaTime);
		mFreqSmoother.update(mProperties.mFrequency->mValue, deltaTime);
		mLinePosFreqSmoother.update(mProperties.mLinePosFrequency->mValue, deltaTime);
		mAmpSmoother.update(mProperties.mAmplitude->mValue, deltaTime);
		mOffsetSmoother.update(mProperties.mOffset->mValue, deltaTime);
		mShiftSmoother.update(mProperties.mShift->mValue, deltaTime);

		// Update current time
		mCurrentTime += (deltaTime * mSpeedSmoother.getValue() * mClockSpeed);

		float offset = mCurrentTime + mOffsetSmoother.getValue() + mid;

		glm::vec2 shift = glm::normalize(glm::mix(glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 0.0f), mLinePosFreqSmoother.getValue())) * mShiftSmoother.getValue();
		shift.x += high;

		// Apply noise based on normal
		int vert_count = mLineIn->getMeshInstance().getNumVertices();

		// Get the normals and vertices to manipulate
		auto& normals = mLineIn->getNormalAttr().getData();
		auto& vertices = mLineIn->getPositionAttr().getData();
		auto& uvs = mLineIn->getUvAttr().getData();

		for (int i = 0; i < vert_count; i++)
		{
			float pos = i / static_cast<float>(vert_count) - 0.5f;
			glm::vec2 uv_sample = glm::mix(glm::vec2(uvs[i].x, uvs[i].y), glm::vec2(pos, 0.0f), mLinePosFreqSmoother.getValue());

			float freq = mFreqSmoother.getValue() + low;
			glm::vec2 freq_shift = uv_sample * freq;
			
			auto uv = freq_shift + shift + offset;
			float v = glm::simplex(uv) * mAmpSmoother.getValue();
			vertices[i] += (normals[i] * v);
		}

		// Update normal based on displaced vertices
		updateNormals(normals, vertices);

		// Push changes to the gpu
		utility::ErrorState error;
		if (!mLineIn->getMeshInstance().update(error))
		{
			nap::Logger::warn(error.toString().c_str());
		}
	}
}
