#include "RandomLevelPopup.hpp"
#include "SightreadingPopup.hpp"
#include "UiHelpers.hpp"

#include <Geode/binding/GameLevelManager.hpp>
#include <Geode/binding/GJSearchObject.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/LevelInfoLayer.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemToggler.hpp>
#include <random>

// GD search filters: "-3" = Auto, "1".."5" = Easy..Insane,
// "-2" = Demon, narrowed by demonFilter (1 Easy ... 5 Extreme).
static constexpr DifficultyTier TIERS[] = {
    { "Auto",          "difficulty_auto_btn_001.png", "-3", 0 },
    { "Easy",          "difficulty_01_btn_001.png",   "1",  0 },
    { "Normal",        "difficulty_02_btn_001.png",   "2",  0 },
    { "Hard",          "difficulty_03_btn_001.png",   "3",  0 },
    { "Harder",        "difficulty_04_btn_001.png",   "4",  0 },
    { "Insane",        "difficulty_05_btn_001.png",   "5",  0 },
    { "Easy Demon",    "difficulty_07_btn_001.png",   "-2", 1 },
    { "Medium Demon",  "difficulty_08_btn_001.png",   "-2", 2 },
    { "Hard Demon",    "difficulty_06_btn_001.png",   "-2", 3 },
    { "Insane Demon",  "difficulty_09_btn_001.png",   "-2", 4 },
    { "Extreme Demon", "difficulty_10_btn_001.png",   "-2", 5 },
};
static constexpr int TIER_COUNT = sizeof(TIERS) / sizeof(TIERS[0]);
static constexpr int PAGE_SIZE = 10;     // levels per search page
static constexpr int MAX_FETCH_TRIES = 3;

static int randInt(int lo, int hi) {
    static std::mt19937 rng{ std::random_device{}() };
    if (hi <= lo) return lo;
    return std::uniform_int_distribution<int>(lo, hi)(rng);
}

RandomLevelPopup* RandomLevelPopup::create() {
    auto ret = new RandomLevelPopup();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

RandomLevelPopup::~RandomLevelPopup() {
    this->detachDelegate();
}

static constexpr char const* LENGTH_NAMES[] = { "Tiny", "Short", "Medium", "Long", "XL", "Plat" };

bool RandomLevelPopup::init() {
    if (!Popup::init(360.f, 300.f)) return false;
    this->setTitle("Random Level");

    auto mod = Mod::get();
    m_minTier = std::clamp(mod->getSavedValue<int>("min-tier", 1), 0, TIER_COUNT - 1);
    m_maxTier = std::clamp(mod->getSavedValue<int>("max-tier", 5), 0, TIER_COUNT - 1);
    if (m_minTier > m_maxTier) std::swap(m_minTier, m_maxTier);

    this->addTierRow("Min", 92.f, 0);
    this->addTierRow("Max", 56.f, 1);
    this->refreshTierRow(true);
    this->refreshTierRow(false);

    // Length filter: all ticked (or none) means "any length".
    ui::addLabel(m_mainLayer, "Length", { 0.f, 26.f });
    int lengthMask = mod->getSavedValue<int>("length-mask", 0b111111);
    for (int i = 0; i < LENGTH_COUNT; i++) {
        CCPoint pos = { -120.f + (i % 3) * 105.f, 4.f - (i / 3) * 22.f };
        m_lengthToggles[i] = ui::addToggle(
            m_mainLayer, this, menu_selector(RandomLevelPopup::onToggle),
            LENGTH_NAMES[i], pos, (lengthMask >> i) & 1
        );
    }

    m_ratedToggle = ui::addToggle(m_mainLayer, this, menu_selector(RandomLevelPopup::onToggle),
        "Rated only", { -95.f, -52.f }, mod->getSavedValue<bool>("rated-only", true));
    m_uncompletedToggle = ui::addToggle(m_mainLayer, this, menu_selector(RandomLevelPopup::onToggle),
        "Skip completed", { 35.f, -52.f }, mod->getSavedValue<bool>("skip-completed", false));

    m_status = CCLabelBMFont::create("", "chatFont.fnt");
    m_status->setScale(0.7f);
    m_mainLayer->addChildAtPosition(m_status, Anchor::Center, { 0.f, -76.f });

    auto btnMenu = CCMenu::create();
    btnMenu->setContentSize({ 0.f, 0.f });
    m_rollBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Roll!"), this, menu_selector(RandomLevelPopup::onRoll)
    );
    m_rollBtn->setPosition({ -80.f, 0.f });
    btnMenu->addChild(m_rollBtn);

    auto srSpr = ButtonSprite::create("Sightreading", "goldFont.fnt", "GJ_button_05.png");
    srSpr->setScale(0.8f);
    auto srBtn = CCMenuItemSpriteExtra::create(
        srSpr, this, menu_selector(RandomLevelPopup::onSightreading)
    );
    srBtn->setPosition({ 70.f, 0.f });
    btnMenu->addChild(srBtn);
    m_mainLayer->addChildAtPosition(btnMenu, Anchor::Bottom, { 0.f, 30.f });

    return true;
}

std::string RandomLevelPopup::lengthFilter() const {
    std::string out;
    int count = 0;
    for (int i = 0; i < LENGTH_COUNT; i++) {
        if (!m_lengthToggles[i]->isToggled()) continue;
        if (!out.empty()) out += ",";
        out += std::to_string(i);
        count++;
    }
    // GD's search uses "-" for "no length filter".
    if (count == 0 || count == LENGTH_COUNT) return "-";
    return out;
}

void RandomLevelPopup::savePrefs() {
    auto mod = Mod::get();
    mod->setSavedValue("min-tier", m_minTier);
    mod->setSavedValue("max-tier", m_maxTier);
    mod->setSavedValue("rated-only", m_ratedToggle->isToggled());
    mod->setSavedValue("skip-completed", m_uncompletedToggle->isToggled());
    int mask = 0;
    for (int i = 0; i < LENGTH_COUNT; i++) {
        if (m_lengthToggles[i]->isToggled()) mask |= 1 << i;
    }
    mod->setSavedValue("length-mask", mask);
}

void RandomLevelPopup::onSightreading(CCObject*) {
    if (m_stage != Stage::Idle) return;
    this->savePrefs();
    this->onClose(nullptr);
    if (auto popup = SightreadingPopup::create()) popup->show();
}

void RandomLevelPopup::addTierRow(char const* label, float y, int row) {
    auto menu = CCMenu::create();
    menu->setContentSize({ 0.f, 0.f });
    menu->setTag(100 + row);
    m_mainLayer->addChildAtPosition(menu, Anchor::Center, { 0.f, y });

    auto title = CCLabelBMFont::create(label, "goldFont.fnt");
    title->setScale(0.6f);
    title->setPosition({ -128.f, 0.f });
    menu->addChild(title);

    auto left = CCSprite::createWithSpriteFrameName("edit_leftBtn_001.png");
    auto leftBtn = CCMenuItemSpriteExtra::create(left, this, menu_selector(RandomLevelPopup::onTierArrow));
    leftBtn->setTag(row * 2);
    leftBtn->setPosition({ -95.f, 0.f });
    menu->addChild(leftBtn);

    auto right = CCSprite::createWithSpriteFrameName("edit_rightBtn_001.png");
    auto rightBtn = CCMenuItemSpriteExtra::create(right, this, menu_selector(RandomLevelPopup::onTierArrow));
    rightBtn->setTag(row * 2 + 1);
    rightBtn->setPosition({ 128.f, 0.f });
    menu->addChild(rightBtn);

    auto name = CCLabelBMFont::create("", "bigFont.fnt");
    name->setAnchorPoint({ 0.f, 0.5f });
    name->setPosition({ -35.f, 0.f });
    menu->addChild(name);
    (row == 0 ? m_minLabel : m_maxLabel) = name;
}

void RandomLevelPopup::refreshTierRow(bool isMin) {
    int tier = isMin ? m_minTier : m_maxTier;
    auto& icon = isMin ? m_minIcon : m_maxIcon;
    auto label = isMin ? m_minLabel : m_maxLabel;
    auto menu = m_mainLayer->getChildByTag(isMin ? 100 : 101);
    if (!menu || !label) return;

    if (icon) icon->removeFromParent();
    CCNode* spr = CCSprite::createWithSpriteFrameName(TIERS[tier].frame);
    if (!spr) spr = CCNode::create(); // missing frame: keep layout, skip the icon
    spr->setScale(0.6f);
    spr->setPosition({ -60.f, 0.f });
    menu->addChild(spr);
    icon = spr;

    label->setString(TIERS[tier].name);
    label->limitLabelWidth(120.f, 0.5f, 0.1f);
}

void RandomLevelPopup::onTierArrow(CCObject* sender) {
    if (m_stage != Stage::Idle) return;
    int tag = static_cast<CCNode*>(sender)->getTag();
    bool isMin = tag < 2;
    int dir = (tag % 2 == 0) ? -1 : 1;

    // Wrap around, then keep min <= max by dragging the other end along.
    if (isMin) {
        m_minTier = (m_minTier + dir + TIER_COUNT) % TIER_COUNT;
        if (m_minTier > m_maxTier) { m_maxTier = m_minTier; this->refreshTierRow(false); }
    } else {
        m_maxTier = (m_maxTier + dir + TIER_COUNT) % TIER_COUNT;
        if (m_maxTier < m_minTier) { m_minTier = m_maxTier; this->refreshTierRow(true); }
    }
    this->refreshTierRow(isMin);
    m_status->setString("");
}

void RandomLevelPopup::setBusy(bool busy, std::string const& text) {
    m_rollBtn->setEnabled(!busy);
    m_rollBtn->setOpacity(busy ? 120 : 255);
    m_status->setColor({ 255, 255, 255 });
    m_status->setString(text.c_str());
}

void RandomLevelPopup::onRoll(CCObject*) {
    if (m_stage != Stage::Idle) return;

    this->savePrefs();

    // Each tier in the range is equally likely, regardless of how many levels it has.
    m_tier = randInt(m_minTier, m_maxTier);
    m_total = -1;
    m_fetchAttempts = 0;
    m_probeLevels = nullptr;
    m_stage = Stage::Probe;

    this->setBusy(true, fmt::format("Searching {} levels...", TIERS[m_tier].name));
    this->requestPage(0);
}

void RandomLevelPopup::requestPage(int page) {
    auto const& tier = TIERS[m_tier];
    auto search = GJSearchObject::create(
        SearchType::Search, "", tier.searchDiff, this->lengthFilter(), page,
        m_ratedToggle->isToggled(),       // star (rated only)
        m_uncompletedToggle->isToggled(), // uncompleted
        false, 0, false, false, false, false, false, false, false, false, false, false,
        tier.demonFilter, 0, 0
    );

    m_key = search->getKey();
    m_lastLevels = nullptr;
    m_lastFailed = false;

    auto glm = GameLevelManager::get();
    glm->m_levelManagerDelegate = this;
    glm->getOnlineLevels(search); // may answer synchronously if the page is cached
}

void RandomLevelPopup::loadLevelsFinished(CCArray* levels, char const* key, int) {
    if (m_closed || m_stage == Stage::Idle || !key || m_key != key) return;
    m_lastLevels = levels;
    this->queueResult();
}

void RandomLevelPopup::loadLevelsFailed(char const* key, int) {
    if (m_closed || m_stage == Stage::Idle || !key || m_key != key) return;
    m_lastFailed = true;
    this->queueResult();
}

void RandomLevelPopup::setupPageInfo(gd::string info, char const* key) {
    if (m_closed || m_stage == Stage::Idle || !key || m_key != key) return;
    // Format: "total:offset:pageSize"
    std::string str = info;
    auto colon = str.find(':');
    m_total = utils::numFromString<int>(str.substr(0, colon)).unwrapOr(-1);
}

void RandomLevelPopup::queueResult() {
    // GD sends the level list and the page info as two separate callbacks;
    // wait a frame so both have arrived before deciding anything.
    if (m_resultQueued) return;
    m_resultQueued = true;
    Ref<RandomLevelPopup> self = this;
    queueInMainThread([self] {
        self->m_resultQueued = false;
        if (!self->m_closed) self->handleResult();
    });
}

void RandomLevelPopup::handleResult() {
    bool gotLevels = !m_lastFailed && m_lastLevels && m_lastLevels->count() > 0;

    if (m_stage == Stage::Probe) {
        if (!gotLevels) {
            this->finishWithError(fmt::format("No {} levels match these filters.", TIERS[m_tier].name));
            return;
        }
        m_probeLevels = m_lastLevels;
        int pages = m_total > 0 ? (m_total + PAGE_SIZE - 1) / PAGE_SIZE : 1;
        int page = randInt(0, pages - 1);
        if (page == 0) {
            this->openRandomFrom(m_probeLevels);
            return;
        }
        m_pageCap = pages;
        m_fetchAttempts = 1;
        m_stage = Stage::Fetch;
        this->requestPage(page);
        return;
    }

    if (m_stage == Stage::Fetch) {
        if (gotLevels) {
            this->openRandomFrom(m_lastLevels);
            return;
        }
        // Very deep pages can come back empty; retry in a smaller window,
        // and fall back to the first page if that keeps failing.
        if (m_fetchAttempts < MAX_FETCH_TRIES) {
            m_pageCap = std::max(1, m_pageCap / 4);
            int page = randInt(0, m_pageCap - 1);
            if (page > 0) {
                m_fetchAttempts++;
                this->requestPage(page);
                return;
            }
        }
        this->openRandomFrom(m_probeLevels);
    }
}

void RandomLevelPopup::openRandomFrom(CCArray* levels) {
    Ref<RandomLevelPopup> self = this; // closing below may otherwise free us
    m_stage = Stage::Idle;
    this->detachDelegate();

    GJGameLevel* level = nullptr;
    if (levels && levels->count() > 0) {
        int idx = randInt(0, static_cast<int>(levels->count()) - 1);
        level = typeinfo_cast<GJGameLevel*>(levels->objectAtIndex(idx));
    }
    if (!level) {
        this->finishWithError("Couldn't read the level list. Try again.");
        return;
    }

    Ref<GJGameLevel> keep = level;
    this->onClose(nullptr);
    CCDirector::get()->pushScene(
        CCTransitionFade::create(0.5f, LevelInfoLayer::scene(level, false))
    );
}

void RandomLevelPopup::finishWithError(std::string const& msg) {
    m_stage = Stage::Idle;
    this->detachDelegate();
    this->setBusy(false);
    m_status->setColor({ 255, 110, 110 });
    m_status->setString(msg.c_str());
}

void RandomLevelPopup::detachDelegate() {
    auto glm = GameLevelManager::get();
    if (glm && glm->m_levelManagerDelegate == static_cast<LevelManagerDelegate*>(this)) {
        glm->m_levelManagerDelegate = nullptr;
    }
}

void RandomLevelPopup::onClose(CCObject* sender) {
    if (m_closed) return;
    m_closed = true;
    m_stage = Stage::Idle;
    this->detachDelegate();
    Popup::onClose(sender);
}
