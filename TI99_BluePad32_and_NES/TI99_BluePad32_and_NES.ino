
// Loosely based on the https://gitlab.com/ricardoquesada/unijoysticle2. Pretty much everything was changed except used the same GPIO pins for the joystick out pins.
// Tested with ESP32 arduino board version 2.0.17 and esp32 + bluepad32 board version 4.1.0 installed from the boards manager per https://bluepad32.readthedocs.io/en/latest/plat_arduino/
// Tested with NESControllerInterface library version 1.0.3 installed from the library manager.
// The circuit uses CD4066 analog switch ICs to connect the TI Joystick A or B scan lines to the TI up, down, right, etc lines if the ESP32 GPIO pin is active for that signal. A diode on
// each scan lines blocks the signal when the TI isn't scanning the that joystick so that if the ESP32 is sending out signals for both the same outputs (ie J1_FIRE and J2_FIRE) at the same time.
// If the JBUTTON is held down for more than 2seconds it causes the adapter to forget the paired Bluetooth gamepads after the next power cycle.
// The adapter seems to scan for pairing after power cycling or reseting. 
// Order of operation seems to be:
// 1. Push the JBUTTON for more than 2 seconds
// 2. Put J1 into pairing mode and reset the ESP32. After pairing is complete (J1 LED comes on), turn off J1
// 3. Put J2 into pairing mode and reset the ESP32. After pairing is complete (J1 LED comes on), turn off J2
// 4. Turn J1 on and wait for the J1 LED, Turn J2 on and wait for the J2 LED.


#include <Bluepad32.h>
#include <Arduino.h>
#include <ArduinoBluepad32.h>
#include <pins_arduino.h>
#include <NESControllerInterface.h>

//Define ESP joystick output pins
#define J1_UP 26
#define J1_DN 18
#define J1_LT 19
#define J1_RT 23
#define J1_FIRE 14
#define J2_UP 27
#define J2_DN 25
#define J2_LT 32
#define J2_RT 17
#define J2_FIRE 12

//Define BT clear button and connection indicator LED pins
#define JBUTTON 10
#define J1_LED 5
#define J2_LED 13

//Define NES controller pins
#define NES1_CLK 33
#define NES1_LOAD 34
#define NES1_DATA 35
#define NES2_CLK 36
#define NES2_LOAD 39
#define NES2_DATA 21

//Define BT/NES Select switch pin
#define BT_NES_SEL 22

//Ids of for BT joystick 1 and 2
int j1Id = 0;
int j2Id = 1;

//Counter for how long the BT clear button has been pressed
int resetLoopCount = 0;

//Variable that will indicate if BT controllers are used, else NES controllers are used.
bool useBT = true;

//BT controller
ControllerPtr myControllers[BP32_MAX_GAMEPADS];

//Constructor for the NES controllers
NESControllerInterface nes1(NES1_DATA, NES1_LOAD, NES1_CLK);
NESControllerInterface nes2(NES1_DATA, NES1_LOAD, NES1_CLK);

// This callback gets called any time a new gamepad is connected.
// Up to 4 gamepads can be connected at the same time.
void onConnectedController(ControllerPtr ctl) {
    bool foundEmptySlot = false;
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == nullptr) {
            Serial.printf("CALLBACK: Controller is connected, index=%d\n", i);
            // Additionally, you can get certain gamepad properties like:
            // Model, VID, PID, BTAddr, flags, etc.
            ControllerProperties properties = ctl->getProperties();
            Serial.printf("Controller model: %s, VID=0x%04x, PID=0x%04x\n", ctl->getModelName().c_str(), properties.vendor_id,
                           properties.product_id);
            myControllers[i] = ctl;
            foundEmptySlot = true;
            if (i==0) {
                digitalWrite(J1_LED,HIGH);
            }
            if (i==1) {
                digitalWrite(J2_LED,HIGH);
            }
            break;
        }
    }
    if (!foundEmptySlot) {
        Serial.println("CALLBACK: Controller connected, but could not found empty slot");
    }
}

void onDisconnectedController(ControllerPtr ctl) {
    bool foundController = false;

    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == ctl) {
            Serial.printf("CALLBACK: Controller disconnected from index=%d\n", i);
            myControllers[i] = nullptr;
            foundController = true;
            if (i==0) {
                digitalWrite(J1_LED,LOW);
            }
            if (i==1) {
                digitalWrite(J2_LED,LOW);
            }
            break;
        }
    }

    if (!foundController) {
        Serial.println("CALLBACK: Controller disconnected, but not found in myControllers");
    }
}

void dumpGamepad(ControllerPtr ctl) {
    Serial.printf(
        "idx=%d, dpad: 0x%02x, buttons: 0x%04x, axis L: %4d, %4d, axis R: %4d, %4d, brake: %4d, throttle: %4d, "
        "misc: 0x%02x, gyro x:%6d y:%6d z:%6d, accel x:%6d y:%6d z:%6d\n",
        ctl->index(),        // Controller Index
        ctl->dpad(),         // D-pad
        ctl->buttons(),      // bitmask of pressed buttons
        ctl->axisX(),        // (-511 - 512) left X Axis
        ctl->axisY(),        // (-511 - 512) left Y axis
        ctl->axisRX(),       // (-511 - 512) right X axis
        ctl->axisRY(),       // (-511 - 512) right Y axis
        ctl->brake(),        // (0 - 1023): brake button
        ctl->throttle(),     // (0 - 1023): throttle (AKA gas) button
        ctl->miscButtons(),  // bitmask of pressed "misc" buttons
        ctl->gyroX(),        // Gyro X
        ctl->gyroY(),        // Gyro Y
        ctl->gyroZ(),        // Gyro Z
        ctl->accelX(),       // Accelerometer X
        ctl->accelY(),       // Accelerometer Y
        ctl->accelZ()        // Accelerometer Z
    );


}

void dumpMouse(ControllerPtr ctl) {
    Serial.printf("idx=%d, buttons: 0x%04x, scrollWheel=0x%04x, delta X: %4d, delta Y: %4d\n",
                   ctl->index(),        // Controller Index
                   ctl->buttons(),      // bitmask of pressed buttons
                   ctl->scrollWheel(),  // Scroll Wheel
                   ctl->deltaX(),       // (-511 - 512) left X Axis
                   ctl->deltaY()        // (-511 - 512) left Y axis
    );
}

void dumpKeyboard(ControllerPtr ctl) {
    static const char* key_names[] = {
        // clang-format off
        // To avoid having too much noise in this file, only a few keys are mapped to strings.
        // Starts with "A", which is offset 4.
        "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V",
        "W", "X", "Y", "Z", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
        // Special keys
        "Enter", "Escape", "Backspace", "Tab", "Spacebar", "Underscore", "Equal", "OpenBracket", "CloseBracket",
        "Backslash", "Tilde", "SemiColon", "Quote", "GraveAccent", "Comma", "Dot", "Slash", "CapsLock",
        // Function keys
        "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
        // Cursors and others
        "PrintScreen", "ScrollLock", "Pause", "Insert", "Home", "PageUp", "Delete", "End", "PageDown",
        "RightArrow", "LeftArrow", "DownArrow", "UpArrow",
        // clang-format on
    };
    static const char* modifier_names[] = {
        // clang-format off
        // From 0xe0 to 0xe7
        "Left Control", "Left Shift", "Left Alt", "Left Meta",
        "Right Control", "Right Shift", "Right Alt", "Right Meta",
        // clang-format on
    };
    Serial.printf("idx=%d, Pressed keys: ", ctl->index());
    for (int key = Keyboard_A; key <= Keyboard_UpArrow; key++) {
        if (ctl->isKeyPressed(static_cast<KeyboardKey>(key))) {
            const char* keyName = key_names[key-4];
            Serial.printf("%s,", keyName);
       }
    }
    for (int key = Keyboard_LeftControl; key <= Keyboard_RightMeta; key++) {
        if (ctl->isKeyPressed(static_cast<KeyboardKey>(key))) {
            const char* keyName = modifier_names[key-0xe0];
            Serial.printf("%s,", keyName);
        }
    }
    Console.printf("\n");
}

void dumpBalanceBoard(ControllerPtr ctl) {
    Serial.printf("idx=%d,  TL=%u, TR=%u, BL=%u, BR=%u, temperature=%d\n",
                   ctl->index(),        // Controller Index
                   ctl->topLeft(),      // top-left scale
                   ctl->topRight(),     // top-right scale
                   ctl->bottomLeft(),   // bottom-left scale
                   ctl->bottomRight(),  // bottom-right scale
                   ctl->temperature()   // temperature: used to adjust the scale value's precision
    );
}

void processGamepad(ControllerPtr ctl) {
    if (ctl->index() == j1Id) {
        digitalWrite(J1_UP, (ctl->dpad() & 1));
        digitalWrite(J1_DN, ((ctl->dpad() & 2) >>1));
        digitalWrite(J1_RT, ((ctl->dpad() & 4) >>2));
        digitalWrite(J1_LT, ((ctl->dpad() & 8) >>3));
        if (ctl->buttons() > 0) {
            digitalWrite(J1_FIRE, HIGH);
        } else {
            digitalWrite(J1_FIRE, LOW);
        }
        Serial.printf("idx=%d, up=%d, dn=%d, right=%d, left=%d, fire=%d\n", ctl->index(), (ctl->dpad() & 1),((ctl->dpad() & 2) >>1), ((ctl->dpad() & 4) >>2),((ctl->dpad() & 8) >>3),ctl->buttons());
    }
    if (ctl->index() == j2Id) {
        digitalWrite(J2_UP, (ctl->dpad() & 1));
        digitalWrite(J2_DN, ((ctl->dpad() & 2) >>1));
        digitalWrite(J2_RT, ((ctl->dpad() & 4) >>2));
        digitalWrite(J2_LT, ((ctl->dpad() & 8) >>3));
        if (ctl->buttons() > 0) {
            digitalWrite(J2_FIRE, HIGH);
        } else {
            digitalWrite(J2_FIRE, LOW);
        }
         Serial.printf("idx=%d, up=%d, dn=%d, right=%d, left=%d, fire=%d\n", ctl->index(), (ctl->dpad() & 1),((ctl->dpad() & 2) >>1), ((ctl->dpad() & 4) >>2),((ctl->dpad() & 8) >>3),ctl->buttons());

    }   


    // Another way to query controller data is by getting the buttons() function.
    // See how the different "dump*" functions dump the Controller info.
    //dumpGamepad(ctl);
}

void processMouse(ControllerPtr ctl) {
    // This is just an example.
    if (ctl->scrollWheel() > 0) {
        // Do Something
    } else if (ctl->scrollWheel() < 0) {
        // Do something else
    }

    // See "dumpMouse" for possible things to query.
    dumpMouse(ctl);
}

void processKeyboard(ControllerPtr ctl) {
    if (!ctl->isAnyKeyPressed())
        return;

    // This is just an example.
    if (ctl->isKeyPressed(Keyboard_A)) {
        // Do Something
        Serial.println("Key 'A' pressed");
    }

    // Don't do "else" here.
    // Multiple keys can be pressed at the same time.
    if (ctl->isKeyPressed(Keyboard_LeftShift)) {
        // Do something else
        Serial.println("Key 'LEFT SHIFT' pressed");
    }

    // Don't do "else" here.
    // Multiple keys can be pressed at the same time.
    if (ctl->isKeyPressed(Keyboard_LeftArrow)) {
        // Do something else
        Serial.println("Key 'Left Arrow' pressed");
    }

    // See "dumpKeyboard" for possible things to query.
    dumpKeyboard(ctl);
}

void processBalanceBoard(ControllerPtr ctl) {
    // This is just an example.
    if (ctl->topLeft() > 10000) {
        // Do Something
    }

    // See "dumpBalanceBoard" for possible things to query.
    dumpBalanceBoard(ctl);
}

void processControllers() {
    for (auto myController : myControllers) {
        if (myController && myController->isConnected() && myController->hasData()) {
            if (myController->isGamepad()) {
                processGamepad(myController);
            } else if (myController->isMouse()) {
                processMouse(myController);
            } else if (myController->isKeyboard()) {
                processKeyboard(myController);
            } else if (myController->isBalanceBoard()) {
                processBalanceBoard(myController);
            } else {
                Serial.println("Unsupported controller");
            }
        }
    }
}

void processNEScontrollers() {


}

// Arduino setup function. Runs in CPU 1
void setup() {
    Serial.begin(115200);
    pinMode(J1_UP, OUTPUT);
    pinMode(J1_DN, OUTPUT);
    pinMode(J1_LT, OUTPUT);
    pinMode(J1_RT, OUTPUT);
    pinMode(J1_FIRE, OUTPUT);
    pinMode(J2_UP, OUTPUT);
    pinMode(J2_DN, OUTPUT);
    pinMode(J2_LT, OUTPUT);
    pinMode(J2_RT, OUTPUT);
    pinMode(J2_FIRE, OUTPUT);

    pinMode(J1_LED, OUTPUT);
    pinMode(J2_LED, OUTPUT);

    pinMode(JBUTTON, INPUT_PULLUP);
    
    pinMode(BT_NES_SEL, INPUT);
    delay(10);
    if(digitalRead(BT_NES_SEL)) {
        //If BT/NES Sel switch is HIGH, use BT controllers
        useBT = true;
        const uint8_t* addr = BP32.localBdAddress();
        Serial.printf("BD Addr: %2X:%2X:%2X:%2X:%2X:%2X\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
        // Setup the Bluepad32 callbacks
        BP32.setup(&onConnectedController, &onDisconnectedController);
        // Enables mouse / touchpad support for gamepads that support them.
        // When enabled, controllers like DualSense and DualShock4 generate two connected devices:
        // - First one: the gamepad
        // - Second one, which is a "virtual device", is a mouse.
        // By default, it is disabled.
        BP32.enableVirtualDevice(false);

    } else {
        // If BT/NES Sel switch is LOW, use NES controllers
        useBT = false;
    }
    Serial.printf("Firmware: %s\n", BP32.firmwareVersion());  
  
}

// Arduino loop function. Runs in CPU 1.
void loop() {
    if (useBT) {
        // This call fetches all the controllers' data.
        // Call this function in your main loop.
        bool dataUpdated = BP32.update();
        if (dataUpdated)
            processControllers();

        // The main loop must have some kind of "yield to lower priority task" event.
        // Otherwise, the watchdog will get triggered.
        // If your main loop doesn't have one, just add a simple `vTaskDelay(1)`.
        // Detailed info here:
        // https://stackoverflow.com/questions/66278271/task-watchdog-got-triggered-the-tasks-did-not-reset-the-watchdog-in-time
        vTaskDelay(1);
        if (digitalRead(JBUTTON)==LOW) {
            ++resetLoopCount ;
        } else {
            resetLoopCount = 0;
        }
        if (resetLoopCount > 80) {
            resetLoopCount = 0;
            BP32.forgetBluetoothKeys();
        }
    } else {
        NESInput input1 = nes1.getNESInput();
        
        digitalWrite(J1_FIRE, input1.anyButtonPressed());
        digitalWrite(J1_UP, input1.buttonUp);
        digitalWrite(J1_DN, input1.buttonDown);
        digitalWrite(J1_LT, input1.buttonLeft);
        digitalWrite(J1_RT, input1.buttonRight);
        
        NESInput input2 = nes2.getNESInput();
        digitalWrite(J2_FIRE, input2.anyButtonPressed());
        digitalWrite(J2_UP, input2.buttonUp);
        digitalWrite(J2_DN, input2.buttonDown);
        digitalWrite(J2_LT, input2.buttonLeft);
        digitalWrite(J2_RT, input2.buttonRight);


    }
    // The main loop must have some kind of "yield to lower priority task" event.
    // Otherwise, the watchdog will get triggered.
    // If your main loop doesn't have one, just add a simple `vTaskDelay(1)`.
    // Detailed info here:
    // https://stackoverflow.com/questions/66278271/task-watchdog-got-triggered-the-tasks-did-not-reset-the-watchdog-in-time
    vTaskDelay(1);
    delay(25);
}
