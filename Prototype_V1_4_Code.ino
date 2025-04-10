#include <SD.h>

#define VOLTAGE_SENSOR A0    // Voltage sensor connected to A0
#define CURRENT_SENSOR A1    // Current sensor connected to A1
#define RELAY_PIN 7          // Relay/MOSFET to lift ground
#define BUZZER_PIN 6         // Buzzer pin
#define BUTTON_PIN 5         // Button pin

const int chipSelect = 10;
const float actual_ref = 5.0;             // ADC reference voltage for UNO R4
const float resistor = 10.0;              // Shunt resistor value (Ohms)
const float milli_conv = 1000.0;          // Conversion to milliamps

File datafile;

bool groundLifted = false;
float current_calibration = 0.0;          // Offset in case current sensor reads nonzero at 0A
float voltage_calibration = 0.0;          // (Optional) add if voltage needs calibration

void setup() {
    Serial.begin(9600);  // Initialize Serial Communication
    while (!Serial);
    Serial.print("Initializing SD card!");

    analogReadResolution(14); // Set ADC resolution to 14 bits (0–16383)

    if (!SD.begin(chipSelect)) {
        Serial.println("initialization failed!");
        while (1); //stops here if SD fails
    }
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
    } else { //if tester.txt already works
        Serial.println("tester.txt exists."); //tester already exists
    }

    //pinmodes for measurements and output
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP); // Using internal pull-up resistor

    digitalWrite(RELAY_PIN, LOW); // Ensure ground is initially connected
    digitalWrite(BUZZER_PIN, LOW); // Ensure buzzer is off

}

void loop() {
    // Check button state
    bool buttonPressed = digitalRead(BUTTON_PIN) == LOW;

    // Update relay and buzzer state
    if (buttonPressed) {
        digitalWrite(RELAY_PIN, HIGH);  // Open ground
        digitalWrite(BUZZER_PIN, HIGH); // Buzzer on
        groundLifted = true;            //not grounded
        Serial.println("Button was pressed!");
    } else {
        digitalWrite(RELAY_PIN, LOW);   // Close ground
        digitalWrite(BUZZER_PIN, LOW);  // Buzzer off
        groundLifted = false;           // grounded
    }

    // Take measurements
    float current_raw = getAverageReading(CURRENT_SENSOR, 10);              //  average of 10 current readings
    float current = (current_raw * actual_ref / 16383.0) / resistor;        //  convert input into proper current value
    current = (current * milli_conv) - (current_calibration * milli_conv);  // In mA

    float voltage = 0;

    if (!groundLifted) {
        float voltage_raw = getAverageReading(VOLTAGE_SENSOR, 10);          //  average of 10 voltage readings
        voltage = (voltage_raw * actual_ref / 16383.0) - voltage_calibration;   //  convert input into proper voltage value
    }

    String log = "Current: " + String(current, 3) + " mA";
    if (!groundLifted) {
        log += " | Voltage: " + String(voltage, 3) + " V";
    }
    log += " | Ground: ";
    log += (groundLifted ? "Lifted" : "Connected");

    Serial.println(log);

    // Write to SD
    datafile = SD.open("tester.txt", FILE_WRITE);
    if (datafile) {
        datafile.println(log);
        datafile.close();
        Serial.println("Tester.txt wrote to file");
    } else {
        Serial.println("Error opening tester.txt for writing."); //tester txt couldn't be wrote into
    }

    delay(500);
}

// === Averaging Function ===
float getAverageReading(int pin, int samples) {
    long total = 0;
    long final_total = 0;
    for (int i = 0; i < samples; ++i) {
        total += analogRead(pin);
        delay(2);                               // Optional small delay to reduce ADC noise
    }
    final_total = (total / (float)samples);
    return final_total;
}
