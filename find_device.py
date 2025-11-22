import hid

# Logitech G502 HERO Gaming Mouse VID/PID
VID = 0x046D
PID = 0xC08B

print(f"Looking for device with VID=0x{VID:04X} and PID=0x{PID:04X}...")

found = False
try:
    for device in hid.enumerate():
        if device['vendor_id'] == VID and device['product_id'] == PID:
            print("\nFound device!")
            print(f"  Manufacturer: {device.get('manufacturer_string')}")
            print(f"  Product:      {device.get('product_string')}")
            print(f"  Serial:       {device.get('serial_number')}")
            print(f"  Interface:    {device.get('interface_number')}")
            print(f"  Path:         {device.get('path')}")
            found = True
except Exception as e:
    print(f"Error enumerating devices: {e}")
    print("Make sure you have the 'hid' library installed (pip install hid)")

if not found:
    print("\nDevice not found.")
else:
    print("\nSearch complete.")
