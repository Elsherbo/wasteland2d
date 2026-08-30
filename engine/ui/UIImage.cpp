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

} // namespace engine::ui
