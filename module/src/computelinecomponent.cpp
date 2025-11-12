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
	RTTI_PROPERTY("ClockSpeed",			&nap::NoiseProperties::mClockSpeed,			nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("Wavelength",			&nap::NoiseProperties::mWavelength,			nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("Offset",				&nap::NoiseProperties::mOffset,				nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("Amplitude",			&nap::NoiseProperties::mAmplitude,			nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("Shift",				&nap::NoiseProperties::mShift,				nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("Peak",				&nap::NoiseProperties::mPeak,				nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("SmoothTime",			&nap::NoiseProperties::mSmoothTime,			nap::rtti::EPropertyMetaData::Default)
RTTI_END_STRUCT

RTTI_BEGIN_CLASS(nap::ComputeLineComponent)
	RTTI_PROPERTY("LineMesh",			&nap::ComputeLineComponent::mLineMesh,		nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("Properties",			&nap::ComputeLineComponent::mProperties,	nap::rtti::EPropertyMetaData::Required | nap::rtti::EPropertyMetaData::Embedded)
	RTTI_PROPERTY("ClockSpeed",			&nap::ComputeLineComponent::mClockSpeed,	nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("ResetStorage",		&nap::ComputeLineComponent::mResetStorage,	nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::ComputeLineComponentInstance)
	RTTI_CONSTRUCTOR(nap::EntityInstance&, nap::Component&)
RTTI_END_CLASS

namespace nap
{
	template <typename T>
	static void createBufferBinding(const std::string& name, TypedGPUBufferNumeric<T>& buffer, ComputeMaterialInstance& mtl)
	{
		auto* binding = mtl.getOrCreateBuffer<TypedBufferBindingNumericInstance<T>>(name);
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
		mResetStorage = resource->mResetStorage;

		// Buffer bindings
		createBufferBinding("InPositions", mLineMesh->getPositionBuffer(LineMesh::EBufferRank::Original), getMaterialInstance());
		createBufferBinding("OutPositions", mLineMesh->getPositionBuffer(LineMesh::EBufferRank::Write), getMaterialInstance());
		createBufferBinding("InNormals", mLineMesh->getNormalBuffer(LineMesh::EBufferRank::Read), getMaterialInstance());
		createBufferBinding("InUVs", mLineMesh->getUVBuffer(LineMesh::EBufferRank::Read), getMaterialInstance());
		createBufferBinding("InColors", mLineMesh->getColorBuffer(LineMesh::EBufferRank::Read), getMaterialInstance());

		// Set smooth timing values
		for (auto* smoother : { &mAmplitudeSmoother, &mWavelengthSmoother, &mOffsetSmoother, &mSpeedSmoother, &mShiftSmoother, &mPeakSmoother })
			smoother->mSmoothTime = mProperties.mSmoothTime;

		mAmplitudeSmoother.setValue(mProperties.mAmplitude->mValue);
		mWavelengthSmoother.setValue(mProperties.mWavelength->mValue);
		mOffsetSmoother.setValue(mProperties.mOffset->mValue);
		mSpeedSmoother.setValue(mProperties.mClockSpeed->mValue);
		mShiftSmoother.setValue(mProperties.mShift->mValue);
		mPeakSmoother.setValue(mProperties.mPeak->mValue);

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
		if (!mEnabled)
			return;

		// Update smoothers
		mSpeedSmoother.update(mProperties.mClockSpeed->mValue, deltaTime);
		mWavelengthSmoother.update(mProperties.mWavelength->mValue, deltaTime);
		mAmplitudeSmoother.update(mProperties.mAmplitude->mValue, deltaTime);
		mOffsetSmoother.update(mProperties.mOffset->mValue, deltaTime);
		mShiftSmoother.update(mProperties.mShift->mValue, deltaTime);
		mPeakSmoother.update(mProperties.mPeak->mValue, deltaTime);

		// Update current time
		mElapsedClockTime += (deltaTime * mSpeedSmoother.getValue() * mClockSpeed);
		auto* ubo = getMaterialInstance().getOrCreateUniform("UBO"); assert(ubo != nullptr);
		ubo->getOrCreateUniform<UniformFloatInstance>("elapsedTime")->setValue(static_cast<float>(mElapsedClockTime));
		ubo->getOrCreateUniform<UniformFloatInstance>("wavelength")->setValue(mWavelengthSmoother.getValue());
		ubo->getOrCreateUniform<UniformFloatInstance>("amplitude")->setValue(mAmplitudeSmoother.getValue());
		ubo->getOrCreateUniform<UniformFloatInstance>("offset")->setValue(mOffsetSmoother.getValue());
		ubo->getOrCreateUniform<UniformFloatInstance>("shift")->setValue(mShiftSmoother.getValue());
		ubo->getOrCreateUniform<UniformFloatInstance>("peak")->setValue(mPeakSmoother.getValue());
		ubo->getOrCreateUniform<UniformUIntInstance>("count")->setValue(mLineMesh->getMeshInstance().getNumVertices());
	}


	void ComputeLineComponentInstance::onCompute(VkCommandBuffer commandBuffer, uint numInvocations)
	{
		if (!mEnabled)
			return;

		if (mResetStorage)
			mLineMesh->reset();

		auto* binding = getMaterialInstance().getOrCreateBuffer<BufferBindingVec4Instance>("InPositions");
		binding->setBuffer(mLineMesh->getPositionBuffer(LineMesh::EBufferRank::Read));

		binding = getMaterialInstance().getOrCreateBuffer<BufferBindingVec4Instance>("OutPositions");
		binding->setBuffer(mLineMesh->getPositionBuffer(LineMesh::EBufferRank::Write));

		mLineMesh->swapPositionBuffer();

		ComputeComponentInstance::onCompute(commandBuffer, numInvocations);
	}
}
