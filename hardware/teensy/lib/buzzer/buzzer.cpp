#include "buzzer.h"

#include <Arduino.h>

void play_startup_melody(uint8_t buzzer_pin)
{
  tone(buzzer_pin, NOTE_D4, SHORT);
  delay(SHORT * 1.1);
  tone(buzzer_pin, NOTE_F4, SHORT);
  delay(SHORT * 1.1);
  tone(buzzer_pin, NOTE_A4, MEDIUM);
  delay(MEDIUM * 1.2);
  tone(buzzer_pin, NOTE_C5, LONG);
  delay(LONG * 1.5);
}

void play_error_melody(uint8_t buzzer_pin)
{
  tone(buzzer_pin, NOTE_B3, SHORT);
  delay(SHORT * 1.2);
  tone(buzzer_pin, NOTE_A4, SHORT);
  delay(SHORT * 1.2);
  tone(buzzer_pin, NOTE_F4, LONG);
  delay(1000);
}
