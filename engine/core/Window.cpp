#include "core/Window.h"
#include "core/Logger.h"

#include <stdexcept>

namespace engine {

Window::Window(const std::string& title, int width, int height)
    : width_(width), height_(height) {
    window_ = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (!window_) {
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    }

    // Prefer vsync-locked presentation; falls back to software if no
    // accelerated driver is available (e.g. some CI/headless setups).
    renderer_ = SDL_CreateRenderer(
        window_, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!renderer_) {
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    }

    if (!renderer_) {
        SDL_DestroyWindow(window_);
        throw std::runtime_error(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());
    }

    // Store initial vsync state based on what we got
    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer_, &info);
    vsync_ = (info.flags & SDL_RENDERER_PRESENTVSYNC) != 0;
}

Window::~Window() {
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
}

void Window::clear(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    SDL_SetRenderDrawColor(renderer_, r, g, b, a);
    SDL_RenderClear(renderer_);
}

void Window::present() {
    SDL_RenderPresent(renderer_);
}

void Window::setVSync(bool enabled) {
    if (vsync_ == enabled) {
        return;  // Already in desired state
    }

    // Note: SDL_RENDERER_PRESENTVSYNC is set at renderer creation time.
    // Changing it requires recreating the renderer, which breaks UIRenderer
    // that holds a pointer to it. For now, we just log the request and
    // would need proper renderer recreation + UI rebuild to support this.
    // In a real implementation, this would require:
    // 1. Recreating the renderer
    // 2. Notifying all UIRenderers to update their renderer pointer
    // 3. Rebuilding UI resources (textures, etc.)
    
    vsync_ = enabled;
    LOG_INFO(LogCategory::Core, "VSync change requested (requires restart for effect)");
}

void Window::setFullscreen(bool enabled) {
    if (fullscreen_ == enabled) {
        return;  // Already in desired state
    }

    Uint32 flags = enabled ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
    if (SDL_SetWindowFullscreen(window_, flags) != 0) {
        LOG_ERROR(LogCategory::Core, "Failed to set fullscreen", SDL_GetError());
        return;
    }

    fullscreen_ = enabled;
    LOG_INFO(LogCategory::Core, "Fullscreen", enabled ? "enabled" : "disabled");
}

} // namespace engine
