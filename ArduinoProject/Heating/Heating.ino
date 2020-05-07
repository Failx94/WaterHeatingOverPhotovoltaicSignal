
#include <LiquidCrystal.h>
#include <Wire.h>
#include <DS3231.h>
#include <math.h>

DS3231 clock;
RTCDateTime dt;

int pinSSRHeizstab = 2;
int pinSSRWaermePumpe = 3;
int pinPVSignal = 4;


bool isHeatPumpOn = false;
bool isTimeForHeatPump = false;
bool isEnoughPVPower = false;
bool isHeatingElementOn = false;
bool isMaxTemperatur = false;
const int maxTemperatur = 75;
double TempDouble = 0.0;
int TempInteger = 0;
int rawTemperatur = 0;
uint8_t  HeatPumpTimeStart = 10;
uint8_t  HeatPumpTimeEnd = 18;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

void setup() {
  lcd.begin(16, 2);
  clock.begin();
  //clock.setDateTime(__DATE__, __TIME__);

  pinMode(pinSSRHeizstab, OUTPUT); // SSR Heizstab
  pinMode(pinSSRWaermePumpe, OUTPUT); // SSR Waermepumpe
  pinMode(pinPVSignal, INPUT);  // PV SIGNAL

  pinMode(A0, INPUT); //Temperatur Fuehler
}

void loop() {
  // Calculate Temp (NTC100)
  rawTemperatur = analogRead(A0);
  TempDouble = convertRawToTemperature(rawTemperatur, true);
  TempInteger = (int) TempDouble;

  // Check Params
  isTimeForHeatPump = isHeatPumpTime();
  isMaxTemperatur = checkMaxTemp();
  isEnoughPVPower = checkPVPower();

  // Send Signals
  regelung();

  // Print Status on LCD
  printTimeOnLCD();
  printTempOnLCD();
  printStatusOnLCD();

  // Sleep 1 sec
  delay(1000);
}

void regelung() {
  if (isMaxTemperatur) {
    // Max Heat nothing on
    digitalWrite(pinSSRWaermePumpe, LOW);
    isHeatPumpOn = false;
    digitalWrite(pinSSRHeizstab, LOW);
    isHeatingElementOn = false;
  } else {
    if (isEnoughPVPower) {
      // HeatingElement on
      digitalWrite(pinSSRWaermePumpe, LOW);
      isHeatPumpOn = false;
      digitalWrite(pinSSRHeizstab, HIGH);
      isHeatingElementOn = true;
    } else {
      if (isTimeForHeatPump) {
        // Heatpump on
        digitalWrite(pinSSRWaermePumpe, HIGH);
        isHeatPumpOn = true;
        digitalWrite(pinSSRHeizstab, LOW);
        isHeatingElementOn = false;
      } else {
        // Both off
        digitalWrite(pinSSRWaermePumpe, LOW);
        isHeatPumpOn = false;
        digitalWrite(pinSSRHeizstab, LOW);
        isHeatingElementOn = false;
      }
    }
  }
}

void printTimeOnLCD() {
  dt = clock.getDateTime();
  lcd.setCursor(0, 0);
  lcd.print(dt.day);
  lcd.setCursor(2, 0);
  lcd.print(".");
  lcd.setCursor(3, 0);
  lcd.print(dt.month);
  lcd.setCursor(5, 0);
  lcd.print(" ");
  lcd.setCursor(6, 0);
  lcd.print(dt.hour);
  lcd.setCursor(8, 0);
  lcd.print(":");
  lcd.setCursor(9, 0);
  lcd.print(dt.minute);
}

void printTempOnLCD() {
  lcd.setCursor(11, 0);
  lcd.print(" ");
  lcd.setCursor(12, 0);
  lcd.print(TempInteger);
  lcd.setCursor(15, 0);
  lcd.print("C");
}

void printStatusOnLCD() {
  lcd.setCursor(0, 1);
  if (isHeatPumpOn) {
    lcd.print("WP: ON ");
  } else {
    lcd.print("WP: OFF");
  }

  lcd.setCursor(8, 1);
  if (isHeatingElementOn) {
    lcd.print("HS: ON ");
  } else {
    lcd.print("HS: OFF");
  }
}

double convertRawToTemperature(int raw, bool celsius)
{
  double temperatureInF;

  temperatureInF = log(10000.0 * ((1024.0 / raw - 1)));
  temperatureInF = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * temperatureInF * temperatureInF )) * temperatureInF );
  temperatureInF = temperatureInF - 273.15;

  if (celsius == false) {
    temperatureInF = (temperatureInF * 9.0) / 5.0 + 32.0;

  }

  return temperatureInF;
}

bool isHeatPumpTime() {
  RTCDateTime dt = clock.getDateTime();
  if (dt.hour >= HeatPumpTimeStart && dt.hour <= HeatPumpTimeEnd) {
    return true;
  } else {
    return false;
  }
}

bool checkMaxTemp() {
  if (TempInteger <= maxTemperatur) {
    return false;
  } else {
    return true;
  }
}

bool checkPVPower() {
  int val = digitalRead(pinPVSignal);
  if (val == 0) {
    return false;
  } else {
    return true;
  }
}
