"""
DogePet Companion App - Serial Communication Module
Handles USB Serial communication with ESP32 device
"""

import serial
import serial.tools.list_ports
import json
import threading
import time
import collections
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
        self.log_buffer = collections.deque(maxlen=50) # Buffer last 50 lines
        
        # Passive Sensor Caching
        self.sensor_cache = {
            "vbat": 0.0,
            "vbat_pct": 0,
            "mic_db": 0,
            "ax": 0.0, "ay": 0.0, "az": 0.0,
            "gx": 0.0, "gy": 0.0, "gz": 0.0,
            "qw": 1.0, "qx": 0.0, "qy": 0.0, "qz": 0.0, # DMP Quaternion
            "last_update": 0
        }

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
            # ESP32-S3 native USB (Espressif VID: 0x303A)
            if p.vid == 0x303A:
                return p.device
            # Common USB-to-Serial chips used with ESP32
            if "CP210" in p.description or "CH340" in p.description or \
               "ESP32" in p.description or "USB-SERIAL" in p.description:
                return p.device
        # Fallback: any USB Serial Device
        for p in ports:
            if "USB Serial" in p.description:
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
                self.log_buffer.clear() # Clear logs on new connection
                
                # Start read thread
                self.running = True
                self.read_thread = threading.Thread(target=self._read_loop, daemon=True)
                self.read_thread.start()
                
                # Wait for device ready (ESP32 might reset on DTR)
                time.sleep(2.0)
                
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
                # Drain input buffer to logs (don't delete data!)
                while self.port.in_waiting > 0:
                    try:
                        line = self.port.readline().decode('utf-8').strip()
                        if line:
                            self.log_buffer.append(line)
                    except Exception:
                        pass

                # Send command
                cmd_str = json.dumps(cmd) + "\n"
                self.port.write(cmd_str.encode('utf-8'))
                self.port.flush()
                
                # Read response (with timeout)
                # Helper to read non-empty line
                response_line = None
                start_t = time.time()
                while (time.time() - start_t) < 1.0:
                    if self.port.in_waiting > 0:
                        line = self.port.readline().decode('utf-8').strip()
                        if line:
                            self.log_buffer.append(f"< {line}") # Log the response too
                            response_line = line
                            break
                    time.sleep(0.005)
                
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
        return self.send_command({"cmd": "get_eyes"}) or {"status": "error"}
    
    def trigger_action(self, action: str, value: Any = None) -> Dict[str, Any]:
        """Trigger an animation action"""
        cmd = {"cmd": "action", "type": action}
        if value is not None:
            cmd["value"] = value
        return self.send_command(cmd) or {"status": "error"}

    def get_pinout(self) -> Dict[str, Any]:
        """Get current pinout from device"""
        return self.send_command({"cmd": "get_pinout"}) or {"status": "error"}

    def set_pinout(self, pinout: Dict[str, Any]) -> Dict[str, Any]:
        """Update pinout settings"""
        cmd = {"cmd": "set_pinout", **pinout}
        return self.send_command(cmd) or {"status": "error"}

    def get_sensors(self) -> Dict[str, Any]:
        """Get live sensor data (Returns cached values from passive stream)"""
        # NO COMMAND SENT - Passive reading
        data = self.sensor_cache.copy()
        data["status"] = "ok"
        return data
    
    def get_logs(self) -> list:
        """Get recent collected logs"""
        return list(self.log_buffer)

    def get_config(self) -> Dict[str, Any]:
        """Get runtime config from device"""
        return self.send_command({"cmd": "get_config"}) or {"status": "error"}

    def set_config(self, config: Dict[str, Any]) -> Dict[str, Any]:
        """Update runtime config"""
        cmd = {"cmd": "set_config", **config}
        return self.send_command(cmd) or {"status": "error"}

    def _read_loop(self):
        """Background thread for reading events from device"""
        while self.running:
            try:
                if self.port and self.port.is_open and self.port.in_waiting:
                    with self.lock:
                        line_bytes = self.port.readline()
                        
                    try:
                        line = line_bytes.decode('utf-8').strip()
                    except:
                        line = ""

                    if line:
                        self.log_buffer.append(line)
                        try:
                            data = json.loads(line)
                            
                            # INTERCEPT SENSOR DATA from Firmware Stream
                            if data.get("type") == "imu":
                                # Scale Raw Integers to Floats
                                self.sensor_cache["ax"] = data.get("ax", 0) / 16384.0
                                self.sensor_cache["ay"] = data.get("ay", 0) / 16384.0
                                self.sensor_cache["az"] = data.get("az", 0) / 16384.0
                                
                                self.sensor_cache["gx"] = data.get("gx", 0) / 131.0
                                self.sensor_cache["gy"] = data.get("gy", 0) / 131.0
                                self.sensor_cache["gz"] = data.get("gz", 0) / 131.0
                                
                                self.sensor_cache["last_update"] = time.time()
                            
                            # Update other sensors
                            if "vbat" in data: self.sensor_cache["vbat"] = data["vbat"]
                            if "vbat_pct" in data: self.sensor_cache["vbat_pct"] = data["vbat_pct"]
                            if "mic_db" in data: self.sensor_cache["mic_db"] = data["mic_db"]

                            # Handle events
                            if "event" in data and self.on_event:
                                self.on_event(data)
                        except json.JSONDecodeError:
                            pass
                else:
                    time.sleep(0.005)
            except Exception as e:
                # Keep thread alive
                pass


# Singleton instance
serial_comm = SerialComm()
