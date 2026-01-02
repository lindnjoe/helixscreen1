# APPROACHES.md - Active Work Tracking

> Track ongoing work with tried approaches and next steps.
> When completed, review for lessons to extract.

## Active Approaches


### [A001] Modal Callback Collision Fix
- **Status**: completed | **Phase**: review | **Agent**: user
- **Created**: 2025-12-30 | **Updated**: 2025-12-30
- **Files**: ui_xml/modal_dialog.xml, src/ui/ui_panel_settings.cpp, ui_xml/theme_restart_modal.xml, ui_xml/factory_reset_modal.xml
- **Description**: Fix modal callback collisions by creating dedicated XML components with unique callback names

**Tried**:
1. [success] Created theme_restart_modal.xml, factory_reset_modal.xml, and updated SettingsPanel
2. [success] All modal callbacks renamed to unique names

**Next**: 

---

### [A002] Move hardware row under SYSTEM, remove section hea...
- **Status**: in_progress | **Phase**: implementing | **Agent**: user
- **Created**: 2026-01-01 | **Updated**: 2026-01-02
- **Files**: 
- **Description**: 
- **Checkpoint**: Looking at the conversation excerpt above, here's a VERY BRIEF summary:

**Tried**:
1. [success] Move hardware row under SYSTEM, remove section header
2. [success] Add reactive label binding for issue count
3. [success] Restyle hardware_issue_row to match notification history
4. [success] Add action buttons to hardware_issue_row.xml
5. [success] Implement button handlers with confirmation for Save
6. [success] Build and test
7. [success] Add is_destroyed() guard to PIDCalibrationPanel destructor
8. [success] Add is_destroyed() guard to ZOffsetCalibrationPanel destructor
9. [success] Add is_destroyed() guard to HistoryDashboardPanel destructor
10. [success] Add is_destroyed() guard to HistoryListPanel destructor
11. [success] Add is_destroyed() guard to InputShaperPanel destructor
12. [success] Add is_destroyed() guard to PowerPanel destructor
13. [success] Add is_destroyed() guard to PrintSelectPanel destructor
14. [success] Add is_destroyed() guard to ScrewsTiltPanel destructor
15. [success] Build and verify compilation
16. [success] Test shutdown and verify no crash
17. [success] Add toggle_filament_runout() declaration to moonraker_client_mock.h
18. [success] Add filament_runout_state_ member to mock header
19. [success] Implement toggle_filament_runout() in moonraker_client_mock.cpp
20. [success] Add F key handler to application.cpp
21. [success] Plan plugin developer documentation structure
22. [success] Write comprehensive plugin developer guide
23. [success] Review and refine documentation
24. [success] Update message text to friendlier wording
25. [success] Add outlines to Load/Unload/Purge buttons
26. [success] Simplify Close button (remove icon and styling)
27. [success] Test the modal visually
28. [success] Refactor dismiss buttons to match standard dialog pattern
29. [success] Phase 3: Plan LED Effects plugin structure
30. [success] Phase 3: Create plugin template structure
31. [success] Phase 3: Implement LED Effects plugin
32. [success] Phase 3: Test full plugin lifecycle
33. [success] Phase 3: Review, test, and commit
34. [success] Add should_show_runout_modal() to runtime_config.h
35. [success] Implement should_show_runout_modal() in runtime_config.cpp
36. [success] Add auto-pause on runout to moonraker_client_mock.cpp
37. [success] Add AMS guard to check_and_show_runout_guidance()
38. [success] Revert imperative button transformation code
39. [success] Add separate Reprint button in XML with visibility binding
40. [success] Add reprint click handler registration
41. [success] Add PrintOutcome enum to printer_state.h
42. [success] Add print_outcome_ subject and init/register it
43. [success] Set print_outcome when print ends (COMPLETE/CANCELLED/ERROR)
44. [success] Clear print_outcome when new print starts
45. [success] Update XML bindings to use print_outcome
46. [success] Remove preserve_state hack from printer_state.cpp
47. [success] Remove Ignoring transition hack from ui_panel_print_status.cpp
48. [success] Phase 4: Implement version compatibility checking
49. [success] Phase 4: Add error reporting UI in Settings → Plugins
50. [success] Phase 4: Create IDEAS.md with deferred items
51. [success] Phase 4: Review, test, and commit
52. [success] Part 1: Fix click handler registration
53. [success] Part 2: Add Spoolman discovery to mock client
54. [success] Violation #4: Add mock capabilities (speaker, firmware_retraction)
55. [success] Violation #4: Remove test_mode overrides from PrinterState
56. [success] Violation #3: Remove test_mode check from SoundManager
57. [success] Violation #2: Move USB demo drives into mock backend
58. [success] Violation #1: Add base class method for filament runout toggle
59. [success] Build and test all violations fixed
60. [success] Run comprehensive code review on plugin system
61. [success] Fix high priority issues (3 found)
62. [success] Fix constructor duplication with delegating constructor
63. [success] Build and verify
64. [success] Step 4: Remove FanPanel legacy code
65. [success] Step 1: Add per-fan subject infrastructure to PrinterState
66. [success] Step 2: Update FanControlOverlay for reactive per-fan updates
67. [success] Step 3: Update ControlsPanel secondary fan rows
68. [success] Step 5: Handle main fan subject consistency
69. [success] Fix HIGH: Use unique_ptr for lv_subject_t in map (prevent rehash issues)
70. [success] Fix MEDIUM: Add reserve() before map insertions
71. [success] Fix MEDIUM: Clear fan_dials_ and auto_fan_cards_ in cleanup()
72. [success] Fix MEDIUM: Add documentation comments for cleanup order
73. [success] Rebuild and test
74. [success] Fix medium priority issues (6 found)
75. [success] Final report and commit
76. [success] [A002] Add per-fan reactive subjects for real-time speed updates
77. [success] [A002] Code review and fix high/medium priority issues
78. [success] [A002] Final commit
79. [success] Document architectural debt in docs/ for future work
80. [success] Fix layer violations in ams_state.cpp (dynamic_cast to backends)
81. [success] Fix layer violations in gcode_streaming_controller.cpp
82. [success] Create panel singleton macro to reduce boilerplate
83. [success] Create SubjectManagedPanel base class
84. [success] Code review all changes
85. [success] Phase 1: Create worktree and branch
86. [success] Phase 2: Implement Moonraker API (get/set machine limits)
87. [success] Phase 2: Code review - API implementation
88. [success] Phase 3: Create machine_limits_overlay.xml
89. [success] Phase 3: Code review - XML layout
90. [success] Phase 4: C++ integration in SettingsPanel
91. [success] Phase 4: Code review - C++ integration
92. [success] Phase 5: Testing and polish
93. [success] Phase 5: Final code review (tip to stern)
94. [success] Commit changes
95. [success] Part 1: Root cause found - globals.xml has wrong codepoints
96. [success] Part 2: Create scripts/gen_icon_consts.py
97. [success] Part 2: Add marker comments to globals.xml
98. [success] Part 2: Run generator to populate icons
99. [success] Part 2: Add Makefile target
100. [success] Rebuild and test tuning panel icons
101. [success] Update print_tune_panel.xml with dual-icon visibility binding
102. [success] Test with mock printer
103. [success] Add init_early() declaration to logging_init.h
104. [success] Add init_early() implementation to logging_init.cpp
105. [success] Call init_early() at start of Application::run()
106. [success] Build and test --help
107. [success] Update printer_detector.cpp paths
108. [success] Update printer_detector.h comments
109. [success] Update mk/cross.mk preset path
110. [success] git mv config/printers to config/presets
111. [success] Update config/presets/README.md
112. [success] Create config/printer_database.d/README.md
113. [success] Update CLAUDE.md file tree
114. [success] Create docs/GALLERY.md with all 8 screenshots
115. [success] Create docs/FEATURES.md with detailed features + full comparison
116. [success] Trim README.md to ~175 lines
117. [success] Phase 1: Delete orphaned icon migration scripts and test XML
118. [success] Phase 2: Delete test/debug scripts
119. [success] Phase 3: Remove dead function from InputShaperPanel
120. [success] Phase 4: Verify and delete unused headers
121. [success] Phase 5: Build and test
122. [success] Write new ROADMAP.md following plan template
123. [success] Validate line count (~235 lines)
124. [success] Verify all numbers are current (tests, panels, printers)
125. [success] Rewrite config/helix_macros.cfg with merged content
126. [success] Refactor macro_manager.cpp to read from file
127. [success] Update macro_manager.h header
128. [success] Update print_start_collector.cpp signal detection
129. [success] Update moonraker-plugin/helix_print.py
130. [success] Delete config/helix_phase_tracking.cfg
131. [success] Update docs/PRINT_START_INTEGRATION.md
132. [success] Update tests

**Next**: Final report and commit

---
