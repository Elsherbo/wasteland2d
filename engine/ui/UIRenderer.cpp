#include "UIRenderer.h"
#include "UIButton.h"
#include "UILabel.h"
#include "UIImage.h"
#include "UISlider.h"
#include "UICheckbox.h"
#include "UIScrollContainer.h"
#include <glm/vec2.hpp>
#include <cmath>
#include <vector>
#include <algorithm>

namespace engine::ui {

namespace {
constexpr float kPi = 3.14159265358979323846f;
}

UIRenderer::UIRenderer(SDL_Renderer* renderer, render::TextRenderer& textRenderer, const render::Font& font,
                       UIThemeManager& themeManager, const render::Font* headingFont)
    : renderer_(renderer), textRenderer_(textRenderer), font_(font), headingFont_(headingFont), themeManager_(themeManager) {
    // Every color in the theme system carries an alpha channel (translucent
    // panel backgrounds, invisible borders, etc.), but SDL2's default blend
    // mode (SDL_BLENDMODE_NONE) ignores alpha entirely and paints it as
    // opaque. Without this, every "0.5 alpha" panel and every "alpha 0"
    // (invisible) border in the theme silently renders fully opaque instead.
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
}

void UIRenderer::render(UIComponent* component) {
    if (!component || !component->isVisible()) {
        return;
    }
    
    // Call the component's renderUI method (polymorphic rendering)
    component->renderUI(*this);
}

// ---------------------------------------------------------------------
// Rounded-rectangle primitives
// ---------------------------------------------------------------------

void UIRenderer::fillRoundedRect(const SDL_FRect& rect, float radius, render::Color color) {
    if (color.a == 0) {
        return;  // fully transparent -- nothing to draw
    }
    
    float r = std::min(radius, std::min(rect.w, rect.h) * 0.5f);
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    
    if (r <= 0.5f) {
        SDL_RenderFillRectF(renderer_, &rect);
        return;
    }
    
#if SDL_VERSION_ATLEAST(2, 0, 18)
    // Body: a vertical strip plus two horizontal strips covers everything
    // except the four corners.
    SDL_FRect vStrip{rect.x, rect.y + r, rect.w, rect.h - 2.0f * r};
    SDL_FRect hTop{rect.x + r, rect.y, rect.w - 2.0f * r, r};
    SDL_FRect hBottom{rect.x + r, rect.y + rect.h - r, rect.w - 2.0f * r, r};
    SDL_RenderFillRectF(renderer_, &vStrip);
    SDL_RenderFillRectF(renderer_, &hTop);
    SDL_RenderFillRectF(renderer_, &hBottom);
    
    // Corners: each is a triangle-fan quarter circle, expressed as a flat
    // triangle list (center + two perimeter points per triangle) since
    // SDL_RenderGeometry with no index buffer treats vertices as a plain
    // triangle list, not a fan. SDL2's SDL_Vertex::color is SDL_Color
    // (0-255 ints) -- not SDL_FColor, which is an SDL3-only type.
    SDL_Color sc{color.r, color.g, color.b, color.a};
    struct Corner { float cx, cy, startDeg; };
    const Corner corners[4] = {
        {rect.x + r,          rect.y + r,          180.0f},  // top-left
        {rect.x + rect.w - r, rect.y + r,          270.0f},  // top-right
        {rect.x + rect.w - r, rect.y + rect.h - r,   0.0f},  // bottom-right
        {rect.x + r,          rect.y + rect.h - r,  90.0f},  // bottom-left
    };
    constexpr int kSegments = 8;
    std::vector<SDL_Vertex> verts;
    verts.reserve(static_cast<size_t>(kSegments) * 3);
    for (const auto& c : corners) {
        verts.clear();
        for (int i = 0; i < kSegments; ++i) {
            float a0 = (c.startDeg + 90.0f * (static_cast<float>(i) / kSegments)) * (kPi / 180.0f);
            float a1 = (c.startDeg + 90.0f * (static_cast<float>(i + 1) / kSegments)) * (kPi / 180.0f);
            SDL_Vertex center{{c.cx, c.cy}, sc, {0.0f, 0.0f}};
            SDL_Vertex p0{{c.cx + std::cos(a0) * r, c.cy + std::sin(a0) * r}, sc, {0.0f, 0.0f}};
            SDL_Vertex p1{{c.cx + std::cos(a1) * r, c.cy + std::sin(a1) * r}, sc, {0.0f, 0.0f}};
            verts.push_back(center);
            verts.push_back(p0);
            verts.push_back(p1);
        }
        SDL_RenderGeometry(renderer_, nullptr, verts.data(), static_cast<int>(verts.size()), nullptr, 0);
    }
#else
    // Older SDL2 without SDL_RenderGeometry: fall back to a sharp rect
    // rather than hand-rolling per-pixel circle drawing.
    SDL_RenderFillRectF(renderer_, &rect);
#endif
}

void UIRenderer::drawRoundedRectBorder(const SDL_FRect& rect, float radius, render::Color color) {
    if (color.a == 0) {
        return;
    }
    
    float r = std::min(radius, std::min(rect.w, rect.h) * 0.5f);
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    
    if (r <= 0.5f) {
        SDL_RenderDrawRectF(renderer_, &rect);
        return;
    }
    
    // Straight edges, inset by the corner radius at each end.
    SDL_RenderDrawLineF(renderer_, rect.x + r, rect.y, rect.x + rect.w - r, rect.y);
    SDL_RenderDrawLineF(renderer_, rect.x + r, rect.y + rect.h, rect.x + rect.w - r, rect.y + rect.h);
    SDL_RenderDrawLineF(renderer_, rect.x, rect.y + r, rect.x, rect.y + rect.h - r);
    SDL_RenderDrawLineF(renderer_, rect.x + rect.w, rect.y + r, rect.x + rect.w, rect.y + rect.h - r);
    
    // Corner arcs as polylines -- available on every SDL2 version, no
    // version gate needed (unlike the filled-corner geometry above).
    struct Corner { float cx, cy, startDeg; };
    const Corner corners[4] = {
        {rect.x + r,          rect.y + r,          180.0f},
        {rect.x + rect.w - r, rect.y + r,          270.0f},
        {rect.x + rect.w - r, rect.y + rect.h - r,   0.0f},
        {rect.x + r,          rect.y + rect.h - r,  90.0f},
    };
    constexpr int kSegments = 8;
    std::vector<SDL_FPoint> pts;
    pts.reserve(kSegments + 1);
    for (const auto& c : corners) {
        pts.clear();
        for (int i = 0; i <= kSegments; ++i) {
            float a = (c.startDeg + 90.0f * (static_cast<float>(i) / kSegments)) * (kPi / 180.0f);
            pts.push_back(SDL_FPoint{c.cx + std::cos(a) * r, c.cy + std::sin(a) * r});
        }
        SDL_RenderDrawLinesF(renderer_, pts.data(), static_cast<int>(pts.size()));
    }
}

// ---------------------------------------------------------------------
// Thick line primitive (used by the checkbox checkmark below -- SDL2's
// SDL_RenderDrawLineF is always 1px, which can't draw a checkmark that
// actually reads as one at UI scale)
// ---------------------------------------------------------------------

void UIRenderer::fillThickLine(glm::vec2 from, glm::vec2 to, float thickness, render::Color color) {
    glm::vec2 dir = to - from;
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len < 0.0001f) {
        return;
    }
    
#if SDL_VERSION_ATLEAST(2, 0, 18)
    glm::vec2 perp{-dir.y / len, dir.x / len};
    glm::vec2 offset = perp * (thickness * 0.5f);
    
    SDL_Color sc{color.r, color.g, color.b, color.a};
    SDL_Vertex v0{{from.x + offset.x, from.y + offset.y}, sc, {0.0f, 0.0f}};
    SDL_Vertex v1{{from.x - offset.x, from.y - offset.y}, sc, {0.0f, 0.0f}};
    SDL_Vertex v2{{to.x + offset.x, to.y + offset.y}, sc, {0.0f, 0.0f}};
    SDL_Vertex v3{{to.x - offset.x, to.y - offset.y}, sc, {0.0f, 0.0f}};
    SDL_Vertex verts[6] = {v0, v1, v2, v1, v3, v2};
    SDL_RenderGeometry(renderer_, nullptr, verts, 6, nullptr, 0);
#else
    // Older SDL2 without SDL_RenderGeometry: fall back to a 1px line
    // rather than hand-rolling per-pixel quad rasterization.
    (void)thickness;
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLineF(renderer_, from.x, from.y, to.x, to.y);
#endif
}

// ---------------------------------------------------------------------
// Color helpers
// ---------------------------------------------------------------------

render::Color UIRenderer::getStateColor(UIState state) const {
    switch (state) {
        case UIState::Hover:
            return hoverColor_;
        case UIState::Active:
            return activeColor_;
        case UIState::Disabled:
            return disabledColor_;
        case UIState::Focus:
            return activeColor_;
        default:
            return normalColor_;
    }
}

render::Color UIRenderer::getColorFromStyle(const UIStyle* style, UIState state, const render::Color& fallback) const {
    if (!style) {
        return fallback;
    }
    
    glm::vec4 color = style->getBackgroundColor(state, glm::vec4(
        fallback.r / 255.0f,
        fallback.g / 255.0f,
        fallback.b / 255.0f,
        fallback.a / 255.0f
    ));
    
    return render::Color{
        static_cast<uint8_t>(color.r * 255),
        static_cast<uint8_t>(color.g * 255),
        static_cast<uint8_t>(color.b * 255),
        static_cast<uint8_t>(color.a * 255)
    };
}

namespace {
render::Color toColor(const glm::vec4& v) {
    return render::Color{
        static_cast<uint8_t>(std::clamp(v.r, 0.0f, 1.0f) * 255),
        static_cast<uint8_t>(std::clamp(v.g, 0.0f, 1.0f) * 255),
        static_cast<uint8_t>(std::clamp(v.b, 0.0f, 1.0f) * 255),
        static_cast<uint8_t>(std::clamp(v.a, 0.0f, 1.0f) * 255)
    };
}

glm::vec4 fromColor(const render::Color& c) {
    return glm::vec4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
}
}  // namespace

// ---------------------------------------------------------------------
// Button
// ---------------------------------------------------------------------

void UIRenderer::renderButtonHelper(UIButton* button) {
    glm::vec2 pos = button->getWorldPosition();
    glm::vec2 size = button->getWorldSize();
    SDL_FRect rect{pos.x, pos.y, size.x, size.y};
    
    const UIStyle* style = themeManager_.getStyle(button->getStyleName());
    UIState state = button->getState();
    bool isPrimary = (button->getStyleName() == "Button.Primary");
    bool isGhost = (button->getStyleName() == "Button.Ghost");
    
    render::Color bgFallback = isPrimary ? accentColor_
                              : isGhost  ? render::Color{0, 0, 0, 0}
                                         : normalColor_;
    render::Color bgColor = getColorFromStyle(style, state, bgFallback);
    if (button->isToggled()) {
        bgColor = getColorFromStyle(style, UIState::Active, accentColor_);
    }
    
    fillRoundedRect(rect, cornerRadius_, bgColor);
    
    render::Color borderFallback = isGhost
        ? render::Color{static_cast<uint8_t>(accentColor_.r * 0.7f), static_cast<uint8_t>(accentColor_.g * 0.7f), static_cast<uint8_t>(accentColor_.b * 0.7f), 255}
        : render::Color{0, 0, 0, 0};
    render::Color borderColor = style
        ? toColor(style->getBorderColor(state, fromColor(borderFallback)))
        : borderFallback;
    drawRoundedRectBorder(rect, cornerRadius_, borderColor);
    
    if (!button->getText().empty()) {
        render::Color textDefault = isPrimary ? render::Color{10, 10, 12, 255} : textColor_;
        render::Color textCol = style ? toColor(style->getTextColor(state, fromColor(textDefault))) : textDefault;
        
        int textW = 0, textH = 0;
        textRenderer_.measure(font_, button->getText(), textCol, textW, textH);
        
        int textX = static_cast<int>(pos.x + (size.x - textW) / 2);
        int textY = static_cast<int>(pos.y + (size.y - textH) / 2);
        
        textRenderer_.draw(font_, button->getText(), textX, textY, textCol);
    }
}

// ---------------------------------------------------------------------
// Label
// ---------------------------------------------------------------------

void UIRenderer::renderLabelHelper(UILabel* label) {
    glm::vec2 pos = label->getWorldPosition();
    glm::vec2 size = label->getWorldSize();
    
    bool heading = label->isHeading();
    const render::Font& font = (heading && headingFont_) ? *headingFont_ : font_;
    const UIStyle* style = themeManager_.getStyle(heading ? "Label.Heading" : "Label");
    
    if (!label->getText().empty()) {
        render::Color defaultCol = heading ? render::Color{154, 154, 162, 255} : textColor_;
        render::Color textCol = style ? toColor(style->getTextColor(label->getState(), fromColor(defaultCol))) : defaultCol;
        
        int textW = 0, textH = 0;
        textRenderer_.measure(font, label->getText(), textCol, textW, textH);
        
        int textX = static_cast<int>(pos.x);
        // Vertically center the text within the label's own box instead of
        // pinning it to the top -- the slider track next to it is already
        // vertically centered in its row, so a top-anchored label/value
        // reads as visibly misaligned against it, especially once a row's
        // box is taller than a single line of text.
        int textY = static_cast<int>(pos.y + (size.y - textH) / 2.0f);
        
        if (label->getAlignment() == TextAlignment::Center) {
            textX = static_cast<int>(pos.x + (size.x - textW) / 2);
        } else if (label->getAlignment() == TextAlignment::Right) {
            textX = static_cast<int>(pos.x + size.x - textW);
        }
        
        textRenderer_.draw(font, label->getText(), textX, textY, textCol);
    }
}

// ---------------------------------------------------------------------
// Image
// ---------------------------------------------------------------------

void UIRenderer::renderImageHelper(UIImage* image) {
    glm::vec2 pos = image->getWorldPosition();
    glm::vec2 size = image->getWorldSize();
    
    glm::vec4 tint = image->getTintColor();
    render::Color color = toColor(tint);
    
    SDL_FRect rect{pos.x, pos.y, size.x, size.y};
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderFillRectF(renderer_, &rect);
}

// ---------------------------------------------------------------------
// Slider
// ---------------------------------------------------------------------

void UIRenderer::renderSliderHelper(UISlider* slider) {
    glm::vec2 pos = slider->getWorldPosition();
    glm::vec2 size = slider->getWorldSize();
    
    const UIStyle* style = themeManager_.getStyle("Slider");
    UIState state = slider->getState();
    
    // Track: a capsule spanning the full slider width, split into filled
    // (accent, left of the thumb) and unfilled (dark gray, right of it)
    // portions instead of one flat undifferentiated bar.
    float trackHeight = std::max(4.0f, size.y * 0.3f);
    float trackY = pos.y + (size.y - trackHeight) / 2.0f;
    float trackRadius = trackHeight / 2.0f;
    
    // Normalize against the slider's actual range rather than assuming
    // getValue() is already a 0-1 percent -- that only happened to hold
    // because every slider in the current demo keeps the default [0,1]
    // range; a volume slider set up as [0,100] would render its thumb
    // miles off the track. Guarded the same way as UISlider's own math.
    float minValue = slider->getMinValue();
    float maxValue = slider->getMaxValue();
    float valueRange = maxValue - minValue;
    float percent = (valueRange != 0.0f) ? (slider->getValue() - minValue) / valueRange : 0.0f;
    percent = std::clamp(percent, 0.0f, 1.0f);
    
    float thumbSize = slider->getThumbSize();
    float trackSpan = std::max(0.0f, size.x - thumbSize);
    float thumbX = pos.x + percent * trackSpan;
    float thumbCenterX = thumbX + thumbSize / 2.0f;
    
    SDL_FRect trackRect{pos.x, trackY, size.x, trackHeight};
    render::Color trackColor{42, 42, 46, 255};
    fillRoundedRect(trackRect, trackRadius, trackColor);
    
    SDL_FRect filledRect{pos.x, trackY, std::max(trackHeight, thumbCenterX - pos.x), trackHeight};
    render::Color filledColor = getColorFromStyle(style, UIState::Normal, accentColor_);
    fillRoundedRect(filledRect, trackRadius, filledColor);
    
    // Thumb: a compact round knob centered on the control's vertical
    // midline, sized relative to the track rather than stretched to the
    // full component height -- using size.y directly here (as before)
    // produced a tall thin capsule instead of a circular handle whenever
    // the slider's hit-box was taller than the visual track.
    float thumbDiameter = std::clamp(trackHeight * 2.4f, 14.0f, size.y);
    float thumbVisualX = thumbCenterX - thumbDiameter / 2.0f;
    float thumbVisualY = pos.y + (size.y - thumbDiameter) / 2.0f;
    SDL_FRect thumbRect{thumbVisualX, thumbVisualY, thumbDiameter, thumbDiameter};
    render::Color thumbColor = getColorFromStyle(style, state, accentColor_);
    if (state == UIState::Hover || state == UIState::Active) {
        // Lighten slightly on interaction so the thumb visibly responds.
        thumbColor.r = static_cast<uint8_t>(std::min(255, thumbColor.r + 25));
        thumbColor.g = static_cast<uint8_t>(std::min(255, thumbColor.g + 25));
        thumbColor.b = static_cast<uint8_t>(std::min(255, thumbColor.b + 25));
    }
    fillRoundedRect(thumbRect, thumbDiameter / 2.0f, thumbColor);
    
    // A thin dark ring around the knob reads as a proper handle rather
    // than a flat dot sitting on the track.
    drawRoundedRectBorder(thumbRect, thumbDiameter / 2.0f, render::Color{10, 10, 12, 90});
}

// ---------------------------------------------------------------------
// Checkbox
// ---------------------------------------------------------------------

void UIRenderer::renderCheckboxHelper(UICheckbox* checkbox) {
    glm::vec2 pos = checkbox->getWorldPosition();
    
    const UIStyle* style = themeManager_.getStyle("Checkbox");
    UIState state = checkbox->getState();
    
    float boxSize = 20.0f;
    float boxRadius = 5.0f;
    SDL_FRect boxRect{pos.x, pos.y, boxSize, boxSize};
    
    render::Color boxColor;
    if (checkbox->isChecked()) {
        boxColor = getColorFromStyle(style, UIState::Active, accentColor_);
    } else {
        render::Color unchecked{28, 28, 31, 255};
        boxColor = getColorFromStyle(style, UIState::Normal, unchecked);
        if (state == UIState::Hover) {
            boxColor = getColorFromStyle(style, UIState::Hover, render::Color{38, 38, 42, 255});
        }
    }
    fillRoundedRect(boxRect, boxRadius, boxColor);
    
    if (!checkbox->isChecked()) {
        render::Color borderColor{60, 60, 66, 255};
        drawRoundedRectBorder(boxRect, boxRadius, borderColor);
    }
    
    if (checkbox->isChecked()) {
        // Checkmark drawn dark-on-accent for contrast against the fill, as
        // two actual diagonal strokes (short down-stroke, long up-stroke)
        // rather than two disconnected flat bars -- proportioned against
        // boxSize so it stays a real checkmark shape if boxSize changes.
        render::Color checkColor{10, 10, 12, 255};
        float t = std::max(2.0f, boxSize * 0.11f);  // stroke thickness
        glm::vec2 p0{pos.x + boxSize * 0.22f, pos.y + boxSize * 0.52f};
        glm::vec2 p1{pos.x + boxSize * 0.42f, pos.y + boxSize * 0.72f};
        glm::vec2 p2{pos.x + boxSize * 0.80f, pos.y + boxSize * 0.28f};
        fillThickLine(p0, p1, t, checkColor);
        fillThickLine(p1, p2, t, checkColor);
    }
    
    if (!checkbox->getText().empty()) {
        render::Color textDefault = textColor_;
        render::Color textCol = style ? toColor(style->getTextColor(state, fromColor(textDefault))) : textDefault;
        
        int textW = 0, textH = 0;
        textRenderer_.measure(font_, checkbox->getText(), textCol, textW, textH);
        
        int textX = static_cast<int>(pos.x + boxSize + 10);
        int textY = static_cast<int>(pos.y + (boxSize - textH) / 2);
        
        textRenderer_.draw(font_, checkbox->getText(), textX, textY, textCol);
    }
}

// ---------------------------------------------------------------------
// Container
// ---------------------------------------------------------------------

void UIRenderer::renderContainerHelper(UIContainer* container) {
    glm::vec2 pos = container->getWorldPosition();
    glm::vec2 size = container->getWorldSize();
    SDL_FRect rect{pos.x, pos.y, size.x, size.y};
    
    // Get style from theme manager, keyed by this container's own role
    // (e.g. "Panel", "Section", "Container") instead of one style for every
    // nesting level -- this is what lets an outer panel and its inner
    // sections look visually distinct instead of stacked identical boxes.
    const UIStyle* style = themeManager_.getStyle(container->getStyleName());
    UIState state = container->getState();
    
    render::Color bgColor{30, 30, 35, 150};
    if (style) {
        bgColor = toColor(style->getBackgroundColor(state, fromColor(bgColor)));
    }
    fillRoundedRect(rect, cornerRadius_, bgColor);
    
    render::Color borderColor{0, 0, 0, 0};
    if (style) {
        borderColor = toColor(style->getBorderColor(state, fromColor(borderColor)));
    }
    drawRoundedRectBorder(rect, cornerRadius_, borderColor);
    
    // Check if this is a ScrollContainer and set clip rect
    SDL_Rect oldClipRect;
    SDL_RenderGetClipRect(renderer_, &oldClipRect);
    bool hadClipRect = (oldClipRect.w != 0 && oldClipRect.h != 0);
    
    if (auto* scrollContainer = dynamic_cast<UIScrollContainer*>(container)) {
        SDL_Rect ownRect{
            static_cast<int>(pos.x),
            static_cast<int>(pos.y),
            static_cast<int>(size.x),
            static_cast<int>(size.y)
        };
        
        // SDL_RenderSetClipRect *replaces* whatever clip rect is currently
        // active -- it does not intersect with it. So a scroll container
        // nested inside another one (this "Scrollable List Demo" list sits
        // inside the outer "Panel" scroll view) must explicitly intersect
        // its own rect with the ambient clip before setting it, or content
        // can render past the OUTER container's edge whenever this inner
        // rect's position (which moves as the outer scrolls) extends
        // beyond it -- which is exactly what caused list items to spill
        // out past the bottom of the settings panel while scrolling.
        SDL_Rect clipRect = ownRect;
        if (hadClipRect) {
            SDL_Rect intersected;
            if (SDL_IntersectRect(&oldClipRect, &ownRect, &intersected)) {
                clipRect = intersected;
            } else {
                // No overlap with the ambient clip at all -- this container
                // is fully scrolled out of its ancestor's view, so nothing
                // in it should be visible. Clip to zero area rather than
                // falling back to ownRect, which would un-clip it entirely.
                clipRect = SDL_Rect{ownRect.x, ownRect.y, 0, 0};
            }
        }
        SDL_RenderSetClipRect(renderer_, &clipRect);
        
        for (auto* child : container->getChildren()) {
            child->renderUI(*this);
        }
        
        if (hadClipRect) {
            SDL_RenderSetClipRect(renderer_, &oldClipRect);
        } else {
            SDL_RenderSetClipRect(renderer_, nullptr);
        }
        
        renderScrollbarHelper(scrollContainer);
    } else {
        for (auto* child : container->getChildren()) {
            child->renderUI(*this);
        }
    }
}

void UIRenderer::renderScrollbarHelper(UIScrollContainer* container) {
    glm::vec2 pos = container->getWorldPosition();
    glm::vec2 size = container->getWorldSize();
    glm::vec2 contentSize = container->getContentSize();
    
    float scrollbarWidth = 8.0f;
    float scrollbarHeight = size.y;
    float scrollbarX = pos.x + size.x - scrollbarWidth - 3.0f;
    
    if ((contentSize.y + container->getPadding().y * 2.0f) > size.y) {
        glm::vec2 scrollPos = container->getScrollPosition();
        float maxScroll = (contentSize.y + container->getPadding().y * 2.0f) - size.y;
        float scrollRatio = (maxScroll > 0) ? scrollPos.y / maxScroll : 0;
        float thumbRatio = size.y / contentSize.y;
        float thumbHeight = std::max(24.0f, scrollbarHeight * thumbRatio);
        float thumbY = pos.y + (scrollbarHeight - thumbHeight) * scrollRatio;
        
        SDL_FRect trackRect{scrollbarX, pos.y, scrollbarWidth, scrollbarHeight};
        fillRoundedRect(trackRect, scrollbarWidth / 2.0f, render::Color{60, 60, 66, 70});
        
        SDL_FRect thumbRect{scrollbarX, thumbY, scrollbarWidth, thumbHeight};
        fillRoundedRect(thumbRect, scrollbarWidth / 2.0f, render::Color{110, 110, 118, 200});
    }
}

} // namespace engine::ui