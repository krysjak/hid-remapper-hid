import hid
import struct
import time
import zlib

# Logitech G502 HERO Gaming Mouse VID/PID (as per find_device.py)
VID = 0x046D
PID = 0xC08B

# Our custom configuration interface
CONFIG_USAGE_PAGE = 0xFF00
CONFIG_USAGE = 0x20

# Command constants
COMMAND_INJECT_INPUT = 0x42
CONFIG_VERSION = 18
CONFIG_SIZE = 32

class RemapperControl:
    def __init__(self):
        self.device = None
        self._find_device()

    def _find_device(self):
        print(f"Looking for device with VID=0x{VID:04X} and PID=0x{PID:04X}...")
        for device_info in hid.enumerate():
            if device_info['vendor_id'] == VID and device_info['product_id'] == PID:
                if device_info['usage_page'] == CONFIG_USAGE_PAGE and device_info['usage'] == CONFIG_USAGE:
                    print("Found RP2350 configuration interface!")
                    try:
                        self.device = hid.device()
                        self.device.open_path(device_info['path'])
                        print("Device opened successfully.")
                        return
                    except Exception as e:
                        print(f"Failed to open device: {e}")
        
        if self.device is None:
            raise Exception("Device not found or could not be opened.")

    def inject_input(self, x, y, buttons):
        # Report ID from our_descriptor.h
        report_id = 100 
        
        # Structure matches set_feature_t in types.h:
        # [Version (1)] [Command (1)] [Data (26)] [CRC32 (4)]
        # Data for INJECT_INPUT: [X (2)] [Y (2)] [Buttons (1)]
        
        # 1. Construct the data part (Version + Command + Payload)
        # Payload: x (h), y (h), buttons (B)
        data_part = struct.pack('<B B h h B', CONFIG_VERSION, COMMAND_INJECT_INPUT, x, y, buttons)
        
        # 2. Pad to CONFIG_SIZE - 4 (reserved for CRC)
        # Current size: 1+1+2+2+1 = 7 bytes.
        # Target size: 32 - 4 = 28 bytes.
        # Padding needed: 21 bytes.
        data_part += b'\x00' * (CONFIG_SIZE - 4 - len(data_part))
        
        # 3. Calculate CRC32
        # zlib.crc32 returns unsigned 32-bit integer in Python 3
        checksum = zlib.crc32(data_part)
        
        # 4. Append CRC32
        full_payload = data_part + struct.pack('<I', checksum)
        
        # 5. Prepend Report ID for hidapi
        report_data = bytes([report_id]) + full_payload
        
        try:
            self.device.send_feature_report(report_data)
        except Exception as e:
            print(f"Error sending feature report: {e}")

    def move(self, x, y):
        self.inject_input(x, y, 0)

    def click(self, button=1):
        # Button 1 = 0x01, Button 2 = 0x02, etc.
        self.inject_input(0, 0, 1 << (button - 1))
        time.sleep(0.05)
        self.inject_input(0, 0, 0)

if __name__ == "__main__":
    try:
        ctl = RemapperControl()
        
        print("Moving mouse in a square pattern...")
        for _ in range(4):
            ctl.move(100, 0)
            time.sleep(0.5)
            ctl.move(0, 100)
            time.sleep(0.5)
            ctl.move(-100, 0)
            time.sleep(0.5)
            ctl.move(0, -100)
            time.sleep(0.5)
            
        print("Clicking...")
        ctl.click(1)
        
    except Exception as e:
        print(f"Error: {e}")
