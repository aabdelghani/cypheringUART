# ESP32-S3 Encrypted UART Receiver

This project implements an ESP32-S3 based receiver that receives encrypted data over UART and decrypts it using AES-128 CTR mode.

## Features

- **AES-128 CTR Mode Decryption**: Uses the tiny-AES-c library for efficient decryption
- **UART Communication**: Receives encrypted data over UART2 (GPIO 16/17)
- **Automatic Packet Parsing**: Extracts [NONCE(16 bytes)][ENCRYPTED_DATA]
- **Real-time Decryption**: Decrypts and displays plaintext messages

## Hardware Configuration

- **RX Pin**: GPIO 16 (receives data from sender's TX)
- **TX Pin**: GPIO 17 (not used for reception, but configured)
- **Baud Rate**: 115200
- **UART**: UART2
- **Target**: ESP32-S3

## Wiring

Connect to sender ESP32-S3:
```
Sender GPIO17 (TX)  →  Receiver GPIO16 (RX)
Sender GPIO16 (RX)  ←  Receiver GPIO17 (TX)  [optional for bidirectional]
Sender GND          →  Receiver GND
```

## Project Structure

```
reciever/
├── CMakeLists.txt          # Root CMake configuration
├── main/
│   ├── CMakeLists.txt      # Main component CMake
│   ├── main.c              # Main receiver application
│   ├── aes_wrapper.h       # AES decryption wrapper API
│   └── aes_wrapper.c       # AES decryption implementation
└── README.md
```

## Building the Project

1. Source ESP-IDF environment:
   ```bash
   . $HOME/.espressif/v5.5.2/esp-idf/export.sh
   ```

2. Navigate to receiver directory:
   ```bash
   cd reciever
   ```

3. Set target to ESP32-S3:
   ```bash
   idf.py set-target esp32s3
   ```

4. Build the project:
   ```bash
   idf.py build
   ```

5. Flash to ESP32-S3:
   ```bash
   idf.py -p /dev/ttyACM0 flash
   ```

6. Monitor the output:
   ```bash
   idf.py -p /dev/ttyACM0 monitor
   ```

## How It Works

1. **Initialization**:
   - AES-128 is initialized with the same pre-shared key as the sender
   - UART2 is configured for communication at 115200 baud

2. **Data Reception**:
   - First, 16 bytes (nonce) are read from UART
   - Then, the encrypted payload is read
   - Both are logged for debugging

3. **Decryption**:
   - The encrypted data is decrypted using AES-128 CTR mode with the received nonce
   - The plaintext is displayed in both hex and string format

4. **Message Display**:
   - Each received message is numbered
   - Shows nonce, encrypted data, decrypted data, and plaintext

## Security Notes

- **Pre-Shared Key**: Must match the sender's key exactly
- **Same Key**: Both sender and receiver use the hardcoded key
- **Nonce Handling**: Each message uses a unique nonce sent with the encrypted data

## Expected Output

```
=== ESP32-S3 Encrypted UART Receiver ===
AES initialized with shared key
UART initialized on RX: GPIO16, TX: GPIO17
Receiver ready, waiting for encrypted data...

Received nonce:
00 b0 62 ed f5 95 8e 0b 40 b4 62 b0 8b a8 71 e0

Received encrypted data (25 bytes):
48 65 6c 6c 6f 20 66 72 6f 6d 20 45 53 50 33 32...

Decrypted data:
48 65 6c 6c 6f 20 66 72 6f 6d 20 45 53 50 33 32...

========================================
Message #1 successfully decrypted!
========================================
Plaintext message: "Hello from ESP32 Sender!"
Total packet size: 41 bytes (nonce: 16 + data: 25)
========================================
```

## Customization

### Change UART Pins
Edit `main/main.c`:
```c
#define RXD_PIN (GPIO_NUM_16)  // Change RX pin
#define TXD_PIN (GPIO_NUM_17)  // Change TX pin
```

### Change Decryption Key
Edit `main/main.c` (must match sender):
```c
static const uint8_t AES_SHARED_KEY[AES_KEY_SIZE] = {
    // Your 16-byte key here
};
```

## Troubleshooting

- **No data received**: Check UART wiring (TX to RX crossover)
- **Garbage output**: Verify both devices use the same AES key
- **Incomplete packets**: Check baud rate matches (115200)
- **Build errors**: Ensure ESP-IDF is properly sourced

## Testing

To test the complete system:
1. Flash and run the sender on one ESP32-S3
2. Flash and run the receiver on another ESP32-S3
3. Connect TX (GPIO17) of sender to RX (GPIO16) of receiver
4. Connect GND between both devices
5. Monitor receiver output to see decrypted messages
