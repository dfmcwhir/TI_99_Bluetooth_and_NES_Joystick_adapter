Bluetooth and NES controller to TI99/4A joystick port adapter.

Uses Bluepad32 with the Arduino IDE on a WEMOS ESP32 D1 board. See https://bluepad32.readthedocs.io/en/latest/plat_arduino/ for Bluepad32 installation instructions. It uses the NESControllerInterface library by https://harryslab.net/nes-controller-breakout-board/ for the NES controllers

Boards made by and available from PCBWay.com


The TI99/4A's joystick port scans the up,down,left,right, and fire signals using two joystick scan lines (one of each joystick), similar to the way it scans the keyboard rows and columns. So to interface with that circuit, the ESP32 controllers analog switch ICs (CD4066) to simulate the connection between the scan lines the input signals the same way a button being pressed on a stock controller would.
