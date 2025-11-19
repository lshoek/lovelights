/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// External includes
#include <component.h>
#include <nap/signalslot.h>
#include <nap/logger.h>
#include <oscevent.h>
#include <queue>
#include <parameternumeric.h>
#include <parametergroup.h>

namespace nap
{
    // Forward Declare
    class OscHandlerComponentInstance;
   
	/**
	 * Component that converts incoming osc messages into a string and stores them for display later on.
	 */
    class NAPAPI OscHandlerComponent : public Component
    {
        RTTI_ENABLE(Component)
        DECLARE_COMPONENT(OscHandlerComponent, OscHandlerComponentInstance)
        
    public:
        OscHandlerComponent() : Component() { }
        
        /**
         * Get a list of all component types that this component is dependent on (i.e. must be initialized before this one)
         * @param components the components this object depends on
         */
    	void getDependentComponents(std::vector<rtti::TypeInfo>& components) const override;

		std::vector<ResourcePtr<ParameterGroup>>	mParameterGroups;
		bool										mVerbose = false;
    };

    
	/**
	* Instance part of the osc handler component. This object registers itself to an osc input component and processes incoming messages.
	*/
    class NAPAPI OscHandlerComponentInstance : public ComponentInstance
    {
        RTTI_ENABLE(ComponentInstance)
    public:
        OscHandlerComponentInstance(EntityInstance& entity, Component& resource) : ComponentInstance(entity, resource) { }
        
        // Initialize the component
        bool init(utility::ErrorState& errorState) override;

		// Return the given parameter's associated osc address
		bool getParameterAddress(ParameterFloat* parameter, std::string& outAddress) const;

		// Return a list of all osc addresses
		const std::vector<std::string>& getAddresses() const;

    private:
		/**
		 * Called when the slot above is send a new message
		 * @param OSCEvent the new osc event
		 */
		void onEventReceived(const OSCEvent&);

		/**
		 * Slot that is connected to the osc input component that receives new messages
		 */
        Slot<const OSCEvent&> eventReceivedSlot = { this, &OscHandlerComponentInstance::onEventReceived };

		// This map holds all the various callbacks based on id
		typedef void (OscHandlerComponentInstance::*OscEventFunc)(const OSCEvent&, Parameter&);

		// Simple struct that binds a function to a parameter
		struct OSCFunctionMapping
		{
			OSCFunctionMapping(OscEventFunc oscFunction, Parameter& parameter) :
				mFunction(oscFunction), mParameter(&parameter)	{ }

			OscEventFunc mFunction = nullptr;
			Parameter* mParameter  = nullptr;
		};
		std::unordered_map<std::string, OSCFunctionMapping> mOscEventFunctions;

		// Adds a parameter mapping
    	template<typename T>
		void addParameter(std::string oscAddress, ParameterNumeric<T>& parameter);

		// Generic parameter update function
		void updateParameter(const OSCEvent& oscEvent, Parameter& parameter);

		// Cached list of addresses for display in the OSC menu
		std::vector<std::string> mCachedAddresses;

		OscHandlerComponent* mResource = nullptr;
	};

	// Template definitions
	template<typename T>
	void OscHandlerComponentInstance::addParameter(std::string oscAddress, ParameterNumeric<T>& parameter)
	{
		auto it = mOscEventFunctions.emplace(oscAddress, OSCFunctionMapping(&OscHandlerComponentInstance::updateParameter, parameter));
		bool success = it.second;

		if (!success)
		{
			nap::Logger::warn("%s: Duplicate parameter with name: %s", mResource->mID.c_str(), parameter.getDisplayName().c_str());
		}
		else if (mResource->mVerbose)
		{
			nap::Logger::info("%s: Parameter registered with OSC address '%s'", mResource->mID.c_str(), oscAddress.c_str());
		}
	}
}
