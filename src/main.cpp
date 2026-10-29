#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <cstdint>

using namespace geode::prelude;

namespace {
    constexpr int kModes[] = { 0, 2, 4, 6, 8, 16 };
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

    bool playerGrounded(bool isP1) {
        auto player = isP1 ? m_player1 : m_player2;
        return player && (player->m_isOnGround || player->m_isOnGround2);
    }

    void handleButton(bool down, int button, bool isP1) {
        if (m_fields->injecting || !bufferEnabled() ||
            selectedFrames() == 0 || button != 1) {
            GJBaseGameLayer::handleButton(down, button, isP1);
            return;
        }

        auto& jump = isP1 ? m_fields->p1 : m_fields->p2;

        // A press is remembered even if the player releases before landing.
        // The request expires after the selected number of game ticks.
        if (down) {
            jump.pending = true;
            jump.expiresAt = m_fields->tick + static_cast<uint64_t>(selectedFrames());
        }
        // The release intentionally does not cancel the buffered jump.
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        if (isLastTick) {
            ++m_fields->tick;

            auto tryBufferedJump = [this](BufferedJump& jump, bool isP1) {
                if (!jump.pending) return;
                if (m_fields->tick > jump.expiresAt) {
                    jump.pending = false;
                    return;
                }
                if (playerGrounded(isP1)) {
                    jump.pending = false;
                    // Deliver a fresh tap on the first tick jumping is possible.
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
