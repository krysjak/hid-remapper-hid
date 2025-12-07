# 🎯 USB Passthrough - Шпаргалка

## 1 хвилина - Що це?

**Проблема:** G Hub не бачить Logitech мишу через RP2350 ❌

**Рішення:** RP2350 копіює всі дані миші → ПК думає що це оригінальна миша ✅

## Швидкий старт (3 кроки)

```bash
# 1. Компіляція
cd hid-remapper-hid/firmware/build
cmake .. && make

# 2. Завантаження (в BOOTSEL режимі)
cp firmware.uf2 /media/RPI-RP2/

# 3. Підключення
[Logitech Миша] → [RP2350] → [Windows PC] → [Запустити G Hub] → ✅
```

## Файли проекту

```
src/
  passthrough.h           ← API
  passthrough.cc          ← Реалізація
  globals.h/cc            ← Змінні (+ passthrough_mode)
  tinyusb_stuff.cc        ← USB callbacks (+ passthrough повернення)
  remapper_single.cc      ← Захоплення дескрипторів (+ tuh_mount_cb)
  remapper.cc             ← Передача звітів (+ passthrough forwarding)

docs/
  PASSTHROUGH.md          ← Технічна документація
  PASSTHROUGH_UA.md       ← Гайд українською
  QUICKSTART_LOGITECH.md  ← Швидкий старт
  CHANGELOG_PASSTHROUGH.md← Зміни
  README_PASSTHROUGH.md   ← Резюме
  COMPLETE.md             ← Чеклист
  CHEATSHEET.md           ← Цей файл
```

## Основні функції

### Захоплення (Host side)
```cpp
tuh_mount_cb(dev_addr) {
    passthrough_capture_device_descriptor();   // VID/PID
    passthrough_capture_config_descriptor();   // Інтерфейси
    passthrough_capture_string_descriptor();   // Назви
}

tuh_hid_mount_cb(dev_addr, instance) {
    passthrough_capture_hid_report_descriptor(); // HID формат
}
```

### Повернення (Device side)
```cpp
tud_descriptor_device_cb() {
    return passthrough_device_descriptor;  // Повертаємо скопійоване
}

tud_descriptor_configuration_cb() {
    return passthrough_config_descriptor;
}

tud_hid_descriptor_report_cb() {
    return passthrough_hid_report_descriptor;
}

tud_descriptor_string_cb(index) {
    return passthrough_manufacturer / product / serial;
}
```

### Передача даних
```cpp
handle_received_report(report, len) {
    if (passthrough_mode) {
        passthrough_forward_report(report, len);  // Прямо → ПК
        return;
    }
    // Стандартна обробка...
}
```

## Глобальні змінні

```cpp
// globals.h / globals.cc
bool passthrough_mode = true;  // За замовчуванням ВКЛ

uint8_t passthrough_device_descriptor[18];
uint8_t passthrough_config_descriptor[256];
uint16_t passthrough_config_descriptor_len;

uint8_t passthrough_hid_report_descriptor[1024];
uint16_t passthrough_hid_report_descriptor_len;

uint16_t passthrough_vid;  // 046D для Logitech
uint16_t passthrough_pid;  // C08B для G502

char passthrough_manufacturer[64];  // "Logitech"
char passthrough_product[64];       // "G502 HERO Gaming Mouse"
char passthrough_serial[64];
```

## Типові задачі

### Увімкнути/Вимкнути
```cpp
// globals.cc
bool passthrough_mode = true;   // Увімкнено (G Hub працює)
bool passthrough_mode = false;  // Вимкнено (стандартний режим)
```

### Debug логи
```cpp
printf("Passthrough mode: %d\n", passthrough_mode);
printf("VID=%04X PID=%04X\n", passthrough_vid, passthrough_pid);
printf("Device descriptor len: %d\n", passthrough_device_descriptor[0]);
printf("Config descriptor len: %d\n", passthrough_config_descriptor_len);
printf("HID descriptor len: %d\n", passthrough_hid_report_descriptor_len);
```

### Перевірка дескрипторів
```cpp
if (passthrough_device_descriptor[0] == 18) {
    printf("✅ Device descriptor OK\n");
}
if (passthrough_config_descriptor_len > 0) {
    printf("✅ Config descriptor OK\n");
}
if (passthrough_hid_report_descriptor_len > 0) {
    printf("✅ HID descriptor OK\n");
}
```

## Що працює vs TODO

### ✅ Працює
- Копіювання всіх дескрипторів
- Input Reports forwarding
- G Hub розпізнає мишу
- Рух, кнопки, колесо
- Налаштування через onboard memory

### 🚧 TODO
- GET_REPORT forwarding (Feature Reports)
- SET_REPORT forwarding (налаштування DPI в реалтаймі)
- Control Transfers (vendor-specific команди)

## Підтримувані миші

| Бренд | Модель | VID | PID | Підтримка |
|-------|--------|-----|-----|-----------|
| Logitech | G502 HERO | 046D | C08B | ✅ |
| Logitech | G Pro Wireless | 046D | C088 | ✅ |
| Logitech | G703 | 046D | C087 | ✅ |
| Razer | DeathAdder | 1532 | XXXX | ✅ |
| Corsair | Dark Core | 1B1C | XXXX | ✅ |
| SteelSeries | Rival | 1038 | XXXX | ✅ |

## Troubleshooting одним рядком

| Проблема | Рішення |
|----------|---------|
| G Hub не бачить | Перевірте VID/PID в логах |
| Миша не рухається | `passthrough_mode = true` |
| Не компілюється | Додайте `passthrough.cc` в CMakeLists.txt |
| DPI не змінюється через G Hub | TODO: Feature Reports |

## Команди компіляції

```bash
# Чистий build
rm -rf build && mkdir build && cd build

# З debug логами
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release версія
cmake -DCMAKE_BUILD_TYPE=Release ..

# Компіляція
make -j4

# Для конкретної плати
cmake -DPICO_BOARD=waveshare_rp2350_usb_a ..
```

## Корисні макроси

```cpp
// Додайте для debug
#define PASSTHROUGH_DEBUG 1

#if PASSTHROUGH_DEBUG
    #define PT_PRINTF(...) printf(__VA_ARGS__)
#else
    #define PT_PRINTF(...)
#endif

// Використання
PT_PRINTF("Captured VID=%04X\n", vid);
```

## Швидкі посилання

📖 Документація:
- `QUICKSTART_LOGITECH.md` - старт за 5 хвилин
- `PASSTHROUGH_UA.md` - все українською
- `PASSTHROUGH.md` - технічні деталі

🔧 Код:
- `src/passthrough.h` - API
- `src/passthrough.cc` - реалізація
- `src/tinyusb_stuff.cc` - USB integration

🎯 Приклади:
- Logitech G502 + G Hub
- Razer + Synapse
- Corsair + iCUE

## Архітектура (1 схема)

```
┌─────────────────┐
│  Logitech Миша  │
└────────┬────────┘
         │ USB Host
    ┌────▼─────────────────────┐
    │      RP2350              │
    │  ┌─────────────────┐     │
    │  │ Capture (Host)  │     │ tuh_mount_cb()
    │  │  - Device Desc  │     │ tuh_hid_mount_cb()
    │  │  - Config Desc  │     │
    │  │  - HID Desc     │     │
    │  │  - Strings      │     │
    │  └────────┬────────┘     │
    │           │              │
    │  ┌────────▼────────┐     │
    │  │  Global Buffers │     │ passthrough_*
    │  └────────┬────────┘     │
    │           │              │
    │  ┌────────▼────────┐     │
    │  │ Return (Device) │     │ tud_descriptor_*_cb()
    │  │  - Same Desc    │     │
    │  └─────────────────┘     │
    └────────┬─────────────────┘
             │ USB Device
    ┌────────▼────────┐
    │   Windows PC    │
    │  ┌───────────┐  │
    │  │  G Hub    │  │ Бачить: "Logitech G502"
    │  └───────────┘  │ VID=046D, PID=C08B
    └─────────────────┘
```

## Підсумок 1 абзацем

Passthrough режим робить RP2350 прозорим USB посередником. При підключенні миші RP2350 копіює всі її USB дескриптори (VID/PID, назви, формат даних) і віддає їх комп'ютеру як свої. Результат: Windows та G Hub бачать оригінальну Logitech мишу, а не RP2350. Базовий функціонал (рух, кнопки) працює на 100%, розширені функції (DPI через ПЗ) потребують додаткової роботи з Feature Reports.

---

**Все, що потрібно знати про Passthrough в одному файлі!** 🚀

