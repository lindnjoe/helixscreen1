// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __HELIX_CONFIG_H__
#define __HELIX_CONFIG_H__

#include "spdlog/spdlog.h"

#include <string>

#include "hv/json.hpp"

using json = nlohmann::json;

class Config {
  private:
    static Config* instance;
    std::string path;

  protected:
    json data;
    std::string default_printer;

    // Allow test fixture to access protected members
    friend class ConfigTestFixture;

  public:
    Config();
    Config(Config& o) = delete;
    void operator=(const Config&) = delete;

    // Initialize config from file path
    void init(const std::string& config_path);

    // Template get/set with JSON pointer syntax
    template <typename T> T get(const std::string& json_ptr) {
        return data[json::json_pointer(json_ptr)].template get<T>();
    };

    template <typename T> T get(const std::string& json_ptr, const T& default_value) {
        json::json_pointer ptr(json_ptr);
        if (data.contains(ptr)) {
            return data[ptr].template get<T>();
        }
        return default_value;
    };

    template <typename T> T set(const std::string& json_ptr, T v) {
        return data[json::json_pointer(json_ptr)] = v;
    };

    // Get JSON sub-object
    json& get_json(const std::string& json_path);

    // Save current config to file
    void save();

    // Get default printer path prefix (e.g., "/printers/default_printer/")
    std::string& df();

    // Get config file path
    std::string get_path();

    // Check if first-run wizard is required
    bool is_wizard_required();

    // Singleton accessor
    static Config* get_instance();
};

#endif // __HELIX_CONFIG_H__
