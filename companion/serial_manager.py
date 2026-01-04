import serial
import serial.tools.list_ports
import time
import json
import threading
from typing import Optional, Dict, Any, Callable

class SerialManager:
    def __init__(self):
        self.port: Optional[serial.Serial] = None
        self.connected = False
        self.port_name = ""
        self.lock = threading.RLock()
        self.running = False
        self.thread: Optional[threading.Thread] = None
        self.log_buffer = []
        
        # Callbacks
        self.on_config_received: Optional[Callable[[Dict], None]] = None
        self.last_config = {}

    def list_ports(self):
        return [p.device for p in serial.tools.list_ports.comports()]

    def connect(self, port_name: str) -> bool:
        self.disconnect()
        try:
            self.port = serial.Serial(port_name, 115200, timeout=1)
            self.port_name = port_name
            self.connected = True
            self.running = True
            self.thread = threading.Thread(target=self._read_loop, daemon=True)
            self.thread.start()
            return True
        except Exception as e:
            print(f"Error connecting to {port_name}: {e}")
            return False

    def disconnect(self):
        self.running = False
        if self.thread:
            self.thread.join(timeout=1.0)
        if self.port:
            try:
                self.port.close()
            except:
                pass
            self.port = None
        self.connected = False

    def send(self, data: str):
        if not self.connected or not self.port:
            return
        try:
            with self.lock:
                self.port.write((data + "\n").encode('utf-8'))
        except Exception as e:
            print(f"Write error: {e}")
            self.disconnect()

    def send_command(self, cmd_key: str, payload: Dict = None):
        """Send a JSON command or a simple string command"""
        if payload:
            payload["cmd"] = cmd_key # Inject cmd key if needed for consistency
            self.send(json.dumps(payload))
        else:
            self.send(cmd_key) # Just send the string (e.g., "get_config")

    def _read_loop(self):
        while self.running and self.port:
            try:
                if self.port.in_waiting:
                    line = self.port.readline().decode('utf-8', errors='ignore').strip()
                    if not line:
                        continue
                    
                    # Store log
                    self.log_buffer.append(line)
                    if len(self.log_buffer) > 100:
                        self.log_buffer.pop(0)
                        
                    # Parse JSON
                    if line.startswith('{') and line.endswith('}'):
                        try:
                            data = json.loads(line)
                            
                            # Check for config response
                            if data.get("type") == "config":
                                self.last_config = data.get("data", {})
                                if self.on_config_received:
                                    self.on_config_received(self.last_config)
                                    
                        except json.JSONDecodeError:
                            pass
                else:
                    time.sleep(0.01)
            except Exception as e:
                print(f"Read error: {e}")
                self.connected = False
                break

serial_manager = SerialManager()
