/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Basic Mozzi VCO
//  https://diyelectromusic.com/2025/01/05/eurorack-6hp-arduino-mozzi-module-basic-vco/
//
      MIT License
      
      Copyright (c) 2025 diyelectromusic (Kevin)
      
      Permission is hereby granted, free of charge, to any person obtaining a copy of
      this software and associated documentation files (the "Software"), to deal in
      the Software without restriction, including without limitation the rights to
      use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
      the Software, and to permit persons to whom the Software is furnished to do so,
      subject to the following conditions:
      
      The above copyright notice and this permission notice shall be included in all
      copies or substantial portions of the Software.
      
      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
      IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
      FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
      COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHERIN
      AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
      WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include <MozziConfigValues.h>
#define MOZZI_AUDIO_MODE MOZZI_OUTPUT_2PIN_PWM
#define MOZZI_CONTROL_RATE 128

#include <Mozzi.h>
#include <Oscil.h> // oscillator template
#include <tables/sin2048_int8.h> // sine table for oscillator
#include <tables/saw2048_int8.h> // saw table for oscillator
#include <tables/triangle2048_int8.h> // triangle table for oscillator
#include <tables/square_no_alias_2048_int8.h> // square table for oscillator

#include "v2voct.h"

// Uncomment this if you want specific ratios for OSC2.
// Comment out for continuous OSC2 multiplier.
//#define DISCRETE_OSC2

// Uncomment this if you want OSC2 always enabled.
// Comment out if you want POT 2 turned down to mean "off".
// Only relevant if not using DISCRETE_OSC2
#define NEVEROFF_OSC2

// use: Oscil <table_size, update_rate> oscilName (wavetable), look in .h file of table #included above
Oscil <SIN2048_NUM_CELLS, MOZZI_AUDIO_RATE> aOsc1;
Oscil <SIN2048_NUM_CELLS, MOZZI_AUDIO_RATE> aOsc2;

// Gain table taken from https://note.com/solder_state/n/n71c67b5aaeca
const static byte bGainTable[2][128] PROGMEM = {
 {127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  124,  121,  118,  115,  112,  109,  106,  103,  100,  97, 94, 91, 88, 85, 82, 79, 76, 73, 70, 67, 64, 62, 59, 56, 53, 50, 47, 44, 41, 38, 35, 32, 29, 26, 23, 20, 17, 14, 11, 8,  5,  2,  0,  2,  5,  8,  11, 14, 17, 20, 23, 26, 29, 32, 35, 38, 41, 44, 47, 50, 53, 56, 59, 62, 64, 67, 70, 73, 76, 79, 82, 85, 88, 91, 94, 97, 100,  103,  106,  109,  112,  115,  118,  127},
 {127,  124,  121,  118,  115,  112,  109,  106,  103,  100,  97, 94, 91, 88, 85, 82, 79, 76, 73, 70, 67, 64, 62, 59, 56, 53, 50, 47, 44, 41, 38, 35, 32, 29, 26, 23, 20, 17, 14, 11, 8,  5,  2,  0,  2,  5,  8,  11, 14, 17, 20, 23, 26, 29, 32, 35, 38, 41, 44, 47, 50, 53, 56, 59, 62, 64, 67, 70, 73, 76, 79, 82, 85, 88, 91, 94, 97, 100,  103,  106,  109,  112,  115,  118,  121,  124,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  127}
};

// Analog controls
#define POT1 A4
#define POT2 A5
#define POT3 A2
#define POT4 A3
#define CV1  A6
#define CV2  A7
#define VOCT A0

byte bOsc1Gain;
byte bOsc2Gain;
byte bWave;

#ifdef DISCRETE_OSC2
UFix<16,16> q16n16ratios[8] = {0.0, 0.25, 0.333333, 0.5, 0.666666, 1.0, 1.333333, 2.0};
#endif

UFix<16,16> q16n16octaves[4] = {0.5, 1.0, 2.0, 4.0};

void setWavetable(byte wave) {
  switch (wave) {
  case 1:
    aOsc1.setTable(TRIANGLE2048_DATA);
    aOsc2.setTable(TRIANGLE2048_DATA);
    break;
  case 2:
    aOsc1.setTable(SAW2048_DATA);
    aOsc2.setTable(SAW2048_DATA);
    break;
  case 3:
    aOsc1.setTable(SQUARE_NO_ALIAS_2048_DATA);
    aOsc2.setTable(SQUARE_NO_ALIAS_2048_DATA);
    break;
  default: // case 0
    aOsc1.setTable(SIN2048_DATA);
    aOsc2.setTable(SIN2048_DATA);
    break;
  }
}

void setup(){
  startMozzi(); // uses the default control rate of 64
  bWave = 0;
  setWavetable(bWave);
  aOsc1.setFreq(q16n16c4); // set the frequency to C4
  aOsc2.setFreq(q16n16c4);
  bOsc1Gain = 31;
  bOsc2Gain = 31;
}

void updateControl(){
  // Read V/Oct and CVs
  uint voct = mozziAnalogRead(VOCT);
  uint cv1 = mozziAnalogRead(CV1);
  uint cv2 = mozziAnalogRead(CV2);

  // Potentiometers - OSC1 Frequency
  // Read directly in 0..1023 range, coverted to 16.16 format
  UFix<16,16> q16n16potfreq = mozziAnalogRead(POT1);

  // Pot - OSC2 mutliplier
#ifdef DISCRETE_OSC2
  byte bRatio = mozziAnalogRead(POT2) >> 7;  // Convert to 3-bit
  UFix<16,16> q16n16Mult = q16n16ratios[bRatio];
#else
  // Read directly in 0..1023 range, but then divided by 256
  // by << 8 and reading raw into 16.16 format (so 10 bits become 2 but in 2.8 format)
  uint32_t potval = mozziAnalogRead(POT2);
#ifdef NEVEROFF_OSC2
  if (potval < 64) potval = 64;  // sets minimum for OSC 2 at 2 octaves below, or 00.01000000
#endif
  UFix<16,16> q16n16Mult = UFix<16,16>::fromRaw(potval << 8);  // Convert to 2.8 bits
#endif

  // Pot - OSC1 and OSC2 wave
  // Convert to 0..3
  byte newwave = mozziAnalogRead(POT3) >> 8;  // Convert to 2-bit
  if (bWave != newwave) {
    // Change waveform
    bWave = newwave;
    setWavetable(bWave);
  }

  // Pot - Octave
  // Convert to 0..3
  byte bOct =  mozziAnalogRead(POT4) >> 8; // Convert to 2-bits
  UFix<16,16> q16n16oct = q16n16octaves[bOct];

  // Update OSC1 and OSC2 Gain
  bOsc1Gain = pgm_read_byte(&(bGainTable[0][cv1>>3]));;
  bOsc2Gain = pgm_read_byte(&(bGainTable[1][cv1>>3]));

  // Update modulation CV
  UFix<16,16> q16n16mod = cv2;

  // Update frequencies
  UFix<16,16> fpow = UFix<16,16>::fromRaw(pgm_read_dword(&q16n16fp[voct]));

  // OSC1 based on oct, V/Oct, frequency pot and cv2
  UFix<16,16> q16n16freq1 = q16n16oct * (q16n16c4 + q16n16potfreq + q16n16mod) * fpow;

  // OSC2 based on oct, V/Oct, cv2 and OSC1 * multiplier
  UFix<16,16> q16n16freq2 = q16n16Mult * q16n16freq1;

  aOsc1.setFreq(q16n16freq1);
  aOsc2.setFreq(q16n16freq2);
}

AudioOutput updateAudio(){
  return MonoOutput::from16Bit(bOsc1Gain*aOsc1.next()+bOsc2Gain*aOsc2.next());
}

void loop(){
  audioHook(); // required here
}
