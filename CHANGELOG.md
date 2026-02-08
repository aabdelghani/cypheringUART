# Changelog

All notable changes to the CypheringUART project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added - 2026-02-08 22:13:27

#### HMAC-SHA256 Message Authentication (UL1998 Section 1.5i)

**Security Enhancement**: Implements message authentication to detect corruption, tampering, and masquerade attacks as required by UL1998 standard for safety-related software in programmable components.

**Changes:**
- Added HMAC-SHA256 computation and verification functions using mbedtls library
- Updated packet structure from `[NONCE(16)][ENCRYPTED_DATA]` to `[NONCE(16)][ENCRYPTED_DATA][HMAC(32)]`
- **Sender**: Computes HMAC over nonce + encrypted data before transmission
- **Receiver**: Verifies HMAC before decryption (authenticate-then-decrypt pattern)
- Implemented constant-time comparison to prevent timing attacks
- Added separate HMAC key (32 bytes) distinct from encryption key
- Linked mbedtls library in CMakeLists.txt for both sender and receiver

**Security Improvements:**
- ✓ Detects message corruption during transmission
- ✓ Prevents tampering with encrypted data
- ✓ Protects against masquerade/impersonation attacks
- ✓ Ensures message authenticity and integrity
- ✓ Constant-time HMAC verification prevents timing side-channels

**Modified Files:**
- `sender/main/aes_wrapper.h` - Added HMAC function declarations
- `sender/main/aes_wrapper.c` - Implemented HMAC functions
- `sender/main/main.c` - Compute and send HMAC with packets
- `sender/main/CMakeLists.txt` - Added mbedtls dependency
- `reciever/main/aes_wrapper.h` - Added HMAC function declarations
- `reciever/main/aes_wrapper.c` - Implemented HMAC functions
- `reciever/main/main.c` - Verify HMAC before decryption
- `reciever/main/CMakeLists.txt` - Added mbedtls dependency

**UL1998 Compliance:**
Addresses requirement 1.5(i): "Failures in data communication such as transmission errors, repetitions, deletion, insertion, resequencing, corruption, delay and masquerade"

**Status**: ✅ Implemented - Partially addresses UL1998 Section 1.5(i)

**Remaining UL1998 Requirements:**
- ⏳ Sequence numbers (prevent replay, resequencing, deletion)
- ⏳ Timeout detection (prevent delay attacks)
- ⏳ Acknowledgment system (detect message loss)
- ⏳ CRC/Checksum (basic error detection)
- ⏳ Error state management and fail-safe procedures

---

## [0.1.0] - 2026-02-08

### Added
- Initial release with AES-128 CTR mode encryption
- ESP32 sender firmware with hardware RNG for nonce generation
- ESP32-S3 receiver firmware with automatic decryption
- UART communication at 115200 baud
- Python and C monitoring/sniffing tools
- Comprehensive project documentation
- GitHub repository setup with submodules

### Security Notes
- Uses pre-shared AES-128 key (hardcoded for development)
- Unique nonce per message using ESP32 hardware RNG
- No authentication in initial release (added in unreleased)
