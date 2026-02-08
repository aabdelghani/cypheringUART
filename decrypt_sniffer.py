#!/usr/bin/env python3
"""
Enhanced UART Sniffer with AES-128 CTR Decryption
Shows encrypted data AND decrypted plaintext
"""

import serial
import sys
import time
from datetime import datetime

# Configuration
SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200
NONCE_SIZE = 16

# AES-128 Pre-shared Key (must match sender/receiver)
AES_SHARED_KEY = bytes([
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
])

def format_hex(data, bytes_per_line=16):
    """Format bytes as hex string with line breaks"""
    lines = []
    for i in range(0, len(data), bytes_per_line):
        chunk = data[i:i+bytes_per_line]
        hex_str = ' '.join(f'{b:02x}' for b in chunk)
        ascii_str = ''.join(chr(b) if 32 <= b < 127 else '.' for b in chunk)
        lines.append(f"   {hex_str:<48}  |{ascii_str}|")
    return '\n'.join(lines)

def aes_ctr_decrypt(ciphertext, key, nonce):
    """
    Decrypt using AES-128 CTR mode (pure Python implementation)
    This matches the tiny-AES-c implementation
    """
    try:
        from Crypto.Cipher import AES
        cipher = AES.new(key, AES.MODE_CTR, nonce=nonce[:8], initial_value=nonce[8:])
        return cipher.decrypt(ciphertext)
    except ImportError:
        # Fallback: Use cryptography library
        try:
            from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
            from cryptography.hazmat.backends import default_backend

            cipher = Cipher(
                algorithms.AES(key),
                modes.CTR(nonce),
                backend=default_backend()
            )
            decryptor = cipher.decryptor()
            return decryptor.update(ciphertext) + decryptor.finalize()
        except ImportError:
            return b"[Install pycryptodome: pip3 install pycryptodome --break-system-packages]"

def sniff_and_decrypt():
    print("=" * 80)
    print(" 🔐 UART Sniffer with AES-128 CTR Decryption")
    print("=" * 80)
    print(f" Port: {SERIAL_PORT} @ {BAUD_RATE} baud")
    print(f" Packet Format: [16-byte NONCE][ENCRYPTED DATA]")
    print(f" AES Key: {format_hex(AES_SHARED_KEY, 16).strip()}")
    print("=" * 80)
    print()

    try:
        # Open serial port
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.5)
        print(f"✓ Connected to {SERIAL_PORT}")
        print("✓ Listening for encrypted packets... (Press Ctrl+C to exit)\n")

        buffer = bytearray()
        packet_count = 0

        while True:
            # Read available data
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                buffer.extend(data)

            # Try to parse packets
            if len(buffer) >= NONCE_SIZE:
                # Extract nonce (first 16 bytes)
                nonce = bytes(buffer[:NONCE_SIZE])
                buffer = buffer[NONCE_SIZE:]

                # Wait a bit for the encrypted payload
                time.sleep(0.05)
                if ser.in_waiting > 0:
                    more_data = ser.read(ser.in_waiting)
                    buffer.extend(more_data)

                # Assume rest of buffer is the encrypted payload
                # (until next packet or reasonable size)
                if len(buffer) > 0:
                    # Take a reasonable amount (max 256 bytes per packet)
                    payload_len = min(len(buffer), 256)
                    encrypted_data = bytes(buffer[:payload_len])
                    buffer = buffer[payload_len:]

                    # Decrypt the data
                    decrypted_data = aes_ctr_decrypt(encrypted_data, AES_SHARED_KEY, nonce)

                    # Display packet
                    packet_count += 1
                    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]

                    print(f"{'═' * 80}")
                    print(f"📦 Packet #{packet_count} @ {timestamp}")
                    print(f"{'═' * 80}")

                    print(f"🔑 Nonce ({NONCE_SIZE} bytes):")
                    print(format_hex(nonce))

                    print(f"\n🔒 ENCRYPTED Data ({len(encrypted_data)} bytes):")
                    print(format_hex(encrypted_data))

                    print(f"\n🔓 DECRYPTED Plaintext ({len(decrypted_data)} bytes):")
                    print(format_hex(decrypted_data))

                    # Try to display as readable text
                    try:
                        plaintext = decrypted_data.decode('utf-8', errors='replace')
                        print(f"\n📝 Message: \"{plaintext}\"")
                    except:
                        plaintext = decrypted_data.decode('ascii', errors='replace')
                        print(f"\n📝 Message: \"{plaintext}\"")

                    print(f"\n📊 Total packet size: {NONCE_SIZE + len(encrypted_data)} bytes")
                    print()

            time.sleep(0.01)

    except serial.SerialException as e:
        print(f"\n❌ Error: {e}")
        print(f"\nTroubleshooting:")
        print(f"  • Check FTDI connection: ls -l /dev/ttyUSB*")
        print(f"  • Verify wiring: Sender TX (GPIO17) → FTDI RX")
        return 1
    except KeyboardInterrupt:
        print(f"\n\n{'═' * 80}")
        print(f"✓ Sniffer stopped")
        print(f"✓ Total packets captured: {packet_count}")
        print(f"{'═' * 80}")
        return 0
    except Exception as e:
        print(f"\n❌ Unexpected error: {e}")
        import traceback
        traceback.print_exc()
        return 1
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("✓ Serial port closed")

if __name__ == "__main__":
    # Check for crypto library
    try:
        from Crypto.Cipher import AES
        print("✓ Using PyCryptodome for AES decryption\n")
    except ImportError:
        try:
            from cryptography.hazmat.primitives.ciphers import Cipher
            print("✓ Using cryptography library for AES decryption\n")
        except ImportError:
            print("⚠ Warning: No AES library found!")
            print("Install one of these:")
            print("  pip3 install pycryptodome --break-system-packages")
            print("  OR")
            print("  pip3 install cryptography --break-system-packages")
            print()
            response = input("Continue anyway? (y/n): ")
            if response.lower() != 'y':
                sys.exit(1)

    sys.exit(sniff_and_decrypt())
