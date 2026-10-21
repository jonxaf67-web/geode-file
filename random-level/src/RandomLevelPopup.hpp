#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <array>

using namespace geode::prelude;

// One selectable difficulty tier, mapped to GD's search filters.
struct DifficultyTier {
    char const* name;
    char const* frame;      // difficulty face sprite frame
    char const* searchDiff; // "diff" filter sent to the servers
    int demonFilter;        // 1-5 for demon tiers, 0 otherwise
};

class RandomLevelPopup : public Popup, public LevelManagerDelegate {
protected:
    enum class Stage { Idle, Probe, Fetch };
    static constexpr int LENGTH_COUNT = 6; // Tiny, Short, Medium, Long, XL, Plat

    int m_minTier = 1;
    int m_maxTier = 5;
    int m_tier = 0;             // tier rolled for the current search
    Stage m_stage = Stage::Idle;
    std::string m_key;          // search key we're waiting on
    int m_total = -1;           // total results reported by the server
    int m_pageCap = 0;          // highest page we'll try on the next fetch
    int m_fetchAttempts = 0;
    bool m_closed = false;
    bool m_resultQueued = false;
    bool m_lastFailed = false;

    Ref<CCArray> m_probeLevels;
    Ref<CCArray> m_lastLevels;

    CCNode* m_minIcon = nullptr;
    CCNode* m_maxIcon = nullptr;
    CCLabelBMFont* m_minLabel = nullptr;
    CCLabelBMFont* m_maxLabel = nullptr;
    CCLabelBMFont* m_status = nullptr;
    CCMenuItemToggler* m_ratedToggle = nullptr;
    CCMenuItemToggler* m_uncompletedToggle = nullptr;
    std::array<CCMenuItemToggler*, LENGTH_COUNT> m_lengthToggles{};
    CCMenuItemSpriteExtra* m_rollBtn = nullptr;

    ~RandomLevelPopup();
    bool init() override;
    void onClose(CCObject*) override;

    void addTierRow(char const* label, float y, int row);
    void refreshTierRow(bool isMin);
    void onTierArrow(CCObject* sender);
    void onToggle(CCObject*) {}
    void onSightreading(CCObject*);

    std::string lengthFilter() const;
    void savePrefs();

    void onRoll(CCObject*);
    void requestPage(int page);
    void queueResult();
    void handleResult();
    void openRandomFrom(CCArray* levels);
    void finishWithError(std::string const& msg);
    void setBusy(bool busy, std::string const& text = "");
    void detachDelegate();

    // LevelManagerDelegate
    void loadLevelsFinished(CCArray* levels, char const* key, int type) override;
    void loadLevelsFailed(char const* key, int type) override;
    void setupPageInfo(gd::string info, char const* key) override;

public:
    static RandomLevelPopup* create();
};
