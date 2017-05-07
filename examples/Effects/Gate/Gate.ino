#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSynthNoiseWhite     noise1; //xy=203,482
AudioSynthSimpleDrum     drum1; //xy=203,522
AudioMixer4              mixer1;         //xy=433,407
AudioEffectGate          gate1;          //xy=687,440
AudioOutputAnalog        dac1;           //xy=823,335
AudioOutputUSB           usb2; //xy=823,402
AudioConnection          patchCord1(noise1, 0, mixer1, 0);
AudioConnection          patchCord2(drum1, 0, mixer1, 1);
AudioConnection          patchCord3(mixer1, gate1);
AudioConnection          patchCord4(mixer1, 0, usb2, 0);
AudioConnection          patchCord5(gate1, 0, usb2, 1);
// GUItool: end automatically generated code

unsigned long last_time = millis();

void setup() {
  AudioMemory(10);
  
  noise1.amplitude(0.1);
  drum1.frequency(180);
  drum1.length(400);
  drum1.secondMix(0.3);
  drum1.pitchMod(0.7);
  mixer1.gain(1,1.6);
  
  gate1.threshold(0.2);
  gate1.attack(10);
  gate1.release(100);
  gate1.hold(100);
  gate1.floor(0.1);
  
  AudioProcessorUsageMaxReset();
  AudioMemoryUsageMaxReset();
}

void loop() {
  if(1) {
    if(millis() - last_time >= 1000) {
      Serial.print("Proc = ");
      Serial.print(AudioProcessorUsage());
      Serial.print(" (");    
      Serial.print(AudioProcessorUsageMax());
      Serial.print("),  Mem = ");
      Serial.print(AudioMemoryUsage());
      Serial.print(" (");    
      Serial.print(AudioMemoryUsageMax());
      Serial.println(")");
      last_time = millis();
      drum1.noteOn();
    }
  }
}