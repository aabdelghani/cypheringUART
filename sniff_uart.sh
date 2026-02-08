#!/bin/bash
# Simple UART sniffer using hexdump
# Press Ctrl+C to stop

echo "======================================================================"
echo "UART Sniffer - Monitoring /dev/ttyUSB0 at 115200 baud"
echo "======================================================================"
echo "Packet format: [16-byte NONCE][ENCRYPTED DATA]"
echo "Press Ctrl+C to stop"
echo "======================================================================"
echo ""

# Configure serial port
stty -F /dev/ttyUSB0 115200 cs8 -cstopb -parenb raw

# Read and display as hex
cat /dev/ttyUSB0 | hexdump -v -e '"%08_ax  " 16/1 "%02x " "  |" 16/1 "%_p" "|\n"'
