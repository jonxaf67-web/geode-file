#include "SightreadingPopup.hpp"
#include "UiHelpers.hpp"

#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/MenuLayer.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <random>

using namespace sightread;

namespace {
    Ref<GJGameLevel> s_generated;

    enum Tags { DifficultyLeft = 10, DifficultyRight, LengthLeft = 20, LengthRight };

    char const* difficultyWord(int d) {
        if (d <= 2) return "Relaxed";
        if (d <= 4) return "Moderate";
        if (d <= 6) return "Tricky";
        if (d <= 8) return "Hard";
        return "Brutal";
    }

    uint64_t freshSeed() {
        std::random_device rd;
        return (uint64_t(rd()) << 32) | rd();
    }
}

bool sightread::isGeneratedLevel(GJGameLevel* level) {
    return level && s_generated && level == s_generated.data();
}

void sightread::clearGeneratedLevel() {
    s_generated = nullptr;
}

void sightread::returnToMenu() {
    CCDirector::get()->replaceScene(CCTransitionFade::create(0.5f, MenuLayer::scene(false)));
    GameManager::get()->fadeInMenuMusic();
}

SightreadingPopup* SightreadingPopup::create() {
    auto ret = new SightreadingPopup();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool SightreadingPopup::init() {
    if (!Popup::init(380.f, 300.f)) return false;
    this->setTitle("Sightreading Practice");

    auto mod = Mod::get();
    m_difficulty = std::clamp(mod->getSavedValue<int>("sr-difficulty", 5), 1, 10);
    m_length = std::clamp(mod->getSavedValue<int>("sr-length", (int)Medium), 0, LengthCount - 1);
    int modeMask = mod->getSavedValue<int>("sr-modes", 1 << Cube);
    int speedMask = mod->getSavedValue<int>("sr-speeds", 1 << Normal);

    m_difficultyLabel = ui::addStepper(m_mainLayer, this, menu_selector(SightreadingPopup::onStepper),
        "Difficulty", 95.f, DifficultyLeft);
    m_lengthLabel = ui::addStepper(m_mainLayer, this, menu_selector(SightreadingPopup::onStepper),
        "Length", 62.f, LengthLeft);

    ui::addLabel(m_mainLayer, "Gamemodes", { 0.f, 34.f });
    for (int i = 0; i < ModeCount; i++) {
        CCPoint pos = { -150.f + (i % 4) * 88.f, 12.f - (i / 4) * 22.f };
        m_modeToggles[i] = ui::addToggle(m_mainLayer, this, menu_selector(SightreadingPopup::onToggle),
            MODE_NAMES[i], pos, (modeMask >> i) & 1);
    }

    ui::addLabel(m_mainLayer, "Speed", { 0.f, -38.f });
    for (int i = 0; i < SpeedCount; i++) {
        CCPoint pos = { -150.f + i * 70.f, -60.f };
        m_speedToggles[i] = ui::addToggle(m_mainLayer, this, menu_selector(SightreadingPopup::onToggle),
            SPEED_NAMES[i], pos, (speedMask >> i) & 1);
    }

    m_status = CCLabelBMFont::create("", "chatFont.fnt");
    m_status->setScale(0.7f);
    m_status->setColor({ 255, 110, 110 });
    m_mainLayer->addChildAtPosition(m_status, Anchor::Center, { 0.f, -86.f });

    auto menu = CCMenu::create();
    menu->setContentSize({ 0.f, 0.f });
    auto playBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Generate & Play"), this, menu_selector(SightreadingPopup::onPlay)
    );
    menu->addChild(playBtn);
    m_mainLayer->addChildAtPosition(menu, Anchor::Bottom, { 0.f, 28.f });

    this->refreshLabels();
    return true;
}

void SightreadingPopup::refreshLabels() {
    m_difficultyLabel->setString(fmt::format("{} - {}", m_difficulty, difficultyWord(m_difficulty)).c_str());
    m_difficultyLabel->limitLabelWidth(150.f, 0.5f, 0.1f);
    m_lengthLabel->setString(LENGTH_NAMES[m_length]);
    m_lengthLabel->limitLabelWidth(150.f, 0.5f, 0.1f);
}

void SightreadingPopup::onStepper(CCObject* sender) {
    switch (static_cast<CCNode*>(sender)->getTag()) {
        case DifficultyLeft:  m_difficulty = m_difficulty == 1 ? 10 : m_difficulty - 1; break;
        case DifficultyRight: m_difficulty = m_difficulty == 10 ? 1 : m_difficulty + 1; break;
        case LengthLeft:      m_length = (m_length + LengthCount - 1) % LengthCount; break;
        case LengthRight:     m_length = (m_length + 1) % LengthCount; break;
    }
    this->refreshLabels();
}

void SightreadingPopup::savePrefs(uint32_t modeMask, uint32_t speedMask) {
    auto mod = Mod::get();
    mod->setSavedValue("sr-difficulty", m_difficulty);
    mod->setSavedValue("sr-length", m_length);
    mod->setSavedValue("sr-modes", (int)modeMask);
    mod->setSavedValue("sr-speeds", (int)speedMask);
}

void SightreadingPopup::onPlay(CCObject*) {
    Settings settings;
    settings.difficulty = m_difficulty;
    settings.length = m_length;
    settings.modeMask = 0;
    settings.speedMask = 0;
    for (int i = 0; i < ModeCount; i++) {
        if (m_modeToggles[i]->isToggled()) settings.modeMask |= 1u << i;
    }
    for (int i = 0; i < SpeedCount; i++) {
        if (m_speedToggles[i]->isToggled()) settings.speedMask |= 1u << i;
    }
    if (!settings.modeMask) {
        m_status->setString("Pick at least one gamemode.");
        return;
    }
    if (!settings.speedMask) {
        m_status->setString("Pick at least one speed.");
        return;
    }
    this->savePrefs(settings.modeMask, settings.speedMask);
    settings.seed = freshSeed();

    auto level = GJGameLevel::create();
    level->m_levelName = "Sightreading Practice";
    level->m_creatorName = "Random";
    level->m_levelString = ZipUtils::compressString(generate(settings), false, 0);
    level->m_levelType = GJLevelType::Editor; // local only: nothing is uploaded or added to stats
    level->m_audioTrack = static_cast<int>(settings.seed % 21); // a random main-level song
    level->m_songID = 0;
    s_generated = level;

    this->onClose(nullptr);
    CCDirector::get()->replaceScene(
        CCTransitionFade::create(0.5f, PlayLayer::scene(level, false, false))
    );
}
