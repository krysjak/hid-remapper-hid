#ifndef _PASSTHROUGH_H_
#define _PASSTHROUGH_H_

#include <stdint.h>
#include <tusb.h>
#include "class/hid/hid.h"  // для hid_report_type_t

// Функції для роботи в режимі повного passthrough
// Зберігає всі дескриптори підключеної миші для прозорої передачі

// Захоплює device descriptor від підключеного пристрою
void passthrough_capture_device_descriptor(uint8_t dev_addr, const tusb_desc_device_t* desc);

// Захоплює configuration descriptor від підключеного пристрою
void passthrough_capture_config_descriptor(uint8_t dev_addr, const uint8_t* desc, uint16_t len);

// Захоплює HID report descriptor від підключеного пристрою
void passthrough_capture_hid_report_descriptor(uint8_t dev_addr, uint8_t itf_num, const uint8_t* desc, uint16_t len);

// Захоплює string descriptors від підключеного пристрою
void passthrough_capture_string_descriptor(uint8_t dev_addr, uint8_t index, const uint16_t* desc, uint16_t len);

// Прозоро передає всі HID reports без обробки
void passthrough_forward_report(uint8_t const* report, uint16_t len);

// Обробляє control transfer запити в режимі passthrough
bool passthrough_handle_control_request(uint8_t dev_addr, const tusb_control_request_t* request);

// Прозора передача Feature Reports
uint16_t passthrough_handle_get_report(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen);
bool passthrough_handle_set_report(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, const uint8_t* buffer, uint16_t bufsize);

#endif

