#!/bin/bash

# Configuration
NUM_CLIENTS=10
KEY="DEFPRTVWLMZAYGHQSIUJXKBCNO"
TEST_DIR="test"

# Create test directory if it doesn't exist
mkdir -p "$TEST_DIR"

# Create sample files
echo "Creating sample files in $TEST_DIR/..."
for i in $(seq 1 $NUM_CLIENTS); do
    cat > "$TEST_DIR/input$i.txt" << EOF
This is test file $i
Testing encryption
Multiple lines of text
For client number $i
EOF
done

# Compile the client if needed
if [ ! -f "./client" ]; then
    echo "Compiling client..."
    gcc -o client client.c
    if [ $? -ne 0 ]; then
        echo "Compilation failed!"
        exit 1
    fi
fi

# Function to run a single client
run_single_client() {
    client_num=$1
    (
        echo "$TEST_DIR/input$client_num.txt"
        echo "$KEY"
        echo "No"
    ) | ./client
}

# Run multiple clients simultaneously
echo "Starting $NUM_CLIENTS clients..."
for i in $(seq 1 $NUM_CLIENTS); do
    run_single_client $i &
done

# Wait for all background processes to complete
wait

# # Check results
# echo -e "\nResults:"
# echo "Original files:"
# ls -l $TEST_DIR/input*.txt
# echo -e "\nEncrypted files:"
# ls -l $TEST_DIR/input*.txt.enc