import hid

# Logitech G502 HERO Gaming Mouse VID/PID
VID = 0x046D
PID = 0xC08B

# Our custom configuration interface
CONFIG_USAGE_PAGE = 0xFF00
CONFIG_USAGE = 0x20

print(f"Looking for device with VID=0x{VID:04X} and PID=0x{PID:04X}...")

found_any = False
found_config = False

try:
    for device in hid.enumerate():
        if device['vendor_id'] == VID and device['product_id'] == PID:
            found_any = True
            print("\nFound matching VID/PID!")
            print(f"  Manufacturer: {device.get('manufacturer_string')}")
            print(f"  Product:      {device.get('product_string')}")
            print(f"  Serial:       {device.get('serial_number')}")
            print(f"  Interface:    {device.get('interface_number')}")
            print(f"  Usage Page:   0x{device.get('usage_page'):04X}")
            print(f"  Usage:        0x{device.get('usage'):04X}")
            print(f"  Path:         {device.get('path')}")

            if device.get('usage_page') == CONFIG_USAGE_PAGE and device.get('usage') == CONFIG_USAGE:
                print("  *** THIS IS THE CONFIGURATION INTERFACE (RP2350) ***")
                found_config = True
            
except Exception as e:
    print(f"Error enumerating devices: {e}")
    print("Make sure you have the 'hid' library installed (pip install hid)")

if not found_any:
    print("\nNo devices found with that VID/PID.")
elif found_config:
    print("\nSUCCESS: Found RP2350 configuration interface!")
else:
    print("\nFound devices with matching VID/PID, but none matched the specific Config Interface usage.")
    print("This might be a real G502, or the OS hasn't enumerated the vendor interface yet.")
