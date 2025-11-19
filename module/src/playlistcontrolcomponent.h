#pragma once

// Nap includes
#include <component.h>
#include <parameter.h>
#include <parametergroup.h>
#include <componentptr.h>
#include <parameterblendcomponent.h>

#include "playlistcontrolcomponent.h"

namespace nap
{
    class PlaylistControlComponentInstance;

    /**
     * Component that automatically selects presets on the ParameterBlendComponents
     * It cycles through a sequence of playlist items.
     * The order of the sequence can be shuffled and the duration of each preset can be randomized.
     */
    class NAPAPI PlaylistControlComponent : public Component
    {
        RTTI_ENABLE(Component)
        DECLARE_COMPONENT(PlaylistControlComponent, PlaylistControlComponentInstance)
        
    public:
        void getDependentComponents(std::vector<rtti::TypeInfo> &components) const override;

        struct PresetGroup
        {
            ResourcePtr<ParameterGroup> mParameterGroup = nullptr;		// The parametergroup that contains the preset
            ResourcePtr<ParameterBlendComponent> mBlender = nullptr;	// The parameter blender that contains the parameter blend group
            std::string mPreset = "";								    // name of the json preset file
            bool mImmediate = false;
        };

        // Metadata about one preset in the sequence
        class NAPAPI Item : public Resource
        {
            RTTI_ENABLE(Resource)
        public:
            std::vector<PresetGroup> mPresets;          // group of presets to blend
            float mAverageDuration = 5.f;               // average duration of the preset in the sequence in seconds
            float mDurationDeviation = 0.f;             // random deviation of the preset duration in seconds
            float mTransitionTime = 3.f;                // duration of the video fade into this preset in seconds
        };

        std::vector<ResourcePtr<Item>> mItems;			// List of presets in the sequence accompanied by meta data
        ResourcePtr<Item> mIdleItem;                    //
        ResourcePtr<ParameterInt> mSelectItemIndex;     //
		bool mEnable;									// True to enable the preset cycle
        bool mRandomizePlaylist = false;				// Indicates whether the order of the cycle of presets will be shuffled
        bool mVerbose = true;							// Whether to log playlist changes
    };


    /**
     * Instance of @PlaylistControlComponent
     */
    class NAPAPI PlaylistControlComponentInstance : public ComponentInstance
    {
        RTTI_ENABLE(ComponentInstance)
    public:
        struct ItemPresetGroup
        {
            ItemPresetGroup(int index, ParameterGroup* group, ParameterBlendComponentInstance* blender, const std::string& preset, bool immediate);

            ParameterGroup* mParameterGroup = nullptr;
            ParameterBlendComponentInstance* mBlender = nullptr;
            std::string mPreset = "";
            bool mImmediate = false;
            int mPresetIndex = 0;
        };

        struct Item
        {
            Item(){}
            Item(const PlaylistControlComponent::Item& resource, std::vector<ItemPresetGroup>& groups);

            std::vector<ItemPresetGroup> mGroups;
            float mAverageDuration = 5.f;
            float mDurationDeviation = 0.f;
            float mTransitionTime = 3.f;
            std::string mID;
        };

        PlaylistControlComponentInstance(EntityInstance& entity, Component& resource) : ComponentInstance(entity, resource) { }
        
        // Initialize the component
        bool init(utility::ErrorState& errorState) override;

        /**
         * Checks wether it is time to switch to the next preset and tells the @SwitchPresetComponent to switch.
         */
        void update(double deltaTime) override;

        /**
         * Manually sets playlist item to index
         * @param index index of playlist item
         */
        void setItem(int index, bool immediate = false);

        /**
         * @return the current playlist item
         */
        const Item& getCurrentItem() const				{ return *mCurrentPlaylistItem; }

		/**
		 * @return whether preset cycling is enabled
		 */
		bool isEnabled() const							{ return mResource->mEnable; }

        /**
         * Returns playlist
         * @return playlist
         */
        const std::vector<Item>& getPlaylist() const    { return mPlaylist; }

        /**
         * Returns current playlist index
         * @return current playlist index
         */
        int getCurrentPlaylistIndex() const            { return mCurrentPlaylistIndex; }

    private:
        // Checks validity of the preset index
        bool isIndexValid(int index) const             { return (index >= 0 && index < mPlaylist.size()) || index == IDLE_ITEM_INDEX; }

        // Returns playlist item
        Item* getItem(int index, bool permuted = false);

        // Selects the preset item, for internal use
        void setItemInternal(int index, bool randomize, bool immediate = false);

        // Selects the next preset in the sequence
        void nextItem();

        void onSelectItem(int index) { setItem(index); }
        nap::Slot<int> mSelectItemIndexChangedSlot = { this, &PlaylistControlComponentInstance::onSelectItem };

		PlaylistControlComponent* mResource = nullptr;

        std::vector<Item> mPlaylist;
        std::vector<Item*> mPermutedPlaylist;
        Item mIdleItem;

        static constexpr int IDLE_ITEM_INDEX = -1;

        int mCurrentPlaylistIndex = IDLE_ITEM_INDEX;
        int mCachedPlaylistIndex = IDLE_ITEM_INDEX;

        float mCurrentPlaylistItemDuration = 0.0f;
        float mCurrentPlaylistItemElapsedTime = 0.0f;
        Item* mCurrentPlaylistItem;

        bool mRandomizePlaylist = false;
        bool mVerbose = false;
    };
        
}
