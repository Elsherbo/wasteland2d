#pragma once

#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <type_traits>

namespace engine {

// Base class for all events
class Event {
public:
    virtual ~Event() = default;
};

// Type-safe event bus for decoupled system communication
class EventBus {
public:
    EventBus() = default;
    ~EventBus() = default;
    
    // Delete copy operations
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
    
    // Subscribe to an event type
    // Returns a handle that can be used to unsubscribe
    template<typename EventType>
    size_t subscribe(std::function<void(const EventType&)> callback) {
        static_assert(std::is_base_of<Event, EventType>::value, 
                      "EventType must derive from Event");
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto& callbacks = getCallbacks<EventType>();
        size_t id = nextId_++;
        callbacks.emplace_back(id, std::move(callback));
        
        return id;
    }
    
    // Unsubscribe from an event type
    template<typename EventType>
    void unsubscribe(size_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto& callbacks = getCallbacks<EventType>();
        auto it = std::remove_if(callbacks.begin(), callbacks.end(),
            [id](const auto& pair) { return pair.first == id; });
        callbacks.erase(it, callbacks.end());
    }
    
    // Publish an event immediately (synchronous)
    template<typename EventType>
    void publish(const EventType& event) {
        static_assert(std::is_base_of<Event, EventType>::value, 
                      "EventType must derive from Event");
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto& callbacks = getCallbacks<EventType>();
        for (const auto& [id, callback] : callbacks) {
            callback(event);
        }
    }
    
    // Clear all subscriptions
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        subscribers_.clear();
        nextId_ = 0;
    }
    
    // Get subscriber count for an event type
    template<typename EventType>
    size_t subscriberCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::type_index typeIndex(typeid(EventType));
        auto it = subscribers_.find(typeIndex);
        if (it == subscribers_.end()) return 0;
        
        return static_cast<CallbackList<EventType>*>(it->second.get())->size();
    }

private:
    // Base class for storing callbacks
    struct ICallbackList {
        virtual ~ICallbackList() = default;
        virtual size_t size() const = 0;
    };
    
    // Typed callback list
    template<typename EventType>
    struct CallbackList : ICallbackList {
        std::vector<std::pair<size_t, std::function<void(const EventType&)>>> callbacks;
        
        size_t size() const override { return callbacks.size(); }
    };
    
    // Get callbacks for a specific event type
    template<typename EventType>
    std::vector<std::pair<size_t, std::function<void(const EventType&)>>>& getCallbacks() {
        std::type_index typeIndex(typeid(EventType));
        
        if (subscribers_.find(typeIndex) == subscribers_.end()) {
            subscribers_[typeIndex] = std::make_unique<CallbackList<EventType>>();
        }
        
        return static_cast<CallbackList<EventType>*>(subscribers_[typeIndex].get())->callbacks;
    }
    
    // Get callbacks for a specific event type (const version)
    template<typename EventType>
    const std::vector<std::pair<size_t, std::function<void(const EventType&)>>>& getCallbacks() const {
        std::type_index typeIndex(typeid(EventType));
        
        auto it = subscribers_.find(typeIndex);
        if (it == subscribers_.end()) {
            static const std::vector<std::pair<size_t, std::function<void(const EventType&)>>> empty;
            return empty;
        }
        
        return static_cast<const CallbackList<EventType>*>(it->second.get())->callbacks;
    }
    
    mutable std::mutex mutex_;
    std::unordered_map<std::type_index, std::unique_ptr<ICallbackList>> subscribers_;
    size_t nextId_ = 0;
};

// Convenience macros for subscribing
#define SUBSCRIBE_EVENT(eventBus, EventType, callback) \
    eventBus->subscribe<EventType>(callback)

// Convenience macro for publishing
#define PUBLISH_EVENT(eventBus, EventType, ...) \
    eventBus->publish<EventType>(EventType(__VA_ARGS__))

} // namespace engine
