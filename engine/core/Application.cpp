#include "core/Application.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdexcept>

namespace engine {

Application::Application(ApplicationConfig config)
    : config_(std::move(config)), clock_(config_.fixedUpdateHz) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }

    // SDL2_image can lazily auto-init a format's support on first use
    // (which is why texture loading has worked without this call so
    // far — TextureCache::load()/TileMap's tileset loading), but that's
    // undocumented, best-effort behavior, not a guarantee — an explicit
    // IMG_Init() is the correct, portable way to ensure PNG support is
    // actually loaded before anything tries to use it. Found while
    // adding the equivalent (mandatory, not just best-practice) call
    // for SDL_ttf below and fixed alongside it, same file, same reason.
    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0) {
        SDL_Quit();
        throw std::runtime_error(std::string("IMG_Init failed: ") + IMG_GetError());
    }

    // Unlike IMG_Init, this one is not optional — TTF_OpenFont() (see
    // engine::render::Font) fails outright if TTF_Init() was never
    // called; SDL_ttf has no lazy-init fallback the way SDL2_image
    // does.
    if (TTF_Init() != 0) {
        IMG_Quit();
        SDL_Quit();
        throw std::runtime_error(std::string("TTF_Init failed: ") + TTF_GetError());
    }

    window_ = std::make_unique<Window>(config_.title, config_.width, config_.height);
}

Application::~Application() {
    window_.reset();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

void Application::run() {
    while (!quitting_) {
        clock_.beginFrame(SDL_GetTicks64() / 1000.0);

        input_.beginFrame();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            input_.handleEvent(event);
        }
        if (input_.quitRequested() || input_.wasPressed(Action::Pause)) {
            // Pause-as-quit is a Milestone-1 placeholder; a real pause
            // menu will intercept this once the UI system exists.
        }
        if (input_.quitRequested()) {
            quitting_ = true;
            break;
        }

        // Fixed-timestep simulation: may run 0, 1, or several times
        // this frame depending on how long the frame took.
        while (clock_.shouldStep()) {
            if (onUpdate_) onUpdate_(clock_.fixedDeltaTime());
        }

        if (onRender_) onRender_(*window_, clock_.alpha());
        window_->present();
    }
}

} // namespace engine
