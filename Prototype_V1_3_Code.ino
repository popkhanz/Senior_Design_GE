/*
    Electrical Safety Testing Device for Ultrasound Systems
    Developed by Team B4 for Marquette Senior Design
    Sponsored by GE Healthcare

    Purpose: Measuers the leakage current, and ground voltage for
    an ultrasound system, and saves values to a .txt file on a
    connected SD card.

    Last revision date: April 9th, 2025
*/

#include <SD.h>

//  #include <filter_lib.h>


#define VOLTAGE_SENSOR A0   //  Voltage sensor connected to A0
#define CURRENT_SENSOR A1   //  Current sensor connected to A1
#define RELAY_PIN 7         //  Relay/MOSFET to lift ground
#define BUZZER_PIN 6        //  Buzzer pin
#define BUTTON_PIN 5        //  Button pin
#define CURRENT_PIN 4       //  Current pin
#define CHIP_SELECT 10      //  Chip Select Pin used for SD card

const float resistor = 10.0;                //  10 Ohm resistor
float current_calibration = 0.0;            //  In case the current is offset and needs correction
float voltage_calibration = 0.0;            //  In case the voltage is offset and needs correction
const float milli_conv =  1000.0;           //  Converstion factor to change from base to milli-
const float micro_conv =  1000000.0;        //  Conversion factor to change from base to micro-
const float nano_conv =   1000000000.0;     //  Conversion factor to change from base to nano-

File datafile;

//lowpass_filter low_filter(100); // cutoff frequency of 100 Hz

bool groundLifted = false;
bool init_flag = false;

void setup() {

    analogReadResolution(14);       //  Sets resolution of analog pins to 14-bits

    Serial.begin(9600);             //  Initialize Serial Communication
    while (!Serial);
        Serial.print("Initializing SD card!");
    
    if (!SD.begin(CHIP_SELECT)) {
        Serial.println("initialization failed!");       //  If SD could not be found connected to the chip select pin
    while (1);
    }//stops here if SD fails 
        Serial.println("initialization done.");
    //checks if tester txt works 
    if (!SD.exists("tester.txt")) {
      //creates the txt file 
      Serial.println("Creating tester.txt...");
      //writes to file 
        datafile = SD.open("tester.txt", FILE_WRITE);
        //writes to datafile
        if (datafile) {
            datafile.println("Log Start");
            datafile.close();
        }
    } else {                                    
        Serial.println("tester.txt exists.");   //  tester already exists
    }

    //pinmodes for measurements and output 
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);  //  Using internal pull-up resistor
    pinMode(CURRENT_PIN, OUTPUT);

    digitalWrite(RELAY_PIN, LOW);       //  Ensure ground is initially connected / closed
    digitalWrite(BUZZER_PIN, LOW);      //  Ensure buzzer is off

    //init_flag = true; 
}

void loop() {
    
    // Check button state
    analogReadResolution(14);
    float raw_current, raw_voltage;
    bool buttonPressed = digitalRead(BUTTON_PIN) == LOW;

    // Update relay and buzzer state
    if (buttonPressed) {
        digitalWrite(RELAY_PIN, HIGH);          // Open Ground
        digitalWrite(BUZZER_PIN, HIGH);         // Buzzer On
        groundLifted = true;                    // Ground flag is set to true (Ground is Lifted)
        Serial.println("Button was pressed!");  
    } else {
        digitalWrite(RELAY_PIN, LOW);           // Close Ground
        digitalWrite(BUZZER_PIN, LOW);          // Buzzer Off
        groundLifted = false;                   // Ground flag set to false (Ground is closed)
    }  

    // Take measurements

    //  The gain of the OpAmp is ---, 
    raw_current = analogRead(CURRENT_SENSOR)* (5.0 / 16383.0);
    float current = (milli_conv * raw_current / resistor) - current_calibration; // when ground is close, should be <0.1 mA, and when ground is open, <0.5 mA
    float voltage = 0.0;
    float resistance = 0.0;

    if (!groundLifted) {
        digitalWrite(CURRENT_PIN, HIGH);    // Should output ~8 mA of current
        raw_voltage = analogRead(VOLTAGE_SENSOR) * (5.0 / 16383.0);
        voltage = raw_voltage - voltage_calibration;
        resistance = voltage / 0.008;
        digitalWrite(CURRENT_PIN, LOW);
    }

    String log = "Current: " + String(current, 3) + " mA";
    if (!groundLifted) {
        log += " | Voltage: " + String(voltage, 3) + " V";
    }
    log += " | Ground: ";
    log += (groundLifted ? "Lifted" : "Connected");
    log += " | Resistance: " + String(resistance, 3) + " Ohms";

    Serial.println(log);        //  Prints log to serial
    
    // Write to SD
    datafile = SD.open("tester.txt", FILE_WRITE);       //  open "tester.txt" file and write to it
    if (datafile) {
        datafile.println(log);                          //  Print log to SD
        datafile.close();                               //  close datafile
        Serial.println("Tester.txt wrote to file");     //  file was written to successfully
    } else {
        Serial.println("Error opening tester.txt for writing.");    //tester txt could not be wrote into 
    }

    delay(500);     // delay
}
   
