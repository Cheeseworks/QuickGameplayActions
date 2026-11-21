#pragma once

#include <Geode/Geode.hpp>

template <>
struct matjson::Serialize<cocos2d::CCPoint> final {
    static geode::Result<cocos2d::CCPoint> fromJson(matjson::Value const& value);
    static matjson::Value toJson(cocos2d::CCPoint const& value);
};

namespace cw::gps::ui {
    class ActionMenu final : public cocos2d::CCLayer {
    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;  // old relic :3

        struct ActionItem final {
            bool enabled;
            geode::ZStringView sprite;
            std::string id;
            geode::Button::ButtonCallback callback;
            float scale = 1.0f;
        };

    protected:
        ActionMenu();
        ~ActionMenu();

        void setOpacity(GLubyte opacity);

        void setScale(float scale) override;
        void setVisible(bool visible) override;

        bool init(PlayLayer* pl);

    public:
        static ActionMenu* create(PlayLayer* pl);

        bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent*) override;
        void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent*) override;
        void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent*) override;
    };
};