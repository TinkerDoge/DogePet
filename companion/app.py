from flask import Flask, jsonify, request, send_from_directory
from serial_manager import serial_manager
import time
import json

app = Flask(__name__)

@app.route('/')
def index():
    return send_from_directory('static', 'index.html')

@app.route('/<path:path>')
def static_file(path):
    return send_from_directory('static', path)

@app.route('/api/ports')
def get_ports():
    return jsonify(serial_manager.list_ports())

@app.route('/api/connect', methods=['POST'])
def connect():
    port = request.json.get('port')
    if serial_manager.connect(port):
        # Request config immediately
        time.sleep(2) # Wait for DTR reset
        serial_manager.send_command("get_config")
        return jsonify({"status": "ok"})
    return jsonify({"status": "error", "msg": "Failed to connect"})

@app.route('/api/status')
def status():
    return jsonify({
        "connected": serial_manager.connected,
        "logs": serial_manager.log_buffer[-20:]
    })

@app.route('/api/config', methods=['GET'])
def get_config():
    # Trigger a refresh
    if serial_manager.connected:
        serial_manager.send_command("get_config")
        time.sleep(0.2) # wait briefly for response
    return jsonify(serial_manager.last_config)

@app.route('/api/config', methods=['POST'])
def set_config():
    if not serial_manager.connected:
        return jsonify({"status": "error"})
    
    # Send set_config command
    # Payload should look like {"cmd": "set_config", ...config keys...}
    # User sends us the config object directly
    
    data = request.json
    data["cmd"] = "set_config" # Ensure cmd key exists for firmware logging/parsing if needed
    
    # Actually firmware checks "set_config" substring, then parses the whole line.
    # So we just send the JSON.
    serial_manager.send(json.dumps(data))
    
    return jsonify({"status": "ok"})

@app.route('/api/reboot', methods=['POST'])
def reboot():
    serial_manager.send("reboot")
    return jsonify({"status": "ok"})

@app.route('/api/reset', methods=['POST'])
def factory_reset():
    serial_manager.send("factory_reset")
    return jsonify({"status": "ok"})

if __name__ == '__main__':
    app.run(debug=True, port=5000)
