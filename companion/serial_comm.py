"""
DogePet Companion App - Serial Communication Module
Handles USB Serial communication with ESP32 device
"""

import serial
import serial.tools.list_ports
import json
import threading
import time
from typing import Optional, Callable, Dict, Any

class SerialComm:
    """Manages serial communication with DogePet device"""
    
    def __init__(self):
        self.port: Optional[serial.Serial] = None
        self.connected = False
        self.port_name = ""
        self.lock = threading.Lock()
        self.read_thread: Optional[threading.Thread] = None
        self.running = False
        self.on_event: Optional[Callable[[Dict[str, Any]], None]] = None
        
    @staticmethod
    def list_ports() -> list:
        """List available COM ports"""
        ports = serial.tools.list_ports.comports()
        return [
            {
                "port": p.device,
                "description": p.description,
                "hwid": p.hwid
            }
            for p in ports
        ]
    
    @staticmethod
    def find_esp32() -> Optional[str]:
        """Auto-detect ESP32 COM port"""
        ports = serial.tools.list_ports.comports()
        for p in ports:
            # Common ESP32 identifiers
            if "CP210" in p.description or "CH340" in p.description or \
               "USB Serial" in p.description or "ESP32" in p.description or \
               "USB-SERIAL" in p.description:
                return p.device
        return None
    
    def connect(self, port: str, baudrate: int = 115200) -> bool:
        """Connect to specified COM port"""
        try:
            with self.lock:
                if self.port and self.port.is_open:
                    self.port.close()
                
                self.port = serial.Serial(
                    port=port,
                    baudrate=baudrate,
                    timeout=1.0,
                    write_timeout=1.0
                )
                self.port_name = port
                self.connected = True
                
                # Start read thread
                self.running = True
                self.read_thread = threading.Thread(target=self._read_loop, daemon=True)
                self.read_thread.start()
                
                # Wait for device ready
                time.sleep(0.5)
                
                print(f"[Serial] Connected to {port}")
                return True
                
        except Exception as e:
            print(f"[Serial] Connection error: {e}")
            self.connected = False
            return False
    
    def disconnect(self):
        """Disconnect from device"""
        self.running = False
        with self.lock:
            if self.port and self.port.is_open:
                self.port.close()
            self.port = None
            self.connected = False
            self.port_name = ""
        print("[Serial] Disconnected")
    
    def send_command(self, cmd: Dict[str, Any]) -> Optional[Dict[str, Any]]:
        """Send JSON command and wait for response"""
        if not self.connected or not self.port:
            return {"status": "error", "msg": "Not connected"}
        
        try:
            with self.lock:
                # Clear input buffer
                self.port.reset_input_buffer()
                
                # Send command
                cmd_str = json.dumps(cmd) + "\n"
                self.port.write(cmd_str.encode('utf-8'))
                self.port.flush()
                
                # Read response (with timeout)
                response_line = self.port.readline().decode('utf-8').strip()
                
                if response_line:
                    return json.loads(response_line)
                else:
                    return {"status": "error", "msg": "No response"}
                    
        except json.JSONDecodeError as e:
            return {"status": "error", "msg": f"Invalid JSON response: {e}"}
        except Exception as e:
            print(f"[Serial] Send error: {e}")
            self.connected = False
            return {"status": "error", "msg": str(e)}
    
    def set_eyes(self, settings: Dict[str, Any]) -> Dict[str, Any]:
        """Update eye settings"""
        cmd = {"cmd": "set_eyes", **settings}
        return self.send_command(cmd) or {"status": "error"}
    
    def get_settings(self) -> Dict[str, Any]:
        """Get current settings from device"""
        return self.send_command({"cmd": "get_settings"}) or {"status": "error"}
    
    def trigger_action(self, action: str) -> Dict[str, Any]:
        """Trigger an animation action"""
        return self.send_command({"cmd": "action", "type": action}) or {"status": "error"}
    
    def _read_loop(self):
        """Background thread for reading events from device"""
        while self.running:
            try:
                if self.port and self.port.is_open and self.port.in_waiting:
                    with self.lock:
                        line = self.port.readline().decode('utf-8').strip()
                    if line:
                        try:
                            data = json.loads(line)
                            # Handle events (like button presses)
                            if "event" in data and self.on_event:
                                self.on_event(data)
                        except json.JSONDecodeError:
                            pass
                else:
                    time.sleep(0.01)  # Small delay to prevent CPU spinning
            except Exception as e:
                if self.running:
                    print(f"[Serial] Read error: {e}")
                    self.connected = False
                break


# Singleton instance
serial_comm = SerialComm()
