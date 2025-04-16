let isConnected = false;

async function connect() {
    const ip = document.getElementById('robotIP').value;
    const port = document.getElementById('robotPort').value;
    
    try {
        const response = await fetch('/connect', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({
                ip: ip,
                port: parseInt(port)
            })
        });

        const data = await response.json();
        const statusDiv = document.getElementById('connectionStatus');
        
        if (response.ok) {
            isConnected = true;
            statusDiv.className = 'status success';
            statusDiv.textContent = 'Connected successfully';
        } else {
            statusDiv.className = 'status error';
            statusDiv.textContent = 'Connection failed: ' + data;
        }
    } catch (error) {
        document.getElementById('connectionStatus').className = 'status error';
        document.getElementById('connectionStatus').textContent = 'Connection failed: ' + error;
    }
}

async function drive(direction) {
    if (!isConnected) {
        alert('Please connect to the robot first');
        return;
    }

    const duration = document.getElementById('duration').value;
    const speed = document.getElementById('speed').value;
    const directionMap = {
        'forward': 1,
        'backward': 2,
        'left': 3,
        'right': 4
    };

    try {
        const response = await fetch('/telecommand', {
            method: 'PUT',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({
                command: 'drive',
                direction: directionMap[direction],
                duration: parseInt(duration),
                speed: parseInt(speed)
            })
        });

        if (!response.ok) {
            throw new Error('Command failed');
        }
    } catch (error) {
        alert('Error sending command: ' + error);
    }
}

async function sleep() {
    if (!isConnected) {
        alert('Please connect to the robot first');
        return;
    }

    try {
        const response = await fetch('/telecommand', {
            method: 'PUT',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({
                command: 'sleep'
            })
        });

        if (!response.ok) {
            throw new Error('Sleep command failed');
        }
    } catch (error) {
        alert('Error sending sleep command: ' + error);
    }
}

async function requestTelemetry() {
    if (!isConnected) {
        alert('Please connect to the robot first');
        return;
    }

    try {
        const response = await fetch('/telemetry_request');
        const data = await response.json();
        
        const telemetryDiv = document.getElementById('telemetryData');
        telemetryDiv.innerHTML = `
            <p>Last Packet Counter: ${data.last_pkt_counter}</p>
            <p>Current Grade: ${data.current_grade}</p>
            <p>Hit Count: ${data.hit_count}</p>
            <p>Last Command: ${data.last_cmd}</p>
            <p>Last Command Value: ${data.last_cmd_value}</p>
            <p>Last Command Speed: ${data.last_cmd_speed}</p>
        `;
    } catch (error) {
        alert('Error getting telemetry: ' + error);
    }
} 