# Arduino Medicine Box (6 Slots)

A smart medicine organizer that reminds patients when to take their meds, using Arduino, an RTC module, buzzers, LEDs, and an LCD screen. Designed for 6 individual time slots per day.

---

## Components Used

- Arduino Uno
- 6 LEDs (one per slot)
- 6 Buzzers or shared buzzer
- RTC Module (DS3231)
- LCD 16x2 with I2C
- KiCad for PCB layout
- Optional: Servo motors, buttons

---

## Folder Structure

Arduino-Medicine-Box/
├── src/ → Arduino code (.ino)
├── hardware/ → KiCad design files
├── images/ → Photos & test shots
└── README.md

---

## How It Works

Each slot is tied to a time using the DS3231 RTC. At the correct time:
- The corresponding LED lights up
- A buzzer sounds
- The LCD displays the reminder

---

## License

MIT License – Free to use and modify

---

## Creators (Collaboration Project)

Catherine Q. Bugarin
Jaspher Ikan Dela Cruz Samarista
