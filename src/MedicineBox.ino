#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin definitions
const int buttonStartStopPin = 3;    // Start/Stop timer
const int buttonConfirmPin = 4;      // Confirm medicine taken
const int buttonRotateRefillPin = 5; // Rotate for refill (10 seconds)
const int buzzerPin = 6;
const int ledPin = 7;
const int stepperPin1 = 8;
const int stepperPin2 = 9;
const int stepperPin3 = 10;
const int stepperPin4 = 11;

// Variables
unsigned long currentTime = 0;
unsigned long alarmTime = 0;
unsigned long rotationEndTime = 0;
bool isRunning = false;
bool alarmTriggered = false;
bool isRotating = false;
int currentSlot = 0; // Current medicine slot (0-5)
const unsigned long interval = 4 * 3600; // 4 hours in seconds
const unsigned long rotateDuration = 10; // 10 seconds for rotation

// Button states
int buttonStartStopState = 0;
int buttonConfirmState = 0;
int buttonRotateRefillState = 0;
int lastButtonStartStopState = HIGH;
int lastButtonConfirmState = HIGH;
int lastButtonRotateRefillState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// Buzzer control
bool buzzerActive = false;
unsigned long lastBuzzerToggle = 0;
const unsigned long buzzerInterval = 500; // Buzzer on/off interval in ms

void setup() {
  // Initialize pins
  pinMode(buttonStartStopPin, INPUT_PULLUP);
  pinMode(buttonConfirmPin, INPUT_PULLUP);
  pinMode(buttonRotateRefillPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW); // Ensure buzzer is off initially
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);    // Ensure LED is off initially
  pinMode(stepperPin1, OUTPUT);
  pinMode(stepperPin2, OUTPUT);
  pinMode(stepperPin3, OUTPUT);
  pinMode(stepperPin4, OUTPUT);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Medicine Reminder");
  lcd.setCursor(0, 1);
  lcd.print("Press Start");
  
  Serial.begin(9600);
}

void loop() {
  currentTime = millis() / 1000; // Convert to seconds
  
  readButtons();
  handleButtonActions();
  updateDisplay();
  checkAlarm();
  handleRotation();
  handleBuzzer();
  
  // LED control
  if (!alarmTriggered && !isRotating) {
    digitalWrite(ledPin, LOW);
  }
}

void handleBuzzer() {
  // Only control buzzer if in alarm state and not rotating
  if (alarmTriggered && !isRotating) {
    if (millis() - lastBuzzerToggle > buzzerInterval) {
      buzzerActive = !buzzerActive;
      digitalWrite(buzzerPin, buzzerActive ? HIGH : LOW);
      lastBuzzerToggle = millis();
    }
  } else {
    // Ensure buzzer is off when not in alarm state or when rotating
    digitalWrite(buzzerPin, LOW);
    buzzerActive = false;
  }
}

void readButtons() {
  // Read buttons with debounce
  int reading;
  
  // Button 1 (Start/Stop)
  reading = digitalRead(buttonStartStopPin);
  if (reading != lastButtonStartStopState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonStartStopState) {
      buttonStartStopState = reading;
    }
  }
  lastButtonStartStopState = reading;
  
  // Button 2 (Confirm)
  reading = digitalRead(buttonConfirmPin);
  if (reading != lastButtonConfirmState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonConfirmState) {
      buttonConfirmState = reading;
    }
  }
  lastButtonConfirmState = reading;
  
  // Button 3 (Rotate for refill)
  reading = digitalRead(buttonRotateRefillPin);
  if (reading != lastButtonRotateRefillState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonRotateRefillState) {
      buttonRotateRefillState = reading;
    }
  }
  lastButtonRotateRefillState = reading;
}

void handleButtonActions() {
  // Button 1 - Start/Stop timer
  if (buttonStartStopState == LOW && lastButtonStartStopState == HIGH) {
    if (alarmTriggered) {
      // If alarm is ringing, stop it
      stopAlarm();
      lcd.clear();
      lcd.print("Timer Stopped");
      lcd.setCursor(0, 1);
      lcd.print("Press Start");
    } else {
      // Start/Stop timer
      isRunning = !isRunning;
      if (isRunning) {
        alarmTime = currentTime + interval;
        lcd.clear();
        lcd.print("Timer Running");
        lcd.setCursor(0, 1);
        lcd.print("Next: Slot ");
        lcd.print(currentSlot + 1);
      } else {
        stopAlarm();
        lcd.clear();
        lcd.print("Timer Stopped");
      }
    }
    delay(200);
  }
  
  // Button 2 - Confirm medicine taken
  if (buttonConfirmState == LOW && lastButtonConfirmState == HIGH) {
    if (alarmTriggered) {
      // Start rotation to next container
      stopAlarm();
      startRotation();
    }
    delay(200);
  }
  
  // Button 3 - Rotate for refill (10 seconds)
  if (buttonRotateRefillState == LOW && lastButtonRotateRefillState == HIGH) {
    if (!alarmTriggered) { // Only allow refill when not in alarm state
      startRotation();
    }
    delay(200);
  }
}

void stopAlarm() {
  alarmTriggered = false;
  digitalWrite(buzzerPin, LOW);
  digitalWrite(ledPin, LOW);
  buzzerActive = false;
}

void startRotation() {
  isRotating = true;
  alarmTriggered = false;  // Clear alarm state
  rotationEndTime = currentTime + rotateDuration;
  digitalWrite(ledPin, HIGH);
  digitalWrite(buzzerPin, LOW); // Ensure buzzer is off during rotation
  buzzerActive = false;        // Clear buzzer state
  
  lcd.clear();
  if (alarmTriggered) {
    lcd.print("Rotating to");
    lcd.setCursor(0, 1);
    lcd.print("next slot...");
  } else {
    lcd.print("Rotating for");
    lcd.setCursor(0, 1);
    lcd.print("refill...");
  }
}

void handleRotation() {
  if (isRotating) {
    // Continue rotating motor
    stepMotor(1);
    
    // Check if rotation time has elapsed
    if (currentTime >= rotationEndTime) {
      isRotating = false;
      digitalWrite(ledPin, LOW);
      
      // If this was a container rotation (not refill)
      if (alarmTriggered) {
        currentSlot = (currentSlot + 1) % 6;
        alarmTime = currentTime + interval;
        isRunning = true;
      }
      
      lcd.clear();
      if (isRunning) {
        lcd.print("Timer Running");
        lcd.setCursor(0, 1);
        lcd.print("Next: Slot ");
        lcd.print(currentSlot + 1);
      } else {
        lcd.print("Press Start");
      }
    }
  }
}

void stepMotor(int dir) {
  static int step = 0;
  static unsigned long lastStepTime = 0;
  const unsigned long stepDelay = 5; // ms between steps
  
  if (millis() - lastStepTime < stepDelay) return;
  lastStepTime = millis();
  
  if (dir == 1) {
    step = (step + 1) % 8;
  } else {
    step = (step - 1 + 8) % 8; 
  }
  
  switch (step) {
    case 0:
      digitalWrite(stepperPin1, HIGH);
      digitalWrite(stepperPin2, LOW);
      digitalWrite(stepperPin3, LOW);
      digitalWrite(stepperPin4, LOW);
      break;
    case 1:
      digitalWrite(stepperPin1, HIGH);
      digitalWrite(stepperPin2, HIGH);
      digitalWrite(stepperPin3, LOW);
      digitalWrite(stepperPin4, LOW);
      break;
    case 2:
      digitalWrite(stepperPin1, LOW);
      digitalWrite(stepperPin2, HIGH);
      digitalWrite(stepperPin3, LOW);
      digitalWrite(stepperPin4, LOW);
      break;
    case 3:
      digitalWrite(stepperPin1, LOW);
      digitalWrite(stepperPin2, HIGH);
      digitalWrite(stepperPin3, HIGH);
      digitalWrite(stepperPin4, LOW);
      break;
    case 4:
      digitalWrite(stepperPin1, LOW);
      digitalWrite(stepperPin2, LOW);
      digitalWrite(stepperPin3, HIGH);
      digitalWrite(stepperPin4, LOW);
      break;
    case 5:
      digitalWrite(stepperPin1, LOW);
      digitalWrite(stepperPin2, LOW);
      digitalWrite(stepperPin3, HIGH);
      digitalWrite(stepperPin4, HIGH);
      break;
    case 6:
      digitalWrite(stepperPin1, LOW);
      digitalWrite(stepperPin2, LOW);
      digitalWrite(stepperPin3, LOW);
      digitalWrite(stepperPin4, HIGH);
      break;
    case 7:
      digitalWrite(stepperPin1, HIGH);
      digitalWrite(stepperPin2, LOW);
      digitalWrite(stepperPin3, LOW);
      digitalWrite(stepperPin4, HIGH);
      break;
  }
}

void checkAlarm() {
  if (isRunning && currentTime >= alarmTime && !alarmTriggered && !isRotating) {
    alarmTriggered = true;
    digitalWrite(ledPin, HIGH);
    buzzerActive = true;
    digitalWrite(buzzerPin, HIGH);
    
    lcd.clear();
    lcd.print("TAKE MEDICINE!");
    lcd.setCursor(0, 1);
    lcd.print("Slot ");
    lcd.print(currentSlot + 1);
    lcd.print(" - Press Confirm");
  }
}

void updateDisplay() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 1000) { // Update every second
    lastUpdate = millis();
    
    if (alarmTriggered || isRotating) {
      // These states handle their own display updates
      return;
    }
    
    if (isRunning) {
      unsigned long remaining = alarmTime > currentTime ? alarmTime - currentTime : 0;
      int h = remaining / 3600;
      int m = (remaining % 3600) / 60;
      int s = remaining % 60;
      
      lcd.clear();
      lcd.print("Next: Slot ");
      lcd.print(currentSlot + 1);
      lcd.setCursor(0, 1);
      if (h < 10) lcd.print("0");
      lcd.print(h);
      lcd.print(":");
      if (m < 10) lcd.print("0");
      lcd.print(m);
      lcd.print(":");
      if (s < 10) lcd.print("0");
      lcd.print(s);
      lcd.print(" left");
    } else {
      lcd.clear();
      lcd.print("Press Start");
      lcd.setCursor(0, 1);
      lcd.print("Current Slot: ");
      lcd.print(currentSlot + 1);
    }
  }
}