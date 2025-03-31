# CLDP (Custom Lightweight Discovery Protocol) - Assignment 7

**Course:** CS39006 Networks Laboratory  
**Author:** Dadi Sasank Kumar (22CS10020)  
**Semester:** Spring 2024-25  

## Description

This project implements a Custom Lightweight Discovery Protocol (CLDP) using raw sockets in C (POSIX standard). It operates directly over IPv4 (using custom protocol number 253), bypassing TCP/UDP.

The protocol allows nodes in a closed network environment to:
1. Announce their presence using periodic **HELLO** messages (sent by servers).
2. Query other nodes for specific metadata using **QUERY** messages (sent by clients).
3. Respond to queries with the requested metadata using **RESPONSE** messages (sent by servers).

Supported metadata queries include:
- Hostname
- System Time (Unix timestamp with microseconds)
- CPU Load (1-minute average)

The implementation includes:
- `cldp_server.c`: Listens for queries, responds with metadata, and sends periodic HELLO broadcasts.
- `cldp_client.c`: Allows users to send broadcast queries for specific metadata and displays responses received within a timeout window.

## Prerequisites

- Linux Operating System (tested on Ubuntu/Debian-based systems)
- GCC Compiler (or compatible C compiler)
- `sudo` privileges (for using raw sockets)

## Build Instructions

Compile the client and server using GCC. Open a terminal in the project directory and run:

```bash
gcc -o cldp_client cldp_client.c
gcc -o cldp_server cldp_server.c
```

This will create two executable files: `cldp_client` and `cldp_server`.

## Run Instructions

Raw sockets require elevated privileges. Use `sudo` to run the programs.

### Start the Server
Open a terminal and run:

```bash
sudo ./cldp_server
```

The server will start, print its IP, begin sending HELLO messages every 10 seconds, and listen for QUERY packets. You can run multiple servers on different machines (or the same machine for testing).

### Start the Client
Open another terminal and run:

```bash
sudo ./cldp_client
```

The client will prompt you to enter the type of metadata you want to query (1, 2, or 3). After you enter a number, it will send a broadcast QUERY and wait for RESPONSE packets for a few seconds, displaying any it receives.

## Capturing Traffic (Optional but Recommended)

Use `tcpdump` to monitor the CLDP traffic (IP Protocol 253). You'll need `sudo`.

- On the physical network interface (replace `enp3s0` with your interface name, find using `ip addr` or `tcpdump -D`):

```bash
# Show packets summary and payload (hex/ASCII)
sudo tcpdump -i enp3s0 -v 'ip proto 253' -X

# Save packets to a file for Wireshark analysis
sudo tcpdump -i enp3s0 -w cldp_capture.pcap 'ip proto 253'
```

This is useful for seeing broadcasts (HELLO, QUERY) and responses exchanged between different machines.

- On the loopback interface (`lo`):

```bash
sudo tcpdump -i lo -v 'ip proto 253' -X
```

This is often necessary to see RESPONSE packets when running the client and server on the same machine, as the OS may route unicast traffic locally. Stop `tcpdump` with `Ctrl+C`.

## Assumptions and Limitations

- **Trusted Network:** Designed for closed/trusted LANs. No security (authentication, encryption) is implemented.
- **IPv4 Only:** The protocol and implementation work only over IPv4.
- **Root Privileges:** `sudo` is required due to raw socket usage.
- **Basic Error Handling:** Includes checks for major system call failures but is not robust against all network errors or malformed packets. Client input validation is basic.
- **Payload Size:** CLDP payload is limited to 255 bytes (1-byte length field).
- **Loopback Behavior:** When client and server run on the same machine, unicast RESPONSE packets may only appear on the `lo` interface, not the physical one. Broadcasts may appear on both.

## Demonstration Output (Example)

This shows a scenario with one server and one client running on the same machine (10.5.16.250). (detailed output is in capture.txt)

### Terminal 1 (Server)

```bash
sudo ./cldp_server
Assignment 7: CLDP Server
Server IP: 10.5.16.250
Server started. Listening for CLDP packets (protocol 253)...
Sent HELLO (Trans ID: 22233, Payload: 'HELLO')
Received QUERY from 10.5.16.250 (Type: 1, Trans ID: 20178) # <-- Query from Client
Sent RESPONSE to 10.5.16.250 (Type: 1, Data: 'RogStrix', Trans ID: 20178) # <-- Response Sent
Sent HELLO (Trans ID: 31483, Payload: 'HELLO')
Received QUERY from 10.5.16.250 (Type: 2, Trans ID: 46499) # <-- Query from Client
Sent RESPONSE to 10.5.16.250 (Type: 2, Data: '2025-03-31 16:44:43.620388', Trans ID: 46499) # <-- Response Sent
Sent HELLO (Trans ID: 27718, Payload: 'HELLO')
... (more HELLOs) ...
```

### Terminal 2 (Client)

```bash
sudo ./cldp_client
Assignment 7: CLDP Client
Client IP: 10.5.16.250

Available Metadata Query Types:
    1: Hostname
    2: System Time (Timestamp)
    3: CPU Load (1-min avg)
Enter metadata type number to query (or 0 to Exit): 1 # <-- User enters 1
Sent QUERY for metadata type 1 (Trans ID: 20178) # <-- Query Sent
Waiting for RESPONSE(s)...
Received RESPONSE from 10.5.16.250 (Trans ID: 20178): RogStrix # <-- Response Received

Available Metadata Query Types:
    1: Hostname
    2: System Time (Timestamp)
    3: CPU Load (1-min avg)
Enter metadata type number to query (or 0 to Exit): 2 # <-- User enters 2
Sent QUERY for metadata type 2 (Trans ID: 46499) # <-- Query Sent
Waiting for RESPONSE(s)...
Received RESPONSE from 10.5.16.250 (Trans ID: 46499): 2025-03-31 16:44:43.620388 # <-- Response Received

Available Metadata Query Types:
    1: Hostname
    2: System Time (Timestamp)
    3: CPU Load (1-min avg)
Enter metadata type number to query (or 0 to Exit): 0 # <-- User enters 0
Exiting...
Client shutting down.
```
