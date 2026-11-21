#include "ActionMenu.hpp"

#include <cw.h>

#include <rhatt/prelude.h>

#include <Geode/Geode.hpp>

using namespace geode::prelude;
using namespace cw::gps;

Result<CCPoint> matjson::Serialize<CCPoint>::fromJson(matjson::Value const& value) {
    CCPoint out;

    GEODE_UNWRAP_INTO(out.x, value["x"].asFloat());
    GEODE_UNWRAP_INTO(out.y, value["y"].asFloat());

    return Ok(out);
}

matjson::Value matjson::Serialize<CCPoint>::toJson(CCPoint const& value) {
    auto out = matjson::Value();
    out["x"] = value.x;
    out["y"] = value.y;

    return out;
}

class ActionMenu::Impl final {
public:
    Ref<CircleButtonSprite> sprite = nullptr;

    CCNode* menu = nullptr;
    NineSlice* menuBg = nullptr;

    bool useRestart = Mod::get()->getSettingValue<bool>("restart");
    bool usePractice = Mod::get()->getSettingValue<bool>("practice");
    bool usePause = Mod::get()->getSettingValue<bool>("pause");
    bool useExit = Mod::get()->getSettingValue<bool>("exit");

    bool show = Mod::get()->getSavedValue<bool>("visible", true);
    bool toggleOnPress = Mod::get()->getSettingValue<bool>("toggle-press");

    float scale = Mod::get()->getSettingValue<float>("scale");
    GLubyte opacity = Mod::get()->getSettingValue<GLubyte>("opacity");

    CCSize const screenSize = CCDirector::sharedDirector()->getWinSize();
    CCPoint dragStartPos = { 0.0f, 0.0f };
    CCPoint comparePos = { 0.0f, 0.0f };

    bool isAnimating = false;
    bool isDragging = false;

    bool isDistant(CCPoint const& ccp1, CCPoint const& ccp2, float max) const {
        return ccpDistance(ccp1, ccp2) <= max;
    }
};

ActionMenu::ActionMenu() : m_impl(std::make_unique<Impl>()) {}
ActionMenu::~ActionMenu() = default;

bool ActionMenu::init(PlayLayer* pl) {
    if (!CCLayer::init()) return false;

    auto const pos = Mod::get()->getSavedValue<CCPoint>("menu-pos", { 75.0f, m_impl->screenSize.height - 75.0f });

    setID("menu"_spr);
    setPosition(pos);
    setTouchMode(kCCTouchesOneByOne);
    setAnchorPoint(cw::anchor::center);
    setTouchEnabled(true);

    m_impl->sprite = CircleButtonSprite::createWithSpriteFrameName("edit_areaModeBtn04_001.png");
    m_impl->sprite->setAnchorPoint(cw::anchor::center);
    m_impl->sprite->setScale(m_impl->scale * 0.875f);

    setContentSize(m_impl->sprite->getScaledContentSize());

    m_impl->sprite->setPosition(getScaledContentSize() / 2.0f);

    setScale(m_impl->scale);
    setOpacity(m_impl->opacity);

    addChild(m_impl->sprite, 9);

    auto showMenu = m_impl->toggleOnPress ? m_impl->show : true;

    auto layout = RowLayout::create()
                      ->setGap(2.5f)
                      ->setAutoScale(false)
                      ->setAutoGrowAxis(0.0f);

    m_impl->menu = CCNode::create();
    m_impl->menu->setID("actions-menu");
    m_impl->menu->setAnchorPoint({ 0.0f, 1.0f });
    m_impl->menu->setContentSize({ 0.0f, 25.0f });
    m_impl->menu->setPosition({ (getScaledContentWidth() / 2.0f) + 7.5f, (getScaledContentHeight() / 2.0f) - 5.0f });
    m_impl->menu->setVisible(showMenu);
    m_impl->menu->setLayout(layout);

    auto btns = std::array{
        ActionItem{
            m_impl->useRestart,
            "GJ_replayBtn_001.png",
            "restart-btn",
            [pl](auto) {
                if (pl) pl->resetLevel();
            },
        },
        ActionItem{
            m_impl->usePractice,
            "GJ_practiceBtn_001.png",
            "toggle-practice-btn",
            [this, pl](Button* sender) {
                {
                    if (pl) pl->togglePracticeMode(!pl->m_isPracticeMode);
                    sender->setDisplayNode(CCSprite::createWithSpriteFrameName(pl->m_isPracticeMode ? "GJ_normalBtn_001.png" : "GJ_practiceBtn_001.png"));
                };
            },
        },
        ActionItem{
            m_impl->usePause,
            "GJ_pauseBtn_001.png",
            "pause-btn",
            [pl](auto) {
                if (pl) pl->pauseGame(false);
            },
            1.62f,
        },
        ActionItem{
            m_impl->useExit,
            "GJ_menuBtn_001.png",
            "exit-btn",
            [pl](auto) {
                if (pl) pl->onQuit();
                cw::sfx::play("quitSound_01.ogg");
            },
        },
        ActionItem{
            m_impl->useRestart && pl->m_isPlatformer,
            "GJ_replayFullBtn_001.png",
            "full-restart-btn",
            [pl](auto) {
                if (pl) pl->resetLevelFromStart();
            },
        },
    };

    for (auto& b : btns) {
        if (b.enabled) {
            auto btn = Button::createWithSpriteFrameName(
                b.sprite,
                std::move(b.callback)
            );
            btn->setID(std::move(b.id));
            btn->setScale(m_impl->scale * b.scale);
            btn->setOpacity(m_impl->opacity);
            btn->setScaleMultiplier(1.125f);

            m_impl->menu->addChild(btn);
        };
    };

    addChild(m_impl->menu, 1);
    m_impl->menu->updateLayout();

    m_impl->menuBg = NineSlice::create("square02_001.png");
    m_impl->menuBg->setOpacity(m_impl->opacity / 2);
    m_impl->menuBg->setAnchorPoint(m_impl->menu->getAnchorPoint());
    m_impl->menuBg->setContentSize({ m_impl->menu->getScaledContentWidth() + 15.0f, m_impl->menu->getScaledContentHeight() + 5.0f });
    m_impl->menuBg->setPosition(m_impl->sprite->getPosition());
    m_impl->menuBg->setScaleMultiplier(0.5f);
    m_impl->menuBg->setVisible(showMenu);

    addChild(m_impl->menuBg, 0);

    return true;
}

void ActionMenu::setOpacity(GLubyte opacity) {
    m_impl->opacity = opacity;
    if (m_impl->sprite) m_impl->sprite->setOpacity(opacity / 1.25);
}

void ActionMenu::setScale(float scale) {
    m_impl->scale = scale;

    if (!m_impl->isDragging && !m_impl->isAnimating) {
        if (m_impl->sprite) {
            m_impl->sprite->setScale(scale);
            setContentSize(m_impl->sprite->getScaledContentSize());
        };

        if (m_impl->menu) m_impl->menu->setScale(scale);
        if (m_impl->menuBg) m_impl->menuBg->setScale(scale);
    };
}

void ActionMenu::setVisible(bool visible) {
    if (m_impl->menu) {
        m_impl->show = visible;

        m_impl->menu->setVisible(visible);
        if (m_impl->menuBg) m_impl->menuBg->setVisible(visible);

        (void)Mod::get()->setSavedValue("visible", visible);

#ifdef CW_TESTING
        log::trace("Toggled action menu {}", m_impl->show ? "on" : "off");
#endif
    } else {
        log::error("Couldn't toggle action menu visibility");
    };
}

bool ActionMenu::ccTouchBegan(CCTouch* touch, CCEvent*) {
    if (m_impl->sprite) {
        auto const box = m_impl->sprite->boundingBox();

        if (box.containsPoint(convertToNodeSpace(touch->getLocation()))) {
            m_impl->isDragging = true;

            m_impl->comparePos = getPosition();
            m_impl->dragStartPos = ccpSub(getPosition(), touch->getLocation());

#ifdef CW_TESTING
            log::debug("Menu position starts at ({}, {})", m_impl->dragStartPos.x, m_impl->dragStartPos.y);
#endif

            m_impl->sprite->stopAllActions();
            m_impl->isAnimating = true;
            m_impl->sprite->runAction(
                CCSequence::createWithTwoActions(
                    CCSpawn::createWithTwoActions(
                        CCEaseExponentialOut::create(
                            CCScaleTo::create(0.25f, m_impl->scale * 0.875f)
                        ),
                        CCFadeTo::create(0.25f, 255)
                    ),
                    CallFuncExt::create([this]() {
                        m_impl->isAnimating = false;
                    })
                )
            );

            return true;  // swallow like a
        };
    };

    return false;
}

void ActionMenu::ccTouchMoved(CCTouch* touch, CCEvent*) {
    if (m_impl->isDragging) {
        auto const touchLocation = touch->getLocation();
        auto const newLocation = ccpAdd(touchLocation, m_impl->dragStartPos);

        auto clampX = std::max(0.0f, std::min(newLocation.x, m_impl->screenSize.width - getScaledContentWidth()));
        auto clampY = std::max(0.0f, std::min(newLocation.y, m_impl->screenSize.height - getScaledContentHeight()));

        setPosition({ clampX, clampY });
    };
}

void ActionMenu::ccTouchEnded(CCTouch* touch, CCEvent*) {
    if (m_impl->isDragging) {
        auto pos = ccpSub(getPosition(), touch->getLocation());
        if (m_impl->toggleOnPress && m_impl->isDistant(m_impl->comparePos, getPosition(), 5.0f)) setVisible(!m_impl->show);

        m_impl->isDragging = false;

        (void)Mod::get()->setSavedValue<CCPoint>("menu-pos", getPosition());

        if (m_impl->sprite) {
            m_impl->isAnimating = true;

            m_impl->sprite->stopAllActions();
            m_impl->sprite->runAction(
                CCSequence::create(
                    CCSpawn::createWithTwoActions(
                        CCFadeTo::create(0.125f, 255),
                        CCEaseElasticOut::create(
                            CCScaleTo::create(0.875f, m_impl->scale)
                        )
                    ),
                    CallFuncExt::create([this]() {
                        m_impl->isAnimating = false;
                    }),
                    CCDelayTime::create(1.0f),
                    CCFadeTo::create(0.5f, m_impl->opacity / 1.25),
                    nullptr
                )
            );
        };

        m_impl->dragStartPos = pos;

#ifdef CW_TESTING
        log::debug("Menu position stopped and saved at ({}, {})", m_impl->dragStartPos.x, m_impl->dragStartPos.y);
#endif
    };
}

ActionMenu* ActionMenu::create(PlayLayer* pl) {
    auto ret = new ActionMenu();
    if (ret->init(pl)) {
        ret->autorelease();
        return ret;
    };

    delete ret;
    return nullptr;
}