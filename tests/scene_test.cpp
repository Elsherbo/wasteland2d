// Test Scene and SceneManager
#include "scenes/Scene.h"
#include "scenes/SceneManager.h"
#include <cassert>
#include <iostream>
#include <memory>

// Mock scene for testing
class TestScene : public engine::Scene {
public:
    TestScene(const char* name) : name_(name), updateCount_(0), renderCount_(0) {}
    
    void onEnter() override {
        entered_ = true;
        std::cout << "Scene '" << name_ << "' entered\n";
    }
    
    void onExit() override {
        exited_ = true;
        std::cout << "Scene '" << name_ << "' exited\n";
    }
    
    void update(double dt) override {
        updateCount_++;
    }
    
    void render() override {
        renderCount_++;
    }
    
    const char* getName() const override { return name_; }
    
    bool entered() const { return entered_; }
    bool exited() const { return exited_; }
    int updateCount() const { return updateCount_; }
    int renderCount() const { return renderCount_; }

private:
    const char* name_;
    bool entered_ = false;
    bool exited_ = false;
    int updateCount_;
    int renderCount_;
};

int main() {
    std::cout << "Testing Scene and SceneManager...\n";
    
    auto sceneManager = std::make_unique<engine::SceneManager>();
    
    // Test 1: Load first scene
    auto scene1 = std::make_unique<TestScene>("Scene1");
    auto* scene1Ptr = scene1.get();
    sceneManager->loadScene(std::move(scene1));
    
    assert(sceneManager->currentScene() == scene1Ptr && "Scene1 should be current");
    assert(scene1Ptr->entered() && "Scene1 should have entered");
    assert(scene1Ptr->isActive() && "Scene1 should be active");
    std::cout << "[ok] Load first scene works\n";
    
    // Test 2: Update and render
    sceneManager->update(0.016);
    assert(scene1Ptr->updateCount() == 1 && "Scene1 should have updated once");
    std::cout << "[ok] Update scene works\n";
    
    // Test 3: Push scene (pause current)
    auto scene2 = std::make_unique<TestScene>("Scene2");
    auto* scene2Ptr = scene2.get();
    sceneManager->pushScene(std::move(scene2));
    
    assert(sceneManager->currentScene() == scene2Ptr && "Scene2 should be current");
    assert(scene2Ptr->entered() && "Scene2 should have entered");
    assert(scene2Ptr->isActive() && "Scene2 should be active");
    assert(!scene1Ptr->isActive() && "Scene1 should be paused");
    std::cout << "[ok] Push scene works\n";
    
    // Test 4: Pop scene (resume previous)
    sceneManager->popScene();
    
    assert(sceneManager->currentScene() == scene1Ptr && "Scene1 should be current again");
    assert(scene1Ptr->isActive() && "Scene1 should be active again");
    assert(scene2Ptr->exited() && "Scene2 should have exited");
    std::cout << "[ok] Pop scene works\n";
    
    // Test 5: Load new scene (replace current)
    auto scene3 = std::make_unique<TestScene>("Scene3");
    auto* scene3Ptr = scene3.get();
    sceneManager->loadScene(std::move(scene3));
    
    // Process pending scene change
    sceneManager->update(0.016);
    
    assert(sceneManager->currentScene() == scene3Ptr && "Scene3 should be current");
    assert(scene1Ptr->exited() && "Scene1 should have exited");
    assert(scene3Ptr->entered() && "Scene3 should have entered");
    std::cout << "[ok] Load new scene works\n";
    
    // Test 6: Scene count
    assert(sceneManager->sceneCount() == 1 && "Should have 1 scene");
    std::cout << "[ok] Scene count works\n";
    
    // Test 7: Clear all scenes
    sceneManager->clear();
    assert(sceneManager->currentScene() == nullptr && "Should have no current scene");
    assert(sceneManager->sceneCount() == 0 && "Should have 0 scenes");
    assert(scene3Ptr->exited() && "Scene3 should have exited");
    std::cout << "[ok] Clear scenes works\n";
    
    std::cout << "\nALL SCENE SYSTEM TESTS PASSED\n";
    return 0;
}
