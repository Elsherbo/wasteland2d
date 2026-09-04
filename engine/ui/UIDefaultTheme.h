#pragma once

#include "UIStyle.h"
#include <glm/vec4.hpp>

namespace engine::ui {

class UIRenderer;

// The flat, near-black "professional" theme used by the settings demo,
// extracted out of tests/ui_professional_demo.cpp so any scene can get the
// same look with one call instead of re-registering ~10 UIStyle entries by
// hand. Registers styles under theme name "Default":
//   Panel, Section, Transparent, Container   -- surfaces
//   Button, Button.Primary, Button.Ghost     -- buttons (see below)
//   Label, Label.Heading                     -- text
//   Slider, Checkbox                         -- controls
// and sets the renderer's accent color + corner radius to match.
//
// Widgets still opt in to the non-default variants themselves (a UIButton
// defaults to styleName "Button" -- call setStyleName("Button.Primary") on
// the one emphasized action, "Button.Ghost" on secondary ones; a UILabel
// defaults to plain body text -- call setHeading(true) for a section
// title). That per-widget choice is inherent to how the framework works
// (the same way you'd pick a theme type variation in Godot) and isn't
// something a shared theme function can decide on a scene's behalf -- this
// function only guarantees the *palette* those choices draw from is
// consistent and doesn't have to be redefined per scene.
void installDefaultDarkTheme(UIThemeManager& themeManager, UIRenderer& renderer);

} // namespace engine::ui
