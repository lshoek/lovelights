/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// Local Includes
#include "computelinecomponent.h"

// External Includes
#include <entity.h>
#include <glm/gtc/noise.hpp>
#include <nap/logger.h>
#include <glm/gtc/random.hpp>
#include <renderglobals.h>

RTTI_BEGIN_STRUCT(nap::NoiseProperties)
	RTTI_PROPERTY("Frequency",			&nap::NoiseProperties::mFrequency,			nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("LinePosFrequency",	&nap::NoiseProperties::mLinePosFrequency,	nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("Speed",				&nap::NoiseProperties::mSpeed,				nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("Offset",				&nap::NoiseProperties::mOffset,				nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("Amplitude",			&nap::NoiseProperties::mAmplitude,			nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("Shift",				&nap::NoiseProperties::mShift,				nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("SmoothTime",			&nap::NoiseProperties::mSmoothTime,			nap::rtti::EPropertyMetaData::Default)
RTTI_END_STRUCT

RTTI_BEGIN_CLASS(nap::ComputeLineComponent)
	RTTI_PROPERTY("LineMesh",			&nap::ComputeLineComponent::mLineMesh,		nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("Properties",			&nap::ComputeLineComponent::mProperties,	nap::rtti::EPropertyMetaData::Required | nap::rtti::EPropertyMetaData::Embedded)
	RTTI_PROPERTY("ClockSpeed",			&nap::ComputeLineComponent::mClockSpeed,	nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::ComputeLineComponentInstance)
	RTTI_CONSTRUCTOR(nap::EntityInstance&, nap::Component&)
RTTI_END_CLASS

namespace nap
{
	template <typename T>
	static void createBufferBinding(const std::string& name, TypedGPUBufferNumeric<T>& buffer, ComputeMaterialInstance& mtl)
	{
		auto* binding = mtl.getOrCreateBuffer<TypedBufferBindingNumeric<T>>(name);
		binding->setBuffer(buffer);
	}


	ComputeLineComponentInstance::ComputeLineComponentInstance(EntityInstance& entity, Component& resource) :
		ComputeComponentInstance(entity, resource)
	{ }


	bool ComputeLineComponentInstance::init(utility::ErrorState& errorState)
	{
		if (!ComputeComponentInstance::init(errorState))
			return false;

		auto* resource = getComponent<ComputeLineComponent>();
		mProperties = resource->mProperties;
		mClockSpeed = resource->mClockSpeed;
		mLineMesh = resource->mLineMesh.get();

		// Buffer bindings
		createBufferBinding("OutPositions", mLineMesh->getPositionBuffer(0), getMaterialInstance());
		createBufferBinding("InNormals", mLineMesh->getPositionBuffer(1), getMaterialInstance());
		createBufferBinding("InUVs", mLineMesh->getUVBuffer(), getMaterialInstance());
		createBufferBinding("InColors", mLineMesh->getColorBuffer(), getMaterialInstance());

		// Set smooth timing values
		mAmpSmoother.mSmoothTime = mProperties.mSmoothTime;
		mAmpSmoother.setValue(mProperties.mAmplitude->mValue);

		mFreqSmoother.mSmoothTime = mProperties.mSmoothTime;
		mFreqSmoother.setValue(mProperties.mFrequency->mValue);

		mLinePosFreqSmoother.mSmoothTime = mProperties.mSmoothTime;
		mLinePosFreqSmoother.setValue(mProperties.mLinePosFrequency->mValue);

		mOffsetSmoother.mSmoothTime = mProperties.mSmoothTime;
		mOffsetSmoother.setValue(mProperties.mOffset->mValue);

		mSpeedSmoother.mSmoothTime = mProperties.mSmoothTime;
		mSpeedSmoother.setValue(mProperties.mSpeed->mValue);

		mShiftSmoother.mSmoothTime = mProperties.mSmoothTime;
		mShiftSmoother.setValue(mProperties.mShift->mValue);

		mRandomSeed =
		{
			glm::linearRand<float>(0.0f, 1000.0f),
			glm::linearRand<float>(0.0f, 1000.0f),
			glm::linearRand<float>(0.0f, 1000.0f)
		};

		setInvocations(resource->mLineMesh->getMeshInstance().getNumVertices());
		return true;
	}


	void ComputeLineComponentInstance::update(double deltaTime)
	{
		// Update smoothers
		mSpeedSmoother.update(mProperties.mSpeed->mValue, deltaTime);
		mFreqSmoother.update(mProperties.mFrequency->mValue, deltaTime);
		mLinePosFreqSmoother.update(mProperties.mLinePosFrequency->mValue, deltaTime);
		mAmpSmoother.update(mProperties.mAmplitude->mValue, deltaTime);
		mOffsetSmoother.update(mProperties.mOffset->mValue, deltaTime);
		mShiftSmoother.update(mProperties.mShift->mValue, deltaTime);

		// Update current time
		mElapsedClockTime += (deltaTime * mSpeedSmoother.getValue() * mClockSpeed);
		auto* ubo = getMaterialInstance().getOrCreateUniform("UBO"); assert(ubo != nullptr);
		ubo->getOrCreateUniform<UniformFloatInstance>("elapsedTime")->setValue(static_cast<float>(mElapsedClockTime));
		ubo->getOrCreateUniform<UniformFloatInstance>("freq")->setValue(mFreqSmoother.getValue());
		ubo->getOrCreateUniform<UniformFloatInstance>("posFreq")->setValue(mLinePosFreqSmoother.getValue());
		ubo->getOrCreateUniform<UniformFloatInstance>("amp")->setValue(mAmpSmoother.getValue());
		ubo->getOrCreateUniform<UniformFloatInstance>("offset")->setValue(mOffsetSmoother.getValue());
		ubo->getOrCreateUniform<UniformFloatInstance>("shift")->setValue(mShiftSmoother.getValue());
	}


	void ComputeLineComponentInstance::onCompute(VkCommandBuffer commandBuffer, uint numInvocations)
	{
		auto* binding = getMaterialInstance().getOrCreateBuffer<BufferBindingVec4Instance>("InPositions");
		binding->setBuffer(mLineMesh->getPositionBuffer(0));

		binding = getMaterialInstance().getOrCreateBuffer<BufferBindingVec4Instance>("OutPositions");
		binding->setBuffer(mLineMesh->getPositionBuffer(1));

		ComputeComponentInstance::onCompute(commandBuffer, numInvocations);
	}
}
