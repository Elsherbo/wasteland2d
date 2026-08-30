#include "UILabel.h"
#include "UIRenderer.h"

namespace engine::ui {

UILabel::UILabel() {
    setInteractable(false);  // Labels are not interactive by default
}

void UILabel::renderUI(UIRenderer& uiRenderer) {
    uiRenderer.renderLabelHelper(this);
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

glm::vec2 UILabel::calculateMinSize() const {
    // Calculate minimum size based on text
    // For now, estimate based on text length (10px per character)
    float estimatedWidth = static_cast<float>(text_.length() * 10);
    float estimatedHeight = 20.0f;  // Default line height
    
    // Respect custom minimum size if set
    glm::vec2 calculatedMin(estimatedWidth, estimatedHeight);
    if (customMinimumSize_.x > 0.0f) calculatedMin.x = customMinimumSize_.x;
    if (customMinimumSize_.y > 0.0f) calculatedMin.y = customMinimumSize_.y;
    
    // Ensure minimum size
    if (calculatedMin.x < 50.0f) calculatedMin.x = 50.0f;
    if (calculatedMin.y < 20.0f) calculatedMin.y = 20.0f;
    
    return calculatedMin;
}

} // namespace engine::ui
