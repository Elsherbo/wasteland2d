#pragma once

#include <SDL_ttf.h>
#include <stdexcept>
#include <string>

namespace engine::render {

// RAII wrapper around a single loaded TTF_Font at a fixed point size —
// a different size needs a separate Font instance (that's how SDL_ttf
// itself works; there's no cheap runtime resize of an open font).
// Requires TTF_Init() to have already been called (see Application.cpp)
// before constructing one.
class Font {
public:
    Font(const std::string& path, int pointSize) {
        font_ = TTF_OpenFont(path.c_str(), pointSize);
        if (!font_) {
            throw std::runtime_error("Failed to load font '" + path + "': " + TTF_GetError());
        }
    }

    ~Font() {
        if (font_) TTF_CloseFont(font_);
    }

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    TTF_Font* handle() const { return font_; }

    // Pixel size text would render at, without actually rendering it —
    // for layout (e.g. right-aligning a quantity number, or centering
    // a label). Throws on failure rather than returning a zeroed size
    // a caller might silently lay out around.
    void measure(const std::string& text, int& outWidth, int& outHeight) const {
        if (text.empty()) {
            outWidth = 0;
            outHeight = 0;
            return;
        }
        if (TTF_SizeUTF8(font_, text.c_str(), &outWidth, &outHeight) != 0) {
            throw std::runtime_error(std::string("TTF_SizeUTF8 failed: ") + TTF_GetError());
        }
    }

private:
    TTF_Font* font_ = nullptr;
};

} // namespace engine::render
