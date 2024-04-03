#include <Wire.h>
#include "RTClib.h"
#include <Encoder.h> 
#include <LiquidCrystal.h> 
#include <EEPROM.h> 

RTC_DS1307 RTC;
volatile byte weekend = 0;
byte day_before;
bool buttonState = HIGH; 
bool buttonPressed = false; 
int lastEncoderPosition = 0;
int encoderPosition = 0;
bool alarm_display = false; 
byte alarm_minute = 30;
byte alarm_hour = 5;

Encoder rotaryEncoder(5, 3); 
LiquidCrystal lcd(8, 9, 10, 11, 12, 13); 

enum Mode { NORMAL, ALARM_SET_HOUR, ALARM_SET_MINUTE };
Mode currentMode = NORMAL;

byte last_alarm_hour = 255; // Initialisieren Sie die letzten Alarmstunden mit einem ungültigen Wert
byte last_alarm_minute = 255; // Initialisieren Sie die letzten Alarmminuten mit einem ungültigen Wert

void setup() {
  Serial.begin(750); 
  Wire.begin();
  RTC.begin();
  pinMode(7, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(4, INPUT_PULLUP); 
  pinMode(6, OUTPUT); 

  analogWrite(6, 102); 
  delay(100); 

  lcd.begin(16, 2); 

  // Alarmzeit aus dem EEPROM lesen und in BCD-Format konvertieren
  byte alarm_hour_bcd, alarm_minute_bcd;
  EEPROM.get(0, alarm_hour_bcd);
  EEPROM.get(1, alarm_minute_bcd);
  alarm_hour = bcdToDec(alarm_hour_bcd);
  alarm_minute = bcdToDec(alarm_minute_bcd);

  last_alarm_hour = alarm_hour; // Aktualisieren Sie die letzten Alarmstunden mit dem gelesenen Wert
  last_alarm_minute = alarm_minute; // Aktualisieren Sie die letzten Alarmminuten mit dem gelesenen Wert
}

void loop() {
  static unsigned long lastDisplayUpdateTime = 0; 
  DateTime now = RTC.now();

  if (millis() - lastDisplayUpdateTime >= 1000) {
    if (currentMode == NORMAL) {
      alarm_display = !alarm_display; // Wechsle zwischen Free Days und Alarm
    }
    updateDisplay(now);
    lastDisplayUpdateTime = millis();
  }

  bool currentState = digitalRead(4);
  if (currentState != buttonState) { 
    delay(50); 
    currentState = digitalRead(4); 
    if (currentState != buttonState) { 
      buttonState = currentState; 
      if (buttonState == LOW) { 
        buttonPressed = true;
        
        if (currentMode == NORMAL) {
          currentMode = ALARM_SET_HOUR;
        } else if (currentMode == ALARM_SET_HOUR) {
          currentMode = ALARM_SET_MINUTE;
        } else {
          currentMode = NORMAL;
          // Alarmzeit in den EEPROM schreiben, wenn sich die Alarmzeit geändert hat
          if (alarm_hour != last_alarm_hour || alarm_minute != last_alarm_minute) {
            byte alarm_hour_bcd = decToBcd(alarm_hour);
            byte alarm_minute_bcd = decToBcd(alarm_minute);
            EEPROM.put(0, alarm_hour_bcd);
            EEPROM.put(1, alarm_minute_bcd);
            
            last_alarm_hour = alarm_hour; // Aktualisieren Sie die letzten Alarmstunden
            last_alarm_minute = alarm_minute; // Aktualisieren Sie die letzten Alarmminuten
          }
        }
      }
    }
  }

  if (currentMode == NORMAL && alarm_hour == now.hour() && alarm_minute == now.minute() && weekend == 0) {
    for (int i = 0; i <= 300; i++) {
      digitalWrite(2, HIGH); 
      delay(100);
      digitalWrite(2, LOW); 
      delay(100);
    }
  }

  encoderPosition = rotaryEncoder.read();
  if (encoderPosition != lastEncoderPosition && encoderPosition % 2 == 0) { 
    if (currentMode == ALARM_SET_HOUR) {
      if (encoderPosition > lastEncoderPosition) {
        alarm_hour = (alarm_hour + 1) % 24; // Alarmstunde inkrementieren und auf 0 setzen, wenn 24 erreicht ist
      } else {
        alarm_hour = (alarm_hour == 0) ? 23 : alarm_hour - 1; // Alarmstunde dekrementieren und auf 23 setzen, wenn 0 erreicht ist
      }
    } else if (currentMode == ALARM_SET_MINUTE) {
      if (encoderPosition > lastEncoderPosition) {
        alarm_minute = (alarm_minute + 1) % 60; // Alarmminute inkrementieren und auf 0 setzen, wenn 60 erreicht ist
      } else {
        alarm_minute = (alarm_minute == 0) ? 59 : alarm_minute - 1; // Alarmminute dekrementieren und auf 59 setzen, wenn 0 erreicht ist
      }
    } else {
      if (encoderPosition > lastEncoderPosition && weekend > 0) { 
        weekend--;
      } else if (encoderPosition < lastEncoderPosition && weekend < 255) { 
        weekend++;
      }
    }
    lastEncoderPosition = encoderPosition;
  }
}

void updateDisplay(DateTime now) {
  lcd.clear();
  lcd.setCursor(0, 0);
  
  if (currentMode == ALARM_SET_HOUR) {
    lcd.print("Set Alarm Hour: ");
    printDigits(alarm_hour);
  } else if (currentMode == ALARM_SET_MINUTE) {
    lcd.print("Set Alarm Minute: ");
    printDigits(alarm_minute);
  } else {
    lcd.print("Time: ");
    printDigits(now.hour());
    lcd.print(":");
    printDigits(now.minute());
  }
  
  lcd.setCursor(0, 1);
  if (currentMode == NORMAL) {
    if (alarm_display) {
      lcd.print("Alarm: ");
      printDigits(alarm_hour);
      lcd.print(":");
      printDigits(alarm_minute);
    } else {
      lcd.print("Free Days: ");
      lcd.print(weekend);
    }
  } else {
    lcd.print("Alarm: ");
    printDigits(alarm_hour);
    lcd.print(":");
    printDigits(alarm_minute);
  }
}

void printDigits(int digits) {
  if (digits < 10) {
    lcd.print('0');
  }
  lcd.print(digits);
}

byte decToBcd(byte val) {
  // Dezimal zu BCD-Konvertierung
  return ( (val/10*16) + (val%10) );
}

byte bcdToDec(byte val) {
  // BCD zu Dezimal-Konvertierung
  return ( (val/16*10) + (val%16) );
}
