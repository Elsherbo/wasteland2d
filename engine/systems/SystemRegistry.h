#pragma once

#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <functional>

namespace engine {

// Base class for all systems
class SystemBase {
public:
    virtual ~SystemBase() = default;
    virtual void update(double dt) {}
    virtual void render() {}
    virtual void initialize() {}
    virtual void shutdown() {}
};

// Type-safe system registry for managing game systems
class SystemRegistry {
public:
    SystemRegistry() = default;
    ~SystemRegistry() = default;
    
    // Delete copy operations
    SystemRegistry(const SystemRegistry&) = delete;
    SystemRegistry& operator=(const SystemRegistry&) = delete;
    
    // Register a system
    template<typename SystemType, typename... Args>
    void registerSystem(Args&&... args) {
        static_assert(std::is_base_of<SystemBase, SystemType>::value,
                      "SystemType must derive from SystemBase");
        
        auto system = std::make_unique<SystemType>(std::forward<Args>(args)...);
        std::type_index typeIndex(typeid(SystemType));
        
        systems_[typeIndex] = std::move(system);
        systemOrder_.push_back(typeIndex);
    }
    
    // Get a system
    template<typename SystemType>
    SystemType* getSystem() {
        std::type_index typeIndex(typeid(SystemType));
        auto it = systems_.find(typeIndex);
        
        if (it == systems_.end()) {
            return nullptr;
        }
        
        return static_cast<SystemType*>(it->second.get());
    }
    
    // Get a system (const version)
    template<typename SystemType>
    const SystemType* getSystem() const {
        std::type_index typeIndex(typeid(SystemType));
        auto it = systems_.find(typeIndex);
        
        if (it == systems_.end()) {
            return nullptr;
        }
        
        return static_cast<const SystemType*>(it->second.get());
    }
    
    // Check if a system is registered
    template<typename SystemType>
    bool hasSystem() const {
        std::type_index typeIndex(typeid(SystemType));
        return systems_.find(typeIndex) != systems_.end();
    }
    
    // Initialize all systems
    void initializeAll() {
        for (const auto& typeIndex : systemOrder_) {
            auto it = systems_.find(typeIndex);
            if (it != systems_.end()) {
                it->second->initialize();
            }
        }
    }
    
    // Update all systems
    void updateAll(double dt) {
        for (const auto& typeIndex : systemOrder_) {
            auto it = systems_.find(typeIndex);
            if (it != systems_.end()) {
                it->second->update(dt);
            }
        }
    }
    
    // Render all systems
    void renderAll() {
        for (const auto& typeIndex : systemOrder_) {
            auto it = systems_.find(typeIndex);
            if (it != systems_.end()) {
                it->second->render();
            }
        }
    }
    
    // Shutdown all systems
    void shutdownAll() {
        for (const auto& typeIndex : systemOrder_) {
            auto it = systems_.find(typeIndex);
            if (it != systems_.end()) {
                it->second->shutdown();
            }
        }
    }
    
    // Clear all systems
    void clear() {
        shutdownAll();
        systems_.clear();
        systemOrder_.clear();
    }
    
    // Get system count
    size_t systemCount() const {
        return systems_.size();
    }

private:
    std::unordered_map<std::type_index, std::unique_ptr<SystemBase>> systems_;
    std::vector<std::type_index> systemOrder_; // Maintain registration order
};

} // namespace engine
