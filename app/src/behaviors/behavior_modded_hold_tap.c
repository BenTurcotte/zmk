// SPDX-License-Identifier: MIT

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zmk/behavior.h>
#include <zmk/behavior_hold_tap.h>
#include <zmk/keycode.h>
#include <zmk/hid.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/behavior_queue.h>

#define TAP_DELAY_MS 15

struct behavior_modded_hold_tap_config {
    uint32_t tapping_term_ms;
    uint32_t quick_tap_ms;
    uint32_t require_prior_idle_ms;
    enum behavior_hold_tap_flavor flavor;
};

static int on_keycode_binding_pressed(struct zmk_behavior_binding_event event) {
    const struct behavior_modded_hold_tap_config *cfg =
        event.behavior_dev->config;

    struct behavior_binding *bindings = event.behavior_dev->bindings;

    uint32_t hold_modifier = bindings[event.param1].param1;
    uint32_t tap_modifier = bindings[event.param1].param2;
    uint32_t tap_keycode  = bindings[event.param1].param3;

    if (event.behavior_dev->state == BEHAVIOR_STATE_HOLDING) {
        zmk_hid_press(hold_modifier);
    } else {
        zmk_hid_press(tap_modifier);
        k_msleep(TAP_DELAY_MS);
        zmk_hid_press(tap_keycode);
        k_msleep(TAP_DELAY_MS);
        zmk_hid_release(tap_keycode);
        k_msleep(TAP_DELAY_MS);
        zmk_hid_release(tap_modifier);
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keycode_binding_released(struct zmk_behavior_binding_event event) {
    struct behavior_binding *bindings = event.behavior_dev->bindings;
    uint32_t hold_modifier = bindings[event.param1].param1;

    if (event.behavior_dev->state == BEHAVIOR_STATE_HOLDING) {
        zmk_hid_release(hold_modifier);
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_modded_hold_tap_driver_api = {
    .binding_pressed = on_keycode_binding_pressed,
    .binding_released = on_keycode_binding_released,
};

#define DT_DRV_COMPAT zmk_behavior_modded_hold_tap
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>

#define DEFINE_MODDED_HOLD_TAP(idx) \
    static struct behavior_modded_hold_tap_config behavior_config_##idx = { \
        .tapping_term_ms = DT_PROP(DT_DRV_INST(idx), tapping_term_ms), \
        .quick_tap_ms = DT_PROP(DT_DRV_INST(idx), quick_tap_ms), \
        .require_prior_idle_ms = DT_PROP(DT_DRV_INST(idx), require_prior_idle_ms), \
        .flavor = DT_ENUM(DT_DRV_INST(idx), flavor), \
    }; \
    DEVICE_DT_INST_DEFINE(idx, NULL, NULL, &behavior_config_##idx, NULL, POST_KERNEL, \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
                          &behavior_modded_hold_tap_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_MODDED_HOLD_TAP)