import hid
import struct
import time

# Constants matching firmware
CONFIG_USAGE_PAGE = 0xFF00
CONFIG_USAGE = 0x20

# Config Commands
CMD_SET_IDENTITY = 26
CMD_INJECT_INPUT = 27
CMD_PERSIST_CONFIG = 7
CMD_RESET_INTO_BOOTSEL = 1

class DeviceController:
    def __init__(self, vid=0x046D, pid=0xC08B):
        self.vid = vid
        self.pid = pid
        self.device = None
        self.find_device()

    def find_device(self):
        print(f"Looking for device with VID=0x{self.vid:04X} and PID=0x{self.pid:04X}...")
        for d in hid.enumerate(self.vid, self.pid):
            if d['usage_page'] == CONFIG_USAGE_PAGE and d['usage'] == CONFIG_USAGE:
                print(f"Found Configuration Interface: {d['path']}")
                self.device = hid.device()
                self.device.open_path(d['path'])
                return
        print("Device not found!")
        raise Exception("Device not found")

    def send_command(self, command, data=b''):
        if not self.device:
            raise Exception("Device not connected")
        
        # Report ID 0 (for hidapi), Version (19), Command, Data (variable), CRC (4 bytes)
        # Total 128 bytes payload + 1 byte report ID
        # Payload structure: [Version:1][Command:1][Data:122][CRC:4]
        payload = struct.pack('<BB', 19, command) + data
        payload = payload.ljust(124, b'\x00') # Pad data to 124 bytes (128 - 4 for CRC)
        
        # Calculate CRC32
        import zlib
        crc = zlib.crc32(payload)
        payload += struct.pack('<I', crc)
        
        # Prepend Report ID (0) for hidapi write
        self.device.write(b'\x00' + payload)

    def inject_mouse(self, x, y, buttons=0, wheel=0, pan=0):
        """
        Inject mouse input.
        x, y: 16-bit signed integers
        buttons: 8-bit bitmask
        wheel, pan: 8-bit signed integers
        """
        # struct inject_input_t {
        #     uint8_t buttons;
        #     int16_t x;
        #     int16_t y;
        #     int8_t wheel;
        #     int8_t pan;
        # };
        data = struct.pack('<Bhhbb', buttons, x, y, wheel, pan)
        self.send_command(CMD_INJECT_INPUT, data)

    def set_identity(self, vid, pid, bcd_device, manufacturer, product, serial):
        """
        Set USB Identity.
        vid, pid, bcd_device: 16-bit integers
        manufacturer, product, serial: strings (max 31 chars)
        """
        # struct set_identity_t {
        #     uint16_t usb_vid;
        #     uint16_t usb_pid;
        #     uint16_t bcd_device;
        #     char manufacturer[32];
        #     char product[32];
        #     char serial[32];
        # };
        
        manuf_bytes = manufacturer.encode('utf-8')[:31].ljust(32, b'\x00')
        prod_bytes = product.encode('utf-8')[:31].ljust(32, b'\x00')
        serial_bytes = serial.encode('utf-8')[:31].ljust(32, b'\x00')
        
        data = struct.pack('<HHH', vid, pid, bcd_device) + manuf_bytes + prod_bytes + serial_bytes
        self.send_command(CMD_SET_IDENTITY, data)
        self.send_command(CMD_PERSIST_CONFIG) # Save changes
        print("Identity set! Please reboot the device.")

if __name__ == "__main__":
    # Example usage
    try:
        ctl = DeviceController()
        print("Device connected!")
        
        # Move mouse in a square
        for _ in range(4):
            ctl.inject_mouse(100, 0)
            time.sleep(0.1)
            ctl.inject_mouse(0, 100)
            time.sleep(0.1)
            ctl.inject_mouse(-100, 0)
            time.sleep(0.1)
            ctl.inject_mouse(0, -100)
            time.sleep(0.1)
            
    except Exception as e:
        print(e)
