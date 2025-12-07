# 🔧 Build Fix Applied

## Виправлено помилку компіляції

### Проблема:
```
/home/runner/work/hid-remapper-hid/hid-remapper-hid/firmware/src/remapper_single.cc:79:9: error: 'passthrough_mode' was not declared in this scope
   79 |     if (passthrough_mode) {
      |         ^~~~~~~~~~~~~~~~
```

### Рішення:
Додано `#include "globals.h"` в файл `src/remapper_single.cc`

### Змінено:
```cpp
// Було:
#include <tusb.h>

#include "pio_usb.h"
#include "usb_midi_host.h"

#include "pico/platform.h"
#include "pico/time.h"

#include "descriptor_parser.h"
#include "out_report.h"
#include "passthrough.h"
#include "remapper.h"
#include "tick.h"

// Стало:
#include <tusb.h>

#include "pio_usb.h"
#include "usb_midi_host.h"

#include "pico/platform.h"
#include "pico/time.h"

#include "descriptor_parser.h"
#include "globals.h"        // <--- ДОДАНО
#include "out_report.h"
#include "passthrough.h"
#include "remapper.h"
#include "tick.h"
```

### Тепер має компілюватись без помилок! ✅

Спробуйте:
```bash
cd hid-remapper-hid/firmware/build
cmake ..
make
```

Якщо виникнуть інші помилки компіляції - дайте знати!

