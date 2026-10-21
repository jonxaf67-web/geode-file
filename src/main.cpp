#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/EndLevelLayer.hpp>
#include "RandomLevelPopup.hpp"
#include "SightreadingPopup.hpp"

using namespace geode::prelude;

class $modify(RandomLevelMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto menu = this->getChildByID("bottom-menu");
        if (!menu) return true;

        auto spr = CircleButtonSprite::createWithSpriteFrameName(
            "difficulty_04_btn_001.png", 0.9f,
            CircleBaseColor::Green, CircleBaseSize::MediumAlt
        );
        auto btn = CCMenuItemSpriteExtra::create(
            spr, this, menu_selector(RandomLevelMenuLayer::onRandomLevel)
        );
        btn->setID("random-level-button"_spr);
        menu->addChild(btn);
        menu->updateLayout();

        return true;
    }

    void onRandomLevel(CCObject*) {
        if (auto popup = RandomLevelPopup::create()) {
            popup->show();
        }
    }
};

// Generated sightreading levels aren't saved anywhere, so leaving one goes
// straight back to the main menu rather than GD's usual "previous screen".
class $modify(SightreadPlayLayer, PlayLayer) {
    void onQuit() {
        bool generated = sightread::isGeneratedLevel(m_level);
        PlayLayer::onQuit();
        if (generated) {
            sightread::clearGeneratedLevel();
            sightread::returnToMenu();
        }
    }
};

class $modify(SightreadEndLevelLayer, EndLevelLayer) {
    void onMenu(CCObject* sender) {
        bool generated = m_playLayer && sightread::isGeneratedLevel(m_playLayer->m_level);
        EndLevelLayer::onMenu(sender);
        if (generated) {
            sightread::clearGeneratedLevel();
            sightread::returnToMenu();
        }
    }
};
