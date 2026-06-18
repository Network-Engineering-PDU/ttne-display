# Phase 1 LVGL UI Audit

Project: `ttne-display`
Target: Variscite i.MX7 Linux framebuffer/evdev and Ubuntu SDL simulator
Date: 2026-06-18

## Executive Summary

The current application already has a basic Ubuntu SDL build path and a 5 ms main loop sleep, but the UI is not isolated from backend work. Most screen callbacks and LVGL timers still call synchronous curl wrappers with 1000 ms connect timeouts and 3000 ms request timeouts. A single failed backend request can therefore freeze the UI thread for up to 3 seconds. Several screens perform multiple synchronous requests per refresh, so worst-case freezes can be much longer.

The highest-priority fix is not visual redesign. It is to introduce a backend command/state layer, convert every controller operation used by UI callbacks/timers to asynchronous backend jobs, and make the UI thread consume immutable state snapshots or UI-thread-applied updates.

## Findings

### 1. Synchronous HTTP Runs On The UI Thread

Problem detected:
Most `controller_*` functions call `http_helper_get/post/put()` directly. These functions run `curl_easy_perform()` synchronously.

Root cause:
The controller layer is used directly by screens and LVGL timers. Only three GET operations currently have async wrappers.

Evidence:
- `src/http_helper.c:55-64`, `src/http_helper.c:100-112`, `src/http_helper.c:146-156`
- `src/controller.c:93-706`
- UI callers in `src/scr_outlets.c:57`, `src/scr_power.c:126`, `src/scr_sensors.c:102`, `src/scr_settings_upd.c:149`, `src/scr_settings_nw_blue.c:181`, `src/scr_settings_nw_eth.c:156`

Impact level:
Critical. This is the most likely cause of touch freezes and inconsistent responsiveness.

Proposed solution:
Move all network/backend calls into backend worker threads. UI callbacks should enqueue commands and return immediately. Completed backend jobs should update centralized state and notify the UI thread through a bounded event queue.

Performance improvement estimate:
Worst-case touch stall reduced from 1000-3000 ms per failed request to under 5-20 ms per callback.

Refactored code:
Not applied in Phase 1. Planned for Phase 3/5 after simulator structure is stabilized.

### 2. LVGL Timers Perform Blocking Backend Work

Problem detected:
LVGL timers poll backend data synchronously.

Root cause:
Timers are being used as backend polling loops instead of UI refresh loops.

Evidence:
- Power screen polls up to 6 inputs synchronously every 2 seconds: `src/scr_power.c:122-130`
- Outlet data polls synchronously: `src/scr_outlet_data.c:93-97`
- Sensor live data polls synchronously every 1 second, with fallback request: `src/scr_sensors_data.c:151-173`
- BLE discovery timer performs sync HTTP every 2 seconds: `src/scr_sensors.c:221-227`
- OTA status timer performs sync HTTP every 2 seconds: `src/scr_settings_upd.c:147-153`
- Bluetooth timer performs sync HTTP every 4 seconds: `src/scr_settings_nw_blue.c:149-181`
- Network connectivity timer performs sync HTTP: `src/scr_settings_nw_eth.c:252-272`

Impact level:
Critical. Timer callbacks run from `lv_timer_handler()` and block all input, rendering, and animations.

Proposed solution:
Backend timers/threads should poll hardware/API state. LVGL timers should only compare state revisions and update widgets if values changed.

Performance improvement estimate:
Frame scheduling becomes predictable. Periodic multi-second stalls should be eliminated.

Refactored code:
Not applied in Phase 1.

### 3. UI Callbacks Trigger Long Operations

Problem detected:
Touch handlers directly start BLE scans, confirm sensors, toggle outlets, reboot, factory reset, change Bluetooth state, and save OTA settings via synchronous calls.

Root cause:
UI screens know controller/backend details and call them inline.

Evidence:
- Outlet toggle all does one sync PUT per outlet from a LVGL timer: `src/scr_outlets.c:126-145`
- Single outlet toggle sync PUT: `src/scr_outlet_data.c:151-160`
- BLE scan start/stop/confirm sync calls: `src/scr_sensors.c:71-84`, `src/scr_sensors.c:89-112`, `src/scr_sensors.c:162-192`
- Bluetooth actions sync calls: `src/scr_settings_nw_blue.c:72-89`, `src/scr_settings_nw_blue.c:99-146`
- OTA confirmation/reboot/factory reset sync calls: `src/scr_settings_upd.c:296-318`, `src/scr_settings_upd.c:337-359`
- SSH/SNMP/Modbus service screens call sync controller functions: `src/scr_settings_nw_ssh.c`, `src/scr_settings_nw_snmp.c`, `src/scr_settings_nw_modbus.c`

Impact level:
Critical.

Proposed solution:
Replace direct calls with `backend_submit(command, payload, completion_id)`. UI should optimistically update local pending state where safe and show busy/error state from command results.

Performance improvement estimate:
Button press response becomes immediate; backend latency no longer blocks touch.

Refactored code:
Not applied in Phase 1.

### 4. Shared Model State Is Not Thread Safe

Problem detected:
`models.c` stores global mutable state and returns raw pointers to internal memory. Setters free and replace strings/lists without synchronization.

Root cause:
The project was designed around single-threaded synchronous controller updates. The partial async HTTP layer now runs callbacks on the UI thread, which avoids races for those paths, but future proper backend threads would make this unsafe.

Evidence:
- Global mutable objects: `src/models.c:9-27`
- Raw pointer getters: `src/models.c:49-52`, `src/models.c:123-128`, `src/models.c:179-184`, `src/models.c:321-324`, `src/models.c:380-383`
- Free/reallocate setters: `src/models.c:54-75`, `src/models.c:136-145`, `src/models.c:191-213`, `src/models.c:279-314`, `src/models.c:331-357`

Impact level:
High now; critical after backend threading.

Proposed solution:
Create centralized `app_state` with mutex-protected snapshots. UI should never hold raw pointers across backend updates. Preferred pattern: backend thread builds a new state delta; UI thread applies it during the main loop.

Performance improvement estimate:
Prevents use-after-free crashes and enables safe async backend conversion.

Refactored code:
Not applied in Phase 1.

### 5. Async HTTP Callbacks Still Execute In The UI Loop

Problem detected:
`http_async_process_callbacks()` runs user callbacks from the main UI loop. Those callbacks parse JSON and mutate models.

Root cause:
The async worker only performs curl. Completion work is unbounded and occurs inside the UI frame path.

Evidence:
- Main loop callback processing: `src/main.c:163-170`
- Callback execution: `src/http_async.c:224-259`
- JSON parsing in async callbacks: `src/controller.c:759-873`

Impact level:
High. Large responses or multiple completed requests can exceed the 5 ms UI budget.

Proposed solution:
Either parse JSON in backend workers and enqueue compact state deltas, or rate-limit completion processing per frame. UI thread should only apply already-prepared state changes and update visible widgets.

Performance improvement estimate:
Reduces jitter under backend response bursts; likely 10-100 ms frame spikes eliminated depending on payload size.

Refactored code:
Not applied in Phase 1.

### 6. Main Loop Does Extra Work Every 5 ms

Problem detected:
The main loop calls config access and idle handling every 5 ms.

Root cause:
Inactivity timeout is recomputed every frame instead of cached. Idle transition has no visible guard against repeated calls while already idle.

Evidence:
- `src/main.c:163-175`

Impact level:
Medium.

Proposed solution:
Cache inactivity timeout at config load/update. Check inactivity at a lower rate, e.g. 250-1000 ms, using an LVGL timer. Add an `is_idle`/current-screen guard.

Performance improvement estimate:
Small CPU reduction; prevents unnecessary screen work during idle.

Refactored code:
Not applied in Phase 1.

### 7. Refresh Period Is 30 ms Instead Of 16 ms

Problem detected:
LVGL display refresh is configured to 30 ms, not the requested 16 ms.

Root cause:
`LV_DISP_DEF_REFR_PERIOD` is left at 30.

Evidence:
- `include/lv_conf.h:65`
- Touch read is already 10 ms: `include/lv_conf.h:68`

Impact level:
Medium to high for perceived smoothness.

Proposed solution:
Set `LV_DISP_DEF_REFR_PERIOD` to 16. Keep `LV_INDEV_DEF_READ_PERIOD` at 10. Enable performance monitor during optimization builds.

Performance improvement estimate:
Maximum refresh rate improves from about 33 FPS to about 60 FPS, assuming rendering can keep up.

Refactored code:
Not applied in Phase 1.

### 8. Performance And Memory Monitoring Are Disabled

Problem detected:
LVGL performance and memory monitors are disabled.

Root cause:
Default `lv_conf.h` settings.

Evidence:
- `include/lv_conf.h:235`
- `include/lv_conf.h:242`

Impact level:
Medium.

Proposed solution:
Enable monitors for simulator/debug builds. Keep them disabled or configurable in production.

Performance improvement estimate:
No runtime improvement directly; reduces optimization guesswork.

Refactored code:
Not applied in Phase 1.

### 9. Touch Device Configuration Is Hard-Coded And Possibly Miscalibrated

Problem detected:
evdev uses `/dev/input/event1`, swaps axes, and has calibration disabled. Absolute coordinates are clamped directly to 240x320.

Root cause:
No runtime input device detection/calibration path.

Evidence:
- `include/lv_drv_conf.h:450-459`
- `lv_drivers/indev/evdev.c:112-235`
- Display is software-rotated: `src/main.c:109-116`

Impact level:
High for touch accuracy and perceived latency.

Proposed solution:
Add a hardware diagnostic step/script that records `/proc/bus/input/devices`, `evtest`, axis ranges, swap/invert needs, and selected event device. Then make event device and calibration configurable for i.MX7.

Performance improvement estimate:
Latency may not improve directly, but wrong coordinates, dead zones, and repeated touch attempts should disappear.

Refactored code:
Not applied in Phase 1.

### 10. Network Save Uses Fork/Wait In UI Path

Problem detected:
Network settings save forks and then immediately waits for the first child. The worker child runs `controller_put_nw_if()` in a grandchild.

Root cause:
Attempted nonblocking save implemented manually inside screen code.

Evidence:
- `src/scr_settings_nw_eth.c:353-375`
- Save callback invokes it before loading the loader: `src/scr_settings_nw_eth.c:377-390`

Impact level:
High.

Proposed solution:
Remove process management from UI code. Submit a backend command and update state/result when complete.

Performance improvement estimate:
Avoids process creation overhead and any wait-related UI stalls.

Refactored code:
Not applied in Phase 1.

### 11. File Operations Occur Inside UI Callbacks

Problem detected:
The OTA USB update button reads `/proc/mounts` directly from the click callback.

Root cause:
USB detection is mixed into screen logic.

Evidence:
- `src/scr_settings_upd.c:258-279`

Impact level:
Medium.

Proposed solution:
Move USB/mount detection to backend. UI should display backend-provided removable-media state.

Performance improvement estimate:
Avoids small but unpredictable file I/O stalls.

Refactored code:
Not applied in Phase 1.

### 12. Blocking Wait For Background Script

Problem detected:
`runbg_check_wait()` calls blocking `waitpid(pid, ..., 0)` from the update timer path after the add operation completes.

Root cause:
Script orchestration is done in UI timer code.

Evidence:
- `src/runbg.c:55-60`
- Called by `src/scr_settings_upd.c:327-334`

Impact level:
High if the remove script hangs or takes noticeable time.

Proposed solution:
Move script orchestration into backend worker. Add timeout and result reporting.

Performance improvement estimate:
Prevents UI freeze during update cleanup.

Refactored code:
Not applied in Phase 1.

### 13. Memory Leaks In Several Controller PUT/POST Paths

Problem detected:
Some `cJSON_PrintUnformatted()` buffers are not freed, and curl header lists are not freed in synchronous helper paths.

Root cause:
Manual ownership is inconsistent.

Evidence:
- Missing `free(put_data)` in `src/controller.c:162-177`, `src/controller.c:338-375`, `src/controller.c:437-449`, `src/controller.c:461-473`, `src/controller.c:674-687`, `src/controller.c:690-706`
- Synchronous helper never calls `curl_slist_free_all(headers)`: `src/http_helper.c:49-73`, `src/http_helper.c:91-121`, `src/http_helper.c:139-168`

Impact level:
High over long-running production sessions.

Proposed solution:
Fix ownership immediately. Add helper cleanup labels for curl/header/request buffer. Prefer bounded fixed buffers where possible.

Performance improvement estimate:
Stabilizes memory usage; prevents gradual heap fragmentation and eventual crashes/freezes.

Refactored code:
Not applied in Phase 1.

### 14. Allocation Failures Are Not Handled

Problem detected:
Several realloc/malloc paths assume success.

Root cause:
Low-level helper code does not check allocation return values.

Evidence:
- `src/http_helper.c:23-28`
- `src/http_async.c:59-65`
- `src/models.c:36-44`, `src/models.c:136-145`, `src/models.c:191-213`

Impact level:
High on embedded hardware.

Proposed solution:
Check every allocation. Preserve old pointer on realloc failure. Report backend error state instead of crashing.

Performance improvement estimate:
No direct speed gain; much better stability under memory pressure.

Refactored code:
Not applied in Phase 1.

### 15. Possible Array Bounds Bugs

Problem detected:
Model getters index arrays without validating ids, and power label update indexes phase arrays assuming only 1 or 3 phases even though bi-phase exists.

Root cause:
UI trusts backend model values and ids.

Evidence:
- `models_get_out_sw_id(id)` returns `out_sw_list[id-1]` without range checks: `src/models.c:130-133`
- `models_get_sensor_id(id)` returns `sensor_list[id-1]` without range checks: `src/models.c:186-189`
- `scr_power.c` supports `TYPE_BI` by setting `n_phases = 2`, but `set_line_label()` only handles 1 or 3 and update indexing reads `in_data[2 + n_phases*i]`: `src/scr_power.c:103-118`, `src/scr_power.c:145-163`, `src/scr_power.c:196-212`

Impact level:
High. Can cause crashes or incorrect display.

Proposed solution:
Add validated accessor APIs and explicit phase-count rendering for 1, 2, and 3 phases.

Performance improvement estimate:
Stability improvement; no expected speed gain.

Refactored code:
Not applied in Phase 1.

### 16. Duplicate Timers Can Be Created

Problem detected:
Some page-open callbacks create timers without checking if an existing timer is already active.

Root cause:
Screen lifecycle is coupled to menu events, and timer ownership is ad hoc per screen.

Evidence:
- `src/scr_power.c:70-85`
- `src/scr_outlet_data.c:66-89`
- `src/scr_settings_upd.c:133-141` handles this correctly and should be copied as a pattern.

Impact level:
Medium to high.

Proposed solution:
Use `ensure_timer()` helpers consistently. Stop timers on page leave/delete. Centralize screen lifecycle callbacks.

Performance improvement estimate:
Avoids duplicated polling and extra backend load.

Refactored code:
Not applied in Phase 1.

### 17. Widgets Are Updated Even When Values Do Not Change

Problem detected:
Polling screens set every label/card every interval regardless of value changes.

Root cause:
No cached rendered values or state revision tracking.

Evidence:
- `src/scr_power.c:139-164`
- `src/scr_outlet_data.c:98-131`
- `src/scr_sensors_data.c:120-148`
- `src/scr_settings_nw_blue.c:199-230`

Impact level:
Medium.

Proposed solution:
Add per-screen cached values or use app-state revision counters. Only call LVGL setters when text/state changed.

Performance improvement estimate:
Lower redraw pressure. Expected 10-40% less UI work on polling screens depending on visible content.

Refactored code:
Not applied in Phase 1.

### 18. Simulator Exists But Is Not Cleanly Separated

Problem detected:
The project already has `SIMULATOR_ENABLED`, SDL2, and shared source build support. However simulator/hardware HAL selection is embedded in `main.c`, config paths are hard-coded, and backend remains real HTTP by default.

Root cause:
Simulator support was added as build conditionals, not as a platform/backend abstraction.

Evidence:
- `CMakeLists.txt:7`, `CMakeLists.txt:25-39`, `CMakeLists.txt:57-61`
- SDL HAL in `src/main.c:29-90`
- Hardware HAL in `src/main.c:92-133`
- Simulator config path is user-specific: `src/config.c:26-30`

Impact level:
Medium.

Proposed solution:
Phase 2 should formalize simulator support with platform HAL files and `backend_sim.c` while keeping UI code shared.

Performance improvement estimate:
Developer velocity and reproducibility improvement; not a direct runtime speedup.

Refactored code:
Not applied in Phase 1.

### 19. Process Execution Is Available To UI Code

Problem detected:
`runbg_run()` executes shell scripts with `/bin/bash`; reset uses `execv`.

Root cause:
Process control is exposed broadly instead of confined to backend/platform code.

Evidence:
- `src/runbg.c:15-39`
- `src/main.c:137-142`
- `src/scr_settings_upd.c:282-334`

Impact level:
Medium to high.

Proposed solution:
Move process execution to backend_real/platform layer. UI can request reboot/update/factory reset but must not fork/exec directly.

Performance improvement estimate:
Prevents UI stalls and improves testability.

Refactored code:
Not applied in Phase 1.

### 20. Logging And `printf` In Hot Paths

Problem detected:
Several callbacks and polling paths log every cycle or print full response buffers.

Root cause:
Debug logging remains in production paths.

Evidence:
- Curl logs every request: `src/http_helper.c:65`, `src/http_async.c:124`
- Response printf calls across controller, e.g. `src/controller.c:174`, `src/controller.c:372`, `src/controller.c:385`, `src/controller.c:409`
- Poll logging: `src/scr_settings_upd.c:154-160`, `src/scr_sensors.c:231-232`

Impact level:
Medium.

Proposed solution:
Gate verbose logs behind debug level. Never print full backend buffers in production UI.

Performance improvement estimate:
Small to moderate, especially on slow serial consoles.

Refactored code:
Not applied in Phase 1.

## Architecture Problems

- UI and backend are coupled through `controller.h`.
- Screens own backend polling.
- State is global, mutable, and pointer-based.
- Backend functions parse JSON and mutate UI-facing models directly.
- Platform HAL, app startup, backend startup, and UI construction live in one `main.c`/`ttne_display.c` flow.
- Simulator conditionals exist but are not a full portable architecture.

## Recommended Refactor Order

1. Fix low-risk stability issues first: memory leaks, allocation checks, timer duplication guards, and refresh/perf monitor config.
2. Add `backend/` abstraction and async command queue while keeping existing endpoints.
3. Add `app_state` snapshots and UI-thread state application.
4. Convert one high-impact screen first, preferably Outlets, because touch feedback is obvious and backend commands are simple.
5. Convert polling screens: Power, Outlet Data, Sensor Data, Bluetooth, OTA, Network.
6. Split HAL into simulator/hardware files and formalize CMake simulator target.
7. Move toward the requested `pdu_ui/` structure once behavior is stable, preserving working screens during migration.

## Phase 2 Readiness Notes

The existing build already has SDL2 simulator support:

- `SIMULATOR_ENABLED` CMake option
- SDL display/input code in `src/main.c`
- Shared `src/*.c` compilation

Phase 2 should improve this rather than duplicate UI logic. The simulator must get a fake backend (`backend_sim.c`) so development does not depend on localhost services.

## Touch Diagnostics Needed On Target

The repository alone cannot confirm physical touch calibration. On the i.MX7 target, capture:

```sh
cat /proc/bus/input/devices
ls -l /dev/input/event*
evtest /dev/input/event1
```

Verify:

- Correct event device, not a keyboard/power button.
- ABS_X/ABS_Y or ABS_MT_POSITION_X/Y ranges.
- Whether axes need swap/inversion.
- Whether coordinates already arrive in 240x320 screen space.
- Whether software rotation plus `EVDEV_SWAP_AXES=1` double-transforms touch.

## Current Target Gaps Against Requirements

- Touch latency under 50 ms: not achievable while synchronous UI calls remain.
- 60 FPS: config currently targets about 33 FPS.
- UI loop 5 ms: loop sleeps 5 ms, but work inside the loop is unbounded.
- Touch read 10 ms: configured correctly.
- Refresh 16 ms: not configured yet.
- LVGL UI-thread-only rule: mostly true today, but backend separation is incomplete.
- Backend must not block UI: currently violated in many screen callbacks and timers.
- State management: centralized model exists, but it is not thread-safe and not a clean app state.

