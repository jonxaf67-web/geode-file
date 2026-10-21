#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/CCMenuItemToggler.hpp>

using namespace geode::prelude;

// Small shared widgets for the two popups.
namespace ui {
    // A checkbox with a text label to its right, centred on `pos` (relative to the parent's centre).
    inline CCMenuItemToggler* addToggle(
        CCNode* parent, CCObject* target, SEL_MenuHandler cb,
        char const* label, CCPoint pos, bool on, float textScale = 0.35f
    ) {
        auto menu = CCMenu::create();
        menu->setContentSize({ 0.f, 0.f });
        parent->addChildAtPosition(menu, Anchor::Center, pos);

        auto toggle = CCMenuItemToggler::createWithStandardSprites(target, cb, 0.55f);
        toggle->toggle(on);
        toggle->setPosition({ -8.f, 0.f });
        menu->addChild(toggle);

        auto text = CCLabelBMFont::create(label, "bigFont.fnt");
        text->setAnchorPoint({ 0.f, 0.5f });
        text->setScale(textScale);
        text->setPosition({ 6.f, 0.f });
        menu->addChild(text);
        return toggle;
    }

    inline CCLabelBMFont* addLabel(CCNode* parent, char const* text, CCPoint pos, float scale = 0.5f,
                                   char const* font = "goldFont.fnt") {
        auto label = CCLabelBMFont::create(text, font);
        label->setScale(scale);
        parent->addChildAtPosition(label, Anchor::Center, pos);
        return label;
    }

    // A "< value >" selector. Arrow tags are tagBase (left) and tagBase + 1 (right).
    inline CCLabelBMFont* addStepper(
        CCNode* parent, CCObject* target, SEL_MenuHandler cb,
        char const* title, float y, int tagBase, float halfWidth = 95.f
    ) {
        auto menu = CCMenu::create();
        menu->setContentSize({ 0.f, 0.f });
        parent->addChildAtPosition(menu, Anchor::Center, { 30.f, y });

        auto titleLabel = CCLabelBMFont::create(title, "goldFont.fnt");
        titleLabel->setScale(0.55f);
        titleLabel->setAnchorPoint({ 1.f, 0.5f });
        titleLabel->setPosition({ -halfWidth - 20.f, 0.f });
        menu->addChild(titleLabel);

        auto left = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("edit_leftBtn_001.png"), target, cb);
        left->setTag(tagBase);
        left->setPosition({ -halfWidth, 0.f });
        menu->addChild(left);

        auto right = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("edit_rightBtn_001.png"), target, cb);
        right->setTag(tagBase + 1);
        right->setPosition({ halfWidth, 0.f });
        menu->addChild(right);

        auto value = CCLabelBMFont::create("", "bigFont.fnt");
        value->setScale(0.5f);
        menu->addChild(value);
        return value;
    }
}
