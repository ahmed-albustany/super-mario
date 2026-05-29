#pragma once

#include <functional>
#include <vector>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <algorithm>

/// @brief Subscriber ID returned by subscribe(), used to unsubscribe later.
using SubscriberID = uint32_t;

/// @brief Type-erased base for event channels (used internally by EventBus).
class IEventChannel {
public:
    virtual ~IEventChannel() = default;
};

/// @brief A single-event-type publish/subscribe channel.
///        Pre-reserves subscriber storage to avoid allocations during publish.
template<typename EventType>
class EventChannel : public IEventChannel {
public:
    using HandlerFn = std::function<void(const EventType&)>;

    EventChannel() {
        m_subscribers.reserve(16); // pre-reserve to avoid publish-time allocations
    }

    SubscriberID subscribe(HandlerFn handler) {
        std::lock_guard<std::mutex> lock(m_mutex);
        SubscriberID id = m_nextId++;
        m_subscribers.push_back({id, std::move(handler)});
        return id;
    }

    void unsubscribe(SubscriberID id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_subscribers.erase(
            std::remove_if(m_subscribers.begin(), m_subscribers.end(),
                [id](const Subscriber& s) { return s.id == id; }),
            m_subscribers.end()
        );
    }

    void publish(const EventType& event) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& sub : m_subscribers) {
            sub.handler(event);
        }
    }

private:
    struct Subscriber {
        SubscriberID id;
        HandlerFn handler;
    };

    std::vector<Subscriber> m_subscribers;
    SubscriberID m_nextId = 1;
    std::mutex m_mutex;
};

/// @brief Central event bus — supports any event type via templates.
///        Game owns one global EventBus instance; systems publish/subscribe through it.
class EventBus {
public:
    EventBus() = default;

    // Non-copyable, non-movable (owns mutex)
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    /// @brief Subscribe a handler for events of type T.
    template<typename T>
    SubscriberID subscribe(std::function<void(const T&)> handler) {
        return channel<T>().subscribe(std::move(handler));
    }

    /// @brief Unsubscribe by ID. Must know the event type T.
    template<typename T>
    void unsubscribe(SubscriberID id) {
        channel<T>().unsubscribe(id);
    }

    /// @brief Publish an event to all subscribers of type T.
    template<typename T>
    void publish(const T& event) {
        EventChannel<T>* ch = nullptr;
        {
            auto key = std::type_index(typeid(T));
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_channels.find(key);
            if (it != m_channels.end()) {
                ch = static_cast<EventChannel<T>*>(it->second.get());
            }
        }
        // Publish outside bus mutex — channel has its own mutex
        if (ch) {
            ch->publish(event);
        }
    }

private:
    template<typename T>
    EventChannel<T>& channel() {
        auto key = std::type_index(typeid(T));
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_channels.find(key);
        if (it == m_channels.end()) {
            auto ch = std::make_unique<EventChannel<T>>();
            auto& ref = *ch;
            m_channels[key] = std::move(ch);
            return ref;
        }
        return *static_cast<EventChannel<T>*>(it->second.get());
    }

    std::unordered_map<std::type_index, std::unique_ptr<IEventChannel>> m_channels;
    mutable std::mutex m_mutex;
};
