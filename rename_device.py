from hid_controller import DeviceController
import time

def main():
    print("Searching for device...")
    try:
        ctl = DeviceController()
    except Exception as e:
        print(f"Error: {e}")
        print("Make sure the device is connected and you have permissions (try running as Admin/Root).")
        return

    print("\nCurrent Identity (assumed default or previously set):")
    # Note: We can't easily read back the current identity without implementing GET_IDENTITY, 
    # but we can overwrite it.

    print("\nTarget Identity: Logitech G502 HERO Gaming Mouse")
    
    # Logitech G502 HERO values
    vid = 0x046D
    pid = 0xC08B
    bcd_device = 0x0100 # v1.00
    manufacturer = "pdd"
    product = "G502 HERO Gaming Mouse"
    serial = "000000000000" # Or generate a random one

    print(f"VID: 0x{vid:04X}")
    print(f"PID: 0x{pid:04X}")
    print(f"Manufacturer: {manufacturer}")
    print(f"Product: {product}")
    print(f"Serial: {serial}")

    confirm = input("\nDo you want to apply this identity? (y/n): ")
    if confirm.lower() != 'y':
        print("Aborted.")
        return

    print("Applying new identity...")
    try:
        ctl.set_identity(vid, pid, bcd_device, manufacturer, product, serial)
        print("\nSuccess! The identity has been updated.")
        print("IMPORTANT: You must UNPLUG and REPLUG the device for changes to take effect.")
    except Exception as e:
        print(f"Failed to set identity: {e}")

if __name__ == "__main__":
    main()
