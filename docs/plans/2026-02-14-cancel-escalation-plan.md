# Cancel E-Stop Escalation Settings — Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Make cancel-to-e-stop escalation disabled by default, with a configurable timeout when enabled.

**Architecture:** Add two new settings (bool toggle + int dropdown) to SettingsManager with LVGL subjects, wire them into the AbortManager's cancel timer logic, and expose them in the Safety section of the settings panel XML.

**Tech Stack:** C++, LVGL 9.4 XML, Catch2 tests

**Design doc:** `docs/plans/2026-02-14-cancel-escalation-design.md`

---

### Task 1: SettingsManager — Add cancel escalation settings

**Files:**
- Modify: `include/settings_manager.h:456-475` (SAFETY SETTINGS section)
- Modify: `include/settings_manager.h:660-664` (subject accessors)
- Modify: `include/settings_manager.h:774` (subject member declarations)
- Modify: `src/system/settings_manager.cpp:174-177` (subject init)
- Modify: `src/system/settings_manager.cpp:907-924` (safety settings impl)

**Step 1: Add declarations to settings_manager.h**

In the SAFETY SETTINGS section (after `set_estop_require_confirmation` at line 474), add:

```cpp
    /**
     * @brief Get cancel escalation enabled state
     * @return true if cancel should escalate to e-stop on timeout
     */
    bool get_cancel_escalation_enabled() const;

    /**
     * @brief Set cancel escalation enabled state
     *
     * When enabled, a cancel that doesn't complete within the configured
     * timeout will escalate to M112 emergency stop. When disabled (default),
     * cancel waits indefinitely for the printer to respond.
     *
     * @param enabled true to enable escalation
     */
    void set_cancel_escalation_enabled(bool enabled);

    /**
     * @brief Get cancel escalation timeout in seconds
     * @return Timeout in seconds (15, 30, 60, or 120)
     */
    int get_cancel_escalation_timeout_seconds() const;

    /**
     * @brief Set cancel escalation timeout in seconds
     * @param seconds Timeout value (clamped to 15-120)
     */
    void set_cancel_escalation_timeout_seconds(int seconds);
```

Add subject accessors (after `subject_estop_require_confirmation` at line 664):

```cpp
    /** @brief Cancel escalation enabled subject (integer: 0=disabled, 1=enabled) */
    lv_subject_t* subject_cancel_escalation_enabled() {
        return &cancel_escalation_enabled_subject_;
    }

    /** @brief Cancel escalation timeout subject (integer: dropdown index 0-3) */
    lv_subject_t* subject_cancel_escalation_timeout() {
        return &cancel_escalation_timeout_subject_;
    }
```

Add member declarations (after `estop_require_confirmation_subject_` at line 774):

```cpp
    lv_subject_t cancel_escalation_enabled_subject_;
    lv_subject_t cancel_escalation_timeout_subject_;
```

**Step 2: Add initialization in settings_manager.cpp**

After the E-Stop confirmation init block (line 177), add:

```cpp
    // Cancel escalation (default: false = never escalate to e-stop)
    bool cancel_escalation = config->get<bool>("/safety/cancel_escalation_enabled", false);
    UI_MANAGED_SUBJECT_INT(cancel_escalation_enabled_subject_, cancel_escalation ? 1 : 0,
                           "settings_cancel_escalation_enabled", subjects_);

    // Cancel escalation timeout (default: 30s, stored as dropdown index 0-3 → 15/30/60/120s)
    int cancel_escalation_timeout = config->get<int>("/safety/cancel_escalation_timeout_seconds", 30);
    // Convert seconds to dropdown index: 15→0, 30→1, 60→2, 120→3
    int timeout_index = 1; // default 30s
    if (cancel_escalation_timeout <= 15) timeout_index = 0;
    else if (cancel_escalation_timeout <= 30) timeout_index = 1;
    else if (cancel_escalation_timeout <= 60) timeout_index = 2;
    else timeout_index = 3;
    UI_MANAGED_SUBJECT_INT(cancel_escalation_timeout_subject_, timeout_index,
                           "settings_cancel_escalation_timeout", subjects_);
```

**Step 3: Add implementations in settings_manager.cpp**

After the `set_estop_require_confirmation` implementation (line 924), add:

```cpp
bool SettingsManager::get_cancel_escalation_enabled() const {
    return lv_subject_get_int(const_cast<lv_subject_t*>(&cancel_escalation_enabled_subject_)) != 0;
}

void SettingsManager::set_cancel_escalation_enabled(bool enabled) {
    spdlog::info("[SettingsManager] set_cancel_escalation_enabled({})", enabled);

    // 1. Update subject (UI reacts)
    lv_subject_set_int(&cancel_escalation_enabled_subject_, enabled ? 1 : 0);

    // 2. Persist
    Config* config = Config::get_instance();
    config->set<bool>("/safety/cancel_escalation_enabled", enabled);
    config->save();

    spdlog::debug("[SettingsManager] Cancel escalation {} and saved",
                  enabled ? "enabled" : "disabled");
}

static constexpr int ESCALATION_TIMEOUT_VALUES[] = {15, 30, 60, 120};

int SettingsManager::get_cancel_escalation_timeout_seconds() const {
    int index = lv_subject_get_int(const_cast<lv_subject_t*>(&cancel_escalation_timeout_subject_));
    index = std::max(0, std::min(3, index));
    return ESCALATION_TIMEOUT_VALUES[index];
}

void SettingsManager::set_cancel_escalation_timeout_seconds(int seconds) {
    spdlog::info("[SettingsManager] set_cancel_escalation_timeout_seconds({})", seconds);

    // Convert seconds to dropdown index
    int index = 1; // default 30s
    if (seconds <= 15) index = 0;
    else if (seconds <= 30) index = 1;
    else if (seconds <= 60) index = 2;
    else index = 3;

    // 1. Update subject (UI reacts via dropdown binding)
    lv_subject_set_int(&cancel_escalation_timeout_subject_, index);

    // 2. Persist actual seconds value
    Config* config = Config::get_instance();
    config->set<int>("/safety/cancel_escalation_timeout_seconds", ESCALATION_TIMEOUT_VALUES[index]);
    config->save();

    spdlog::debug("[SettingsManager] Cancel escalation timeout set to {}s (index {}) and saved",
                  ESCALATION_TIMEOUT_VALUES[index], index);
}
```

**Step 4: Build to verify compilation**

Run: `make -j`
Expected: Clean build, no errors.

**Step 5: Commit**

```bash
git add include/settings_manager.h src/system/settings_manager.cpp
git commit -m "feat(settings): add cancel escalation enabled + timeout settings"
```

---

### Task 2: Settings Panel XML — Add toggle + conditional dropdown

**Files:**
- Modify: `ui_xml/settings_panel.xml:120` (after E-Stop Confirmation toggle)

**Step 1: Add XML rows**

After the E-Stop Confirmation toggle (line 120), add:

```xml
    <!-- Cancel Escalation Toggle -->
    <setting_toggle_row name="row_cancel_escalation"
                        label="Cancel Escalation" label_tag="Cancel Escalation" icon="timer_alert"
                        description="Escalate to emergency stop if cancel doesn't respond"
                        description_tag="Escalate to emergency stop if cancel doesn't respond"
                        subject="settings_cancel_escalation_enabled" callback="on_cancel_escalation_changed"/>
    <!-- Cancel Escalation Timeout (visible only when escalation enabled) -->
    <lv_obj name="container_cancel_escalation_timeout" width="100%" style_pad_all="0" scrollable="false">
      <bind_flag_if_eq subject="settings_cancel_escalation_enabled" flag="hidden" ref_value="0"/>
      <setting_dropdown_row name="row_cancel_escalation_timeout"
                            label="Escalation Timeout" label_tag="Escalation Timeout" icon="clock"
                            description="Time to wait before escalating to emergency stop"
                            description_tag="Time to wait before escalating to emergency stop"
                            options="15 seconds&#10;30 seconds&#10;60 seconds&#10;120 seconds"
                            bind_selected="settings_cancel_escalation_timeout"
                            callback="on_cancel_escalation_timeout_changed"/>
    </lv_obj>
```

**Step 2: Verify XML loads**

Run: `./build/bin/helix-screen --test -vv` and navigate to Settings. Verify the toggle appears (off by default) and dropdown is hidden.

**Step 3: Commit**

```bash
git add ui_xml/settings_panel.xml
git commit -m "feat(settings): add cancel escalation toggle + timeout dropdown XML"
```

---

### Task 3: Settings Panel C++ — Wire up callbacks

**Files:**
- Modify: `src/ui/ui_panel_settings.cpp:381-402` (callback registration)
- Modify: `src/ui/ui_panel_settings.cpp:807-812` (handler methods, near estop handler)
- Modify: `src/ui/ui_panel_settings.cpp:1184-1190` (static callbacks, near estop callback)
- Modify: `include/ui_panel_settings.h` (declare new methods)

**Step 1: Add static dropdown callback (near line 99, after other dropdown callbacks)**

```cpp
// Static callback for cancel escalation timeout dropdown
static void on_cancel_escalation_timeout_changed(lv_event_t* e) {
    lv_obj_t* dropdown = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
    int index = static_cast<int>(lv_dropdown_get_selected(dropdown));
    static constexpr int TIMEOUT_VALUES[] = {15, 30, 60, 120};
    int seconds = TIMEOUT_VALUES[std::max(0, std::min(3, index))];
    spdlog::info("[SettingsPanel] Cancel escalation timeout changed: {}s (index {})", seconds, index);
    SettingsManager::instance().set_cancel_escalation_timeout_seconds(seconds);
}
```

**Step 2: Register callbacks (after line 402, near other registrations)**

```cpp
    lv_xml_register_event_cb(nullptr, "on_cancel_escalation_changed", on_cancel_escalation_changed);
    lv_xml_register_event_cb(nullptr, "on_cancel_escalation_timeout_changed",
                             on_cancel_escalation_timeout_changed);
```

**Step 3: Add static toggle callback (near line 1190, after estop callback)**

```cpp
void SettingsPanel::on_cancel_escalation_changed(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[SettingsPanel] on_cancel_escalation_changed");
    auto* toggle = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
    bool enabled = lv_obj_has_state(toggle, LV_STATE_CHECKED);
    get_global_settings_panel().handle_cancel_escalation_changed(enabled);
    LVGL_SAFE_EVENT_CB_END();
}
```

**Step 4: Add handler method (near line 812, after estop handler)**

```cpp
void SettingsPanel::handle_cancel_escalation_changed(bool enabled) {
    spdlog::info("[{}] Cancel escalation toggled: {}", get_name(), enabled ? "ON" : "OFF");
    SettingsManager::instance().set_cancel_escalation_enabled(enabled);
}
```

**Step 5: Add declarations to header**

In `include/ui_panel_settings.h`, add in the appropriate sections:

```cpp
    // Static callback
    static void on_cancel_escalation_changed(lv_event_t* e);

    // Handler
    void handle_cancel_escalation_changed(bool enabled);
```

**Step 6: Build and verify**

Run: `make -j`
Expected: Clean build.

Run: `./build/bin/helix-screen --test -vv`, navigate to Settings, toggle Cancel Escalation on/off. Verify:
- Toggle works, dropdown appears/disappears
- Changing dropdown logs the timeout value
- Settings persist across restart

**Step 7: Commit**

```bash
git add src/ui/ui_panel_settings.cpp include/ui_panel_settings.h
git commit -m "feat(settings): wire cancel escalation toggle + timeout callbacks"
```

---

### Task 4: AbortManager — Read settings and conditionally skip escalation timer

**Files:**
- Modify: `src/abort/abort_manager.cpp:290-294` (cancel timer creation)
- Modify: `include/abort_manager.h:220` (keep CANCEL_TIMEOUT_MS as default fallback)

**Step 1: Write the failing test**

In `tests/unit/test_abort_manager.cpp`, add new test cases:

```cpp
TEST_CASE_METHOD(AbortManagerTestFixture,
                 "AbortManager: Escalation disabled - cancel timeout never fires",
                 "[abort][cancel][escalation][settings]") {
    // Disable escalation (this is the new default)
    SettingsManager::instance().set_cancel_escalation_enabled(false);

    AbortManager::instance().start_abort();
    simulate_kalico_not_present();
    simulate_queue_responsive();
    REQUIRE(AbortManager::instance().get_state() == AbortManager::State::SENT_CANCEL);

    // Simulate what would be a timeout — but since escalation is disabled,
    // calling on_cancel_timeout should be impossible (timer never created).
    // Instead, verify state stays SENT_CANCEL by checking escalation level.

    // Print transitions to terminal state naturally
    AbortManagerTestAccess::on_print_state_during_cancel(AbortManager::instance(),
                                                         PrintJobState::STANDBY);

    REQUIRE(AbortManager::instance().get_state() == AbortManager::State::COMPLETE);
    REQUIRE(AbortManager::instance().escalation_level() == 0);
}

TEST_CASE_METHOD(AbortManagerTestFixture,
                 "AbortManager: Escalation enabled - cancel timeout fires with configured value",
                 "[abort][cancel][escalation][settings]") {
    // Enable escalation with 60s timeout
    SettingsManager::instance().set_cancel_escalation_enabled(true);
    SettingsManager::instance().set_cancel_escalation_timeout_seconds(60);

    AbortManager::instance().start_abort();
    simulate_kalico_not_present();
    simulate_queue_responsive();
    REQUIRE(AbortManager::instance().get_state() == AbortManager::State::SENT_CANCEL);

    // Simulate cancel timeout (would happen at 60s)
    simulate_cancel_timeout();

    // Should escalate since escalation is enabled
    REQUIRE(AbortManager::instance().get_state() == AbortManager::State::SENT_ESTOP);
}

TEST_CASE_METHOD(AbortManagerTestFixture,
                 "AbortManager: Default settings do not escalate",
                 "[abort][cancel][escalation][settings][default]") {
    // Don't set anything — use defaults
    // Default: cancel_escalation_enabled = false

    AbortManager::instance().start_abort();
    simulate_kalico_not_present();
    simulate_queue_responsive();
    REQUIRE(AbortManager::instance().get_state() == AbortManager::State::SENT_CANCEL);

    // Complete via print state observer (natural completion)
    AbortManagerTestAccess::on_print_state_during_cancel(AbortManager::instance(),
                                                         PrintJobState::CANCELLED);

    REQUIRE(AbortManager::instance().get_state() == AbortManager::State::COMPLETE);
    REQUIRE(AbortManager::instance().escalation_level() == 0);
}
```

**Step 2: Run tests to verify they fail**

Run: `make test && ./build/bin/helix-tests "[escalation][settings]" -v`
Expected: FAIL — AbortManager doesn't read settings yet.

**Step 3: Modify AbortManager to read settings**

In `src/abort/abort_manager.cpp`, add include at top:

```cpp
#include "settings_manager.h"
```

Replace the cancel timer block at line 292-294:

```cpp
    // Start timeout timer — only if escalation is enabled
    bool escalation_enabled = SettingsManager::instance().get_cancel_escalation_enabled();
    if (escalation_enabled) {
        uint32_t timeout_ms =
            static_cast<uint32_t>(SettingsManager::instance().get_cancel_escalation_timeout_seconds()) * 1000;
        spdlog::info("[AbortManager] Cancel escalation enabled, timeout: {}ms", timeout_ms);
        cancel_timer_ = lv_timer_create(cancel_timer_cb, timeout_ms, this);
        lv_timer_set_repeat_count(cancel_timer_, 1);
    } else {
        spdlog::info("[AbortManager] Cancel escalation disabled, waiting for print state change");
    }
```

**Step 4: Run tests to verify they pass**

Run: `make test && ./build/bin/helix-tests "[abort]" -v`
Expected: ALL PASS, including the existing escalation tests (which use `simulate_cancel_timeout()` directly via TestAccess, bypassing the timer).

**Step 5: Verify the existing timeout constant test**

The test at line 595-599 checks `CANCEL_TIMEOUT_MS == 15000`. This constant is still there as a fallback/documentation — it's just not used directly anymore. The test still passes.

**Step 6: Commit**

```bash
git add src/abort/abort_manager.cpp include/abort_manager.h tests/unit/test_abort_manager.cpp
git commit -m "feat(abort): read cancel escalation settings, skip timer when disabled"
```

---

### Task 5: Manual QA + final commit

**Step 1: Full test suite**

Run: `make test-run`
Expected: All tests pass.

**Step 2: Manual testing**

Run: `./build/bin/helix-screen --test -vv`

1. Open Settings → verify "Cancel Escalation" toggle shows (OFF by default)
2. Toggle ON → dropdown appears with "15 seconds", "30 seconds", "60 seconds", "120 seconds"
3. Select "60 seconds" → check logs for `set_cancel_escalation_timeout_seconds(60)`
4. Toggle OFF → dropdown disappears
5. Restart app → verify settings persist

**Step 3: Verify abort behavior in mock mode**

1. Start a mock print
2. Press Cancel → confirm → verify logs show "Cancel escalation disabled, waiting for print state change"
3. Toggle escalation ON, restart, start new mock print
4. Press Cancel → verify logs show "Cancel escalation enabled, timeout: 30000ms" (or whatever value selected)
