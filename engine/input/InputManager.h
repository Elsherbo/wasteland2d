#pragma once

#include <SDL.h>
#include <string>
#include <unordered_map>

namespace engine {

// Gameplay code asks "is Action::MoveUp held?" — never "is SDL_SCANCODE_W
// held?". That indirection is what makes rebindable keys possible later
// and keeps game logic from depending on SDL at all.
enum class Action {
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    Interact,
    Inventory,
    Fire,
    Reload,
    Sprint,
    Pause,
    RotateItem, // inventory UI: rotate the currently-dragged item (see game::ui::DragDropController)
    Slot1,      // number-key weapon slots: equip (inventory open + hovering an item) or switch (closed) — see main.cpp
    Slot2,
    Slot3,
    Slot4, // number-key hotbar/quick slots (consumables): assign (Ctrl+key, inventory open + hovering) or use (closed)
    Slot5,
    Slot6,
    Slot7,
    Slot8,
    Slot9,
    EquipBackpack, // inventory open + hovering a backpack-capable item: equip it — no numbered hotkey (see main.cpp)
    // Extend per-game as needed.
};

struct KeyState {
    bool held = false;
    bool pressedThisFrame = false;
    bool releasedThisFrame = false;
};

class InputManager {
public:
    InputManager();

    // Call once per frame, before polling SDL events, to clear the
    // per-frame edge flags (pressed/released) from the previous frame.
    void beginFrame();

    // Feed raw SDL events in (call from the app's event pump).
    void handleEvent(const SDL_Event& event);

    bool isHeld(Action action) const;
    bool wasPressed(Action action) const;   // true only on the frame it went down
    bool wasReleased(Action action) const;  // true only on the frame it went up

    // Rebinding support: point an Action at a different scancode at runtime.
    void bind(Action action, SDL_Scancode scancode);

    // Normalized mouse position in window pixels, and button state.
    // Mouse buttons use the same held/edge-detection model as
    // keyboard Actions (see KeyState) — isMouseButtonHeld existed from
    // Milestone 1; wasMouseButtonPressed/Released are new (Milestone 6,
    // needed for drag-and-drop: picking up an item on mouse-down and
    // dropping it on mouse-up both need an edge, not continuous held
    // state, or a single click-and-hold would try to begin a drag
    // every single frame it stays held).
    void getMousePosition(int& x, int& y) const { x = mouseX_; y = mouseY_; }
    bool isMouseButtonHeld(Uint8 button) const;
    bool wasMouseButtonPressed(Uint8 button) const;
    bool wasMouseButtonReleased(Uint8 button) const;

    // Ctrl (either side) held — for modifier-combined interactions like
    // the inventory UI's quick-grab/quick-store (Ctrl+click). Tracked
    // via the same per-scancode KeyState mechanism as bound Actions
    // (SDL_KEYDOWN/UP already populates keyStates_ for *any* scancode,
    // not just ones passed to bind()), so this needs no separate
    // wiring — just a named query. Deliberately not a generic
    // "isScancodeHeld(SDL_Scancode)" — that would leak raw SDL types
    // into gameplay code's decisions the way Action is specifically
    // meant to prevent; modifiers are common enough across games to
    // earn one small, specific exception rather than a general escape
    // hatch.
    bool isCtrlHeld() const;

    // Net scroll delta accumulated since the last beginFrame() —
    // positive for scroll-up/away-from-user, negative for scroll-down,
    // 0 most frames. Reset every beginFrame() the same way pressed/
    // releasedThisFrame are, since a wheel tick has no "held" state of
    // its own to track — it's inherently a per-frame event, not
    // something to query continuously.
    int mouseWheelDelta() const { return mouseWheelDelta_; }

    // True if the OS asked us to quit (window close, Cmd/Alt+F4, etc.)
    bool quitRequested() const { return quitRequested_; }

private:
    std::unordered_map<Action, SDL_Scancode> bindings_;
    std::unordered_map<SDL_Scancode, KeyState> keyStates_;
    std::unordered_map<Uint8, KeyState> mouseButtons_;
    int mouseX_ = 0;
    int mouseY_ = 0;
    int mouseWheelDelta_ = 0;
    bool quitRequested_ = false;

    KeyState& stateFor(SDL_Scancode scancode);
    const KeyState* stateForAction(Action action) const;
};

} // namespace engine
