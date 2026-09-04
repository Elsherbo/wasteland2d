#include "UIDefaultTheme.h"
#include "UIRenderer.h"

namespace engine::ui {

void installDefaultDarkTheme(UIThemeManager& themeManager, UIRenderer& renderer) {
    // A flat, near-black palette with one accent color. Surfaces get
    // *lighter* the closer they are to the user (window < panel < section
    // < interactive element) instead of every layer sharing one flat
    // mid-gray -- that value ramp is what reads as "layered" instead of
    // "stacked identical boxes".
    const glm::vec4 kTextPrimary(0.95f, 0.95f, 0.94f, 1.0f);     // off-white body text
    const glm::vec4 kTextSecondary(0.62f, 0.64f, 0.72f, 1.0f);   // muted heading tone (cool gray-blue)
    // A saturated "call to action" blue against near-black reads as harsh,
    // not elegant -- desaturated + slightly darkened so it registers as an
    // accent rather than shouting over the grays around it.
    const glm::vec4 kAccent(0.40f, 0.50f, 0.74f, 1.0f);          // muted slate-blue accent
    
    // Button style (default / secondary action)
    UIStyle buttonStyle;
    buttonStyle.backgroundColor[UIState::Normal] = glm::vec4(0.16f, 0.16f, 0.19f, 1.0f);
    buttonStyle.backgroundColor[UIState::Hover] = glm::vec4(0.21f, 0.21f, 0.25f, 1.0f);
    buttonStyle.backgroundColor[UIState::Active] = glm::vec4(0.12f, 0.12f, 0.14f, 1.0f);
    buttonStyle.textColor[UIState::Normal] = kTextPrimary;
    buttonStyle.borderColor[UIState::Normal] = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    themeManager.registerStyle("Default", "Button", buttonStyle);
    
    // Primary button - the one emphasized action on a screen.
    UIStyle primaryButtonStyle;
    primaryButtonStyle.backgroundColor[UIState::Normal] = kAccent;
    primaryButtonStyle.backgroundColor[UIState::Hover] = glm::vec4(0.48f, 0.58f, 0.80f, 1.0f);
    primaryButtonStyle.backgroundColor[UIState::Active] = glm::vec4(0.32f, 0.41f, 0.63f, 1.0f);
    primaryButtonStyle.textColor[UIState::Normal] = glm::vec4(0.04f, 0.04f, 0.05f, 1.0f);
    themeManager.registerStyle("Default", "Button.Primary", primaryButtonStyle);
    
    // Ghost button - border-only, for secondary actions that shouldn't
    // compete visually with the primary action.
    UIStyle ghostButtonStyle;
    ghostButtonStyle.backgroundColor[UIState::Normal] = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    ghostButtonStyle.backgroundColor[UIState::Hover] = glm::vec4(1.0f, 1.0f, 1.0f, 0.06f);
    ghostButtonStyle.backgroundColor[UIState::Active] = glm::vec4(1.0f, 1.0f, 1.0f, 0.02f);
    ghostButtonStyle.textColor[UIState::Normal] = kTextPrimary;
    ghostButtonStyle.borderColor[UIState::Normal] = glm::vec4(0.34f, 0.34f, 0.4f, 1.0f);
    themeManager.registerStyle("Default", "Button.Ghost", ghostButtonStyle);
    
    // Label style (body text)
    UIStyle labelStyle;
    labelStyle.textColor[UIState::Normal] = kTextPrimary;
    themeManager.registerStyle("Default", "Label", labelStyle);
    
    // Heading label style (section titles, page title)
    UIStyle headingLabelStyle;
    headingLabelStyle.textColor[UIState::Normal] = kTextSecondary;
    themeManager.registerStyle("Default", "Label.Heading", headingLabelStyle);
    
    // Slider style - the accent color drives the filled track + thumb
    UIStyle sliderStyle;
    sliderStyle.backgroundColor[UIState::Normal] = kAccent;
    sliderStyle.backgroundColor[UIState::Hover] = kAccent;
    sliderStyle.backgroundColor[UIState::Active] = kAccent;
    themeManager.registerStyle("Default", "Slider", sliderStyle);
    
    // Checkbox style
    UIStyle checkboxStyle;
    checkboxStyle.backgroundColor[UIState::Active] = kAccent;  // checked
    checkboxStyle.textColor[UIState::Normal] = kTextPrimary;
    themeManager.registerStyle("Default", "Checkbox", checkboxStyle);
    
    // Container style (generic fallback / used for genuinely bordered
    // "regions" like a nested scrollable list)
    UIStyle containerStyle;
    containerStyle.backgroundColor[UIState::Normal] = glm::vec4(0.10f, 0.10f, 0.12f, 0.9f);
    containerStyle.borderColor[UIState::Normal] = glm::vec4(0.24f, 0.24f, 0.28f, 1.0f);
    themeManager.registerStyle("Default", "Container", containerStyle);
    
    // Panel style - the single outer frame of a screen. This is the only
    // container that should look like a bordered "box"; everything nested
    // inside it should use Section/Transparent so the UI doesn't turn into
    // boxes-within-boxes.
    UIStyle panelStyle;
    panelStyle.backgroundColor[UIState::Normal] = glm::vec4(0.055f, 0.055f, 0.065f, 0.97f);
    panelStyle.borderColor[UIState::Normal] = glm::vec4(0.20f, 0.20f, 0.24f, 1.0f);
    themeManager.registerStyle("Default", "Panel", panelStyle);
    
    // Section style - a soft "card" for grouping related content, one step
    // lighter than the panel behind it, with a faint hairline border
    // (not fully invisible -- gives a card definition without a heavy box).
    UIStyle sectionStyle;
    sectionStyle.backgroundColor[UIState::Normal] = glm::vec4(0.11f, 0.11f, 0.135f, 0.7f);
    sectionStyle.borderColor[UIState::Normal] = glm::vec4(1.0f, 1.0f, 1.0f, 0.045f);
    themeManager.registerStyle("Default", "Section", sectionStyle);
    
    // Transparent style - purely structural wrappers (layout-only
    // containers that shouldn't be visible at all).
    UIStyle transparentStyle;
    transparentStyle.backgroundColor[UIState::Normal] = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    transparentStyle.borderColor[UIState::Normal] = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    themeManager.registerStyle("Default", "Transparent", transparentStyle);
    
    renderer.setAccentColor(render::Color{
        static_cast<uint8_t>(kAccent.r * 255), static_cast<uint8_t>(kAccent.g * 255),
        static_cast<uint8_t>(kAccent.b * 255), 255});
    renderer.setCornerRadius(6.0f);
}

} // namespace engine::ui
