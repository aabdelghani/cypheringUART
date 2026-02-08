#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Source ESP-IDF
source $HOME/.espressif/v5.5.2/esp-idf/export.sh > /dev/null 2>&1

PROJECT_ROOT="/home/q/1Projects/steineronline/cypheringUART"

echo -e "${GREEN}=== CypheringUART Build and Flash Tool ===${NC}\n"

# Function to build project
build_project() {
    local project_dir=$1
    local project_name=$2

    echo -e "${YELLOW}Building $project_name...${NC}"
    cd "$project_dir"

    if idf.py build; then
        echo -e "${GREEN}✓ $project_name build successful!${NC}\n"
        return 0
    else
        echo -e "${RED}✗ $project_name build failed!${NC}\n"
        return 1
    fi
}

# Function to flash project
flash_project() {
    local project_dir=$1
    local project_name=$2
    local port=$3

    echo -e "${YELLOW}Flashing $project_name to $port...${NC}"
    cd "$project_dir"

    if idf.py -p $port flash; then
        echo -e "${GREEN}✓ $project_name flashed successfully!${NC}\n"
        return 0
    else
        echo -e "${RED}✗ $project_name flash failed!${NC}\n"
        return 1
    fi
}

# Main menu
case "$1" in
    build-sender)
        build_project "$PROJECT_ROOT/sender" "Sender"
        ;;
    build-receiver)
        build_project "$PROJECT_ROOT/reciever" "Receiver"
        ;;
    build-all)
        build_project "$PROJECT_ROOT/sender" "Sender"
        build_project "$PROJECT_ROOT/reciever" "Receiver"
        ;;
    flash-sender)
        if [ -z "$2" ]; then
            echo -e "${RED}Error: Please specify port (e.g., /dev/ttyUSB0)${NC}"
            exit 1
        fi
        flash_project "$PROJECT_ROOT/sender" "Sender" "$2"
        ;;
    flash-receiver)
        if [ -z "$2" ]; then
            echo -e "${RED}Error: Please specify port (e.g., /dev/ttyACM0)${NC}"
            exit 1
        fi
        flash_project "$PROJECT_ROOT/reciever" "Receiver" "$2"
        ;;
    monitor-sender)
        if [ -z "$2" ]; then
            echo -e "${RED}Error: Please specify port (e.g., /dev/ttyUSB0)${NC}"
            exit 1
        fi
        cd "$PROJECT_ROOT/sender"
        idf.py -p "$2" monitor
        ;;
    monitor-receiver)
        if [ -z "$2" ]; then
            echo -e "${RED}Error: Please specify port (e.g., /dev/ttyACM0)${NC}"
            exit 1
        fi
        cd "$PROJECT_ROOT/reciever"
        idf.py -p "$2" monitor
        ;;
    *)
        echo "Usage: $0 {build-sender|build-receiver|build-all|flash-sender|flash-receiver|monitor-sender|monitor-receiver} [port]"
        echo ""
        echo "Examples:"
        echo "  $0 build-all"
        echo "  $0 flash-sender /dev/ttyUSB0"
        echo "  $0 flash-receiver /dev/ttyACM0"
        echo "  $0 monitor-sender /dev/ttyUSB0"
        echo ""
        echo "Connected devices:"
        ls -la /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || echo "  No devices found"
        exit 1
        ;;
esac
