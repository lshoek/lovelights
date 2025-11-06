// Local Includes
#include "lovelightsservice.h"
#include "parameterwindow.h"
#include "infowindow.h"

// External Includes
#include <parameterguiservice.h>
#include <appguiservice.h>
#include <nap/core.h>

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::LoveLightsService)
	RTTI_CONSTRUCTOR(nap::ServiceConfiguration*)
RTTI_END_CLASS

namespace nap
{
	bool LoveLightsService::init(nap::utility::ErrorState& errorState)
	{
		// Seed random number generator
		std::srand(0);

        return true;
	}


	void LoveLightsService::getDependentServices(std::vector<rtti::TypeInfo>& dependencies)
	{
        dependencies.emplace_back(RTTI_OF(ParameterGUIService));
	}


    void LoveLightsService::registerObjectCreators(rtti::Factory &factory)
    {
        auto* appgui_service = getCore().getService<AppGUIService>();
        factory.addObjectCreator(std::make_unique<InfoWindowObjectCreator>(*appgui_service));
        factory.addObjectCreator(std::make_unique<ParameterWindowObjectCreator>(*appgui_service));
    }
}
