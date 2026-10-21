#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <array>
#include "SightreadGenerator.hpp"

using namespace geode::prelude;

class SightreadingPopup : public Popup {
protected:
    int m_difficulty = 5;
    int m_length = sightread::Medium;

    CCLabelBMFont* m_difficultyLabel = nullptr;
    CCLabelBMFont* m_lengthLabel = nullptr;
    CCLabelBMFont* m_status = nullptr;
    std::array<CCMenuItemToggler*, sightread::ModeCount> m_modeToggles{};
    std::array<CCMenuItemToggler*, sightread::SpeedCount> m_speedToggles{};

    bool init() override;
    void refreshLabels();
    void onStepper(CCObject* sender);
    void onToggle(CCObject*) {}
    void onPlay(CCObject*);
    void savePrefs(uint32_t modeMask, uint32_t speedMask);

public:
    static SightreadingPopup* create();
};

// Tracks the level currently being played from Sightreading Practice, so quitting
// it returns to the main menu instead of GD's usual "last scene".
namespace sightread {
    bool isGeneratedLevel(GJGameLevel* level);
    void clearGeneratedLevel();
    void returnToMenu();
}
