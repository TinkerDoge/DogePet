from flask import Flask, render_template, request, jsonify
from flask_socketio import SocketIO, emit
import serial
import serial.tools.list_ports
import json
import time
import threading

app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*", async_mode='threading')

# Serial Port Configuration
SERIAL_PORT = None
BAUD_RATE = 115200
ser = None
is_connected = False
read_thread = None

def read_from_serial():
    global ser, is_connected
    while is_connected and ser and ser.is_open:
        try:
            line = ser.readline().decode('utf-8').strip()
            if line:
                print(f"Received: {line}")
                try:
                    data = json.loads(line)
                    # explicitly provide namespace if needed, but default / is usually fine
                    socketio.emit('serial_data', data)
                    print("Emitted serial_data")
                except json.JSONDecodeError:
                    socketio.emit('serial_log', {'msg': line})
                    print("Emitted serial_log")
        except Exception as e:
            print(f"Serial read error: {e}")
            disconnect_serial()
            break

def disconnect_serial():
    global ser, is_connected
    if ser and ser.is_open:
        ser.close()
    ser = None
    is_connected = False
    socketio.emit('status', {'connected': False})
    print("Disconnected from serial")

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/ports')
def get_ports():
    ports = [p.device for p in serial.tools.list_ports.comports()]
    return jsonify(ports)

@app.route('/api/connect', methods=['POST'])
def connect():
    global ser, is_connected, read_thread
    data = request.json
    port = data.get('port')
    
    if is_connected:
        disconnect_serial()
        
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
        is_connected = True
        read_thread = threading.Thread(target=read_from_serial)
        read_thread.daemon = True
        read_thread.start()
        return jsonify({'status': 'ok', 'msg': f'Connected to {port}'})
    except Exception as e:
        return jsonify({'status': 'error', 'msg': str(e)}), 500

@app.route('/api/disconnect', methods=['POST'])
def disconnect():
    disconnect_serial()
    return jsonify({'status': 'ok'})

@app.route('/api/send', methods=['POST'])
def send_command():
    global ser
    if not is_connected or not ser:
        return jsonify({'status': 'error', 'msg': 'Not connected'}), 400
        
    data = request.json
    cmd_str = json.dumps(data) + '\n'
    try:
        ser.write(cmd_str.encode('utf-8'))
        return jsonify({'status': 'ok'})
    except Exception as e:
        return jsonify({'status': 'error', 'msg': str(e)}), 500

if __name__ == '__main__':
    socketio.run(app, debug=True, port=5000)
