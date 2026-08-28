#include "UILabel.h"

namespace engine::ui {

UILabel::UILabel() {
    setInteractable(false);  // Labels are not interactive by default
}

void UILabel::render(Renderer& renderer) {
    // TODO: Implement rendering in Phase 6 with actual Renderer
    // For now, this is a placeholder
    (void)renderer;
    
    // Pseudo-code for rendering:
    // 1. Get style for current state
    // 2. Get font from style
    // 3. Calculate text position based on alignment
    // 4. Render text
    // 5. Apply word wrap if enabled
}

} // namespace engine::ui
