/*
 * UART Sniffer with AES-128 CTR Decryption
 * Uses tiny-AES-c library to decrypt messages in real-time
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include "aes.h"

#define SERIAL_PORT "/dev/ttyUSB0"
#define BAUD_RATE B115200
#define NONCE_SIZE 16
#define BUF_SIZE 1024

// AES-128 Pre-shared Key (must match sender/receiver)
static const uint8_t AES_SHARED_KEY[16] = {
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};

void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("%s\n", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02x ", data[i]);
        if ((i + 1) % 16 == 0) {
            printf(" |");
            for (size_t j = i - 15; j <= i; j++) {
                printf("%c", (data[j] >= 32 && data[j] < 127) ? data[j] : '.');
            }
            printf("|\n");
        }
    }
    if (len % 16 != 0) {
        size_t remaining = len % 16;
        for (size_t i = 0; i < (16 - remaining) * 3; i++) {
            printf(" ");
        }
        printf(" |");
        for (size_t i = len - remaining; i < len; i++) {
            printf("%c", (data[i] >= 32 && data[i] < 127) ? data[i] : '.');
        }
        printf("|\n");
    }
}

void print_plaintext(const char *label, const uint8_t *data, size_t len) {
    printf("%s \"", label);
    for (size_t i = 0; i < len; i++) {
        if (data[i] >= 32 && data[i] < 127) {
            printf("%c", data[i]);
        } else if (data[i] == 0) {
            break;
        } else {
            printf(".");
        }
    }
    printf("\"\n");
}

void decrypt_aes_ctr(const uint8_t *encrypted, uint8_t *decrypted, size_t len,
                     const uint8_t *nonce, const uint8_t *key) {
    struct AES_ctx ctx;

    // Initialize AES context with key and nonce
    AES_init_ctx_iv(&ctx, key, nonce);

    // Copy encrypted data to output buffer
    memcpy(decrypted, encrypted, len);

    // Decrypt in CTR mode (same operation as encryption)
    AES_CTR_xcrypt_buffer(&ctx, decrypted, len);
}

int setup_serial(const char *port) {
    int fd = open(port, O_RDONLY | O_NOCTTY);
    if (fd < 0) {
        perror("Error opening serial port");
        return -1;
    }

    struct termios options;
    tcgetattr(fd, &options);

    // Set baud rate
    cfsetispeed(&options, BAUD_RATE);

    // 8N1 mode
    options.c_cflag &= ~PARENB;  // No parity
    options.c_cflag &= ~CSTOPB;  // 1 stop bit
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;      // 8 data bits

    // Raw input mode
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_oflag &= ~OPOST;

    // Set read timeout
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 5;  // 0.5 second timeout

    tcsetattr(fd, TCSANOW, &options);
    tcflush(fd, TCIFLUSH);

    return fd;
}

void print_timestamp() {
    time_t now;
    struct tm *tm_info;
    char time_str[64];

    time(&now);
    tm_info = localtime(&now);
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);
    printf("@ %s", time_str);
}

int main() {
    uint8_t buffer[BUF_SIZE];
    uint8_t nonce[NONCE_SIZE];
    uint8_t encrypted[BUF_SIZE];
    uint8_t decrypted[BUF_SIZE];
    int packet_count = 0;

    printf("================================================================================\n");
    printf(" 🔐 UART Sniffer with AES-128 CTR Decryption (using tiny-AES-c)\n");
    printf("================================================================================\n");
    printf(" Port: %s @ 115200 baud\n", SERIAL_PORT);
    printf(" Packet Format: [16-byte NONCE][ENCRYPTED DATA]\n");
    printf(" AES Key: ");
    for (int i = 0; i < 16; i++) printf("%02x ", AES_SHARED_KEY[i]);
    printf("\n");
    printf("================================================================================\n\n");

    // Open serial port
    int fd = setup_serial(SERIAL_PORT);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s\n", SERIAL_PORT);
        fprintf(stderr, "Check:\n");
        fprintf(stderr, "  • FTDI connected: ls -l /dev/ttyUSB*\n");
        fprintf(stderr, "  • Wiring: Sender GPIO17 → FTDI RX, GND connected\n");
        return 1;
    }

    printf("✓ Connected to %s\n", SERIAL_PORT);
    printf("✓ Listening for encrypted packets... (Press Ctrl+C to exit)\n\n");

    while (1) {
        // Read nonce (first 16 bytes)
        int nonce_read = 0;
        while (nonce_read < NONCE_SIZE) {
            int n = read(fd, nonce + nonce_read, NONCE_SIZE - nonce_read);
            if (n > 0) {
                nonce_read += n;
            } else if (n < 0) {
                perror("Error reading nonce");
                break;
            }
        }

        if (nonce_read < NONCE_SIZE) {
            continue;
        }

        // Read encrypted payload (read what's available, typically the message length)
        usleep(10000);  // Wait 10ms for data to arrive
        int payload_len = read(fd, encrypted, BUF_SIZE);

        if (payload_len <= 0) {
            continue;
        }

        // Decrypt the data
        decrypt_aes_ctr(encrypted, decrypted, payload_len, nonce, AES_SHARED_KEY);

        // Display packet
        packet_count++;
        printf("════════════════════════════════════════════════════════════════════════════════\n");
        printf("📦 Packet #%d ", packet_count);
        print_timestamp();
        printf("\n");
        printf("════════════════════════════════════════════════════════════════════════════════\n");

        printf("\n🔑 Nonce (%d bytes):\n", NONCE_SIZE);
        print_hex("", nonce, NONCE_SIZE);

        printf("\n🔒 ENCRYPTED Data (%d bytes):\n", payload_len);
        print_hex("", encrypted, payload_len);

        printf("\n🔓 DECRYPTED Plaintext (%d bytes):\n", payload_len);
        print_hex("", decrypted, payload_len);

        printf("\n📝 ");
        print_plaintext("Message:", decrypted, payload_len);

        printf("\n📊 Total packet size: %d bytes\n\n", NONCE_SIZE + payload_len);

        fflush(stdout);
    }

    close(fd);
    return 0;
}
