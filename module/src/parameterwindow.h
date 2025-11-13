#pragma once

// External Includes
#include <appguiwidget.h>
#include <parametergui.h>

namespace nap
{
	class IMGuiService;
	class ParameterService;

    /**
     * PerformanceWindow
     */
    class NAPAPI ParameterWindow : public AppGUIWindow
    {
        RTTI_ENABLE(AppGUIWindow)

    public:
		ParameterWindow(AppGUIService& service);

    	/**
		 * Draw window content
		 */
    	void drawContent(double deltaTime) override;

    	std::vector<ResourcePtr<ParameterGUI>> mParameterGUIs;			///< Property: 'ParameterGUIs'

    protected:
		IMGuiService* mGuiService = nullptr;
    };

    using ParameterWindowObjectCreator = rtti::ObjectCreator<ParameterWindow, AppGUIService>;
}
