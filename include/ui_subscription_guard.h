// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "moonraker_client.h"

#include <functional>
#include <utility>

class MoonrakerAPI;

/**
 * @brief RAII wrapper for Moonraker subscriptions - auto-unsubscribes on destruction
 *
 * Similar to ObserverGuard but for notification subscriptions.
 * Ensures subscriptions are properly cleaned up when the owning object is destroyed.
 *
 * Supports construction from either MoonrakerClient or MoonrakerAPI:
 * @code
 *   // Via MoonrakerClient (legacy)
 *   subscription_ = SubscriptionGuard(client, client->register_notify_update(...));
 *   // Via MoonrakerAPI (preferred)
 *   subscription_ = SubscriptionGuard(api, api->subscribe_notifications(...));
 * @endcode
 */
class SubscriptionGuard {
  public:
    SubscriptionGuard() = default;

    /**
     * @brief Construct guard from client and subscription ID
     *
     * @param client Moonraker client that owns the subscription
     * @param id Subscription ID from register_notify_update()
     */
    SubscriptionGuard(MoonrakerClient* client, SubscriptionId id)
        : subscription_id_(id),
          unsubscribe_fn_(
              client ? [client](SubscriptionId sid) { client->unsubscribe_notify_update(sid); }
                     : std::function<void(SubscriptionId)>{}) {}

    /**
     * @brief Construct guard from MoonrakerAPI and subscription ID
     *
     * @param api MoonrakerAPI that owns the subscription
     * @param id Subscription ID from subscribe_notifications()
     */
    SubscriptionGuard(MoonrakerAPI* api, SubscriptionId id);

    ~SubscriptionGuard() {
        reset();
    }

    SubscriptionGuard(SubscriptionGuard&& other) noexcept
        : subscription_id_(std::exchange(other.subscription_id_, INVALID_SUBSCRIPTION_ID)),
          unsubscribe_fn_(std::exchange(other.unsubscribe_fn_, {})) {}

    SubscriptionGuard& operator=(SubscriptionGuard&& other) noexcept {
        if (this != &other) {
            reset();
            subscription_id_ = std::exchange(other.subscription_id_, INVALID_SUBSCRIPTION_ID);
            unsubscribe_fn_ = std::exchange(other.unsubscribe_fn_, {});
        }
        return *this;
    }

    SubscriptionGuard(const SubscriptionGuard&) = delete;
    SubscriptionGuard& operator=(const SubscriptionGuard&) = delete;

    /**
     * @brief Unsubscribe and release the subscription
     */
    void reset() {
        if (unsubscribe_fn_ && subscription_id_ != INVALID_SUBSCRIPTION_ID) {
            unsubscribe_fn_(subscription_id_);
            subscription_id_ = INVALID_SUBSCRIPTION_ID;
        }
        unsubscribe_fn_ = {};
    }

    /**
     * @brief Release ownership without unsubscribing
     *
     * Use during shutdown when the client may already be destroyed.
     * The subscription will not be removed (it may already be gone).
     */
    void release() {
        unsubscribe_fn_ = {};
        subscription_id_ = INVALID_SUBSCRIPTION_ID;
    }

    /**
     * @brief Check if guard holds a valid subscription
     */
    explicit operator bool() const {
        return unsubscribe_fn_ && subscription_id_ != INVALID_SUBSCRIPTION_ID;
    }

    /**
     * @brief Get the raw subscription ID
     */
    SubscriptionId get() const {
        return subscription_id_;
    }

  private:
    SubscriptionId subscription_id_ = INVALID_SUBSCRIPTION_ID;
    std::function<void(SubscriptionId)> unsubscribe_fn_;
};
