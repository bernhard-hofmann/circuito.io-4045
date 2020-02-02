// Include Libraries
#include "Arduino.h"
#include "./LiquidCrystal_PCF8574.h"
#include "./LDR.h"
#include "./Button.h"
#include "./Relay.h"
#include "./SoilMoisture.h"

// Pin Definitions
#define LDR_PIN_SIG	A3
#define PUSHBUTTON_1_PIN_2	2
#define PUSHBUTTON_2_PIN_2	3
#define PUSHBUTTON_3_PIN_2	4
#define RELAYMODULE_PIN_SIGNAL	5
#define SOILMOISTURE_5V_PIN_SIG	A1

// Global variables and defines
// There are several different versions of the LCD I2C adapter, each might have a different address.
// Try the given addresses by Un/commenting the following rows until LCD works follow the serial monitor prints.
// To find your LCD address go to: http://playground.arduino.cc/Main/I2cScanner and run example.
#define LCD_ADDRESS 0x3F
//#define LCD_ADDRESS 0x27
// Define LCD characteristics
#define LCD_ROWS 2
#define LCD_COLUMNS 16
#define SCROLL_DELAY 150
#define BACKLIGHT 255
#define THRESHOLD_ldr   100
int ldrAverageLight;
// object initialization
LiquidCrystal_PCF8574 lcdI2C;
LDR ldr(LDR_PIN_SIG);
Button pushButton_1(PUSHBUTTON_1_PIN_2);
Button pushButton_2(PUSHBUTTON_2_PIN_2);
Button pushButton_3(PUSHBUTTON_3_PIN_2);
Relay relayModule(RELAYMODULE_PIN_SIGNAL);
SoilMoisture soilMoisture_5v(SOILMOISTURE_5V_PIN_SIG);

// Setup the essentials for your circuit to work. It runs first every time your circuit is powered with electricity.
void setup()
{
  Serial.begin(9600);
  while (!Serial) ; // wait for serial port to connect. Needed for native USB
  Serial.println("Started");

  // initialize the lcd
  lcdI2C.begin(LCD_COLUMNS, LCD_ROWS, LCD_ADDRESS, BACKLIGHT);
  ldrAverageLight = ldr.readAverage();
  pushButton_1.init();
  pushButton_2.init();
  pushButton_3.init();

  runDiagnostics();
}

void runDiagnostics()
{
  Serial.println("## Diagnostics");
  char buf[21];

  // Show the diagnostics on the LCD
  lcdI2C.clear();                    // Clear LCD screen.
  lcdI2C.print("AutoWater:TEST");    // Print print String to LCD on first line
  lcdI2C.selectLine(2);              // Set cursor at the begining of line 2
  lcdI2C.print("Starting...");       // Print print String to LCD on second line
  delay(1000);

  // LDR (Mini Photocell)
  int ldrSample = ldr.read();
  sprintf(buf, "LDR: %04d       ", ldrSample);
  lcdI2C.selectLine(2);
  lcdI2C.print(buf);
  Serial.println(buf);
  delay(1000);

  // Push buttons
  bool pushButton_1Val = pushButton_1.read();
  bool pushButton_2Val = pushButton_2.read();
  bool pushButton_3Val = pushButton_3.read();
  sprintf(buf, "Buttons: %01d%01d%01d", pushButton_1Val, pushButton_2Val, pushButton_3Val);
  lcdI2C.selectLine(2);
  lcdI2C.print(buf);
  Serial.println(buf);
  delay(1000);

  // Relay Module - Test Code
  lcdI2C.selectLine(2);
  lcdI2C.print("Relay test...   ");
  Serial.println("Relay test");
  relayModule.on();      // 1. turns on
  delay(50);             // 2. waits 500 milliseconds (0.5 sec). Change the value in the brackets (500) for a longer or shorter delay in milliseconds.
  relayModule.off();     // 3. turns off.
  delay(50);             // 4. waits 500 milliseconds (0.5 sec). Change the value in the brackets (500) for a longer or shorter delay in milliseconds.

  // Soil Moisture Sensor - Test Code
  int soilMoisture_5vVal = soilMoisture_5v.read();
  sprintf(buf, "Moisture: %04d  ", soilMoisture_5vVal);
  lcdI2C.selectLine(2);
  lcdI2C.print(buf);
  Serial.println(buf);
  delay(1000);
}

// Main logic of your circuit. It defines the interaction between the components you selected. After setup, it runs over and over again, in an eternal loop.
void loop()
{
  runDiagnostics();
  delay(1000);
}
