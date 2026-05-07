#ifndef _BUZZER_H_
#define _BUZZER_H_

#include <stdint.h>

// Notes (frequencies in Hz)
// Retrieved from https://docs.arduino.cc/built-in-examples/digital/toneMelody/
const uint16_t NOTE_B0 = 31;
const uint16_t NOTE_C1 = 33;
const uint16_t NOTE_CS1 = 35;
const uint16_t NOTE_D1 = 37;
const uint16_t NOTE_DS1 = 39;
const uint16_t NOTE_E1 = 41;
const uint16_t NOTE_F1 = 44;
const uint16_t NOTE_FS1 = 46;
const uint16_t NOTE_G1 = 49;
const uint16_t NOTE_GS1 = 52;
const uint16_t NOTE_A1 = 55;
const uint16_t NOTE_AS1 = 58;
const uint16_t NOTE_B1 = 62;
const uint16_t NOTE_C2 = 65;
const uint16_t NOTE_CS2 = 69;
const uint16_t NOTE_D2 = 73;
const uint16_t NOTE_DS2 = 78;
const uint16_t NOTE_E2 = 82;
const uint16_t NOTE_F2 = 87;
const uint16_t NOTE_FS2 = 93;
const uint16_t NOTE_G2 = 98;
const uint16_t NOTE_GS2 = 104;
const uint16_t NOTE_A2 = 110;
const uint16_t NOTE_AS2 = 117;
const uint16_t NOTE_B2 = 123;
const uint16_t NOTE_C3 = 131;
const uint16_t NOTE_CS3 = 139;
const uint16_t NOTE_D3 = 147;
const uint16_t NOTE_DS3 = 156;
const uint16_t NOTE_E3 = 165;
const uint16_t NOTE_F3 = 175;
const uint16_t NOTE_FS3 = 185;
const uint16_t NOTE_G3 = 196;
const uint16_t NOTE_GS3 = 208;
const uint16_t NOTE_A3 = 220;
const uint16_t NOTE_AS3 = 233;
const uint16_t NOTE_B3 = 247;
const uint16_t NOTE_C4 = 262;
const uint16_t NOTE_CS4 = 277;
const uint16_t NOTE_D4 = 294;
const uint16_t NOTE_DS4 = 311;
const uint16_t NOTE_E4 = 330;
const uint16_t NOTE_F4 = 349;
const uint16_t NOTE_FS4 = 370;
const uint16_t NOTE_G4 = 392;
const uint16_t NOTE_GS4 = 415;
const uint16_t NOTE_A4 = 440;
const uint16_t NOTE_AS4 = 466;
const uint16_t NOTE_B4 = 494;
const uint16_t NOTE_C5 = 523;
const uint16_t NOTE_CS5 = 554;
const uint16_t NOTE_D5 = 587;
const uint16_t NOTE_DS5 = 622;
const uint16_t NOTE_E5 = 659;
const uint16_t NOTE_F5 = 698;
const uint16_t NOTE_FS5 = 740;
const uint16_t NOTE_G5 = 784;
const uint16_t NOTE_GS5 = 831;
const uint16_t NOTE_A5 = 880;
const uint16_t NOTE_AS5 = 932;
const uint16_t NOTE_B5 = 988;
const uint16_t NOTE_C6 = 1047;
const uint16_t NOTE_CS6 = 1109;
const uint16_t NOTE_D6 = 1175;
const uint16_t NOTE_DS6 = 1245;
const uint16_t NOTE_E6 = 1319;
const uint16_t NOTE_F6 = 1397;
const uint16_t NOTE_FS6 = 1480;
const uint16_t NOTE_G6 = 1568;
const uint16_t NOTE_GS6 = 1661;
const uint16_t NOTE_A6 = 1760;
const uint16_t NOTE_AS6 = 1865;
const uint16_t NOTE_B6 = 1976;
const uint16_t NOTE_C7 = 2093;
const uint16_t NOTE_CS7 = 2217;
const uint16_t NOTE_D7 = 2349;
const uint16_t NOTE_DS7 = 2489;
const uint16_t NOTE_E7 = 2637;
const uint16_t NOTE_F7 = 2794;
const uint16_t NOTE_FS7 = 2960;
const uint16_t NOTE_G7 = 3136;
const uint16_t NOTE_GS7 = 3322;
const uint16_t NOTE_A7 = 3520;
const uint16_t NOTE_AS7 = 3729;
const uint16_t NOTE_B7 = 3951;
const uint16_t NOTE_C8 = 4186;
const uint16_t NOTE_CS8 = 4435;
const uint16_t NOTE_D8 = 4699;
const uint16_t NOTE_DS8 = 4978;

// Tone durations
const uint32_t SHORT = 120;
const uint32_t MEDIUM = 200;
const uint32_t LONG = 400;

/// Play a brief startup melody.
void play_startup_melody(uint8_t buzzer_pin);

/// Play an error melody.
void play_error_melody(uint8_t buzzer_pin);

#endif
