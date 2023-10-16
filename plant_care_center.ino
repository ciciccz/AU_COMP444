#include <LiquidCrystal.h>

// LCD
LiquidCrystal lcd(13, 12, 2, 1, 0, 8);
const int contrastPin = 6;
const int contrastLevel = 100;
const int backlightPin = 3;

// Temperature and Light
float voltage = 0;
float degreesC = 0;
int lightIntensity = 0;

// Soil Moisture
const int soilPin = A5;
int soilMoisture;

// Tracking averages
int readingsCount = 0;
float totalDegreesC = 0;
float totalLightIntensity = 0;
float totalSoilMoisture = 0;
float avgDegreesC = 0;
float avgLightIntensity = 0;
float avgSoilMoisture = 0;

// Timing
unsigned long lastReadingTime = 0;
unsigned long readingInterval = 5000;
unsigned long lastSwitchTime = 0;
unsigned long switchInterval = 5000;
unsigned long lastResetTime = 0;
unsigned long resetInterval = 86400000;
const int minReadingsForWarning = 60;

// Screen state
int currentScreenState = 0;

// Proximity sensor
const int trigPin = 5;
const int echoPin = 4;
long echoTime;
int calculatedDistance;

// Warnings
const int minTemp = 10;                       // Minimum safe temperature
const int maxTemp = 40;                       // Maximum safe temperature
const int minMoisture = 800;                  // Minimum safe moisture
const int maxMoisture = 980;                  // Maximum safe moisture

// LED
const int redPin = 9;                         // Red LED pin
const int greenPin = 10;                      // Green LED pin
const int bluePin = 11;                       // Blue LED pin
unsigned long yellowWarningStartTime = 0;     // To store the time a condition goes out of range
const long yellowWarningDuration = 10800000;  // Amount of time before Red turns on

// Functions
void readSensors();
void updateAverages();
void switchScreen();
void checkProximity();
void displayData();
void checkConditions();
void turnOnLED(int pin);

void setup() {
  lcd.begin(16, 2);
  pinMode(contrastPin, OUTPUT);
  analogWrite(contrastPin, contrastLevel);
  pinMode(soilPin, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(backlightPin, OUTPUT);
  analogWrite(backlightPin, 255);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
}

void loop() {
  readSensors();
  updateAverages();
  switchScreen();
  checkProximity();
  displayData();
  checkConditions();
  delay(1000);
}

void readSensors() {
  voltage = analogRead(A0) * 0.004882813;
  degreesC = (voltage - 0.5) * 100.0;
  lightIntensity = analogRead(A1);
  soilMoisture = analogRead(soilPin);
}

void updateAverages() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastReadingTime >= readingInterval) {
    totalDegreesC += degreesC;
    totalLightIntensity += lightIntensity;
    totalSoilMoisture += soilMoisture;
    readingsCount++;
    avgDegreesC = totalDegreesC / readingsCount;
    avgLightIntensity = totalLightIntensity / readingsCount;
    avgSoilMoisture = totalSoilMoisture / readingsCount;
    lastReadingTime = currentMillis;
  }
}

void switchScreen() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastSwitchTime >= switchInterval) {
    currentScreenState = (currentScreenState + 1) % 3;
    lastSwitchTime = currentMillis;
  }
}

void checkProximity() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  echoTime = pulseIn(echoPin, HIGH);
  calculatedDistance = echoTime / 148.0;
  analogWrite(backlightPin, (calculatedDistance < 50) ? 255 : 0);
}

void checkConditions() {

  bool isWarning = false;

  if (avgDegreesC < minTemp || avgDegreesC > maxTemp || avgSoilMoisture < minMoisture || avgSoilMoisture > maxMoisture) {
    isWarning = true;
    if (yellowWarningStartTime == 0) {  // start the timer when a parameter first goes out of range
      yellowWarningStartTime = millis();
    }
  } else {
    yellowWarningStartTime = 0;  // reset the timer when all parameters are back in range
  }

  if (isWarning) {
    if (millis() - yellowWarningStartTime > yellowWarningDuration) {
      turnOnLED(255, 0, 0);  // turn on red led if a warning has persisted for over 3 hours
    } else {
      turnOnLED(100, 40, 0);  // turn on yellow led for an immediate warning
    }
  } else {
    turnOnLED(0, 100, 0);  // turn on green led when all is safe
  }
}

void turnOnLED(int red, int green, int blue) {
  analogWrite(redPin, red);
  analogWrite(greenPin, green);
  analogWrite(bluePin, blue);
}

void displayData() {
  lcd.clear();

  switch (currentScreenState) {
    case 0:  // temperature screen
      lcd.setCursor(0, 0);
      lcd.print("Temp ");
      lcd.print((char)223);
      lcd.print("C: ");
      lcd.print(degreesC, 1);

      lcd.setCursor(0, 1);
      if (avgDegreesC < minTemp && readingsCount >= minReadingsForWarning) {
        lcd.print("Low Temp");
      } else if (avgDegreesC > maxTemp && readingsCount >= minReadingsForWarning) {
        lcd.print("High Temp");
      } else {
        lcd.print("Average: ");
        lcd.print(avgDegreesC, 2);
      }
      break;

    case 1:  // light screen
      lcd.setCursor(0, 0);
      lcd.print("Light: ");
      lcd.print(lightIntensity);

      lcd.setCursor(0, 1);
      lcd.print("Average: ");
      lcd.print(avgLightIntensity);
      break;

    case 2:  // moisture screen
      lcd.setCursor(0, 0);
      lcd.print("Moisture: ");
      lcd.print(soilMoisture);

      lcd.setCursor(0, 1);
      if (avgSoilMoisture < minMoisture && readingsCount >= minReadingsForWarning) {
        lcd.print("Too Wet");
      } else if (avgSoilMoisture > maxMoisture && readingsCount >= minReadingsForWarning) {
        lcd.print("Too Dry");
      } else {
        lcd.print("Average: ");
        lcd.print(avgSoilMoisture);
      }
      break;
  }
}