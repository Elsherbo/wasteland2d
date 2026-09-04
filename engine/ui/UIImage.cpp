#include "UIImage.h"
#include "UIRenderer.h"

namespace engine::ui {

UIImage::UIImage() {
    setInteractable(false);  // Images are not interactive by default
}

void UIImage::renderUI(UIRenderer& uiRenderer) {
    uiRenderer.renderImageHelper(this);
}

void UIImage::render(Renderer& renderer) {
    // TODO: Implement rendering in Phase 6 with actual Renderer
    // For now, this is a placeholder
    (void)renderer;
    
    // Pseudo-code for rendering:
    // 1. Load texture from texturePath (cached in ResourceManager)
    // 2. Calculate destination rect based on mode (Fit, Fill, Stretch, Tile)
    // 3. Apply preserve aspect if enabled
    // 4. Apply tint color
    // 5. Render texture
}

glm::vec2 UIImage::calculateMinSize() const {
    // UIComponent's default falls back to minSize_, which is never actually
    // set anywhere in this codebase (dead field) -- every UIImage would
    // therefore report (0,0) regardless of its real size, invisible to any
    // parent VBox/HBox doing bottom-up width/height accounting (exactly
    // what let a 760px-wide separator image contribute nothing to its
    // panel's auto-computed width). An image's natural footprint is
    // whatever size it was actually given.
    if (customMinimumSize_.x > 0.0f || customMinimumSize_.y > 0.0f) {
        return customMinimumSize_;
    }
    return size_;
}

} // namespace engine::ui
