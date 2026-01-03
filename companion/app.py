"""
DogePet Companion App - Flask Web Server
Provides REST API and serves the settings UI
"""

from flask import Flask, jsonify, request, send_from_directory
from serial_comm import serial_comm
import os

app = Flask(__name__, static_folder='static', static_url_path='')

# =============================================================================
# STATIC FILES
# =============================================================================

@app.route('/')
def index():
    """Serve the main settings page"""
    return send_from_directory('static', 'index.html')

# =============================================================================
# API ENDPOINTS
# =============================================================================

@app.route('/api/ports', methods=['GET'])
def get_ports():
    """List available COM ports"""
    ports = serial_comm.list_ports()
    auto_port = serial_comm.find_esp32()
    return jsonify({
        "ports": ports,
        "suggested": auto_port,
        "connected": serial_comm.connected,
        "current_port": serial_comm.port_name
    })

@app.route('/api/connect', methods=['POST'])
def connect():
    """Connect to specified COM port"""
    data = request.get_json() or {}
    port = data.get('port')
    
    if not port:
        # Try auto-detect
        port = serial_comm.find_esp32()
        if not port:
            return jsonify({"status": "error", "msg": "No port specified and auto-detect failed"})
    
    success = serial_comm.connect(port)
    if success:
        # Get initial settings from device
        settings = serial_comm.get_settings()
        return jsonify({"status": "ok", "port": port, "settings": settings})
    else:
        return jsonify({"status": "error", "msg": f"Failed to connect to {port}"})

@app.route('/api/disconnect', methods=['POST'])
def disconnect():
    """Disconnect from device"""
    serial_comm.disconnect()
    return jsonify({"status": "ok"})

@app.route('/api/status', methods=['GET'])
def get_status():
    """Get connection status"""
    return jsonify({
        "connected": serial_comm.connected,
        "port": serial_comm.port_name
    })

@app.route('/api/eyes', methods=['GET'])
def get_eyes():
    """Get current eye settings"""
    if not serial_comm.connected:
        return jsonify({"status": "error", "msg": "Not connected"})
    
    result = serial_comm.get_settings()
    return jsonify(result)

@app.route('/api/eyes', methods=['POST'])
def set_eyes():
    """Update eye settings"""
    if not serial_comm.connected:
        return jsonify({"status": "error", "msg": "Not connected"})
    
    data = request.get_json() or {}
    result = serial_comm.set_eyes(data)
    return jsonify(result)

@app.route('/api/action', methods=['POST'])
def trigger_action():
    """Trigger an animation action"""
    if not serial_comm.connected:
        return jsonify({"status": "error", "msg": "Not connected"})
    
    data = request.get_json() or {}
    action = data.get('action', 'blink')
    result = serial_comm.trigger_action(action)
    return jsonify(result)

@app.route('/api/pinout', methods=['GET', 'POST'])
def handle_pinout():
    """Get or Set hardware pinout"""
    if not serial_comm.connected:
        return jsonify({"status": "error", "msg": "Not connected"})
    
    if request.method == 'GET':
        return jsonify(serial_comm.get_pinout())
    else:
        data = request.get_json() or {}
        return jsonify(serial_comm.set_pinout(data))

@app.route('/api/sensors', methods=['GET'])
def handle_sensors():
    """Get sensor data (battery, mic, imu)"""
    if not serial_comm.connected:
        return jsonify({"status": "error", "msg": "Not connected"})
    return jsonify(serial_comm.get_sensors())

@app.route('/api/logs', methods=['GET'])
def get_logs():
    """Get recent serial logs"""
    if not serial_comm.connected:
        return jsonify({"status": "error", "msg": "Not connected"})
    
    return jsonify({
        "status": "ok", 
        "logs": serial_comm.get_logs()
    })

# =============================================================================
# MAIN
# =============================================================================

if __name__ == '__main__':
    print("\n" + "="*50)
    print("  DogePet Companion App")
    print("="*50)
    print("\n  Open http://localhost:5000 in your browser\n")
    
    # Run Flask server
    # Watch static files for changes too so server restarts on JS/CSS updates
    extra_files = []
    if os.path.isdir('static'):
        for dirname, dirs, files in os.walk('static'):
            for filename in files:
                filename = os.path.join(dirname, filename)
                if os.path.isfile(filename):
                    extra_files.append(filename)

    app.run(
        host='127.0.0.1', 
        port=5000, 
        debug=True, 
        threaded=True, 
        use_reloader=True, 
        extra_files=extra_files
    )
