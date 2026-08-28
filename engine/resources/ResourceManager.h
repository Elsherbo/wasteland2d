#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>

namespace engine {

// Base class for all resources
class Resource {
public:
    virtual ~Resource() = default;
};

// Simplified resource manager with type-safe storage
class ResourceManager {
public:
    ResourceManager() = default;
    ~ResourceManager() = default;
    
    // Delete copy operations
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    
    // Store a resource
    template<typename ResourceType>
    void store(const std::string& key, std::shared_ptr<ResourceType> resource) {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_[key] = resource;
    }
    
    // Get a resource
    template<typename ResourceType>
    std::shared_ptr<ResourceType> get(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cache_.find(key);
        if (it == cache_.end()) {
            return nullptr;
        }
        
        return std::static_pointer_cast<ResourceType>(it->second);
    }
    
    // Check if a resource exists
    bool has(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_.find(key) != cache_.end();
    }
    
    // Remove a resource
    void remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.erase(key);
    }
    
    // Clear all resources
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
    }
    
    // Get cache size
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_.size();
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<Resource>> cache_;
};

} // namespace engine
