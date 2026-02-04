#!/bin/bash
# helper-functions.sh - Shared utility functions for CVPipe AWS scripts

# Check if a file exists, exit with message if not
check_file() {
    local file=$1
    local message=$2
    
    if [ ! -f "$file" ]; then
        echo "ERROR: Required file not found: $file"
        if [ -n "$message" ]; then
            echo "$message"
        fi
        exit 1
    fi
}

# Check if a command exists
check_command() {
    local cmd=$1
    local message=$2
    
    if ! command -v $cmd &> /dev/null; then
        echo "ERROR: Required command not found: $cmd"
        if [ -n "$message" ]; then
            echo "$message"
        fi
        exit 1
    fi
}

# Format bytes to human readable
format_bytes() {
    local bytes=$1
    if [ $bytes -lt 1024 ]; then
        echo "${bytes}B"
    elif [ $bytes -lt 1048576 ]; then
        echo "$(($bytes / 1024))KB"
    elif [ $bytes -lt 1073741824 ]; then
        echo "$(($bytes / 1048576))MB"
    else
        echo "$(($bytes / 1073741824))GB"
    fi
}

# Calculate cost estimate
estimate_cost() {
    local hours=$1
    local instances=$2
    local rate=${3:-0.30}  # Default spot rate
    
    echo "scale=2; $hours * $instances * $rate" | bc
}

# Check AWS connectivity
check_aws() {
    if ! aws sts get-caller-identity &> /dev/null; then
        echo "ERROR: Cannot connect to AWS"
        echo "Run: aws configure"
        exit 1
    fi
}

# Wait for user confirmation
confirm() {
    local message=$1
    read -p "$message (y/n): " response
    if [ "$response" != "y" ]; then
        return 1
    fi
    return 0
}

# Display separator
separator() {
    echo "========================================"
}
