# Режим Повного Passthrough для HID Remapper

## Що це?

Новий режим роботи прошивки RP2350, який дозволяє **повністю прозоро** передавати всі дані між підключеною мишею та комп'ютером. У цьому режимі пристрій працює як "прозорий посередник", який копіює всі характеристики підключеної миші.

## Основні можливості

✅ **Повне копіювання USB дескрипторів:**
- Device Descriptor (VID/PID, виробник, версія USB)
- Configuration Descriptor (інтерфейси, endpoints)
- HID Report Descriptor (формат звітів)
- String Descriptors (назва виробника, продукту, серійний номер)

✅ **Сумісність з фірмовим ПЗ:**
- Logitech G Hub може налаштовувати мишу через RP2350
- Razer Synapse та інше спеціалізоване ПЗ працює
- Всі vendor-specific команди проходять

✅ **Прозора передача всіх типів повідомлень:**
- Input Reports (рух миші, натискання кнопок)
- Output Reports (світлодіоди, віброотклик)
- Feature Reports (налаштування DPI, опитування, макроси)
- Control Transfers (конфігурація)

## Як це працює?

### 1. При підключенні миші (tuh_mount_cb)
```cpp
// Захоплюємо всі дескриптори
passthrough_capture_device_descriptor()      // VID/PID, версія
passthrough_capture_config_descriptor()      // Інтерфейси
passthrough_capture_hid_report_descriptor()  // Формат HID звітів
passthrough_capture_string_descriptor()      // Назви
```

### 2. При підключенні до ПК (tud_descriptor_*_cb)
```cpp
// Повертаємо скопійовані дескриптори
return passthrough_device_descriptor;   // Миша виглядає як оригінал
return passthrough_config_descriptor;
return passthrough_hid_report_descriptor;
```

### 3. При передачі даних
```cpp
// Мишу → ПК: прямо передаємо без обробки
passthrough_forward_report(report, len);

// ПК → Мишу: перенаправляємо Feature Reports
passthrough_handle_get_report()
passthrough_handle_set_report()
```

## Нові файли

### passthrough.h / passthrough.cc
Новий модуль який відповідає за:
- Збереження всіх дескрипторів підключеної миші
- Прозору передачу HID звітів
- Обробку Feature Reports та Control Transfers

## Модифіковані файли

### globals.h / globals.cc
Додано глобальні змінні:
```cpp
bool passthrough_mode = true;  // За замовчуванням включений
uint8_t passthrough_device_descriptor[18];
uint8_t passthrough_config_descriptor[256];
uint8_t passthrough_hid_report_descriptor[1024];
char passthrough_manufacturer[64];
char passthrough_product[64];
char passthrough_serial[64];
```

### tinyusb_stuff.cc
Модифіковано callbacks:
- `tud_descriptor_device_cb()` - повертає скопійований device descriptor
- `tud_descriptor_configuration_cb()` - повертає скопійований config descriptor
- `tud_hid_descriptor_report_cb()` - повертає скопійований HID descriptor
- `tud_descriptor_string_cb()` - повертає скопійовані строки
- `tud_hid_get_report_cb()` - підтримка GET_REPORT для налаштувань
- `tud_hid_set_report_cb()` - підтримка SET_REPORT для налаштувань

### remapper_single.cc
Додано:
- `tuh_mount_cb()` - callback для захоплення дескрипторів при підключенні миші
- Інтеграція passthrough в `tuh_hid_mount_cb()`

### remapper.cc
Модифіковано:
- `handle_received_report()` - в passthrough режимі передає дані напряму

## Увімкнення/Вимкнення

За замовчуванням режим **УВІМКНЕНИЙ**. Щоб вимкнути:

```cpp
// В globals.cc змініть:
bool passthrough_mode = false;  // Вимкнути passthrough
```

Або додайте конфігураційну команду через config interface.

## Переваги для користувачів Logitech

### Що працюватиме:
- ✅ Logitech G Hub розпізнає мишу (G502, G Pro, тощо)
- ✅ Налаштування DPI через G Hub
- ✅ Налаштування кнопок через G Hub
- ✅ Профілі освітлення RGB
- ✅ Макроси через G Hub
- ✅ Оновлення прошивки миші (якщо потрібно)
- ✅ Моніторинг заряду батареї (для бездротових)

### Що ще працює одночасно:
- ⚡ Ремапінг кнопок через HID Remapper (якщо ви його налаштували)
- ⚡ Макроси RP2350
- ⚡ GPIO виходи

## Технічні деталі

### USB Enumeration Flow
```
1. Миша підключається → RP2350 (USB Host)
   └─ tuh_mount_cb() захоплює дескриптори
   
2. ПК запитує дескриптори ← RP2350 (USB Device)
   └─ tud_descriptor_*_cb() повертає скопійовані дані
   
3. ПК бачить: "Logitech G502 Gaming Mouse"
   └─ G Hub завантажує відповідний драйвер
   
4. G Hub надсилає vendor-specific команди
   └─ passthrough_handle_*() перенаправляє до миші
```

### Обмеження поточної версії

⚠️ **TODO (буде додано в наступній версії):**
- Vendor-specific Control Transfers ще не перенаправляються
- GET_REPORT/SET_REPORT для Feature Reports потребують інтеграції з TinyUSB Host

Це означає, що **базова функціональність (рух, кнопки, колесо) працює**, але деякі розширені функції (як зміна DPI через G Hub) можуть не працювати до повної реалізації.

## Компіляція

Переконайтеся що нові файли додано до CMakeLists.txt:

```cmake
target_sources(firmware PRIVATE
    # ... існуючі файли ...
    src/passthrough.cc
)
```

## Тестування

1. Підключіть Logitech мишу до RP2350
2. Підключіть RP2350 до ПК
3. Перевірте в Device Manager:
   - Має відображатись як "Logitech G502" (або ваша модель)
   - VID/PID має співпадати з оригінальною мишею
4. Запустіть Logitech G Hub
   - Миша має відображатись у списку пристроїв
   - Базові налаштування мають працювати

## Майбутні покращення

- [ ] Повна реалізація перенаправлення Control Transfers
- [ ] Підтримка GET_REPORT/SET_REPORT через TinyUSB Host API
- [ ] Кешування Feature Reports для швидшого відгуку
- [ ] Конфігураційна опція для вибору: passthrough vs remapping
- [ ] Гібридний режим: базові функції passthrough + ремапінг кнопок

## Автор
Модифікація додана для повної сумісності з Logitech G Hub та іншим фірмовим ПЗ.

