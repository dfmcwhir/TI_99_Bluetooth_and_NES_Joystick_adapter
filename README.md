<h2>Bluetooth and NES controller to TI99/4A joystick port adapter.</h2>
<p>
Uses Bluepad32 with the Arduino IDE on a WEMOS ESP32 D1 board. See https://bluepad32.readthedocs.io/en/latest/plat_arduino/ for Bluepad32 installation instructions. The NES gamepad code is based on the NESControllerInterface library by https://harryslab.net/nes-controller-breakout-board/ for the NES controllers </p>
<p>
Boards made by and available from PCBWay.com</p>

!Assembled.jpg

<h2>Theory of operation</h2>
<p>
The TI99/4A's joystick port scans the up,down,left,right, and fire signals using two joystick scan lines (one for each joystick), similar to the way it scans the keyboard rows and columns. So to interface with that circuit, the ESP32 control analog switch ICs (CD4066) to simulate the connection between the scan lines the input signals the same way a button being pressed on a stock controller would.</p>

<h2>Connecting the adapter</h2>
<p>In order to plug the adapter directly into the joystick port on the TI99, the two standoff on the DB9 of the adapter have to be removed. I couldn't find a source for a right angle DB9 that didn't have standoff on them. If plugging directly into the TI99, it would be recommended to have the TI99 on a flat, level surface and then put something under the adapter so that it is at the same level of the TI99. Otherwise there will be a fair amount of weight hanging off the connector and it might fall out or wear out the connectors. A better idea is probably to connect the adapter to the TI99 with a short DB9 cable.</p> 
<p>Power is supplied to the barrel jack from a 5V, center positive, wall wart. Something like what you might find for powering an old wifi router.</p>

<h2>Operation</h2>
<p>There are two push buttons and two switches on the adapter. The JButton is on the side of the adapter (with the NES ports facing you) and towards the back. That button is pressed for binding a bluetooth controller. Holding it for more than 2 seconds will cause the adapter to forget bound BT controllers. The Reset button is the side button closer to the front just resets the adapter. The switch on the top, closest to the power input jack is the power on-off switch. The other switch on the top, closer to the front of the adapter switches between using the NES gamepads or BT controllers. It is only checked on power up, so to switch between modes, set it to the mode you want and then power up the adapter.</p>
