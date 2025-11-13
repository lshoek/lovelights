#include "playlistcontrolcomponent.h"

// Nap includes
#include <entity.h>
#include <nap/core.h>
#include <mathutils.h>
#include <nap/logger.h>

// RTTI

RTTI_BEGIN_STRUCT(nap::PlaylistControlComponent::PresetGroup)
    RTTI_PROPERTY_FILELINK("Preset", &nap::PlaylistControlComponent::PresetGroup::mPreset, nap::rtti::EPropertyMetaData::Default, nap::rtti::EPropertyFileType::Any)
    RTTI_PROPERTY("ParameterGroup", &nap::PlaylistControlComponent::PresetGroup::mParameterGroup, nap::rtti::EPropertyMetaData::Required)
    RTTI_PROPERTY("Blender", &nap::PlaylistControlComponent::PresetGroup::mBlender, nap::rtti::EPropertyMetaData::Required)
    RTTI_PROPERTY("Immediate", &nap::PlaylistControlComponent::PresetGroup::mImmediate, nap::rtti::EPropertyMetaData::Default)
RTTI_END_STRUCT

RTTI_BEGIN_CLASS(nap::PlaylistControlComponent::Item)
    RTTI_PROPERTY("Groups", &nap::PlaylistControlComponent::Item::mPresets, nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("AverageDuration", &nap::PlaylistControlComponent::Item::mAverageDuration, nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("DurationDeviation", &nap::PlaylistControlComponent::Item::mDurationDeviation, nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("TransitionTime", &nap::PlaylistControlComponent::Item::mTransitionTime, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::PlaylistControlComponent)
	RTTI_PROPERTY("Items", &nap::PlaylistControlComponent::mItems, nap::rtti::EPropertyMetaData::Embedded)
	RTTI_PROPERTY("IdleItem", &nap::PlaylistControlComponent::mIdleItem, nap::rtti::EPropertyMetaData::Embedded)
	RTTI_PROPERTY("RandomizePlaylist", &nap::PlaylistControlComponent::mRandomizePlaylist, nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("Enable", &nap::PlaylistControlComponent::mEnable, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("Verbose", &nap::PlaylistControlComponent::mVerbose, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::PlaylistControlComponentInstance)
	RTTI_CONSTRUCTOR(nap::EntityInstance&, nap::Component&)
RTTI_END_CLASS

namespace nap
{
	// utility function to find ParameterBlendComponentInstance* matched to ParameterBlendComponent*
	static ParameterBlendComponentInstance* findBlender(const ParameterBlendComponent* component,
		const std::vector<ParameterBlendComponentInstance*> blenders)
	{
		for (auto& blender_instance : blenders)
		{
			if (blender_instance->getComponent()->mID==component->mID)
				return blender_instance;
		}
		return nullptr;
	}


	static bool createItem(PlaylistControlComponent::Item& resource,
		const std::vector<ParameterBlendComponentInstance*> blenders,
		PlaylistControlComponentInstance::Item& outItem,
		utility::ErrorState& errorState)
	{
		std::vector<PlaylistControlComponentInstance::ItemPresetGroup> preset_groups;
		for (auto& group : resource.mPresets)
		{
			// find the blender instance
			auto* blender_instance = findBlender(group.mBlender.get(), blenders);
			if (!errorState.check(blender_instance!= nullptr, "Could not find instance for blender %s", group.mBlender->mID.c_str()))
				return false;

			// find preset index
			bool found = false;
			int idx = 0;
			const auto& blender_instance_presets = blender_instance->getPresets();
			for (const auto& blender_instance_preset : blender_instance_presets)
			{
				if (blender_instance_preset==utility::getFileName(group.mPreset))
				{
					found = true;
					break;
				}
				idx++;
			}
			if (!errorState.check(found, "Could not find preset %s in blender %s", group.mPreset.c_str(), blender_instance->mID.c_str()))
				return false;

			preset_groups.emplace_back(idx, group.mParameterGroup.get(), blender_instance, group.mPreset, group.mImmediate);
		}
		outItem = PlaylistControlComponentInstance::Item(resource, preset_groups);
		return true;
	}


	// Permutes a list of presets. Helper method.
	static void permute(std::vector<PlaylistControlComponentInstance::Item*>& list)
	{
		for (auto i = 0; i < list.size(); ++i)
		{
			auto swapIndex = math::random<int>(0, list.size() - 1);
			auto temp = list[swapIndex];
			list[swapIndex] = list[i];
			list[i] = temp;
		}
	}


	// Utility function to get root entity
	static EntityInstance* findRootEntity(EntityInstance* entity)
	{
		EntityInstance* last_entity = entity;
		while (entity != nullptr)
		{
			last_entity = entity;
			entity = entity->getParent();
		}
		return last_entity;
	}


    //////////////////////////////////////////////////////////////////////////
    // PlaylistControlComponent
    //////////////////////////////////////////////////////////////////////////

    void PlaylistControlComponent::getDependentComponents(std::vector<rtti::TypeInfo> &components) const
    {
        components.emplace_back(RTTI_OF(ParameterBlendComponent));
    }


    //////////////////////////////////////////////////////////////////////////
    // PlaylistControlComponentInstance::ItemPresetGroup
    //////////////////////////////////////////////////////////////////////////

    PlaylistControlComponentInstance::ItemPresetGroup::ItemPresetGroup(
    	int index, ParameterGroup* group, ParameterBlendComponentInstance* blender, const std::string& preset, bool immediate)
    {
        mPresetIndex = index;
        mParameterGroup = group;
        mBlender = blender;
        mPreset = preset;
		mImmediate = immediate;
    }


    //////////////////////////////////////////////////////////////////////////
    // PlaylistControlComponentInstance::Item
    //////////////////////////////////////////////////////////////////////////

    PlaylistControlComponentInstance::Item::Item(const PlaylistControlComponent::Item& resource,
                                                 std::vector<ItemPresetGroup>& groups)
    {
        mAverageDuration = resource.mAverageDuration;
        mDurationDeviation = resource.mDurationDeviation;
        mTransitionTime = resource.mTransitionTime;
        mGroups = groups;
        mID = resource.mID;
    }


    //////////////////////////////////////////////////////////////////////////
    // PlaylistControlComponentInstance
    //////////////////////////////////////////////////////////////////////////

	bool PlaylistControlComponentInstance::init(utility::ErrorState& errorState)
	{
        // Fetch resource
		mResource = getComponent<PlaylistControlComponent>();
		mRandomizePlaylist = mResource->mRandomizePlaylist;
		mVerbose = mResource->mVerbose;

        // Check if we have any presets
        if (!errorState.check(mPlaylist.empty(), "No playlist created"))
            return false;

		if (!errorState.check(mResource->mIdleItem != nullptr, "No idle item specified"))
			return false;

        // Gather all blenders in scene
		auto* root_entity = findRootEntity(getEntityInstance());
        std::vector<ParameterBlendComponentInstance*> blenders;
        root_entity->getComponentsOfTypeRecursive(blenders);

        // Create the playlist
        for (auto& preset : mResource->mItems)
        {
        	Item item;
        	if (!createItem(*preset, blenders, item, errorState))
        		return false;
	        mPlaylist.emplace_back(std::move(item));
        }

		// Create idle item
		{
			Item idle_item;
			if (!createItem(*mResource->mIdleItem, blenders, idle_item, errorState))
				return false;

			mIdleItem = std::move(idle_item);
		}

		// Exit early if there are no items
		if (mPlaylist.empty())
			return true;

		// Gather unique parameter groups
		const auto* param_service = getEntityInstance()->getCore()->getService<ParameterService>();
		std::set<const ParameterGroup*> unique_parameter_groups;
        for (const auto& item : mPlaylist)
        {
	        for (const auto& group : item.mGroups)
	        	unique_parameter_groups.insert(group.mParameterGroup);
        }

		// Ensure presets are available on disk
		for (const auto& group : unique_parameter_groups)
		{
	        const auto presets = param_service->getPresets(*group);
	        if (!errorState.check(!presets.empty(), "%s: No presets available", mID.c_str()))
	            return false;
        }

		// Keep track of a permuted playlist
		for (auto& preset : mPlaylist)
			mPermutedPlaylist.emplace_back(&preset);
		permute(mPermutedPlaylist);

		// Reset variables
		if (isEnabled())
		{
			//mCurrentPlaylistItem = &mPlaylist[mCurrentPlaylistIndex];
			setItemInternal(mCurrentPlaylistIndex, true);

			for (auto& group : mCurrentPlaylistItem->mGroups)
			{
				auto* blender = group.mBlender->getComponent<ParameterBlendComponent>();
				blender->mPresetIndex->setValue(group.mPresetIndex);
			}
        }
		return true;
	}


	void PlaylistControlComponentInstance::update(double deltaTime)
	{
		if (!isEnabled() || mPlaylist.empty())
			return;

		mCurrentPlaylistItemElapsedTime += deltaTime;
		if (mCurrentPlaylistItemElapsedTime >= mCurrentPlaylistItemDuration)
            nextItem();
	}


	void PlaylistControlComponentInstance::nextItem()
	{
		if (mCurrentPlaylistIndex != IDLE_ITEM_INDEX)
		{
			setItemInternal(IDLE_ITEM_INDEX, false);
			return;
		}

		// Increment cached index
		mCachedPlaylistIndex = (mCachedPlaylistIndex+1)%mPlaylist.size();

		// Check in the index wrapped around. In that case we may need to permute again
		if (mCachedPlaylistIndex == 0 && mRandomizePlaylist)
			permute(mPermutedPlaylist);

		setItemInternal(mCachedPlaylistIndex, mRandomizePlaylist);
	}


    void PlaylistControlComponentInstance::setItem(int index, bool immediate)
    {
        if (isIndexValid(index))
        {
            setItemInternal(index, false, immediate);
        	return;
        }
		Logger::error(*this, "Wrong index %i", index);
    }


	PlaylistControlComponentInstance::Item* PlaylistControlComponentInstance::getItem(int index, bool permuted)
	{
		assert(isIndexValid(index));
		return index != IDLE_ITEM_INDEX ? (!permuted ? &mPlaylist[index] : mPermutedPlaylist[index]) : &mIdleItem;
	}


    void PlaylistControlComponentInstance::setItemInternal(int index, bool randomize, bool immediate)
    {
        assert(isIndexValid(index));
        mCurrentPlaylistIndex = index;

        auto* item = getItem(mCurrentPlaylistIndex, randomize);
        mCurrentPlaylistItemDuration = item->mAverageDuration + math::random(-item->mDurationDeviation / 2.f, item->mDurationDeviation / 2.f);
        mCurrentPlaylistItemElapsedTime = 0.0f;
        mCurrentPlaylistItem = item;

        for (const auto& group : item->mGroups)
        {
            auto* blender = group.mBlender;
            auto* comp = blender->getComponent<ParameterBlendComponent>();
            comp->mPresetIndex->setValue(group.mPresetIndex);
            comp->mPresetBlendTime->setValue(immediate ? 0.0f : group.mImmediate ? 0.0f : getCurrentItem().mTransitionTime);
        }

        if (mVerbose)
            Logger::info(*this, "Switching to playlist item %s", item->mID.c_str());
    }
}
