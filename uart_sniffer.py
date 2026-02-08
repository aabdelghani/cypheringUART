#!/usr/bin/env python3
"""
UART Sniffer for AES Encrypted Communication
Monitors UART bus and displays packet structure: [NONCE(16 bytes)][ENCRYPTED DATA]
"""

import serial
import sys
import time
from datetime import datetime

# Configuration
SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200
NONCE_SIZE = 16

def format_hex(data):
    """Format bytes as hex string"""
    return ' '.join(f'{b:02x}' for b in data)

def sniff_uart():
    print("=" * 70)
    print("UART Sniffer - AES Encrypted Communication Monitor")
    print("=" * 70)
    print(f"Port: {SERIAL_PORT}")
    print(f"Baud Rate: {BAUD_RATE}")
    print(f"Packet Format: [16-byte NONCE][ENCRYPTED DATA]")
    print("=" * 70)
    print()

    try:
        # Open serial port
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"✓ Connected to {SERIAL_PORT}")
        print("✓ Listening for data... (Press Ctrl+C to exit)\n")

        buffer = bytearray()
        packet_count = 0

        while True:
            # Read available data
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                buffer.extend(data)

                # Try to parse packets (nonce + data)
                while len(buffer) >= NONCE_SIZE:
                    # Look for packet boundary
                    # In this simple version, we assume continuous data stream
                    # Each packet should be at least NONCE_SIZE bytes

                    # Extract nonce
                    nonce = buffer[:NONCE_SIZE]
                    buffer = buffer[NONCE_SIZE:]

                    # Read encrypted data (wait a bit for the rest)
                    time.sleep(0.01)
                    if ser.in_waiting > 0:
                        more_data = ser.read(ser.in_waiting)
                        buffer.extend(more_data)

                    # Find potential end of packet (next nonce start or timeout)
                    # For now, take all available data as the encrypted payload
                    encrypted_len = len(buffer)

                    if encrypted_len > 0:
                        encrypted_data = buffer[:encrypted_len]
                        buffer = buffer[encrypted_len:]

                        # Display packet
                        packet_count += 1
                        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]

                        print(f"{'─' * 70}")
                        print(f"📦 Packet #{packet_count} @ {timestamp}")
                        print(f"{'─' * 70}")
                        print(f"🔑 Nonce ({NONCE_SIZE} bytes):")
                        print(f"   {format_hex(nonce)}")
                        print(f"\n🔒 Encrypted Data ({encrypted_len} bytes):")

                        # Display in rows of 16 bytes
                        for i in range(0, len(encrypted_data), 16):
                            chunk = encrypted_data[i:i+16]
                            print(f"   {format_hex(chunk)}")

                        # Try to display as ASCII (will be garbage for encrypted)
                        try:
                            ascii_attempt = encrypted_data.decode('ascii', errors='replace')
                            print(f"\n📝 ASCII (encrypted, will be garbage):")
                            print(f"   {ascii_attempt}")
                        except:
                            pass

                        print(f"\n📊 Total Size: {NONCE_SIZE + encrypted_len} bytes")
                        print()

            time.sleep(0.01)

    except serial.SerialException as e:
        print(f"❌ Error: {e}")
        print(f"\nTroubleshooting:")
        print(f"  1. Check if FTDI is connected: ls -l /dev/ttyUSB*")
        print(f"  2. Check permissions: sudo usermod -a -G dialout $USER")
        print(f"  3. Re-login or reboot after adding to group")
        return 1
    except KeyboardInterrupt:
        print("\n\n✓ Sniffer stopped by user")
        print(f"✓ Total packets captured: {packet_count}")
        return 0
    except Exception as e:
        print(f"❌ Unexpected error: {e}")
        return 1
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("✓ Serial port closed")

if __name__ == "__main__":
    sys.exit(sniff_uart())
