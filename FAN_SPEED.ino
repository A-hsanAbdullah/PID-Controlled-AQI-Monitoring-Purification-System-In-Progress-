#include <Arduino.h>

#define FAN_PWM 11 // Blue wire (PWM control pin)

void setup() {
  pinMode(FAN_PWM, OUTPUT);

  // Configure Timer1 for 25kHz PWM on pin 9
  TCCR1A = (1 << WGM11) | (1 << COM1A1);  
  TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10);
  ICR1 = 900;  // Set PWM frequency to ~25kHz

  OCR1A = 500; // Start with full speed (100% duty cycle)
}

void loop() {
  delay(3000);

  OCR1A = 320; // 50% speed
  delay(3000);

  OCR1A = -20;   // 0% duty cycle = Fan OFF
  delay(3000);

  OCR1A = 640; // Back to full speed
  delay(3000);
}
