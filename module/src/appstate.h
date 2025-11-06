/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// External includes
#include <nap/resource.h>
#include <utility/dllexport.h>

namespace nap
{
    /**
     * AppState is a simple resource used to configure basic application behavior.
     * Properties can be authored in JSON and consumed by the application at startup.
     */
    class NAPAPI AppState : public Resource
    {
        RTTI_ENABLE(Resource)
    public:
        // Properties
        bool mCapFramerate = false;         ///< Property: 'CapFramerate' When true, cap the application framerate
        float mFramesPerSecond = 60.0f;     ///< Property: 'FramesPerSecond' Target framerate when capping is enabled
        bool mHideCursor = false;           ///< Property: 'HideCursor' When true, hide the OS cursor

        bool init(utility::ErrorState &errorState) override;
    };
}
