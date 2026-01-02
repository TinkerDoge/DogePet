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

# =============================================================================
# MAIN
# =============================================================================

if __name__ == '__main__':
    print("\n" + "="*50)
    print("  DogePet Companion App")
    print("="*50)
    print("\n  Open http://localhost:5000 in your browser\n")
    
    # Run Flask server
    app.run(host='127.0.0.1', port=5000, debug=True, threaded=True)
