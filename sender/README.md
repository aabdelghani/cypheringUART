# ESP32 Encrypted UART Sender

This project implements an ESP32-based sender that encrypts data using AES-128 CTR mode and transmits it over UART.

## Features

- **AES-128 CTR Mode Encryption**: Uses the tiny-AES-c library for efficient encryption
- **UART Communication**: Sends encrypted data over UART2 (GPIO 16/17)
- **Random Nonce Generation**: Uses ESP32 hardware RNG for secure nonce generation
- **Automatic Packet Structure**: Sends [NONCE(16 bytes)][ENCRYPTED_DATA]

## Hardware Configuration

- **TX Pin**: GPIO 17
- **RX Pin**: GPIO 16
- **Baud Rate**: 115200
- **UART**: UART2

## Project Structure

```
sender/
├── CMakeLists.txt          # Root CMake configuration
├── main/
│   ├── CMakeLists.txt      # Main component CMake
│   ├── main.c              # Main application
│   ├── aes_wrapper.h       # AES encryption wrapper API
│   └── aes_wrapper.c       # AES encryption implementation
└── README.md
```

## Building the Project

1. Make sure ESP-IDF is installed and configured:
   ```bash
   . $HOME/esp/esp-idf/export.sh
   ```

2. Navigate to the sender directory:
   ```bash
   cd sender
   ```

3. Configure the project (optional):
   ```bash
   idf.py menuconfig
   ```

4. Build the project:
   ```bash
   idf.py build
   ```

5. Flash to ESP32:
   ```bash
   idf.py -p /dev/ttyUSB0 flash
   ```

6. Monitor the output:
   ```bash
   idf.py -p /dev/ttyUSB0 monitor
   ```

## How It Works

1. **Initialization**:
   - AES-128 is initialized with a pre-shared key (16 bytes)
   - UART2 is configured for communication

2. **Data Transmission**:
   - A random 16-byte nonce is generated using ESP32 hardware RNG
   - Plaintext data is encrypted using AES-128 CTR mode with the nonce
   - The packet format is: `[NONCE][ENCRYPTED_DATA]`
   - First, 16 bytes of nonce are sent
   - Then, the encrypted data is sent

3. **Test Messages**:
   - The sender automatically cycles through test messages every 5 seconds
   - Each message is encrypted with a unique nonce

## Security Considerations

- **Pre-Shared Key**: Currently uses a hardcoded key. In production, implement secure key storage (e.g., NVS encrypted partition, secure boot)
- **Nonce Uniqueness**: Each message uses a unique random nonce (critical for CTR mode security)
- **Key Management**: The same key must be configured on both sender and receiver

## Customization

### Change UART Pins
Edit `main/main.c`:
```c
#define TXD_PIN (GPIO_NUM_17)  // Change to your TX pin
#define RXD_PIN (GPIO_NUM_16)  // Change to your RX pin
```

### Change Encryption Key
Edit `main/main.c`:
```c
static const uint8_t AES_SHARED_KEY[AES_KEY_SIZE] = {
    // Your 16-byte key here
};
```

### Modify Send Interval
Edit `main/main.c` in the `sender_task` function:
```c
vTaskDelay(pdMS_TO_TICKS(5000));  // Change delay in milliseconds
```

## API Reference

### aes_wrapper.h

```c
// Initialize AES with 128-bit key
void aes_init(const uint8_t *key);

// Encrypt data using AES-128 CTR mode
void aes_encrypt_ctr(const uint8_t *input, uint8_t *output,
                     size_t length, const uint8_t *nonce);

// Decrypt data using AES-128 CTR mode
void aes_decrypt_ctr(const uint8_t *input, uint8_t *output,
                     size_t length, const uint8_t *nonce);

// Generate random nonce
void aes_generate_nonce(uint8_t *nonce);
```

## Next Steps

1. Build and test the sender
2. Implement the receiver with matching configuration
3. Connect TX (GPIO 17) of sender to RX of receiver
4. Connect RX (GPIO 16) of sender to TX of receiver
5. Connect GND between both ESP32 devices

## Troubleshooting

- **Build errors**: Ensure ESP-IDF is properly sourced
- **Flash errors**: Check USB port and permissions
- **No output**: Verify UART connections and baud rate
- **Decryption fails**: Ensure both devices use the same AES key
