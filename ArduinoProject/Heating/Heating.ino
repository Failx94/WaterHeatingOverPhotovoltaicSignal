
#include <LiquidCrystal.h>
#include <Wire.h>
#include <DS3231.h>
#include <math.h>

bool const isDebug = false;

DS3231 clock;
RTCDateTime dt;

int pinSSRHeizstab = 2;
int pinSSRWaermePumpe = 3;
int pinPVSignal = 4;
int pinSZWZ = 13;
int pinManual = 6;
int pinBoost = 5;

int pinNTC100 = A0;
int pinPumpStartTimeManual = A2;
int pinPumpStopTimeManual = A3;
int pinMaxTempManual = A1;

bool isHeatPumpOn = false;
bool isTimeForHeatPump = false;
bool isEnoughPVPower = false;
bool isHeatingElementOn = false;
bool isMaxTemperatur = false;
bool isManual = false;
bool isSZWZ = false;
bool isBoost = false;
bool isBoostMode = false;

const int maxConstTemperatur = 65;
int maxTemperaturManual = 0;
int maxTemperatur = 0;
double TempDouble = 0.0;
int TempInteger = 0;
int rawTemperatur = 0;

uint8_t  HeatPumpTimeStartSZ = 13;
uint8_t  HeatPumpTimeEndSZ = 21;

uint8_t  HeatPumpTimeStartWZ = 10;
uint8_t  HeatPumpTimeEndWZ = 18;

uint8_t  HeatPumpTimeStartManual = 0;
uint8_t  HeatPumpTimeEndManual = 0;

uint8_t  HeatPumpTimeStart = 0;
uint8_t  HeatPumpTimeEnd = 0;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

void setup() {
  pinMode(pinSSRHeizstab, OUTPUT); // SSR Heizstab
  pinMode(pinSSRWaermePumpe, OUTPUT); // SSR Waermepumpe
  pinMode(pinPVSignal, INPUT);  // PV SIGNAL
  pinMode(pinSZWZ, INPUT);  // Switch Summer Winter Time
  pinMode(pinManual, INPUT);  // Switch Manual Mode
  pinMode(pinBoost, INPUT);  // Taster Boost

  pinMode(pinNTC100, INPUT); //Temperatur Fuehler
  pinMode(pinPumpStartTimeManual, INPUT); //Manual start time
  pinMode(pinPumpStopTimeManual, INPUT); //Manual stop time
  pinMode(pinMaxTempManual, INPUT); //manual max temp

  lcd.begin(16, 2);
  clock.begin();
  if (isDebug) {
    Serial.begin(9600);
    clock.setDateTime(__DATE__, __TIME__);
  }
}

void loop() {
  // Calculate Temp (NTC100)
  rawTemperatur = analogRead(pinNTC100);
  TempDouble = convertRawToTemperature(rawTemperatur, true);
  TempInteger = (int) TempDouble;

  // Check Params
  isTimeForHeatPump = isHeatPumpTime();
  isMaxTemperatur = checkMaxTemp();
  isEnoughPVPower = checkPVPower();
  isManual = checkManual();
  isSZWZ = checkSZWZ();
  isBoost = checkBoost();

  if (isManual) {
    checkManualSettings();
  }

  if (isBoost) {
    isBoostMode = true;
  }

  if (isDebug) {
    Serial.print("isTimeForHeatPump: ");
    Serial.println(isTimeForHeatPump);

    Serial.print("isMaxTemperatur: ");
    Serial.println(isMaxTemperatur);

    Serial.print("isEnoughPVPower: ");
    Serial.println(isEnoughPVPower);

    Serial.print("isManual: ");
    Serial.println(isManual);

    Serial.print("isSZWZ: ");
    Serial.println(isSZWZ);

    Serial.print("isBoost: ");
    Serial.println(isBoost);
  }

  // Send Signals
  HeatpumpController();
  HeaterController();

  // Print Status on LCD
  if(isBoost){
    printBoostOnLCD();
    delay(3000);
  }

  if(isManual){
    for(int i = 0; i<50; i++){
      checkManualSettings();
      printManualOnLCD();
      delay(100);
    }
  }

  printTimeOnLCD();
  printTempOnLCD();
  printStatusOnLCD();

  if (isDebug) {
    Serial.println("##############################################");
    Serial.println("##############################################");
    Serial.println("##############################################");
  }

  // Sleep 1 sec
  delay(1000);
}

void HeatpumpController() {
  if (isTimeForHeatPump) {
    // Heatpump on
    digitalWrite(pinSSRWaermePumpe, HIGH);
    isHeatPumpOn = true;
  } else {
    // Both off
    digitalWrite(pinSSRWaermePumpe, LOW);
    isHeatPumpOn = false;
  }
}

void HeaterController() {
  if (isMaxTemperatur) {
    digitalWrite(pinSSRHeizstab, LOW);
    isHeatingElementOn = false;
    isBoostMode = false;
  } else {
    if (isBoostMode) {
      digitalWrite(pinSSRHeizstab, HIGH);
      isHeatingElementOn = true;
    } else {
      if (isEnoughPVPower) {
        digitalWrite(pinSSRHeizstab, HIGH);
        isHeatingElementOn = true;
      } else {
        digitalWrite(pinSSRHeizstab, LOW);
        isHeatingElementOn = false;
      }
    }
  }
}

void printBoostOnLCD(){
  lcd.setCursor(0, 0);
  lcd.print("BoostMode active");
}

void printManualOnLCD(){
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 0);
  lcd.print("ST:");
  lcd.setCursor(3, 0);
  lcd.print(HeatPumpTimeStartManual);
  lcd.setCursor(5, 0);
  lcd.print("END:");
  lcd.setCursor(9, 0);
  lcd.print(HeatPumpTimeEndManual);
  lcd.setCursor(11, 0);
  lcd.print("T:");
  lcd.setCursor(13, 0);
  lcd.print(maxTemperaturManual);
  lcd.setCursor(15, 0);
  lcd.print("C");
}

void printTimeOnLCD() {
  lcd.setCursor(0, 0);
  lcd.print("                ");
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
  if (isManual) {
    HeatPumpTimeStart = HeatPumpTimeStartManual;
    HeatPumpTimeEnd = HeatPumpTimeEndManual;
    if (isDebug) {
      Serial.println("HeatPumpMode: Manual");
    }
  } else {
    if (isSZWZ) {
      //SZ
      HeatPumpTimeStart = HeatPumpTimeStartSZ;
      HeatPumpTimeEnd = HeatPumpTimeEndSZ;
      if (isDebug) {
        Serial.println("HeatPumpMode: SZ");
      }
    } else {
      HeatPumpTimeStart = HeatPumpTimeStartWZ;
      HeatPumpTimeEnd = HeatPumpTimeEndWZ;
      if (isDebug) {
        Serial.println("HeatPumpMode: WZ");
      }
    }
  }

  if (isDebug) {
    Serial.print("HeatPumpTimeStart: ");
    Serial.println(HeatPumpTimeStart);

    Serial.print("HeatPumpTimeEnd: ");
    Serial.println(HeatPumpTimeEnd);
  }

  RTCDateTime dt = clock.getDateTime();
  if (dt.hour >= HeatPumpTimeStart && dt.hour <= HeatPumpTimeEnd) {
    return true;
  } else {
    return false;
  }
}

bool checkMaxTemp() {
  if (isManual) {
    maxTemperatur = maxTemperaturManual;
    if (isDebug) {
      Serial.print("Heater Mode: Manual Value: ");
      Serial.println(maxTemperatur);
    }
  } else {
    maxTemperatur = maxConstTemperatur;
    if (isDebug) {
      Serial.print("Heater Mode: SZWZ Value: ");
      Serial.println(maxTemperatur);
    }
  }

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

bool checkManual() {
  return digitalRead(pinManual);
}

bool checkSZWZ() {
  return digitalRead(pinSZWZ);
}

bool checkBoost() {
  return digitalRead(pinBoost);
}

void checkManualSettings() {
  int startT = analogRead(pinPumpStartTimeManual);
  int stopT = analogRead(pinPumpStopTimeManual);
  int maxTemp = analogRead(pinMaxTempManual);

  HeatPumpTimeStartManual = startT / 44;
  HeatPumpTimeEndManual = stopT / 44;
  maxTemperaturManual = maxTemp / 13;

  if (isDebug) {
    Serial.print("Manual Start Value: ");
    Serial.print(startT);
    Serial.print(" Manual Start Time: ");
    Serial.println(HeatPumpTimeStartManual);

    Serial.print("Manual Stop Value: ");
    Serial.print(stopT);
    Serial.print(" Manual Stop Time: ");
    Serial.println(HeatPumpTimeEndManual);

    Serial.print("Manual Temp Value: ");
    Serial.print(maxTemp);
    Serial.print(" Manual Max Temp: ");
    Serial.println(maxTemperaturManual);
  }
}
