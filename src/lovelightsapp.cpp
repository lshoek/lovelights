/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// Local Includes
#include "lovelightsapp.h"

// External Includes
#include <inputrouter.h>
#include <perspcameracomponent.h>
#include <rendertotexturecomponent.h>
#include <renderbloomcomponent.h>
#include <computecomponent.h>
#include <depthsorter.h>
#include <sdlhelpers.h>

namespace nap 
{    
    bool LoveLightsApp::init(utility::ErrorState& errorState)
    {
		// Retrieve services
		mRenderService			= getCore().getService<RenderService>();
		mRenderAdvancedService	= getCore().getService<RenderAdvancedService>();
		mSceneService			= getCore().getService<SceneService>();
		mInputService			= getCore().getService<InputService>();
		mGuiService				= getCore().getService<IMGuiService>();

		// Fetch the resource manager
        mResourceManager 		= getCore().getResourceManager();

    	// Get app state
    	mAppState = mResourceManager->findObject<AppState>("AppState");
    	if (!errorState.check(mAppState != nullptr, "No nap::AppState with name `AppState` found in scene"))
    		return false;

    	capFramerate(mAppState->mCapFramerate);
    	setFramerate(mAppState->mFramesPerSecond);
    	if (mAppState->mHideCursor)
    		SDL::hideCursor();

		// Get the render window
		mRenderWindow = mResourceManager->findObject<RenderWindow>("Window");
		if (!errorState.check(mRenderWindow != nullptr, "unable to find nap::RenderWindow with name: %s", "Window"))
			return false;

        // Get the control window
        mControlWindow = mResourceManager->findObject<RenderWindow>("ControlWindow");
        if (!errorState.check(mControlWindow != nullptr, "unable to find nap::RenderWindow with name: %s", "ControlWindow"))
            return false;

		mColorTarget = mResourceManager->findObject<RenderTarget>("ColorTarget");
		if (!errorState.check(mColorTarget != nullptr, "unable to find nap::RenderTarget with name: %s", "ColorTarget"))
			return false;

		// Stencil target (not required)
		mStencilTarget = mResourceManager->findObject<RenderTarget>("StencilTarget");

		// Get the scene that contains our entities and components
		mScene = mResourceManager->findObject<Scene>("Scene");
		if (!errorState.check(mScene != nullptr, "unable to find scene with name: %s", "Scene"))
			return false;

		mCameraEntity 			= mScene->findEntity("CameraEntity");
		mWorldEntity 			= mScene->findEntity("WorldEntity");
		mRenderEntity 			= mScene->findEntity("RenderEntity");
		mComputeEntity 			= mScene->findEntity("ComputeEntity");
		mCompositeEntity 		= mScene->findEntity("CompositeEntity");
		mRenderCameraEntity 	= mScene->findEntity("RenderCameraEntity");
        mPlaylistEntity         = mScene->findEntity("PlaylistEntity");

		mAppGUIs = mResourceManager->getObjects<AppGUI>();

        // Connect hot reload slot
        mResourceManager->mPostResourcesLoadedSignal.connect(mHotReloadSlot);
        onReset();

		return true;
    }


    void LoveLightsApp::onReset()
    {
        if (mStencilTarget == nullptr || mStencilTarget->mColorTexture == nullptr)
            return;

        mRenderService->queueHeadlessCommand([tex = mStencilTarget->mColorTexture](RenderService& renderService)
        {
            VkImageSubresourceRange image_subresource_range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, tex->getMipLevels(), 0, tex->getLayerCount() };
            VkClearColorValue clear_color = { 0, 0, 0, 0 };
            vkCmdClearColorImage(renderService.getCurrentCommandBuffer(), std::as_const(*tex).getHandle().mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &image_subresource_range);
        });
    }


    // Called when the window is going to renderco
    void LoveLightsApp::render()
    {
		// Signal the beginning of a new frame, allowing it to be recorded.
		// The system might wait until all commands that were previously associated with the new frame have been processed on the GPU.
		// Multiple frames are in flight at the same time, but if the graphics load is heavy the system might wait here to ensure resources are available.
		mRenderService->beginFrame();

    	// Compute
    	if (mRenderService->beginComputeRecording())
    	{
    		std::vector<ComputeComponentInstance*> compute_comps;
    		mComputeEntity->getComponentsOfTypeRecursive<ComputeComponentInstance>(compute_comps);
    		mRenderService->computeObjects(compute_comps);
    		mRenderService->endComputeRecording();
    	}

		// Begin recording the render commands for the offscreen render target. Rendering always happens after compute.
		// This prepares a command buffer and starts a render pass.
		if (mRenderService->beginHeadlessRecording())
		{
			// The world entity holds all visible renderable components in the scene.
			std::vector<RenderableComponentInstance*> render_comps;
			mWorldEntity->getComponentsOfTypeRecursive<RenderableComponentInstance>(render_comps);

			// Get Perspective camera to render with
			auto& cam = mCameraEntity->getComponent<CameraComponentInstance>();

			// Render stencil geometry to stencil target
			if (mStencilTarget != nullptr)
			{
				auto stencil_mask = mRenderService->getRenderMask("Stencil");
				mStencilTarget->beginRendering();
				mRenderService->renderObjects(*mStencilTarget, cam, render_comps, stencil_mask);
				mStencilTarget->endRendering();
			}

			// Offscreen color pass -> Render all available geometry to the color texture bound to the render target.
			mColorTarget->beginRendering();
			{
				auto mask = mRenderService->getRenderMask("Default");
				mRenderService->renderObjects(*mColorTarget, cam, render_comps, std::bind(&sorter::sortObjectsByZ, std::placeholders::_1), (mask != 0) ? mask : mask::all);
			}
			mColorTarget->endRendering();

			// Invoke draw() on components in render entity in order
			if (mRenderEntity != nullptr)
			{
				std::vector<RenderableComponentInstance*> comps;
				mRenderEntity->getComponentsOfTypeRecursive(comps);

				for (auto& comp : comps)
				{
					if (!comp->isVisible())
						continue;

					// Find draw method
					auto draw_method = rtti::findMethodRecursive(comp->get_type(), "draw");
					if (!draw_method.is_valid())
						continue;

					// Invoke draw method
					draw_method.invoke(*comp);
				}
			}

			// End headless recording
			mRenderService->endHeadlessRecording();
		}

		// Begin recording the render commands for the main render window
		if (mRenderService->beginRecording(*mRenderWindow))
		{
			// Begin render pass
			mRenderWindow->beginRendering();

			// Get Perspective camera to render with
			auto& cam = mRenderCameraEntity->getComponent<CameraComponentInstance>();

			std::vector<RenderableComponentInstance*> comps;
			mCompositeEntity->getComponentsOfTypeRecursive(comps);
			mRenderService->renderObjects(*mRenderWindow, cam, comps);
            mRenderWindow->endRendering();
            mRenderService->endRecording();
		}

        // Begin recording the render commands for the control window
        if (mRenderService->beginRecording(*mControlWindow))
        {
            mControlWindow->beginRendering();
            mGuiService->draw();
            mControlWindow->endRendering();
            mRenderService->endRecording();
        }

		// Proceed to next frame
		mRenderService->endFrame();
    }


    void LoveLightsApp::windowMessageReceived(WindowEventPtr windowEvent)
    {
		mRenderService->addEvent(std::move(windowEvent));
    }


    void LoveLightsApp::inputMessageReceived(InputEventPtr inputEvent)
    {
		// If we pressed escape, quit the loop
		if (inputEvent->get_type().is_derived_from(RTTI_OF(KeyPressEvent)))
		{
			auto* press_event = static_cast<KeyPressEvent*>(inputEvent.get());

			// Evaluate key
			switch (press_event->mKey)
			{
				case EKeyCode::KEY_ESCAPE:
				{
					quit();
					break;
				}

				case EKeyCode::KEY_f:
				{
					mRenderWindow->toggleFullscreen();
					break;
				}

				case EKeyCode::KEY_g:
				{
					mShowGUI = !mShowGUI;
					break;
				}

				case EKeyCode::KEY_m:
				{
					mShowCursor = !mShowCursor;
					if (mShowCursor)
						SDL::showCursor();
					else
						SDL::hideCursor();
					break;
				}
			}

		}
		mInputService->addEvent(std::move(inputEvent));
    }


    void LoveLightsApp::update(double deltaTime)
    {
		// Use a default input router to forward input events (recursively) to all input components in the scene
		// This is explicit because we don't know what entity should handle the events from a specific window.
		DefaultInputRouter input_router(true);
		mInputService->processWindowEvents(*mRenderWindow, input_router, { &mScene->getRootEntity() });

        // tell GUI service what window to render to
        mGuiService->selectWindow(mControlWindow);

		if (mShowGUI)
		{
			for (auto& gui : mAppGUIs)
				gui->draw(deltaTime);
		}
    }
}
