#pragma once

#include <component.h>
#include <componentptr.h>
#include <transformcomponent.h>
#include <nap/resourceptr.h>
#include <parameternumeric.h>
#include <parametervec.h>

namespace nap
{
	class UpdateTransformComponentInstance;

	/**
	 * UpdateTransformComponent
	 */
	class NAPAPI UpdateTransformComponent : public Component
	{
		RTTI_ENABLE(Component)
			DECLARE_COMPONENT(UpdateTrzansformComponent, UpdateTransformComponentInstance)
	public:
		void getDependentComponents(std::vector<rtti::TypeInfo>& components) const override;

		ResourcePtr<ParameterVec3> mPosition;
		ResourcePtr<ParameterVec3> mScale;
		ResourcePtr<ParameterFloat> mAngle;
		bool mEnable = true;
	};


	/**
	 * UpdateTransformComponentInstance	
	 */
	class NAPAPI UpdateTransformComponentInstance : public ComponentInstance
	{
		RTTI_ENABLE(ComponentInstance)
	public:
		UpdateTransformComponentInstance(EntityInstance& entity, Component& resource) :
			ComponentInstance(entity, resource)	{ }

		/**
		 * Initialize UpdateTransformComponentInstance based on the UpdateTransformComponent resource
		 * @param entityCreationParams when dynamically creating entities on initialization, add them to this this list.
		 * @param errorState should hold the error message when initialization fails
		 * @return if the UpdateTransformComponentInstance is initialized successfully
		 */
		bool init(utility::ErrorState& errorState) override;

		/**
		 * update UpdateTransformComponentInstance. This is called by NAP core automatically
		 * @param deltaTime time in between frames in seconds
		 */
		void update(double deltaTime) override;

	private:
		UpdateTransformComponent* mResource = nullptr;
		TransformComponentInstance* mTransformComponent = nullptr;
		bool mEnabled = true;
	};
}
