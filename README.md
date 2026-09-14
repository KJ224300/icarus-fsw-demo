# ICARUS FSW Telemetry Demo

A demo-ready closed-loop telemetry pipeline for the ICARUS project.

The system simulates a CubeSat On-Board Computer (OBC) generating spacecraft telemetry and sends it through a Python telemetry bridge to NASA Open MCT for live visualization.

The demo includes:

- Thermal telemetry
- Vibration telemetry
- Battery telemetry
- 90-minute simulated LEO orbit cycle
- 60-minute sunlit phase
- 30-minute eclipse phase
- Manual thermal anomaly injection
- Manual battery anomaly injection
- Safe Mode alert through Open MCT

## System Architecture


```               
C OBC Simulator -----UDP :5000-------> Python Telemetry Bridge
                                      |
                                      | WebSocket :8765
                                      v
                                 NASA Open MCT
                                      |
                                      | HTTP :8080
                                      v
                                Web Browser
```
##The roles of each component are:

- C OBC Simulator — represents the spacecraft's onboard computer and generates telemetry.
- Python Telemetry Bridge — receives telemetry over UDP and forwards it over WebSockets.
- NASA Open MCT — provides the ground-side live telemetry visualization and anomaly alert.
- Project Structure
```
icarus-fsw-demo/
├── bridge/
│   ├── bridge.py
│   └── requirements.txt
├── obc/
│   └── src/
│       └── obc.c
├── openmct/
│   ├── dictionary.json
│   ├── dictionary-plugin.js
│   ├── realtime-telemetry-plugin.js
│   └── index.html
├── .gitignore
└── README.md
```

The compiled OBC executable is not stored in Git. It is generated locally during setup.
The Python virtual environment is also not stored in Git.

##Requirements

The project is intended to run on Linux.

- Install:

1. Git
2.GCC
3.Python 3
4.Python virtual environment support
5.Node.js
6.npm

- The Python bridge requires:

1. websockets==17.1


##Installation
1. Clone this repository

- Clone the ICARUS FSW repository and enter the project directory.

-git clone [<ICARUS_REPOSITORY_URL>](https://github.com/KJ224300/icarus-fsw-demo)
```
cd icarus-fsw-demo
```
2. Set up the Python telemetry bridge

Create a Python virtual environment:
```
python3 -m venv .venv
```
Activate it:
```
source .venv/bin/activate
```
Install the required Python dependency:
```
pip install -r bridge/requirements.txt
```
- Verify the installation:
```
python -c "import websockets; print(websockets.__version__)"
```
The expected version is:
```
17.1
```
- Open MCT Setup

The project uses the NASA Open MCT tutorial as the Open MCT base.
Clone the Open MCT tutorial separately from the ICARUS repository.
Then enter the Open MCT tutorial directory and install its dependencies:
```
npm install
```
The ICARUS repository contains the files that customize Open MCT for this telemetry demo.

From the Open MCT tutorial directory, copy the ICARUS files:
```
cp ../icarus-fsw-demo/openmct/dictionary.json .
cp ../icarus-fsw-demo/openmct/dictionary-plugin.js .
cp ../icarus-fsw-demo/openmct/realtime-telemetry-plugin.js .
cp ../icarus-fsw-demo/openmct/index.html .
```
NOTE: The Open MCT historical telemetry plugin is intentionally disabled.

This demo uses live telemetry through the Python WebSocket bridge and does not maintain a historical telemetry database.

Compile the C OBC Simulator

From the ICARUS project directory:
```
gcc obc/src/obc.c -o obc/bin/obc_sim -lm
```
This creates:
obc/bin/obc_sim

The executable is generated locally and is intentionally excluded from Git.

Running the System

The system uses three terminals.

##Start the components in this order:

1. Open MCT
2. Python telemetry bridge
3. C OBC simulator

1. Terminal 1 — Start Open MCT

Enter the Open MCT tutorial directory:
```
cd ~/openmct-tutorial
```
Start Open MCT:
```
npm start
```
Open the dashboard in a browser:
http://localhost:8080


2. Terminal 2 — Start the Python Bridge

Enter the ICARUS project:
```
cd ~/icarus-fsw-demo
```
Activate the Python environment:
```
source .venv/bin/activate
```
Start the bridge:
```
python bridge/bridge.py
```
The bridge listens for telemetry from the C OBC on: UDP :5000
and provides the live WebSocket connection on:WebSocket :8765


3. Terminal 3 — Start the C OBC

Enter the ICARUS project:
```
cd ~/icarus-fsw-demo
```
Run the OBC simulator:
```
./obc/bin/obc_sim
```
The simulator will begin generating telemetry once per simulated minute.
0.2 seconds represents one simulated minute.

##Telemetry

The OBC generates three telemetry measurements.

1. Thermal
ID: icarus.thermal
Unit: °C
2. Vibration
ID: icarus.vibration
Unit: g
3. Battery
ID: icarus.battery
Unit: %

Each telemetry packet is sent as JSON over UDP.

Example:
```
{
    "id": "icarus.thermal",
    "timestamp": 1234567890000,
    "value": 45.5
}
```
The Python bridge forwards the same telemetry data to Open MCT over WebSockets.

##Orbital Simulation

The OBC simulates a 90-minute Low Earth Orbit (LEO) cycle.

- Sunlit Phase

Minutes 1–60 represent the spacecraft being in sunlight.

During this phase:

Battery level increases.
Thermal level increases.

- Eclipse Phase

Minutes 61–90 represent the spacecraft being in eclipse.

During this phase:

Battery level decreases.
Thermal level decreases.

After minute 90, the simulator starts the next orbit.

The OBC displays the current orbit number and simulated minute in the terminal.

##Anomaly Injection

The OBC supports manual anomaly injection from the terminal.

1. Thermal Anomaly

Press:
```
t
```
The reported thermal telemetry is forced to: 90 °C

This exceeds the critical thermal threshold and causes the Open MCT Safe Mode alert to trigger.

2. Battery Anomaly

Press:
```
b
```
The reported battery telemetry is forced to: 0 %

This falls below the critical battery threshold and causes the Open MCT Safe Mode alert to trigger.

3. Clear Anomalies

Press:
```
r
```
This clears all injected anomalies.

The simulator then returns to reporting the current nominal simulated spacecraft state.

##Open MCT Dashboard

- The dashboard contains:

Thermal live plot
Vibration live plot
Battery live plot
ICARUS Safe Mode Alert condition widget

- The telemetry dictionary maps the following IDs:

icarus.thermal
icarus.vibration
icarus.battery

to their corresponding Open MCT measurements.

##Safe Mode Conditions

The Open MCT Condition Set is:

ICARUS Anomaly Conditions

The critical conditions are:

THERMAL CRITICAL
Thermal Temperature > 80 °C

and:

BATTERY CRITICAL
Battery Level < 20 %

When either condition is triggered, the output is:

CRITICAL ANOMALY: SATELLITE SAFE MODE

The nominal condition is:

NOMINAL

The Condition Widget is configured to display the condition output as its label.

Creating the Open MCT Dashboard

The Open MCT dashboard is stored in the browser's local storage.

Therefore, the dashboard layout is not automatically transferred when the repository is cloned.

If setting up the project on a new system, create the following in Open MCT:
```
My Items
└── ICARUS Telemetry Dashboard
    ├── Thermal
    ├── Vibration
    ├── Battery Level
    └── ICARUS SAFE MODE ALERT
```
Configure the three telemetry plots using the ICARUS telemetry dictionary.

Create the condition set:
ICARUS Anomaly Conditions

with:
Thermal Temperature > 80

and:
Battery Level < 20

The critical output should be:

CRITICAL ANOMALY: SATELLITE SAFE MODE

The nominal output should be:

NOMINAL

Demo Sequence

A typical demonstration can be performed as follows:

Start Open MCT.

1.Open the ICARUS Telemetry Dashboard.
2.Start the Python telemetry bridge.
3.Start the C OBC simulator.
4.Show live Thermal, Vibration and Battery telemetry.
5.Show the spacecraft progressing through the simulated orbital cycle.
6.Press t to inject a thermal anomaly.
7.Show the thermal value rising to 90 °C.
8.Show the Safe Mode alert triggering.
9.Press r to clear the anomaly.
10.Show the system returning to the current nominal state.
11.Press b to inject a battery anomaly.
12.Show the battery value dropping to 0%.
13.Show the Safe Mode alert triggering again.
14.Press r to clear the anomaly.
15.Network Ports
16.Connection	Protocol	Port
	C OBC → Python Bridge	UDP	5000	
	Python Bridge → Open MCT	WebSocket	8765
	Browser → Open MCT	HTTP	8080

All components run locally on the same machine using:
127.0.0.1

##Troubleshooting
- Open MCT does not show telemetry

- Check that all three components are running:

	Open MCT
	Python Bridge
	C OBC

- Check that the Python bridge reports:

	UDP receiver listening on 127.0.0.1:5000
	WebSocket server listening on ws://localhost:8765

- Check that the C OBC is running and displaying telemetry.

- Python reports that websockets is missing

	Make sure the virtual environment is activated:
```
	source .venv/bin/activate
```
- Then install the dependencies:
```
pip install -r bridge/requirements.txt
C OBC does not compile
```
- Make sure GCC is installed:
```
gcc --version
```
Compile using:
```
gcc obc/src/obc.c -o obc/bin/obc_sim -lm
```
- The -lm option links the math library required by the vibration calculation.

- Open MCT shows a history/request error
	The ICARUS demo does not use the Open MCT historical telemetry server.
	Make sure the historical telemetry plugin remains disabled in:
```
openmct/index.html
```
- The realtime telemetry plugin should remain enabled.

##Limitations

This project is a demonstration simulator and is not flight-qualified spacecraft software.

The orbital and spacecraft physics are intentionally simplified.

The simulation uses simple mathematical models for:

Battery charging and discharging
Thermal variation
Vibration

The primary purpose of the demonstration is to show:
```
OBC telemetry generation
        ↓
Telemetry transport
        ↓
Live ground visualization
        ↓
Anomaly detection
        ↓
Safe Mode response
```
##Repository Contents

The repository contains:

C OBC telemetry generator
Python UDP/WebSocket telemetry bridge
Open MCT telemetry dictionary
Open MCT realtime telemetry plugin
Open MCT configuration
Setup and execution instructions

Generated files such as the Python virtual environment and compiled OBC executable are excluded using .gitignore.

