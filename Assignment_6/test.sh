#!/bin/bash

# Test script for My_SMTP client and server
# Assumes mysmtp_client and mysmtp_server executables are in the current directory

# Configuration
SERVER_PORT=2525
SERVER_IP="127.0.0.1"
CLIENT="./mysmtp_client"
SERVER="./mysmtp_server"
MAILBOX_DIR="mailbox"
TEST_LOG="test_log.txt"
SERVER_PID=""

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Function to start the server
start_server() {
    $SERVER $SERVER_PORT > server_log.txt 2>&1 &
    SERVER_PID=$!
    sleep 1 # Give server time to start
    if ! ps -p $SERVER_PID > /dev/null; then
        echo -e "${RED}Failed to start server${NC}"
        exit 1
    fi
    echo "Server started with PID $SERVER_PID"
}

# Function to stop the server
stop_server() {
    if [ -n "$SERVER_PID" ]; then
        kill $SERVER_PID
        wait $SERVER_PID 2>/dev/null
        echo "Server stopped"
    fi
}

# Function to run a test case
run_test() {
    local test_name="$1"
    local input_file="$2"
    local expected_output="$3"
    local output_file="output_$test_name.txt"

    echo "Running test: $test_name"
    $CLIENT $SERVER_IP $SERVER_PORT < "$input_file" > "$output_file" 2>&1
    sleep 1 # Allow time for response

    if grep -q "$expected_output" "$output_file"; then
        echo -e "${GREEN}Test $test_name: PASSED${NC}"
    else
        echo -e "${RED}Test $test_name: FAILED${NC}"
        echo "Expected: $expected_output"
        echo "Got: $(cat $output_file)"
    fi
}

# Cleanup function
cleanup() {
    stop_server
    rm -rf $MAILBOX_DIR *.txt
}

# Trap to ensure cleanup on script exit
trap cleanup EXIT

# Start the server
start_server

# Create mailbox directory if it doesn't exist
mkdir -p $MAILBOX_DIR

# Test 1: Valid email sending workflow
cat > test1_input.txt << EOF
HELO test.com
MAIL FROM: alice@test.com
RCPT TO: bob@test.com
DATA
Hello Bob,
This is a test email.
.
QUIT
EOF
run_test "Valid_Email_Sending" "test1_input.txt" "250 Message stored successfully"

# Test 2: List emails for a recipient
cat > test2_input.txt << EOF
HELO test.com
LIST bob@test.com
QUIT
EOF
run_test "List_Emails" "test2_input.txt" "200 OK"

# Test 3: Retrieve a specific email
cat > test3_input.txt << EOF
HELO test.com
GET_MAIL bob@test.com 1
QUIT
EOF
run_test "Get_Email" "test3_input.txt" "200 OK"

# Test 4: Invalid command syntax
cat > test4_input.txt << EOF
HELO
QUIT
EOF
run_test "Invalid_HELO" "test4_input.txt" "400 ERR"

# Test 5: Out-of-order command (MAIL FROM before HELO)
cat > test5_input.txt << EOF
MAIL FROM: alice@test.com
QUIT
EOF
run_test "Out_of_Order_MAIL_FROM" "test5_input.txt" "403 FORBIDDEN"

# Test 6: Invalid email address (no @)
cat > test6_input.txt << EOF
HELO test.com
MAIL FROM: alice
QUIT
EOF
run_test "Invalid_Email_Address" "test6_input.txt" "400 ERR"

# Test 7: Empty input
cat > test7_input.txt << EOF

QUIT
EOF
run_test "Empty_Input" "test7_input.txt" "400 ERR"

# Test 8: Multiple emails to same recipient
cat > test8_input.txt << EOF
HELO test.com
MAIL FROM: charlie@test.com
RCPT TO: bob@test.com
DATA
Hi Bob,
This is another email.
.
LIST bob@test.com
QUIT
EOF
run_test "Multiple_Emails" "test8_input.txt" "200 OK"

# Test 9: Retrieve non-existent email
cat > test9_input.txt << EOF
HELO test.com
GET_MAIL bob@test.com 999
QUIT
EOF
run_test "Non_Existent_Email" "test9_input.txt" "401 NOT FOUND"

# Test 10: Quit immediately
cat > test10_input.txt << EOF
QUIT
EOF
run_test "Immediate_Quit" "test10_input.txt" "221 Goodbye"

echo "All tests completed."

# Cleanup handled by trap