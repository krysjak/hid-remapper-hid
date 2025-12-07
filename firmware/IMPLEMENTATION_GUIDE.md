# 🔧 GET_REPORT / SET_REPORT Implementation Guide

## Архітектура реалізації

### 1. Зберігання адреси пристрою

При підключенні миші зберігаємо її USB адресу:

```cpp
// globals.h
extern uint8_t passthrough_dev_addr;

// globals.cc
uint8_t passthrough_dev_addr = 0;

// passthrough.cc
void passthrough_capture_device_descriptor(uint8_t dev_addr, const tusb_desc_device_t* desc) {
    passthrough_dev_addr = dev_addr;  // Зберігаємо для подальшого використання
}
```

### 2. GET_REPORT Implementation

#### Структури для результату:
```cpp
static struct {
    bool completed;
    bool success;
    uint8_t* buffer;
    uint16_t actual_len;
} get_report_result;
```

#### Callback функція:
```cpp
static void get_report_complete_cb(tuh_xfer_t* xfer) {
    get_report_result.completed = true;
    get_report_result.success = (xfer->result == XFER_RESULT_SUCCESS);
    if (get_report_result.success && xfer->actual_len > 0) {
        get_report_result.actual_len = xfer->actual_len;
    }
}
```

#### Основна функція:
```cpp
uint16_t passthrough_handle_get_report(uint8_t itf, uint8_t report_id, 
                                       hid_report_type_t report_type, 
                                       uint8_t* buffer, uint16_t reqlen) {
    if (!passthrough_mode || passthrough_dev_addr == 0) return 0;
    
    // 1. Створюємо control request
    tusb_control_request_t const request = {
        .bmRequestType = 0xA1,  // Device to Host
        .bRequest      = HID_REQ_CONTROL_GET_REPORT,
        .wValue        = (report_type << 8) | report_id,
        .wIndex        = passthrough_interface_num,
        .wLength       = reqlen
    };
    
    // 2. Ініціалізуємо результат
    get_report_result.completed = false;
    get_report_result.success = false;
    get_report_result.buffer = buffer;
    get_report_result.actual_len = 0;
    
    // 3. Виконуємо transfer
    tuh_xfer_t xfer = {
        .daddr       = passthrough_dev_addr,
        .ep_addr     = 0,
        .setup       = &request,
        .buffer      = buffer,
        .complete_cb = get_report_complete_cb,
        .user_data   = 0
    };
    
    if (!tuh_control_xfer(&xfer)) return 0;
    
    // 4. Чекаємо завершення
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    while (!get_report_result.completed) {
        tuh_task();
        if (to_ms_since_boot(get_absolute_time()) - start_time > 100) {
            return 0;  // Timeout
        }
    }
    
    // 5. Повертаємо результат
    return get_report_result.success ? get_report_result.actual_len : 0;
}
```

### 3. SET_REPORT Implementation

#### Структури для результату:
```cpp
static struct {
    bool completed;
    bool success;
} set_report_result;
```

#### Callback функція:
```cpp
static void set_report_complete_cb(tuh_xfer_t* xfer) {
    set_report_result.completed = true;
    set_report_result.success = (xfer->result == XFER_RESULT_SUCCESS);
}
```

#### Основна функція:
```cpp
bool passthrough_handle_set_report(uint8_t itf, uint8_t report_id, 
                                   hid_report_type_t report_type, 
                                   const uint8_t* buffer, uint16_t bufsize) {
    if (!passthrough_mode || passthrough_dev_addr == 0) return false;
    
    // 1. Створюємо control request
    tusb_control_request_t const request = {
        .bmRequestType = 0x21,  // Host to Device
        .bRequest      = HID_REQ_CONTROL_SET_REPORT,
        .wValue        = (report_type << 8) | report_id,
        .wIndex        = passthrough_interface_num,
        .wLength       = bufsize
    };
    
    // 2. Ініціалізуємо результат
    set_report_result.completed = false;
    set_report_result.success = false;
    
    // 3. Виконуємо transfer
    tuh_xfer_t xfer = {
        .daddr       = passthrough_dev_addr,
        .ep_addr     = 0,
        .setup       = &request,
        .buffer      = (uint8_t*)buffer,
        .complete_cb = set_report_complete_cb,
        .user_data   = 0
    };
    
    if (!tuh_control_xfer(&xfer)) return false;
    
    // 4. Чекаємо завершення
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    while (!set_report_result.completed) {
        tuh_task();
        if (to_ms_since_boot(get_absolute_time()) - start_time > 100) {
            return false;  // Timeout
        }
    }
    
    // 5. Повертаємо результат
    return set_report_result.success;
}
```

### 4. Інтеграція з TinyUSB Device

У `tinyusb_stuff.cc`:

```cpp
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, 
                               hid_report_type_t report_type, 
                               uint8_t* buffer, uint16_t reqlen) {
    // Спробуємо passthrough
    uint16_t len = passthrough_handle_get_report(itf, report_id, report_type, buffer, reqlen);
    if (len > 0) {
        return len;  // Успішно в passthrough режимі
    }
    
    // Стандартна обробка
    // ...
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, 
                           hid_report_type_t report_type, 
                           const uint8_t* buffer, uint16_t bufsize) {
    // Спробуємо passthrough
    if (passthrough_handle_set_report(itf, report_id, report_type, buffer, bufsize)) {
        return;  // Успішно в passthrough режимі
    }
    
    // Стандартна обробка
    // ...
}
```

## 🎯 Ключові моменти

1. **Async операції:** Використовуємо callbacks для async transfers
2. **Timeout:** Завжди використовуємо timeout (100ms) для уникнення зависання
3. **tuh_task():** Викликаємо в циклі очікування для обробки USB events
4. **Error handling:** Перевіряємо результат кожного кроку
5. **Debug logging:** Логуємо всі операції для діагностики

## 🚀 Результат

Тепер RP2350 повністю прозорий для G Hub і інших програм налаштування!

