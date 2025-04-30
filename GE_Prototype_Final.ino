#include <SD.h>
#include "Arduino_LED_Matrix.h"

#define RESIST_SENSOR A0    // Voltage sensor connected to A0
#define CURRENT_SENSOR A1    // Current sensor connected to A1
#define RELAY_PIN 7          // Relay/MOSFET to control ground lift
#define BUZZER_PIN 6         // Buzzer pin
#define BUTTON_PIN 5         // Button pin (pull-down logic: pressed = LOW)

const int chipSelect = 10;
const float actual_ref = 5.0;           //  ADC reference voltage for UNO R4
const float resistor = 10.06;           //  Shunt resistor value (Ohms)
const float milli_conv = 1000.0;        //  Conversion to milliamps
const float micro_conv = 1000000.0;      //
const float Gain = 97.3;                //  Gain for grounding and current reading
const float resist_current = 0.13265;
float resist_scale, current_scale;
String File_name = "log.txt";
const float currentThreshold = 100;     //  100 uA is the threshold for Pass/Fail
const float resistanceThreshold = 0.2;  //  0.2 Ohms is the threshold for Pass/Fail

//  Matrix for hte LED on Arduino, used for clearing it
uint8_t LEDclear[8][12] = {   
                      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
                      };

File datafile;      //  Data File
ArduinoLEDMatrix matrix;    //  LED Matrix

float current_calibration = 0.0;            //  Optional current offset corecction
float resist_calibration = 0.0;             //  Optional resistance offset correction

void setup() {
    Serial.begin(9600);
    while (!Serial);

    Serial.print("Initializing SD card...");

    analogReadResolution(14);               //  For 0–16383 (2^14) range on ADC
    matrix.begin();                         //  Instantiates the LED Matrix

    if (!SD.begin(chipSelect)) {
        Serial.println("Initialization failed!");
        while (1);                          //  Stay here if SD fails, doesn't go forward
    }
    Serial.println("SD initialization done.");

    // Create log file if it doesn't exist
    if (!SD.exists(File_name)) {
        Serial.println("Creating " + File_name + "...");
        datafile = SD.open("log.txt", FILE_WRITE);
        if (datafile) {
            datafile.println("Log Start");
            datafile.close();
        }
        Serial.println("File created.");
    } else {
        Serial.println(File_name + " already exists.");
    }

    pinMode(RELAY_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);      //  Using internal pull-up

    digitalWrite(RELAY_PIN, LOW);           //  Ground connected initially
    digitalWrite(BUZZER_PIN, LOW);          //  Buzzer off

}

void loop() {
    current_scale = 1.0;                    //  Scaling factor for current value
    resist_scale = 1.0;                     //  Scaling factor for resistance value
    bool buttonPressed = digitalRead(BUTTON_PIN) == HIGH;


    if (buttonPressed) {
        matrix.loadFrame(LEDMATRIX_DANGER);
        // === Calibration Mode: Ground connected ===
        digitalWrite(RELAY_PIN, LOW);       //  Ground connected
        digitalWrite(BUZZER_PIN, HIGH);     //  Buzzer on
   

        float current_raw = getAverageReading(CURRENT_SENSOR, 10);
        float current = current_scale * ((current_raw * actual_ref / 16383.0) / (resistor * Gain)); // Leakage Current Equation
        current = current * micro_conv;     //  Convert from A to uA
        bool currentPass = PassFailTest(current, currentThreshold);

        float resist_raw = getAverageReading(RESIST_SENSOR, 10);
        float resistance = resist_scale * ((resist_raw * actual_ref / 16383.0) / (Gain * resist_current)); // Ground Resistance Equation
        bool resistPass = PassFailTest(resistance, resistanceThreshold);
        
        String log = "Current: " + String(current, 4) + " uA";
        log += " | Resistance: " + String(resistance, 4) + " Ohms";
        log += " | Current Result: ";
        log += (currentPass ? "Pass" : "Fail");
        log += " | Resistance Result: ";
        log += (resistPass ? "Pass" : "Fail");

        Serial.println(log);

        // Write to SD
        datafile = SD.open("log.txt", FILE_WRITE);
        if (datafile) {
            datafile.println(log);
            datafile.close();
            Serial.println(File_name + " wrote to file.");
        } else {
            Serial.println("Error opening " + File_name + " for writing.");
        }
        delay(1000);
        return; //  Goes back to main loop
    }

    matrix.renderBitmap(LEDclear, 8, 12);   //  Clear LED Matrix
    delay(100);                             //  Delay between button readings

}

/* === Averaging Function ===
    Takes in pin, and number of samples desired,
    and obtains readings from selected pin, and
    returns the average of those readings taken.
*/
float getAverageReading(int pin, int samples) {
    long total = 0;
    int reading= 0;
    for (int i = 0; i < samples; ++i) {
        reading += analogRead(pin);
        delay(2);                           //  Optional small delay to reduce ADC noise
    }
    total = ((float)reading / (float)samples);
    return total;
}

bool PassFailTest(float input, float threshold) {
    bool result;

    if (input <= threshold && input > 0)
        result = true;
    else
        result = false;

    return result;
}
