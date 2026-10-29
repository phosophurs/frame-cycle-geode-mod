#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/loader/GameEvent.hpp>

using namespace geode::prelude;

namespace {
    // 64-frame mode is first so it is the default and immediately testable.
    constexpr int kModes[] = { 0, 2, 4, 6, 8, 16, 64 };
    constexpr int kModeCount = sizeof(kModes) / sizeof(kModes[0]);
    int g_modeIndex = 6;

    int extraFrames() { return kModes[g_modeIndex]; }
    bool bufferEnabled() { return Mod::get()->getSettingValue<bool>("enabled"); }

    std::string modeName() {
        auto frames = extraFrames();
        if (frames == 0) return "NORMAL";
        if (frames == 64) return "!!! 64 FRAME WINDOW !!!";
        return std::to_string(frames) + " FRAMES";
    }

    void showModePopup() {
        auto frames = extraFrames();
        auto message = frames == 64
            ? "!!! 64 FRAME WINDOW ACTIVE !!!"
            : "FRAME CYCLE: " + modeName();
        // Keep the 64-frame warning onscreen much longer so it is unmistakable.
        Notification::create(message, NotificationIcon::Info, frames == 64 ? 5.0f : 2.5f)->show();
    }

    void cycleMode() {
        g_modeIndex = (g_modeIndex + 1) % kModeCount;
        showModePopup();
        log::info("Frame Cycle mode: {}", modeName());
    }

    struct PlayerBuffer {
        bool held = false;
        bool releaseQueued = false;
        int framesSincePress = 0;
        int button = 1;
    };
}

class $modify(FrameCycleGameLayer, GJBaseGameLayer) {
    struct Fields {
        PlayerBuffer players[2];
        bool injecting = false;
    };

    void sendRaw(bool down, int button, bool isP1) {
        m_fields->injecting = true;
        GJBaseGameLayer::handleButton(down, button, isP1);
        m_fields->injecting = false;
    }

    void handleButton(bool down, int button, bool isP1) {
        if (m_fields->injecting || !bufferEnabled() || extraFrames() <= 0) {
            GJBaseGameLayer::handleButton(down, button, isP1);
            return;
        }

        auto& state = m_fields->players[isP1 ? 0 : 1];
        if (down) {
            if (state.releaseQueued) {
                state.releaseQueued = false;
                sendRaw(false, state.button, isP1);
            }
            state.held = true;
            state.framesSincePress = 0;
            state.button = button;
            GJBaseGameLayer::handleButton(true, button, isP1);
            return;
        }

        if (state.held && state.framesSincePress < extraFrames()) {
            state.releaseQueued = true;
            return;
        }
        state.held = false;
        GJBaseGameLayer::handleButton(false, button, isP1);
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
        for (int i = 0; i < 2; i++) {
            auto& state = m_fields->players[i];
            if (!state.held) continue;
            state.framesSincePress++;
            if (state.releaseQueued && state.framesSincePress >= extraFrames()) {
                state.releaseQueued = false;
                state.held = false;
                sendRaw(false, state.button, i == 0);
            }
        }
    }
};

class $modify(FrameCyclePauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        auto winSize = CCDirector::get()->getWinSize();
        auto sprite = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
        if (!sprite) return;
        sprite->setScale(0.6f);
        auto button = CCMenuItemSpriteExtra::create(sprite, this, menu_selector(FrameCyclePauseLayer::onFrameCycleSettings));
        auto menu = CCMenu::create();
        menu->addChild(button);
        menu->setPosition({ winSize.width - 30.f, 30.f });
        menu->setID("frame-cycle-settings-menu");
        this->addChild(menu);
    }
    void onFrameCycleSettings(CCObject*) { geode::openSettingsPopup(Mod::get()); }
};

$on_game(Loaded) {
    listenForKeybindSettingPresses("cycle-key", [](Keybind const&, bool down, bool repeat, double) {
        if (down && !repeat) cycleMode();
    });
    listenForKeybindSettingPresses("popup-key", [](Keybind const&, bool down, bool repeat, double) {
        if (down && !repeat) showModePopup();
    });
}
