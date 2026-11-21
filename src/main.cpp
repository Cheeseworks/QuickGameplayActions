#include <cw.h>

#include <rhatt/prelude.h>

#include <Geode/modify/PlayLayer.hpp>

using namespace cw::gps;

class $modify(ShortcutsPlayLayer, PlayLayer) {
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        m_uiLayer->addChild(ActionMenu::create(this), CW_HIGHEST_Z);
    }
};