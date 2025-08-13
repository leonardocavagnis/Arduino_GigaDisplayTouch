/*
 * Copyright 2025 Arduino SA
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#ifdef __ZEPHYR__

#include "Arduino_GigaDisplayTouch.h"
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

typedef struct {
  size_t x;
  size_t y;
  bool pressed;
} touch_point_t;

static uint8_t zephyr_touch_cb_slot_num;
static struct k_sem zephyr_touch_event_sync;
static touch_point_t zephyr_touch_points[CONFIG_INPUT_GT911_MAX_TOUCH_POINTS];

typedef void (*zephyr_input_callback_t)(struct input_event *evt,
                                        void *user_data);
extern "C" void zephyr_input_register_callback(zephyr_input_callback_t cb);
static void touch_event_callback(struct input_event *evt, void *user_data);

Arduino_GigaDisplayTouch::Arduino_GigaDisplayTouch() {}

Arduino_GigaDisplayTouch::~Arduino_GigaDisplayTouch() {}

bool Arduino_GigaDisplayTouch::begin() {
  k_sem_init(&zephyr_touch_event_sync, 0, 1);
  zephyr_input_register_callback(touch_event_callback);
  return true;
}

void Arduino_GigaDisplayTouch::end() {}

uint8_t Arduino_GigaDisplayTouch::getTouchPoints(GDTpoint_t *points) {
  // First wait to see if we get any events.
  if (k_sem_take(&zephyr_touch_event_sync, K_NO_WAIT) != 0) {
    return 0;
  }

  uint8_t count_pressed = 0;
  for (uint8_t i = 0; i <= zephyr_touch_cb_slot_num; i++) {
    if (zephyr_touch_points[i].pressed) {
      points[count_pressed].x = zephyr_touch_points[i].x;
      points[count_pressed].y = zephyr_touch_points[i].y;
      count_pressed++;
    }
  }
  return count_pressed;
}

void Arduino_GigaDisplayTouch::onDetect(void (*handler)(uint8_t,
                                                        GDTpoint_t *)) {
  UNUSED(handler);
}

static void touch_event_callback(struct input_event *evt, void *user_data) {
  static const struct device *const touch_dev =
      DEVICE_DT_GET(DT_CHOSEN(zephyr_touch));

  if (evt->dev != touch_dev) {
    return;
  }

  switch (evt->code) {
  case INPUT_ABS_MT_SLOT:
    zephyr_touch_cb_slot_num = evt->value;
    break;
  case INPUT_ABS_X:
    zephyr_touch_points[zephyr_touch_cb_slot_num].x = evt->value;
    break;
  case INPUT_ABS_Y:
    zephyr_touch_points[zephyr_touch_cb_slot_num].y = evt->value;
    break;
  case INPUT_BTN_TOUCH:
    zephyr_touch_points[zephyr_touch_cb_slot_num].pressed = evt->value;
    break;
  }

  if (evt->sync) {
    k_sem_give(&zephyr_touch_event_sync);
  }
}
#endif
