import serial.tools.list_ports

print("Searching for COM ports...")
ports = serial.tools.list_ports.comports()

if not ports:
    print("No ports found!")
else:
    for p in ports:
        print(f"Found: {p.device}")
        print(f"  - Desc: {p.description}")
        print(f"  - HWID: {p.hwid}")
        
print("\nDone.")
