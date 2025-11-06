/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "appstate.h"

RTTI_BEGIN_CLASS(nap::AppState)
    RTTI_PROPERTY("CapFramerate", &nap::AppState::mCapFramerate, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("FramesPerSecond", &nap::AppState::mFramesPerSecond, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("HideCursor", &nap::AppState::mHideCursor, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

namespace nap
{
	bool AppState::init(utility::ErrorState& errorState)
	{
		return true;
	}
}
