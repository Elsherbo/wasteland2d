#pragma once

#include "UIComponent.h"
#include <string>

namespace engine::ui {

// Image mode
enum class ImageMode {
    Fit,       // Scale to fit, preserve aspect
    Fill,      // Scale to fill, preserve aspect, crop
    Stretch,   // Scale to fill, ignore aspect
    Tile       // Repeat to fill
};

// UIImage - texture/icon display
class UIImage : public UIComponent {
public:
    UIImage();
    ~UIImage() override = default;
    
    // Texture
    const std::string& getTexturePath() const { return texturePath_; }
    void setTexturePath(const std::string& path) { texturePath_ = path; setDirty(); }
    
    // Image mode
    ImageMode getMode() const { return mode_; }
    void setMode(ImageMode mode) { mode_ = mode; setDirty(); }
    
    // Preserve aspect ratio
    bool preserveAspect() const { return preserveAspect_; }
    void setPreserveAspect(bool preserve) { preserveAspect_ = preserve; setDirty(); }
    
    // Tint color
    const glm::vec4& getTintColor() const { return tintColor_; }
    void setTintColor(const glm::vec4& color) { tintColor_ = color; setDirty(); }
    
    // Lifecycle
    void render(Renderer& renderer) override;
    void renderUI(UIRenderer& uiRenderer) override;
    glm::vec2 calculateMinSize() const override;

private:
    std::string texturePath_;
    ImageMode mode_ = ImageMode::Fit;
    bool preserveAspect_ = true;
    glm::vec4 tintColor_ = glm::vec4(1.0f);  // White = no tint
};

} // namespace engine::ui
