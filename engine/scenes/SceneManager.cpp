#include "SceneManager.h"
#include "core/Logger.h"

namespace engine {

void SceneManager::loadScene(std::unique_ptr<Scene> scene) {
    if (sceneStack_.empty()) {
        // First scene - load immediately
        if (scene) {
            scene->onEnter();
            scene->setActive(true);
            sceneStack_.push(std::move(scene));
        }
    } else {
        // Queue for next frame to allow current scene to clean up
        pendingScene_ = std::move(scene);
        pendingAction_ = PendingAction::Load;
    }
}

void SceneManager::pushScene(std::unique_ptr<Scene> scene) {
    if (!scene) return;
    
    // Pause current scene
    if (!sceneStack_.empty()) {
        sceneStack_.top()->setActive(false);
    }
    
    // Load new scene
    scene->onEnter();
    scene->setActive(true);
    sceneStack_.push(std::move(scene));
}

void SceneManager::popScene() {
    if (sceneStack_.empty()) return;
    
    // Exit current scene
    sceneStack_.top()->onExit();
    sceneStack_.pop();
    
    // Resume previous scene
    if (!sceneStack_.empty()) {
        sceneStack_.top()->setActive(true);
    }
}

Scene* SceneManager::currentScene() {
    if (sceneStack_.empty()) return nullptr;
    return sceneStack_.top().get();
}

const Scene* SceneManager::currentScene() const {
    if (sceneStack_.empty()) return nullptr;
    return sceneStack_.top().get();
}

void SceneManager::update(double dt) {
    // Process pending scene changes
    if (pendingAction_ != PendingAction::None) {
        switch (pendingAction_) {
            case PendingAction::Load:
                if (!sceneStack_.empty()) {
                    sceneStack_.top()->onExit();
                    sceneStack_.pop();
                }
                if (pendingScene_) {
                    pendingScene_->onEnter();
                    pendingScene_->setActive(true);
                    sceneStack_.push(std::move(pendingScene_));
                }
                break;
            case PendingAction::Push:
                // Push is handled immediately
                break;
            case PendingAction::Pop:
                // Pop is handled immediately
                break;
            default:
                break;
        }
        pendingAction_ = PendingAction::None;
    }
    
    // Update current scene
    if (Scene* scene = currentScene()) {
        if (scene->isActive()) {
            scene->update(dt);
        }
    }
}

void SceneManager::render() {
    if (Scene* scene = currentScene()) {
        scene->render();
    }
}

size_t SceneManager::sceneCount() const {
    return sceneStack_.size();
}

void SceneManager::clear() {
    while (!sceneStack_.empty()) {
        sceneStack_.top()->onExit();
        sceneStack_.pop();
    }
    pendingScene_.reset();
    pendingAction_ = PendingAction::None;
}

} // namespace engine
