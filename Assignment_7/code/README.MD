# CLDP (Custom Lightweight Discovery Protocol)

Implementation of the CLDP protocol using raw sockets in C (POSIX standard). The protocol enables nodes in a closed network to announce their presence and query each other for application-level metadata, such as hostname, system time, and CPU load, without using transport layer protocols like TCP or UDP.

## Build and Run Instructions

### Building

Compile the source files using the following commands:

```bash
gcc -o client cldp_client.c
gcc -o server cldp_server.c
```

### Running

Since raw sockets require elevated privileges, run the executables using `sudo`.

1. **Start the Server:**

   Open a terminal and run:
   ```bash
   sudo ./server
   ```
   - The server listens for incoming CLDP packets on protocol 253.
   - It broadcasts a HELLO message every 10 seconds to announce its presence.

2. **Run the Client:**

   Open another terminal and run:
   ```bash
   sudo ./client
   ```
   - The client sends a broadcast QUERY requesting metadata (hostname and system time).
   - It then listens for RESPONSE messages from all active servers for a set time window (e.g., 3 seconds).

### Capturing Traffic

To capture CLDP traffic, you can use tcpdump on the appropriate network interface. For example, if running on the loopback interface:
```bash
sudo tcpdump -i lo "ip proto 253" -X
```

## Assumptions and Limitations

- **Network Environment:**  
  The protocol is designed for a closed and controlled network environment. Running the client and server on the same host might lead to duplicate packet receptions due to the loopback interface.

- **Raw Socket Usage:**  
  This implementation relies on raw sockets (`AF_INET`, `SOCK_RAW`) and requires elevated privileges (e.g., using `sudo`).

- **Metadata Support:**  
  The implementation supports a fixed set of metadata queries:  
  - Hostname (using `gethostname()`)
  - System Time (using `gettimeofday()`)
  - CPU Load 

- **Checksum Implementation:**  
  The IP header checksum function is provided as a skeleton and may need further improvements for production workloads.

- **Traffic Duplication:**  
  When multiple servers run on the same machine, the same HELLO or RESPONSE message might be received more than once by the client (due to broadcasting and loopback routing).

## Demo Output

Below is a sample demo output captured during testing:

### Server Output
```
dadi@RogStrix:~/Desktop/Labs_6th sem/Networks_Lab/Assignment_7/code$ make
gcc -o client cldp_client.c
gcc -o server cldp_server.c
sudo ./server
Server IP: 10.5.16.250
Server started. Listening for CLDP packets on protocol 253...
Checksum verification PASSED. Checksum: 0xa008
Sent HELLO (Trans ID: 45298) - 34 bytes
Checksum verification PASSED. Checksum: 0x3ab0
Sent HELLO (Trans ID: 64677) - 34 bytes
Received QUERY from 10.5.16.250 (Type: 1, Trans ID: 62128)
Checksum verification PASSED. Checksum: 0xdcf2
Sent RESPONSE (Type: 1, Data: RogStrix, Trans ID: 62128)
Checksum verification PASSED. Checksum: 0xae9f
Sent HELLO (Trans ID: 30280) - 34 bytes
Received QUERY from 10.5.16.250 (Type: 2, Trans ID: 16726)
Checksum verification PASSED. Checksum: 0x9488
```

### Client Output
```
dadi@RogStrix:~/Desktop/Labs_6th sem/Networks_Lab/Assignment_7/code$ sudo ./client
Client IP: 10.5.16.250

Enter metadata type to query (1: Hostname, 2: Time, 3: CPU Load, 0: Exit): 1
Sent QUERY for metadata type 1 (Trans ID: 45298)
Received RESPONSE (Trans ID: 45298): RogStrix

Enter metadata type to query (1: Hostname, 2: Time, 3: CPU Load, 0: Exit): 2
```

This output demonstrates that:
- The server is periodically broadcasting HELLO messages.
- The client successfully sends a QUERY and receives a RESPONSE containing the hostname.
- Debug messages help trace packet details and confirm correct transaction ID usage.

