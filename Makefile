# Makefile for UART Decrypt Sniffer

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I./tiny-AES-c
LDFLAGS =

SOURCES = uart_decrypt_sniffer.c tiny-AES-c/aes.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = uart_decrypt_sniffer

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo ""
	@echo "✓ Build successful!"
	@echo "Run with: ./$(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "✓ Cleaned"

install:
	@echo "No installation needed. Run ./$(TARGET) directly"

.DEFAULT_GOAL := all
