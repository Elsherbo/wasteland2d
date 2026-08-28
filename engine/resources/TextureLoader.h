#pragma once

#include "resources/ResourceManager.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <memory>

namespace engine::resources {

// Texture resource
class Texture : public Resource {
public:
    Texture(SDL_Texture* texture, int width, int height)
        : texture_(texture), width_(width), height_(height) {}
    
    ~Texture() override {
        if (texture_) {
            SDL_DestroyTexture(texture_);
        }
    }
    
    SDL_Texture* get() const { return texture_; }
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    
    const char* getType() const override { return "Texture"; }

private:
    SDL_Texture* texture_;
    int width_;
    int height_;
};

// Texture loader
class TextureLoader : public ResourceLoader<Texture> {
public:
    TextureLoader(SDL_Renderer* renderer) : renderer_(renderer) {}
    
    std::shared_ptr<Texture> load(const std::string& path) override {
        SDL_Surface* surface = IMG_Load(path.c_str());
        if (!surface) {
            throw std::runtime_error("Failed to load image: " + path);
        }
        
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
        SDL_FreeSurface(surface);
        
        if (!texture) {
            throw std::runtime_error("Failed to create texture: " + path);
        }
        
        int width, height;
        SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
        
        return std::make_shared<Texture>(texture, width, height);
    }
    
    bool canLoad(const std::string& path) const override {
        // Simple check based on file extension
        size_t pos = path.find_last_of('.');
        if (pos == std::string::npos) return false;
        
        std::string ext = path.substr(pos);
        return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp";
    }

private:
    SDL_Renderer* renderer_;
};

} // namespace engine::resources
