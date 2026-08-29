#pragma once

#include <glm/vec2.hpp>
#include <variant>

namespace engine::ui {

// Input event types (following Godot's InputEvent pattern)
struct InputEvent {
    enum class Type {
        None,
        MouseMotion,
        MouseButton,
        Key,
        FocusLost,
        FocusGained
    };
    
    Type type = Type::None;
    
    // Mouse data
    struct MouseData {
        glm::vec2 position;
        glm::vec2 relative;  // Relative motion
        int button = 0;       // 0=left, 1=middle, 2=right
        bool pressed = false;
        bool doubleClick = false;
    };
    
    // Keyboard data
    struct KeyData {
        int scancode = 0;
        int keycode = 0;
        bool pressed = false;
        bool echo = false;    // Key repeat
    };
    
    std::variant<std::monostate, MouseData, KeyData> data;
    
    // Convenience constructors
    static InputEvent mouseMotion(float x, float y, float relX, float relY) {
        InputEvent event;
        event.type = Type::MouseMotion;
        event.data = MouseData{glm::vec2(x, y), glm::vec2(relX, relY), 0, false, false};
        return event;
    }
    
    static InputEvent mouseButton(float x, float y, int button, bool pressed) {
        InputEvent event;
        event.type = Type::MouseButton;
        event.data = MouseData{glm::vec2(x, y), glm::vec2(0), button, pressed, false};
        return event;
    }
    
    static InputEvent key(int keycode, int scancode, bool pressed, bool echo = false) {
        InputEvent event;
        event.type = Type::Key;
        event.data = KeyData{scancode, keycode, pressed, echo};
        return event;
    }
    
    static InputEvent focusGained() {
        InputEvent event;
        event.type = Type::FocusGained;
        return event;
    }
    
    static InputEvent focusLost() {
        InputEvent event;
        event.type = Type::FocusLost;
        return event;
    }
    
    // Accessors
    bool isMouseMotion() const { return type == Type::MouseMotion; }
    bool isMouseButton() const { return type == Type::MouseButton; }
    bool isKey() const { return type == Type::Key; }
    
    const MouseData* getMouseData() const {
        return std::get_if<MouseData>(&data);
    }
    
    const KeyData* getKeyData() const {
        return std::get_if<KeyData>(&data);
    }
};

} // namespace engine::ui
