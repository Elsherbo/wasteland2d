#include "input/InputManager.h"

namespace engine {

InputManager::InputManager() {
    // Sensible top-down survival-shooter defaults (WASD + mouse).
    bind(Action::MoveUp, SDL_SCANCODE_W);
    bind(Action::MoveDown, SDL_SCANCODE_S);
    bind(Action::MoveLeft, SDL_SCANCODE_A);
    bind(Action::MoveRight, SDL_SCANCODE_D);
    bind(Action::Interact, SDL_SCANCODE_E);
    bind(Action::Inventory, SDL_SCANCODE_TAB);
    bind(Action::Reload, SDL_SCANCODE_R);
    bind(Action::Sprint, SDL_SCANCODE_LSHIFT);
    bind(Action::Pause, SDL_SCANCODE_ESCAPE);
    // Shares R with Reload — no actual conflict: they're looked up
    // independently by Action, and RotateItem is only ever checked
    // while dragging an inventory item, which only happens while the
    // inventory is open, during which Fire/Reload are already gated
    // off in game code.
    bind(Action::RotateItem, SDL_SCANCODE_R);
    bind(Action::Slot1, SDL_SCANCODE_1);
    bind(Action::Slot2, SDL_SCANCODE_2);
    bind(Action::Slot3, SDL_SCANCODE_3);
    bind(Action::Slot4, SDL_SCANCODE_4);
    bind(Action::Slot5, SDL_SCANCODE_5);
    bind(Action::Slot6, SDL_SCANCODE_6);
    bind(Action::Slot7, SDL_SCANCODE_7);
    bind(Action::Slot8, SDL_SCANCODE_8);
    bind(Action::Slot9, SDL_SCANCODE_9);
    bind(Action::EquipBackpack, SDL_SCANCODE_B); // no numbered hotkey — 1-3 are weapons, 4-9 are quick slots
    // Fire is typically left mouse button, handled via isMouseButtonHeld();
    // still bindable to a key (e.g. controller/keyboard-only play).
    bind(Action::Fire, SDL_SCANCODE_SPACE);
}

void InputManager::bind(Action action, SDL_Scancode scancode) {
    bindings_[action] = scancode;
}

void InputManager::beginFrame() {
    for (auto& [scancode, state] : keyStates_) {
        state.pressedThisFrame = false;
        state.releasedThisFrame = false;
    }
    for (auto& [button, state] : mouseButtons_) {
        (void)button;
        state.pressedThisFrame = false;
        state.releasedThisFrame = false;
    }
    mouseWheelDelta_ = 0; // no "held" concept for a wheel tick — reset every frame like the edge flags above
}

void InputManager::handleEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_QUIT:
            quitRequested_ = true;
            break;

        case SDL_KEYDOWN: {
            if (event.key.repeat) break; // ignore OS key-repeat, we track our own edges
            KeyState& s = stateFor(event.key.keysym.scancode);
            if (!s.held) s.pressedThisFrame = true;
            s.held = true;
            break;
        }

        case SDL_KEYUP: {
            KeyState& s = stateFor(event.key.keysym.scancode);
            s.held = false;
            s.releasedThisFrame = true;
            break;
        }

        case SDL_MOUSEBUTTONDOWN: {
            KeyState& s = mouseButtons_[event.button.button];
            if (!s.held) s.pressedThisFrame = true;
            s.held = true;
            break;
        }

        case SDL_MOUSEBUTTONUP: {
            KeyState& s = mouseButtons_[event.button.button];
            s.held = false;
            s.releasedThisFrame = true;
            break;
        }

        case SDL_MOUSEMOTION:
            mouseX_ = event.motion.x;
            mouseY_ = event.motion.y;
            break;

        case SDL_MOUSEWHEEL:
            // SDL negates y when SDL_MOUSEWHEEL_FLIPPED is set (some
            // platforms/mice report "natural" scrolling this way) —
            // normalize here so mouseWheelDelta() always means the
            // same physical direction regardless of that flag, rather
            // than every caller needing to know about it.
            mouseWheelDelta_ += (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) ? -event.wheel.y : event.wheel.y;
            break;

        default:
            break;
    }
}

KeyState& InputManager::stateFor(SDL_Scancode scancode) {
    return keyStates_[scancode];
}

const KeyState* InputManager::stateForAction(Action action) const {
    auto bindingIt = bindings_.find(action);
    if (bindingIt == bindings_.end()) return nullptr;

    auto stateIt = keyStates_.find(bindingIt->second);
    if (stateIt == keyStates_.end()) return nullptr;

    return &stateIt->second;
}

bool InputManager::isHeld(Action action) const {
    const KeyState* s = stateForAction(action);
    return s && s->held;
}

bool InputManager::wasPressed(Action action) const {
    const KeyState* s = stateForAction(action);
    return s && s->pressedThisFrame;
}

bool InputManager::wasReleased(Action action) const {
    const KeyState* s = stateForAction(action);
    return s && s->releasedThisFrame;
}

bool InputManager::isMouseButtonHeld(Uint8 button) const {
    auto it = mouseButtons_.find(button);
    return it != mouseButtons_.end() && it->second.held;
}

bool InputManager::wasMouseButtonPressed(Uint8 button) const {
    auto it = mouseButtons_.find(button);
    return it != mouseButtons_.end() && it->second.pressedThisFrame;
}

bool InputManager::wasMouseButtonReleased(Uint8 button) const {
    auto it = mouseButtons_.find(button);
    return it != mouseButtons_.end() && it->second.releasedThisFrame;
}

bool InputManager::isCtrlHeld() const {
    auto left = keyStates_.find(SDL_SCANCODE_LCTRL);
    auto right = keyStates_.find(SDL_SCANCODE_RCTRL);
    return (left != keyStates_.end() && left->second.held) || (right != keyStates_.end() && right->second.held);
}

} // namespace engine
