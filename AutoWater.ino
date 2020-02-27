// Include Libraries
#include "Arduino.h"
#include "DHT.h"
#include "LiquidCrystal_PCF8574.h"
#include "LDR.h"
#include "Button.h"
#include "Relay.h"
#include "SoilMoisture.h"

// Operating parameters
#define LDR_MAX_LIGHT 50             // Determines whether it's day/night (light/dark).  0 (no light) to 1023 (maximal light)
#define TARGET_MOISTURE_LEVEL 400    // Add water if the moisture level goes above this level and it's dark.
#define DANGER_MOISTURE_LEVEL 800    // Water even during the day (or when it's light) if the moistue level goes above this value.
#define WATERING_TIME_MS 3000        // How long to leave the watering relay on for when adding water.
#define POLL_DELAY_MS 60000          // How frequently to read the sensors, update the display, and control the fan and watering relays.
#define FAN_ON_TEMP_C      26        // Temperatures above this value will switch the fan on.
#define FAN_ON_HUMIDITY_PC 50        // Humidity levels above this value will switch the fan on.
#define MIN_FAN_ON_TIME_MS 10000     // The minimum duration the fan should stay on for (prevents thrashing the relay when the conditions waver between on and off values.

// Pin Definitions
#define DHT_PIN_DATA                 2
#define LDR_PIN_SIG                  A3
#define PUSHBUTTON_1_PIN_2           3
#define PUSHBUTTON_2_PIN_2           4
#define PUSHBUTTON_3_PIN_2           5
#define WATER_RELAYMODULE_PIN_SIGNAL 6
#define FAN_RELAYMODULE_PIN_SIGNAL   7
#define SOILMOISTURE_5V_PIN_SIG      A1

// Global variables and defines
// There are several different versions of the LCD I2C adapter, each might have a different address.
// Try the given addresses by Un/commenting the following rows until LCD works follow the serial monitor prints.
// To find your LCD address go to: http://playground.arduino.cc/Main/I2cScanner and run example.
//#define LCD_ADDRESS 0x3F
#define LCD_ADDRESS 0x27
// Define LCD characteristics
#define LCD_ROWS 4
#define LCD_COLUMNS 20
#define SCROLL_DELAY 150
#define BACKLIGHT 255
#define THRESHOLD_ldr   100
int ldrAverageLight;
// object initialization
DHT dht(DHT_PIN_DATA);
LiquidCrystal_PCF8574 lcd;
LDR ldr(LDR_PIN_SIG);
Button pushButton_1(PUSHBUTTON_1_PIN_2);
Button pushButton_2(PUSHBUTTON_2_PIN_2);
Button pushButton_3(PUSHBUTTON_3_PIN_2);
Relay WaterRelayModule(WATER_RELAYMODULE_PIN_SIGNAL);
Relay FanRelayModule(FAN_RELAYMODULE_PIN_SIGNAL);
SoilMoisture soilMoisture_5v(SOILMOISTURE_5V_PIN_SIG);
unsigned long fanOnMillis = 0;

char buf[21];
float dhtHumidity;
float dhtTempC;
int ldrValue;
int soilMoistureValue;

// Setup the essentials for your circuit to work. It runs first every time your circuit is powered with electricity.
void setup()
{
  Serial.begin(9600);
  while (!Serial) ; // wait for serial port to connect. Needed for native USB
  Serial.println(F("Started"));

  dht.begin();
  // initialize the lcd
  lcd.begin(LCD_COLUMNS, LCD_ROWS, LCD_ADDRESS, BACKLIGHT);
  ldrAverageLight = ldr.readAverage();
  pushButton_1.init();
  pushButton_2.init();
  pushButton_3.init();

  runDiagnostics();
}

void runDiagnostics()
{
  Serial.println(F("## Diagnostics"));

  // Show the diagnostics on the LCD
  lcd.clear();                    // Clear LCD screen.
  lcd.print(F("AutoWater:TEST")); // Print print String to LCD on first line
  lcd.selectLine(2);              // Set cursor at the begining of line 2
  lcd.print(F("Starting..."));    // Print print String to LCD on second line
  delay(1000);

  // LDR (Mini Photocell)
  ldrValue = ldr.read();
  sprintf(buf, "LDR: %04d", ldrValue);
  lcd.selectLine(2);
  lcd.print(buf);
  Serial.println(buf);
  delay(1000);

  // Push buttons
  bool pushButton_1Val = pushButton_1.read();
  bool pushButton_2Val = pushButton_2.read();
  bool pushButton_3Val = pushButton_3.read();
  sprintf(buf, "Buttons: %01d%01d%01d   ", pushButton_1Val, pushButton_2Val, pushButton_3Val);
  lcd.selectLine(2);
  lcd.print(buf);
  Serial.println(buf);
  delay(1000);

  // Water Relay Module - Test Code
  lcd.selectLine(2);
  Serial.println(F("Water relay test"));
  lcd.print(F("Water ON        "));
  WaterRelayModule.on();    // 1. turns on
  delay(500);               // 2. waits 500 milliseconds (0.5 sec). Change the value in the brackets (500) for a longer or shorter delay in milliseconds.
  lcd.print(F("Water OFF       "));
  WaterRelayModule.off();   // 3. turns off.
  delay(500);               // 4. waits 500 milliseconds (0.5 sec). Change the value in the brackets (500) for a longer or shorter delay in milliseconds.

  // Fan Relay Module - Test Code
  lcd.selectLine(2);
  Serial.println(F("Fan relay test"));
  lcd.print(F("Fan ON          "));
  FanRelayModule.on();      // 1. turns on
  delay(500);               // 2. waits 500 milliseconds (0.5 sec). Change the value in the brackets (500) for a longer or shorter delay in milliseconds.
  lcd.print(F("Fan OFF         "));
  FanRelayModule.off();     // 3. turns off.
  delay(500);               // 4. waits 500 milliseconds (0.5 sec). Change the value in the brackets (500) for a longer or shorter delay in milliseconds.

  // Soil Moisture Sensor - Test Code
  int soilMoisture_5vVal = soilMoisture_5v.read();
  sprintf(buf, "Moisture: %04d  ", soilMoisture_5vVal);
  lcd.print(buf);
  Serial.println(buf);
  delay(1000);

  dhtHumidity = dht.readHumidity();
  dhtTempC = dht.readTempC();
  Serial.print(F("Humidity: ")); Serial.print(dhtHumidity); Serial.println(F("%"));
  Serial.print(F("Temp (C): ")); Serial.println(dhtTempC);
  sprintf(buf, "Humidity: %04d  ", dhtHumidity);
  lcd.print(buf);
  delay(1000);
  sprintf(buf, "Temp: %04dC     ", dhtTempC);
  lcd.print(buf);
  delay(1000);
}

void loop()
{
  FanControl();

  // Returned values: from 0 (no light) to 1023 (maximal light). - Works in ambient Light, regular daylight.
  ldrValue = ldr.read();
  // Returned Values: from 0 (completely dry) to 1023 (completely moist). (air/soil humidity - ambient conditions).
  soilMoistureValue = soilMoisture_5v.read();
  bool isInvertedSensor = false;
  bool mustWater = false;

  if (TARGET_MOISTURE_LEVEL < DANGER_MOISTURE_LEVEL)
  {
    // Moisture sensor reports higher values for dry soil and lower values for wet soil
    isInvertedSensor = true;
  }

  if ((!isInvertedSensor && soilMoistureValue < DANGER_MOISTURE_LEVEL) || (isInvertedSensor && soilMoistureValue > DANGER_MOISTURE_LEVEL))
  {
    Serial.println(F("Moisture level is below the minimum!"));
    mustWater = true;
  }

  // Show the sensor values
  ShowSensorValues();

  // If it's dark enough, and the moisture level is below the target, turn the pump on for the specified duration
  if (mustWater || ldrValue < LDR_MAX_LIGHT)
  {
    if ((!isInvertedSensor && soilMoistureValue < TARGET_MOISTURE_LEVEL) || (isInvertedSensor && soilMoistureValue > TARGET_MOISTURE_LEVEL))
    {
      lcd.selectLine(1);
      sprintf(buf, "Watering...     ");
      lcd.print(buf);
      Serial.println(buf);

      WaterRelayModule.on();
      delay(WATERING_TIME_MS);
      WaterRelayModule.off();
    }
    else
    {
      Serial.println(F("No need to water at this time."));
    }
  }
  else
  {
    Serial.println(F("Not dark enough to water at this time."));
  }

  // Give the water time to soak in, or wait until it's dark enough and the soil is dry enough
  delay(POLL_DELAY_MS);
}

void ShowSensorValues()
{
  lcd.clear();
  sprintf(buf, "Light   : %04d", ldrValue);
  lcd.print(buf);
  Serial.println(buf);

  lcd.selectLine(2);
  sprintf(buf, "Moisture: %04d", soilMoistureValue);
  lcd.print(buf);
  Serial.println(buf);

  lcd.selectLine(3);
  sprintf(buf, "Humidity: %04d", dhtHumidity);
  lcd.print(buf);

  lcd.selectLine(4);
  sprintf(buf, "Temp    : %04dC", dhtTempC);
  lcd.print(buf);
}

void FanControl()
{
  // Prevent the fan switching on and off too often. If the time now is past when the fan was turned
  // on plus the minimum on time, or the time now is before the fan was turned on (can happen when
  // millis overflows, approximately every 50 days).
  //
  // fanOnMillis is set to 0 when it's turned off to allow checking the humidity and temperature
  // whenever the fan is off.
  unsigned long msNow = millis();
  if (fanOnMillis != 0 && msNow < fanOnMillis + MIN_FAN_ON_TIME_MS && msNow > fanOnMillis) {
    return;
  }

  // Read sensor values
  // DHT22/11 Humidity and Temperature Sensor
  // Reading humidity in %
  dhtHumidity = dht.readHumidity();
  // Read temperature in Celsius, for Fahrenheit use .readTempF()
  dhtTempC = dht.readTempC();
  Serial.print(F("Humidity: ")); Serial.print(dhtHumidity); Serial.print(F(" [%]\t"));
  Serial.print(F("Temp (C): ")); Serial.print(dhtTempC); Serial.println(F(" [C]"));
  bool turnFanOn = false;

  if (dhtTempC > FAN_ON_TEMP_C) {
    turnFanOn = true;
  }

  if (dhtTempC > FAN_ON_TEMP_C || dhtHumidity > FAN_ON_HUMIDITY_PC) {
    turnFanOn = true;
  }

  if (turnFanOn) {
    FanRelayModule.on();
    fanOnMillis = msNow;
  } else {
    FanRelayModule.off();
    fanOnMillis = 0;
  }
}
