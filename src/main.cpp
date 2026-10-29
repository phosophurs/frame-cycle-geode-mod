#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <deque>
#include <cstdint>

using namespace geode::prelude;

namespace {
    // Normal is a true pass-through. Other values delay the complete jump
    // input event (press AND release) by that many game ticks.
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
        return frames == 0 ? "NORMAL" : std::to_string(frames) + " FRAMES";
    }

    void showModePopup() {
        const int frames = selectedFrames();
        const std::string message = frames == 0
            ? "FRAME CYCLE: NORMAL"
            : "FRAME CYCLE: " + std::to_string(frames) + " FRAME INPUT DELAY";
        Notification::create(
            message,
            NotificationIcon::Info,
            frames == 0 ? 2.0f : 2.5f
        )->show();
    }

    void cycleMode() {
        g_modeIndex = (g_modeIndex + 1) % kModeCount;
        if (Mod::get()->getSettingValue<bool>("show-popup")) {
            showModePopup();
        }
        log::info("Frame Cycle mode: {}", modeName());
    }

    struct QueuedInput {
        uint64_t dueTick;
        bool down;
        int button;
        bool isP1;
    };
}

class $modify(FrameCycleGameLayer, GJBaseGameLayer) {
    struct Fields {
        std::deque<QueuedInput> queuedInputs;
        uint64_t tick = 0;
        bool injecting = false;
    };

    void sendRaw(bool down, int button, bool isP1) {
        m_fields->injecting = true;
        GJBaseGameLayer::handleButton(down, button, isP1);
        m_fields->injecting = false;
    }

    void handleButton(bool down, int button, bool isP1) {
        // Only delay the actual jump input. All other buttons and Normal mode
        // are passed directly to the original game handler.
        if (m_fields->injecting || !bufferEnabled() ||
            selectedFrames() == 0 || button != 1) {
            GJBaseGameLayer::handleButton(down, button, isP1);
            return;
        }

        // Queue both edges. If the player taps quickly, the press and release
        // arrive together after the selected delay, preserving a real click.
        m_fields->queuedInputs.push_back(QueuedInput{
            m_fields->tick + static_cast<uint64_t>(selectedFrames()),
            down,
            button,
            isP1
        });
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        // Count game ticks, not render frames. Deliver due input before the
        // original command processing for this tick.
        if (isLastTick) {
            ++m_fields->tick;
            while (!m_fields->queuedInputs.empty() &&
                   m_fields->queuedInputs.front().dueTick <= m_fields->tick) {
                const auto input = m_fields->queuedInputs.front();
                m_fields->queuedInputs.pop_front();
                sendRaw(input.down, input.button, input.isP1);
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
