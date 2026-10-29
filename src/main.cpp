#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <cstdint>

using namespace geode::prelude;

namespace {
    constexpr int kModes[] = { 0, 2, 4, 6, 8, 16, 64, 128 };
    constexpr int kModeCount = sizeof(kModes) / sizeof(kModes[0]);
    int g_modeIndex = 0;

    bool bufferEnabled() {
        return Mod::get()->getSettingValue<bool>("enabled");
    }

    int selectedFrames() {
        return kModes[g_modeIndex];
    }

    std::string modeName() {
        const int frames = selectedFrames();
        return frames == 0 ? "NORMAL" : std::to_string(frames) + " FRAME BUFFER";
    }

    void showModePopup() {
        const int frames = selectedFrames();
        const std::string message = frames == 0
            ? "FRAME CYCLE: NORMAL"
            : "FRAME CYCLE: " + std::to_string(frames) + " FRAME JUMP BUFFER";
        Notification::create(message, NotificationIcon::Info,
            frames == 0 ? 2.0f : 2.5f)->show();
    }

    void cycleMode() {
        g_modeIndex = (g_modeIndex + 1) % kModeCount;
        if (Mod::get()->getSettingValue<bool>("show-popup")) showModePopup();
        log::info("Frame Cycle mode: {}", modeName());
    }

    struct BufferedJump {
        uint64_t expiresAt = 0;
        bool pending = false;
    };
}

class $modify(FrameCycleGameLayer, GJBaseGameLayer) {
    struct Fields {
        uint64_t tick = 0;
        BufferedJump p1;
        BufferedJump p2;
        bool injecting = false;
    };

    void sendRaw(bool down, int button, bool isP1) {
        m_fields->injecting = true;
        GJBaseGameLayer::handleButton(down, button, isP1);
        m_fields->injecting = false;
    }

    PlayerObject* getPlayer(bool isP1) {
        return isP1 ? m_player1 : m_player2;
    }

    bool playerGrounded(bool isP1) {
        auto player = getPlayer(isP1);
        return player && (player->m_isOnGround || player->m_isOnGround2);
    }

    // Ground-style buffering is only meaningful for modes whose input is a
    // jump action. Ship, UFO, Wave, and Swing use continuous/tap flight input,
    // so their controls are always passed through unchanged.
    bool usesGroundJump(bool isP1) {
        auto player = getPlayer(isP1);
        if (!player) return false;
        return !player->m_isShip && !player->m_isBird &&
            !player->m_isDart && !player->m_isSwing;
    }

    void handleButton(bool down, int button, bool isP1) {
        auto& jump = isP1 ? m_fields->p1 : m_fields->p2;

        if (m_fields->injecting || !bufferEnabled() ||
            selectedFrames() == 0 || button != 1 || !usesGroundJump(isP1)) {
            // Clear stale buffered inputs when switching into a flight mode.
            if (!usesGroundJump(isP1)) jump.pending = false;
            GJBaseGameLayer::handleButton(down, button, isP1);
            return;
        }

        // Never swallow real input: always forward the user's press/release.
        // Remember airborne presses so they can trigger on landing.
        if (down) {
            if (playerGrounded(isP1)) {
                jump.pending = false;
            } else {
                jump.pending = true;
                jump.expiresAt = m_fields->tick + static_cast<uint64_t>(selectedFrames());
            }
        }

        GJBaseGameLayer::handleButton(down, button, isP1);
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        if (isLastTick) {
            ++m_fields->tick;

            auto tryBufferedJump = [this](BufferedJump& jump, bool isP1) {
                if (!jump.pending) return;
                if (!usesGroundJump(isP1)) {
                    jump.pending = false;
                    return;
                }
                if (m_fields->tick > jump.expiresAt) {
                    jump.pending = false;
                    return;
                }
                if (playerGrounded(isP1)) {
                    jump.pending = false;
                    sendRaw(true, 1, isP1);
                    sendRaw(false, 1, isP1);
                }
            };

            if (bufferEnabled() && selectedFrames() > 0) {
                tryBufferedJump(m_fields->p1, true);
                tryBufferedJump(m_fields->p2, false);
            } else {
                m_fields->p1.pending = false;
                m_fields->p2.pending = false;
            }
        }

        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
    }
};

class $modify(FrameCyclePauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        auto winSize = CCDirector::get()->getWinSize();
        auto sprite = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
        if (!sprite) return;
        sprite->setScale(0.6f);
        auto button = CCMenuItemSpriteExtra::create(
            sprite, this, menu_selector(FrameCyclePauseLayer::onFrameCycleSettings)
        );
        auto menu = CCMenu::create();
        menu->addChild(button);
        menu->setPosition({ winSize.width - 30.f, 30.f });
        menu->setID("frame-cycle-settings-menu");
        this->addChild(menu);
    }

    void onFrameCycleSettings(CCObject*) {
        geode::openSettingsPopup(Mod::get());
    }
};

$on_game(Loaded) {
    listenForKeybindSettingPresses("cycle-key",
        [](Keybind const&, bool down, bool repeat, double) {
            if (down && !repeat) cycleMode();
        });
    listenForKeybindSettingPresses("popup-key",
        [](Keybind const&, bool down, bool repeat, double) {
            if (down && !repeat) showModePopup();
        });
}
