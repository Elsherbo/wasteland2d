#pragma once

#include <unordered_map>
#include <typeindex>
#include <stdexcept>
#include <mutex>

namespace engine {

// Service Locator pattern for global service access
// Provides dependency injection and testability
class ServiceLocator {
public:
    // Provide a service implementation
    template<typename ServiceType>
    static void provide(ServiceType* service) {
        std::lock_guard<std::mutex> lock(mutex_);
        services_[std::type_index(typeid(ServiceType))] = service;
    }
    
    // Get a service (throws if not provided)
    template<typename ServiceType>
    static ServiceType* get() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = services_.find(std::type_index(typeid(ServiceType)));
        if (it == services_.end()) {
            throw std::runtime_error("Service not provided");
        }
        
        return static_cast<ServiceType*>(it->second);
    }
    
    // Get a service (returns nullptr if not provided)
    template<typename ServiceType>
    static ServiceType* getOrNull() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = services_.find(std::type_index(typeid(ServiceType)));
        if (it == services_.end()) {
            return nullptr;
        }
        
        return static_cast<ServiceType*>(it->second);
    }
    
    // Check if a service is provided
    template<typename ServiceType>
    static bool isProvided() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        return services_.find(std::type_index(typeid(ServiceType))) != services_.end();
    }
    
    // Clear all services (useful for testing)
    static void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        services_.clear();
    }
    
    // Reset (alias for clear, more semantic)
    static void reset() {
        clear();
    }

private:
    static std::mutex mutex_;
    static std::unordered_map<std::type_index, void*> services_;
};

// Convenience macro for providing services
#define PROVIDE_SERVICE(ServiceType, service) \
    engine::ServiceLocator::provide<ServiceType>(service)

// Convenience macro for getting services
#define GET_SERVICE(ServiceType) \
    engine::ServiceLocator::get<ServiceType>()

// Convenience macro for getting optional services
#define GET_SERVICE_OR_NULL(ServiceType) \
    engine::ServiceLocator::getOrNull<ServiceType>()

// Convenience macro for checking service availability
#define SERVICE_IS_PROVIDED(ServiceType) \
    engine::ServiceLocator::isProvided<ServiceType>()

} // namespace engine
