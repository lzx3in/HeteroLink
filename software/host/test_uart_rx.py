#!/usr/bin/env python3
import serial
import time
import sys

port = '/dev/ttyACM1'
baud = 115200

print(f"Opening {port} @ {baud}...")
ser = serial.Serial(port, baud, timeout=1)
time.sleep(2)

print("Reading UART data (10 seconds)...")
start = time.time()
total_bytes = 0
frames_found = 0

while time.time() - start < 10:
    data = ser.read(1024)
    if data:
        total_bytes += len(data)
        print(f"[{time.time()-start:.1f}s] Received {len(data)} bytes: {data[:50].hex()}")
        
        # Look for frame markers (0xAA start, 0x55 end)
        for i in range(len(data)-1):
            if data[i] == 0xAA and i+1 < len(data) and data[i+1] != 0xAA:
                frames_found += 1

print(f"\nSummary:")
print(f"  Total bytes: {total_bytes}")
print(f"  Frame markers found: {frames_found}")
print(f"  Average rate: {total_bytes/10:.1f} bytes/sec")

ser.close()
