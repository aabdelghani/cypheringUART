# Changelog

All notable changes to the CypheringUART project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Fixed - 2026-02-08 22:41:06

#### Critical Bug Fixes: Stack Overflow & Protocol Length Field

**Issues Identified:**

1. **Stack Overflow in Sender Task** (Boot Loop Crash)
   - **Symptom**: Sender crashed immediately after sending first message with error:
     ```
     ***ERROR*** A stack overflow in task sender_task has been detected.
     Backtrace: panic_abort -> vApplicationStackOverflowHook
     ```
   - **Root Cause**: HMAC-SHA256 operations (using mbedtls) require more stack space than originally allocated
   - **Impact**: Sender continuously rebooted, unable to send messages

2. **Receiver Reading Wrong Data Length** (HMAC Verification Failure)
   - **Symptom**: Receiver read 1024 bytes instead of actual message length (~24 bytes), HMAC verification always failed
   - **Root Cause**: Protocol lacked length field, receiver called `uart_read_bytes(UART_NUM, buffer, BUF_SIZE, timeout)` which filled buffer with garbage
   - **Impact**: All messages rejected due to HMAC mismatch, no successful decryption

**Solutions Implemented:**

1. **Stack Overflow Fix**
   - **Change**: Increased sender task stack from 4096 to **8192 bytes**
   - **Location**: `sender/main/main.c:164`
   - **Result**: Task now has sufficient stack for HMAC operations

2. **Protocol Enhancement: Added 2-Byte Length Field**
   - **New Protocol**: `[NONCE(16)][LENGTH(2)][ENCRYPTED_DATA(variable)][HMAC(32)]`
   - **Sender Changes**:
     - Sends 2-byte big-endian length field after nonce
     - Updated HMAC computation: `HMAC([NONCE || LENGTH || ENCRYPTED_DATA])`
   - **Receiver Changes**:
     - Reads 2-byte length field after nonce
     - Validates length (0 < length ≤ BUF_SIZE)
     - Reads **exactly** that many bytes of encrypted data (no more garbage)
     - Updated HMAC verification to match sender's computation
   - **Result**: Receiver now reads correct amount of data, HMAC verification succeeds

**Modified Files:**
- `sender/main/main.c` - Added length field transmission, increased stack size, updated HMAC
- `reciever/main/main.c` - Added length field reception and validation, updated HMAC verification

**Testing Status:**
- ✅ Sender builds successfully (225,360 bytes)
- ✅ Receiver builds successfully (226,960 bytes)
- ✅ No stack overflow errors
- ✅ Protocol now properly handles variable-length messages

---

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
