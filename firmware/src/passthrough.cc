#include "passthrough.h"
#include "globals.h"
#include "platform.h"
#include <cstring>
#include <cstdio>
#include "tusb.h"

// Перевіряємо чи доступний USB Host mode
#if defined(CFG_TUH_HID) && CFG_TUH_HID > 0
#define PASSTHROUGH_HOST_AVAILABLE 1
#include "pico/time.h"
#include "class/hid/hid.h"
#else
#define PASSTHROUGH_HOST_AVAILABLE 0
#endif

// ============================================================================
// HOST MODE ONLY FUNCTIONS - доступні тільки коли USB Host увімкнено
// ============================================================================
#if PASSTHROUGH_HOST_AVAILABLE

// Зберігає копію device descriptor від підключеної миші
void passthrough_capture_device_descriptor(uint8_t dev_addr, const tusb_desc_device_t* desc) {
    if (!passthrough_mode) return;

    // Зберігаємо адресу пристрою для подальших операцій
    passthrough_dev_addr = dev_addr;

    printf("Passthrough: захоплено device descriptor від dev_addr=%d, VID=%04X PID=%04X\n",
           dev_addr, desc->idVendor, desc->idProduct);

    memcpy(passthrough_device_descriptor, desc, sizeof(tusb_desc_device_t));
    passthrough_vid = desc->idVendor;
    passthrough_pid = desc->idProduct;
}

// Зберігає копію configuration descriptor від підключеної миші
void passthrough_capture_config_descriptor(uint8_t dev_addr, const uint8_t* desc, uint16_t len) {
    if (!passthrough_mode) return;

    if (len > sizeof(passthrough_config_descriptor)) {
        printf("Passthrough: config descriptor занадто великий (%d bytes), обрізаю до %d\n", 
               len, sizeof(passthrough_config_descriptor));
        len = sizeof(passthrough_config_descriptor);
    }
    
    printf("Passthrough: захоплено config descriptor від dev_addr=%d, len=%d\n", dev_addr, len);
    
    memcpy(passthrough_config_descriptor, desc, len);
    passthrough_config_descriptor_len = len;
}

// Зберігає копію HID report descriptor від підключеної миші
void passthrough_capture_hid_report_descriptor(uint8_t dev_addr, uint8_t itf_num, const uint8_t* desc, uint16_t len) {
    if (!passthrough_mode) return;
    
    if (len > sizeof(passthrough_hid_report_descriptor)) {
        printf("Passthrough: HID report descriptor занадто великий (%d bytes), обрізаю до %d\n", 
               len, sizeof(passthrough_hid_report_descriptor));
        len = sizeof(passthrough_hid_report_descriptor);
    }
    
    printf("Passthrough: захоплено HID report descriptor від dev_addr=%d itf=%d, len=%d\n", 
           dev_addr, itf_num, len);
    
    memcpy(passthrough_hid_report_descriptor, desc, len);
    passthrough_hid_report_descriptor_len = len;
    passthrough_interface_num = itf_num;
}

// Захоплює string descriptors від підключеного пристрою
void passthrough_capture_string_descriptor(uint8_t dev_addr, uint8_t index, const uint16_t* desc, uint16_t len) {
    if (!passthrough_mode || desc == NULL || len < 2) return;
    
    // desc[0] містить довжину та тип
    // desc[1..n] містять UTF-16 символи
    
    uint8_t str_len = (desc[0] & 0xFF) / 2 - 1;  // відніми header
    if (str_len > 63) str_len = 63;  // максимум 63 символи + null terminator
    
    char* target = NULL;
    if (index == 1) {
        target = passthrough_manufacturer;
    } else if (index == 2) {
        target = passthrough_product;
    } else if (index == 3) {
        target = passthrough_serial;
    }
    
    if (target != NULL) {
        // Конвертуємо UTF-16 до ASCII
        for (int i = 0; i < str_len; i++) {
            target[i] = (char)(desc[i + 1] & 0xFF);
        }
        target[str_len] = '\0';
        printf("Passthrough: string[%d] = '%s'\n", index, target);
    }
}

// Прозоро передає HID reports без обробки
void passthrough_forward_report(uint8_t const* report, uint16_t len) {
    if (!passthrough_mode) return;
    
    // Відправляємо report як є, без жодних змін
    // В стандартному режимі це робить remapper.cc, але в passthrough просто копіюємо
    if (tud_hid_n_ready(0)) {
        tud_hid_n_report(0, 0, report, len);
    }
}

// Обробка control transfer запитів в режимі passthrough
// Це дозволяє Logitech G Hub надсилати vendor-specific команди
bool passthrough_handle_control_request(uint8_t dev_addr, const tusb_control_request_t* request) {
    if (!passthrough_mode) return false;
    
    printf("Passthrough: control request bmRequestType=0x%02X bRequest=0x%02X wValue=0x%04X wIndex=0x%04X wLength=%d\n",
           request->bmRequestType, request->bRequest, request->wValue, request->wIndex, request->wLength);
    
    // TODO: перенаправити запит до підключеного пристрою через TinyUSB host API
    // Поки що повертаємо false - це потребує додаткової інтеграції з host stack
    
    return false;
}

// Структура для зберігання результату GET_REPORT
static struct {
    bool completed;
    bool success;
    uint8_t* buffer;
    uint16_t actual_len;
} get_report_result;

// Callback для завершення GET_REPORT
static void get_report_complete_cb(tuh_xfer_t* xfer) {
    get_report_result.completed = true;
    get_report_result.success = (xfer->result == XFER_RESULT_SUCCESS);
    if (get_report_result.success && xfer->actual_len > 0) {
        get_report_result.actual_len = xfer->actual_len;
    }
    printf("Passthrough: GET_REPORT complete, result=%d, len=%d\n", xfer->result, xfer->actual_len);
}

// Обробка GET_REPORT в режимі passthrough
// Повертає довжину отриманих даних або 0 якщо не оброблено в passthrough режимі
uint16_t passthrough_handle_get_report(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    if (!passthrough_mode) return 0;
    if (passthrough_dev_addr == 0) return 0;

    printf("Passthrough: GET_REPORT itf=%d report_id=0x%02X type=%d len=%d\n", itf, report_id, report_type, reqlen);

    // Підготовка control request для GET_REPORT
    tusb_control_request_t const request = {
        .bmRequestType = 0xA1,  // Device to Host | Class | Interface
        .bRequest      = HID_REQ_CONTROL_GET_REPORT,
        .wValue        = (report_type << 8) | report_id,
        .wIndex        = passthrough_interface_num,
        .wLength       = reqlen
    };

    // Ініціалізація результату
    get_report_result.completed = false;
    get_report_result.success = false;
    get_report_result.buffer = buffer;
    get_report_result.actual_len = 0;

    // Відправка запиту до підключеного пристрою
    tuh_xfer_t xfer = {
        .daddr       = passthrough_dev_addr,
        .ep_addr     = 0,
        .setup       = &request,
        .buffer      = buffer,
        .complete_cb = get_report_complete_cb,
        .user_data   = 0
    };

    if (!tuh_control_xfer(&xfer)) {
        printf("Passthrough: GET_REPORT - не вдалося ініціювати control transfer\n");
        return 0;
    }

    // Очікуємо завершення (з таймаутом)
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    while (!get_report_result.completed) {
        tuh_task();  // Обробляємо USB host events
        if (to_ms_since_boot(get_absolute_time()) - start_time > 100) {  // 100ms timeout
            printf("Passthrough: GET_REPORT timeout\n");
            return 0;
        }
    }

    if (get_report_result.success) {
        printf("Passthrough: GET_REPORT успішно, отримано %d байт\n", get_report_result.actual_len);
        return get_report_result.actual_len;  // Повертаємо фактичну довжину
    }

    return 0;
}

// Структура для зберігання результату SET_REPORT
static struct {
    bool completed;
    bool success;
} set_report_result;

// Callback для завершення SET_REPORT
static void set_report_complete_cb(tuh_xfer_t* xfer) {
    set_report_result.completed = true;
    set_report_result.success = (xfer->result == XFER_RESULT_SUCCESS);
    printf("Passthrough: SET_REPORT complete, result=%d\n", xfer->result);
}

// Обробка SET_REPORT в режимі passthrough
bool passthrough_handle_set_report(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, const uint8_t* buffer, uint16_t bufsize) {
    if (!passthrough_mode) return false;
    if (passthrough_dev_addr == 0) return false;

    printf("Passthrough: SET_REPORT itf=%d report_id=0x%02X type=%d len=%d\n", itf, report_id, report_type, bufsize);

    // Debug: друкуємо перші байти даних
    printf("Passthrough: SET_REPORT data: ");
    for (int i = 0; i < (bufsize < 16 ? bufsize : 16); i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");

    // Підготовка control request для SET_REPORT
    tusb_control_request_t const request = {
        .bmRequestType = 0x21,  // Host to Device | Class | Interface
        .bRequest      = HID_REQ_CONTROL_SET_REPORT,
        .wValue        = (report_type << 8) | report_id,
        .wIndex        = passthrough_interface_num,
        .wLength       = bufsize
    };

    // Ініціалізація результату
    set_report_result.completed = false;
    set_report_result.success = false;

    // Відправка запиту до підключеного пристрою
    tuh_xfer_t xfer = {
        .daddr       = passthrough_dev_addr,
        .ep_addr     = 0,
        .setup       = &request,
        .buffer      = (uint8_t*)buffer,  // TinyUSB приймає non-const
        .complete_cb = set_report_complete_cb,
        .user_data   = 0
    };

    if (!tuh_control_xfer(&xfer)) {
        printf("Passthrough: SET_REPORT - не вдалося ініціювати control transfer\n");
        return false;
    }

    // Очікуємо завершення (з таймаутом)
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    while (!set_report_result.completed) {
        tuh_task();  // Обробляємо USB host events
        if (to_ms_since_boot(get_absolute_time()) - start_time > 100) {  // 100ms timeout
            printf("Passthrough: SET_REPORT timeout\n");
            return false;
        }
    }

    if (set_report_result.success) {
        printf("Passthrough: SET_REPORT успішно\n");
        return true;
    }

    printf("Passthrough: SET_REPORT помилка\n");
    return false;
}

#else // !PASSTHROUGH_HOST_AVAILABLE

// ============================================================================
// STUB FUNCTIONS - для device-only mode (без USB Host)
// ============================================================================

void passthrough_capture_device_descriptor(uint8_t dev_addr, const tusb_desc_device_t* desc) {
    (void)dev_addr; (void)desc;
    // Not available in device-only mode
}

void passthrough_capture_config_descriptor(uint8_t dev_addr, const uint8_t* desc, uint16_t len) {
    (void)dev_addr; (void)desc; (void)len;
    // Not available in device-only mode
}

void passthrough_capture_hid_report_descriptor(uint8_t dev_addr, uint8_t itf_num, const uint8_t* desc, uint16_t len) {
    (void)dev_addr; (void)itf_num; (void)desc; (void)len;
    // Not available in device-only mode
}

void passthrough_capture_string_descriptor(uint8_t dev_addr, uint8_t index, const uint16_t* desc, uint16_t len) {
    (void)dev_addr; (void)index; (void)desc; (void)len;
    // Not available in device-only mode
}

void passthrough_forward_report(uint8_t const* report, uint16_t len) {
    (void)report; (void)len;
    // Not available in device-only mode
}

bool passthrough_handle_control_request(uint8_t dev_addr, const tusb_control_request_t* request) {
    (void)dev_addr; (void)request;
    return false;  // Not available in device-only mode
}

uint16_t passthrough_handle_get_report(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    (void)itf; (void)report_id; (void)report_type; (void)buffer; (void)reqlen;
    return 0;  // Not available in device-only mode
}

bool passthrough_handle_set_report(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, const uint8_t* buffer, uint16_t bufsize) {
    (void)itf; (void)report_id; (void)report_type; (void)buffer; (void)bufsize;
    return false;  // Not available in device-only mode
}

#endif // PASSTHROUGH_HOST_AVAILABLE
