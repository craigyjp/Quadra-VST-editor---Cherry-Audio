/*
  Quadra Editor - Firmware Rev 1.7

  Includes code by:
    Dave Benn - Handling MUXs, a few other bits and original inspiration  https://www.notesandvolts.com/2019/01/teensy-synth-part-10-hardware.html
    ElectroTechnique for general method of menus and updates.

  Arduino IDE
  Tools Settings:
  Board: "Teensy4,1"
  USB Type: "Serial + MIDI"
  CPU Speed: "600"
  Optimize: "Fastest"

  Performance Tests   CPU  Mem
  180Mhz Faster       81.6 44
  180Mhz Fastest      77.8 44
  180Mhz Fastest+PC   79.0 44
  180Mhz Fastest+LTO  76.7 44
  240MHz Fastest+LTO  55.9 44

  Additional libraries:
    Agileware CircularBuffer available in Arduino libraries manager
    Replacement files are in the Modified Libraries folder and need to be placed in the teensy Audio folder.
*/

#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <MIDI.h>
#include <USBHost_t36.h>
#include "MidiCC.h"
#include "Constants.h"
#include "Parameters.h"
#include "PatchMgr.h"
#include "HWControls.h"
#include "EepromMgr.h"
#include <RoxMux.h>
#include <LedControl.h>
#include <SevenSegmentTM1637.h>
#include <SevenSegmentExtended.h>

#define PARAMETER 0      //The main page for displaying the current patch and control (parameter) changes
#define RECALL 1         //Patches list
#define SAVE 2           //Save patch page
#define REINITIALISE 3   // Reinitialise message
#define PATCH 4          // Show current patch bypassing PARAMETER
#define PATCHNAMING 5    // Patch naming page
#define DELETE 6         //Delete patch page
#define DELETEMSG 7      //Delete patch message page
#define SETTINGS 8       //Settings page
#define SETTINGSVALUE 9  //Settings page

unsigned int state = PARAMETER;

#include "ST7735Display.h"

boolean cardStatus = false;

//USB HOST MIDI Class Compliant
USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
MIDIDevice midi1(myusb);

//MIDI 5 Pin DIN
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

#define OCTO_TOTAL 9
#define BTN_DEBOUNCE 50
RoxOctoswitch<OCTO_TOTAL, BTN_DEBOUNCE> octoswitch;

// pins for 74HC165
#define PIN_DATA 33  // pin 9 on 74HC165 (DATA)
#define PIN_LOAD 34  // pin 1 on 74HC165 (LOAD)
#define PIN_CLK 35   // pin 2 on 74HC165 (CLK))

#define SR_TOTAL 10
Rox74HC595<SR_TOTAL> sr;

// pins for 74HC595
#define LED_DATA 21   // pin 14 on 74HC595 (DATA)
#define LED_LATCH 23  // pin 12 on 74HC595 (LATCH)
#define LED_CLK 22    // pin 11 on 74HC595 (CLK)
#define LED_PWM -1    // pin 13 on 74HC595

byte ccType = 0;        //(EEPROM)
byte updateParams = 0;  //(EEPROM)

#include "Settings.h"

LedControl ledpanel = LedControl(6, 7, 8, 1);

int count = 0;  //For MIDI Clk Sync
int DelayForSH3 = 12;
int patchNo = 1;               //Current patch no
int voiceToReturn = -1;        //Initialise
long earliestTime = millis();  //For voice allocation - initialise to now

// LED displays
SevenSegmentExtended trilldisplay(TRILL_CLK, TRILL_DIO);
SevenSegmentExtended display0(SEGMENT_CLK, SEGMENT_DIO);
SevenSegmentExtended display1(SEGMENT_CLK, SEGMENT_DIO);
SevenSegmentExtended display2(SEGMENT_CLK, SEGMENT_DIO);
SevenSegmentExtended display3(SEGMENT_CLK, SEGMENT_DIO);
SevenSegmentExtended display4(SEGMENT_CLK, SEGMENT_DIO);
SevenSegmentExtended display5(SEGMENT_CLK, SEGMENT_DIO);
SevenSegmentExtended display6(SEGMENT_CLK, SEGMENT_DIO);
SevenSegmentExtended display7(SEGMENT_CLK, SEGMENT_DIO);

void setup() {
  SPI.begin();
  octoswitch.begin(PIN_DATA, PIN_LOAD, PIN_CLK);
  octoswitch.setCallback(onButtonPress);
  sr.begin(LED_DATA, LED_LATCH, LED_CLK, LED_PWM);
  setupDisplay();
  setUpSettings();
  setupHardware();
  //setUpLEDS();

  SLIDERintensity = getSLIDERintensity();
  oldSLIDERintensity = SLIDERintensity;

  ledpanel.shutdown(0, false);
  /* Set the brightness to a medium values */
  ledpanel.setIntensity(0, 15);
  /* and clear the display */
  ledpanel.clearDisplay(0);

  LEDintensity = getLEDintensity();
  LEDintensity = LEDintensity * 10;
  oldLEDintensity = LEDintensity;

  trilldisplay.begin();                     // initializes the display
  trilldisplay.setBacklight(LEDintensity);  // set the brightness to 100 %
  trilldisplay.print("   1");               // display INIT on the display
  delay(10);

  setLEDDisplay0();
  display0.begin();                     // initializes the display
  display0.setBacklight(LEDintensity);  // set the brightness to intensity
  display0.print(" 127");               // display INIT on the display
  delay(10);

  setLEDDisplay1();
  display1.begin();                     // initializes the display
  display1.setBacklight(LEDintensity);  // set the brightness to intensity
  display1.print("   0");               // display INIT on the display
  delay(10);

  setLEDDisplay2();
  display2.begin();                     // initializes the display
  display2.setBacklight(LEDintensity);  // set the brightness to intensity
  display2.print(" 127");               // display INIT on the display
  delay(10);

  setLEDDisplay3();
  display3.begin();                     // initializes the display
  display3.setBacklight(LEDintensity);  // set the brightness to intensity
  display3.print("   0");               // display INIT on the display
  delay(10);

  setLEDDisplay4();
  display4.begin();                     // initializes the display
  display4.setBacklight(LEDintensity);  // set the brightness to intensity
  display4.print(" 127");               // display INIT on the display
  delay(10);

  setLEDDisplay5();
  display5.begin();                     // initializes the display
  display5.setBacklight(LEDintensity);  // set the brightness to intensity
  display5.print("   0");               // display INIT on the display
  delay(10);

  setLEDDisplay6();
  display6.begin();                     // initializes the display
  display6.setBacklight(LEDintensity);  // set the brightness to intensity
  display6.print(" 127");               // display INIT on the display
  delay(10);

  setLEDDisplay7();
  display7.begin();                     // initializes the display
  display7.setBacklight(LEDintensity);  // set the brightness to intensity
  display7.print("   0");               // display INIT on the display
  delay(10);

  setLEDDisplay0();

  cardStatus = SD.begin(BUILTIN_SDCARD);
  if (cardStatus) {
    Serial.println("SD card is connected");
    //Get patch numbers and names from SD card
    loadPatches();
    if (patches.size() == 0) {
      //save an initialised patch to SD card
      savePatch("1", INITPATCH);
      loadPatches();
    }
  } else {
    Serial.println("SD card is not connected or unusable");
    reinitialiseToPanel();
    showPatchPage("No SD", "conn'd / usable");
  }

  //Read MIDI Channel from EEPROM
  midiChannel = getMIDIChannel();
  Serial.println("MIDI Ch:" + String(midiChannel) + " (0 is Omni On)");

  //Read CC type from EEPROM
  ccType = getCCType();

  //Read UpdateParams type from EEPROM
  updateParams = getUpdateParams();

  //USB HOST MIDI Class Compliant
  delay(400);  //Wait to turn on USB Host
  myusb.begin();
  midi1.setHandleControlChange(myConvertControlChange);
  midi1.setHandleProgramChange(myProgramChange);
  midi1.setHandleNoteOff(myNoteOff);
  midi1.setHandleNoteOn(myNoteOn);
  midi1.setHandlePitchChange(myPitchBend);
  Serial.println("USB HOST MIDI Class Compliant Listening");

  //USB Client MIDI
  usbMIDI.setHandleControlChange(myConvertControlChange);
  usbMIDI.setHandleProgramChange(myProgramChange);
  usbMIDI.setHandleNoteOff(myNoteOff);
  usbMIDI.setHandleNoteOn(myNoteOn);
  usbMIDI.setHandlePitchChange(myPitchBend);
  Serial.println("USB Client MIDI Listening");

  //MIDI 5 Pin DIN
  MIDI.begin();
  MIDI.setHandleControlChange(myConvertControlChange);
  MIDI.setHandleProgramChange(myProgramChange);
  MIDI.setHandleNoteOn(myNoteOn);
  MIDI.setHandleNoteOff(myNoteOff);
  usbMIDI.setHandlePitchChange(myPitchBend);
  Serial.println("MIDI In DIN Listening");

  //Read Encoder Direction from EEPROM
  encCW = getEncoderDir();
  //Read MIDI Out Channel from EEPROM
  midiOutCh = getMIDIOutCh();

  sr.writePin(POLY_WAVE_LED, HIGH);
  sr.writePin(LEAD_VCO1_WAVE_LED, HIGH);
  sr.writePin(LEAD_VCO2_WAVE_LED, HIGH);

  recallPatch(patchNo);  //Load first patch
}

void myNoteOn(byte channel, byte note, byte velocity) {
  //Check for out of range notes
  //if (note < 13 || note > 115) return;
  learningNote = note;
  noteArrived = true;
  usbMIDI.sendNoteOn(note, velocity, channel);
}

void myNoteOff(byte channel, byte note, byte velocity) {
  usbMIDI.sendNoteOff(note, velocity, channel);
}

void convertIncomingNote() {

  if (learning && noteArrived) {
    switch (learningDisplayNumber) {

      case 1:
        leadBottomNote = learningNote;
        setLEDDisplay1();
        display1.setBacklight(LEDintensity);
        displayLEDNumber(1, leadBottomNote);
        // delay(5);
        // usbMIDI.sendNoteOn(leadBottomNote, 127, midiOutCh);  //MIDI USB is set to Out
        // usbMIDI.sendNoteOff(leadBottomNote, 0, midiOutCh);   //MIDI USB is set to Out
        // delay(5);
        // MIDI.sendNoteOn(leadBottomNote, 127, midiOutCh);  //MIDI DIN is set to Out
        // MIDI.sendNoteOff(leadBottomNote, 0, midiOutCh);
        updateleadTopNote();
        break;

      case 0:
        leadTopNote = learningNote;
        leadLearn = 0;
        updateleadLearn();
        learning = false;
        // delay(5);
        // usbMIDI.sendNoteOn(leadTopNote, 127, midiOutCh);  //MIDI USB is set to Out
        // usbMIDI.sendNoteOff(leadTopNote, 0, midiOutCh);   //MIDI USB is set to Out
        // delay(5);
        // MIDI.sendNoteOn(leadTopNote, 127, midiOutCh);  //MIDI DIN is set to Out
        // MIDI.sendNoteOff(leadTopNote, 0, midiOutCh);
        break;

      case 3:
        polyBottomNote = learningNote;
        setLEDDisplay3();
        display3.setBacklight(LEDintensity);
        displayLEDNumber(3, polyBottomNote);
        updatepolyTopNote();
        break;

      case 2:
        polyTopNote = learningNote;
        polyLearn = 0;
        updatepolyLearn();
        learning = false;
        break;

      case 5:
        stringsBottomNote = learningNote;
        setLEDDisplay5();
        display5.setBacklight(LEDintensity);
        displayLEDNumber(5, stringsBottomNote);
        updatestringsTopNote();
        break;

      case 4:
        stringsTopNote = learningNote;
        stringsLearn = 0;
        updatestringsLearn();
        learning = false;
        break;

      case 7:
        bassBottomNote = learningNote;
        setLEDDisplay7();
        display7.setBacklight(LEDintensity);
        displayLEDNumber(7, bassBottomNote);
        updatebassTopNote();
        break;

      case 6:
        bassTopNote = learningNote;
        bassLearn = 0;
        updatebassLearn();
        learning = false;
        break;
    }
    noteArrived = false;
  }
}

void myConvertControlChange(byte channel, byte number, byte value) {
  int newvalue = value;
  myControlChange(channel, number, newvalue);
}

void single() {
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      //ledpanel.setIntensity(0, 1);
      if (SLIDERintensity == 1) {
        ledpanel.setLed(0, row, col, true);
      } else {
        ledpanel.setLed(0, row, col, false);
      }
    }
  }
}

void myPitchBend(byte channel, int bend) {
  usbMIDI.sendPitchBend(bend, channel);
}

void allNotesOff() {
}

void updatetrillUp() {
  if (trillUp == 1) {
    trillValue++;
    if (trillValue > 60) {
      trillValue = 60;
    }
    if (trillValue == 0) {
      trillValue = 1;
    }
    displayLEDNumber(8, trillValue);
    midiCCOut(CCtrillUp, 127);
    trillUp = 0;
  } else {
    displayLEDNumber(8, trillValue);
  }
}

void updateTrills() {
  displayLEDNumber(8, trillValue);
  if (trillValue > 0) {
    for (int i = 1; i < trillValue; i++) {
      midiCCOut(CCtrillUp, 127);
    }
  }
  if (trillValue < 0) {
    int negativetrillValue = abs(trillValue);
    for (int i = 0; i < negativetrillValue; i++) {
      midiCCOut(CCtrillDown, 127);
    }
  }
}

void updatetrillDown() {
  if (trillDown == 1) {
    trillValue = trillValue - 1;
    if (trillValue < -60) {
      trillValue = -60;
    }
    if (trillValue == 0) {
      trillValue = -1;
    }
    displayLEDNumber(8, trillValue);
    midiCCOut(CCtrillDown, 127);
    trillDown = 0;
  } else {
    displayLEDNumber(8, trillValue);
  }
}
void updatemodWheel() {
  if (modWheel > 63) {
    showCurrentParameterPage("LFO Wheel", String("On"));
    sr.writePin(MOD_WHEEL_LED, HIGH);
    midiCCOut(CCmodWheel, CC_ON);
  } else {
    showCurrentParameterPage("LFO Wheel", String("Off"));
    sr.writePin(MOD_WHEEL_LED, LOW);
    midiCCOut(CCmodWheel, 0);
  }
}

void updateleadMix() {
  showCurrentParameterPage("Lead Mix", String(leadMixstr) + " dB");
  midiCCOut(CCleadMix, leadMix);
}

void updatephaserSpeed() {
  if (modSourcePhaser == 2) {
    showCurrentParameterPage("Sweep Speed", String(phaserSpeedstr) + " Hz");
    midiCCOut(CCphaserSpeed, phaserSpeed);
  } else {
    showCurrentParameterPage("Sweep Speed", String("Inactive"));
  }
}

void updatepolyMix() {
  showCurrentParameterPage("Poly Mix", String(polyMixstr) + " dB");
  midiCCOut(CCpolyMix, polyMix);
}

void updatephaserRes() {
  showCurrentParameterPage("Phaser Res", String(phaserResstr) + " %");
  midiCCOut(CCphaserRes, phaserRes);
}

void updatestringMix() {
  showCurrentParameterPage("String Mix", String(stringMixstr) + " dB");
  midiCCOut(CCstringMix, stringMix);
}

void updateleadVCFCutoff() {
  showCurrentParameterPage("VCF Cutoff", String(leadVCFCutoffstr) + " Hz");
  midiCCOut(CCleadVCFCutoff, leadVCFCutoff);
}

void updatechorusFlange() {
  if (chorusFlange > 63) {
    showCurrentParameterPage("Mode", String("Flange"));
    midiCCOut(CCchorusFlange, CC_ON);
  } else {
    showCurrentParameterPage("Mode", String("Chorus"));
    midiCCOut(CCchorusFlange, 0);
  }
}

void updatebassMix() {
  showCurrentParameterPage("Bass Mix", String(bassMixstr) + " dB");
  midiCCOut(CCbassMix, bassMix);
}

void updatechorusSpeed() {
  if (chorusFlange > 63) {
    showCurrentParameterPage("Flanger Speed", String(chorusSpeedstr) + " Hz");
    midiCCOut(CCchorusSpeed, chorusSpeed);
  } else {
    showCurrentParameterPage("Chorus Speed", String(chorusSpeedstr) + " Hz");
    midiCCOut(CCchorusSpeed, chorusSpeed);
  }
}

void updatelfoDelay() {
  if (lfoDelay > 63) {
    showCurrentParameterPage("LFO Delay", String("On"));
    sr.writePin(LFO_DELAY_LED, HIGH);
    midiCCOut(CClfoDelay, CC_ON);
  } else {
    showCurrentParameterPage("LFO Delay", String("Off"));
    sr.writePin(LFO_DELAY_LED, LOW);
    midiCCOut(CClfoDelay, 0);
  }
}

void updatechorusDepth() {
  if (chorusFlange > 63) {
    showCurrentParameterPage("Flanger Depth", String(chorusDepthstr) + " %");
    midiCCOut(CCchorusDepth, chorusDepth);
  } else {
    showCurrentParameterPage("Chorus Depth", String(chorusDepthstr) + " %");
    midiCCOut(CCchorusDepth, chorusDepth);
  }
}

void updatepolyPWM() {
  showCurrentParameterPage("Poly PWM", String(polyPWMstr) + " %");
  midiCCOut(CCpolyPWM, polyPWM);
}

void updatechorusRes() {
  if (chorusFlange > 63) {
    showCurrentParameterPage("Flanger Res", String(chorusResstr) + " %");
    midiCCOut(CCchorusRes, chorusRes);
  } else {
    showCurrentParameterPage("Flanger Res", String("Inactive"));
  }
}

void updatepolyPW() {
  showCurrentParameterPage("Poly Init PW", String(polyPWstr) + " %");
  midiCCOut(CCpolyPW, polyPW);
}

void updateechoSync() {
  if (echoSync > 63) {
    showCurrentParameterPage("Echo Sync", String("On"));
    sr.writePin(ECHO_SYNC_LED, HIGH);
    midiCCOut(CCechoSync, CC_ON);
  } else {
    showCurrentParameterPage("Echo Sync", String("Off"));
    sr.writePin(ECHO_SYNC_LED, LOW);
    midiCCOut(CCechoSync, 0);
  }
}

void updatelfoSync() {
  if (lfoSync > 63) {
    showCurrentParameterPage("LFO Sync", String("On"));
    sr.writePin(LFO_SYNC_LED, HIGH);
    midiCCOut(CClfoSync, CC_ON);
  } else {
    showCurrentParameterPage("LFO Sync", String("Off"));
    sr.writePin(LFO_SYNC_LED, LOW);
    midiCCOut(CClfoSync, 0);
  }
}

void updatepolyLFOPitch() {
  showCurrentParameterPage("Pitch LFO", String(polyLFOPitchstr) + " %");
  midiCCOut(CCpolyLFOPitch, polyLFOPitch);
}

void updateshSource() {
  if (shSource > 63) {
    showCurrentParameterPage("S/H Source", String("VCO 2"));
    midiCCOut(CCshSource, CC_ON);
  } else {
    showCurrentParameterPage("S/H Source", String("Noise"));
    midiCCOut(CCshSource, 0);
  }
}

void updatepolyLFOVCF() {
  showCurrentParameterPage("VCF LFO", String(polyLFOVCFstr) + " %");
  midiCCOut(CCpolyLFOVCF, polyLFOVCF);
}

void updatepolyVCFRes() {
  showCurrentParameterPage("VCF Res", String(polyVCFResstr) + " %");
  midiCCOut(CCpolyVCFRes, polyVCFRes);
}

void updatestringOctave() {
  switch (stringOctave) {
    case 2:
      showCurrentParameterPage("String Octave", String("Octave Up"));
      midiCCOut(CCstringOctave, CC_ON);
      break;
    case 1:
      showCurrentParameterPage("String Octave", String("Normal"));
      midiCCOut(CCstringOctave, 64);
      break;
    case 0:
      showCurrentParameterPage("String Octave", String("Octave Down"));
      midiCCOut(CCstringOctave, 0);
      break;
  }
}

void updatepolyVCFCutoff() {
  showCurrentParameterPage("VCF Cutoff", String(polyVCFCutoffstr) + " Hz");
  midiCCOut(CCpolyVCFCutoff, polyVCFCutoff);
}

void updatebassOctave() {
  switch (bassOctave) {
    case 2:
      showCurrentParameterPage("Bass Octave", String("Octave Up"));
      midiCCOut(CCbassOctave, CC_ON);
      break;
    case 1:
      showCurrentParameterPage("Bass Octave", String("Normal"));
      midiCCOut(CCbassOctave, 64);
      break;
    case 0:
      showCurrentParameterPage("Bass Octave", String("Octave Down"));
      midiCCOut(CCbassOctave, 0);
      break;
  }
}

void updatepolyADSRDepth() {
  showCurrentParameterPage("ADSR Depth", String(polyADSRDepthstr) + " %");
  midiCCOut(CCpolyADSRDepth, polyADSRDepth);
}

void updatepolyAttack() {
  showCurrentParameterPage("Attack", String(polyAttackstr) + " ms");
  midiCCOut(CCpolyAttack, polyAttack);
}

void updatelfoSpeed() {
  if (lfoSync < 63) {
    showCurrentParameterPage("LFO Speed", String(lfoSpeedstr) + " Hz");
  }
  if (lfoSync > 63) {
    showCurrentParameterPage("LFO Speed", String(lfoSpeedstring));
  }
  midiCCOut(CClfoSpeed, lfoSpeed);
}

void updatepolyDecay() {
  showCurrentParameterPage("Decay", String(polyDecaystr) + " ms");
  midiCCOut(CCpolyDecay, polyDecay);
}

void updateleadPW() {
  showCurrentParameterPage("Lead Init PW", String(leadPWstr) + " %");
  midiCCOut(CCleadPW, leadPW);
}

void updatepolySustain() {
  showCurrentParameterPage("Sustain", String(polySustainstr) + " %");
  midiCCOut(CCpolySustain, polySustain);
}

void updateleadPWM() {
  showCurrentParameterPage("Lead PWM", String(leadPWMstr) + " %");
  midiCCOut(CCleadPWM, leadPWM);
}

void updatepolyRelease() {
  showCurrentParameterPage("Release", String(polyReleasestr) + " ms");
  midiCCOut(CCpolyRelease, polyRelease);
}

void updatebassPitch() {
  showCurrentParameterPage("Bass PB", String(bassPitchstr) + " Semi");
  midiCCOut(CCbassPitch, bassPitch);
}

void updateechoTime() {
  if (echoSync < 63) {
    showCurrentParameterPage("Echo Time", String(echoTimestr) + " ms");
  }
  if (echoSync > 63) {
    showCurrentParameterPage("Echo Time", String(echoTimestring));
  }
  midiCCOut(CCechoTime, echoTime);
}

void updatestringPitch() {
  showCurrentParameterPage("String PB", String(stringPitchstr) + " Semi");
  midiCCOut(CCstringPitch, stringPitch);
}

void updateechoRegen() {
  showCurrentParameterPage("Echo Regen", String(echoRegenstr) + " %");
  midiCCOut(CCechoRegen, echoRegen);
}

void updatepolyPitch() {
  showCurrentParameterPage("Poly PitchBend", String(polyPitchstr) + " Semi");
  midiCCOut(CCpolyPitch, polyPitch);
}

void updateechoDamp() {
  showCurrentParameterPage("Echo Damp", String(echoDampstr) + " %");
  midiCCOut(CCechoDamp, echoDamp);
}

void updatepolyVCF() {
  showCurrentParameterPage("Poly VCF PB", String(polyVCFstr) + " %");
  midiCCOut(CCpolyVCF, polyVCF);
}

void updateechoLevel() {
  showCurrentParameterPage("Echo Level", String(echoLevelstr) + " %");
  midiCCOut(CCechoLevel, echoLevel);
}

void updateleadPitch() {
  showCurrentParameterPage("Lead PB", String(leadPitchstr) + " Semi");
  midiCCOut(CCleadPitch, leadPitch);
}

void updatereverbType() {
  switch (reverbType) {
    case 0:
      showCurrentParameterPage("Reverb Type", String("Hall"));
      midiCCOut(CCreverbType, 0);
      break;
    case 1:
      showCurrentParameterPage("Reverb Type", String("Plate"));
      midiCCOut(CCreverbType, 64);
      break;
    case 2:
      showCurrentParameterPage("Reverb Type", String("Spring"));
      midiCCOut(CCreverbType, 127);
      break;
  }
}

void updateleadVCF() {
  showCurrentParameterPage("Lead VCF PB", String(leadVCFstr) + " %");
  midiCCOut(CCleadVCF, leadVCF);
}

void updatereverbDecay() {
  showCurrentParameterPage("Reverb Decay", String(reverbDecaystr) + " %");
  midiCCOut(CCreverbDecay, reverbDecay);
}

void updatepolyDepth() {
  showCurrentParameterPage("Poly AT Depth", String(polyDepthstr) + " %");
  midiCCOut(CCpolyDepth, polyDepth);
}

void updatereverbDamp() {
  showCurrentParameterPage("Reverb Damp", String(reverbDampstr) + " %");
  midiCCOut(CCreverbDamp, reverbDamp);
}

void updateleadDepth() {
  showCurrentParameterPage("Lead AT Depth", String(leadDepthstr) + " %");
  midiCCOut(CCleadDepth, leadDepth);
}

void updateebassDecay() {
  showCurrentParameterPage("EBass Decay", String(ebassDecaystr) + " ms");
  midiCCOut(CCebassDecay, ebassDecay);
}

void updateebassRes() {
  showCurrentParameterPage("EBass Res", String(ebassResstr) + " %");
  midiCCOut(CCebassRes, ebassRes);
}

void updatestringBassVolume() {
  showCurrentParameterPage("SBass Volume", String(stringBassVolumestr) + " %");
  midiCCOut(CCstringBassVolume, stringBassVolume);
}

void updatereverbLevel() {
  showCurrentParameterPage("Reverb Level", String(reverbLevelstr) + " %");
  midiCCOut(CCreverbLevel, reverbLevel);
}

void updatearpSync() {
  if (arpSync > 63) {
    showCurrentParameterPage("Arp Sync", String("On"));
    sr.writePin(ARP_SYNC_LED, HIGH);
    midiCCOut(CCarpSync, CC_ON);
  } else {
    showCurrentParameterPage("Arp Sync", String("Off"));
    sr.writePin(ARP_SYNC_LED, LOW);
    midiCCOut(CCarpSync, 0);
  }
}

void updatearpSpeed() {
  if (arpSync < 63) {
    showCurrentParameterPage("Arp Speed", String(arpSpeedstr) + " Hz");
  }
  if (arpSync > 63) {
    showCurrentParameterPage("Arp Speed", String(arpSpeedstring));
  }
  midiCCOut(CCarpSpeed, arpSpeed);
}

void updatearpRange() {
  switch (arpRange) {
    case 0:
      showCurrentParameterPage("Arp Range", String("1"));
      midiCCOut(CCarpRange, 0);
      break;
    case 1:
      showCurrentParameterPage("Arp Range", String("2"));
      midiCCOut(CCarpRange, 64);
      break;
    case 2:
      showCurrentParameterPage("Arp Range", String("3"));
      midiCCOut(CCarpRange, CC_ON);
      break;
  }
}

void updatelowEQ() {
  showCurrentParameterPage("Strng Low EQ", String(lowEQstr) + " dB");
  midiCCOut(CClowEQ, lowEQ);
}

void updatehighEQ() {
  showCurrentParameterPage("Strng High EQ", String(highEQstr) + " dB");
  midiCCOut(CChighEQ, highEQ);
}

void updatestringRelease() {
  if (stringReleasestr < 1000) {
    showCurrentParameterPage("String Release", String(stringReleasestr) + " ms");
  } else {
    showCurrentParameterPage("String Release", String(stringReleasestr * 0.001) + " s");
  }
  midiCCOut(CCstringRelease, stringRelease);
}

void updatestringAttack() {
  if (stringAttackstr < 1000) {
    showCurrentParameterPage("String Attack", String(stringAttackstr) + " ms");
  } else {
    showCurrentParameterPage("String Attack", String(stringAttackstr * 0.001) + " s");
  }
  midiCCOut(CCstringAttack, stringAttack);
}

void updatemodSourcePhaser() {
  switch (modSourcePhaser) {
    case 0:
      showCurrentParameterPage("Mod Source", String("ENV"));
      midiCCOut(CCmodSourcePhaser, 0);
      break;
    case 1:
      showCurrentParameterPage("Mod Source", String("S & H"));
      midiCCOut(CCmodSourcePhaser, 64);
      break;
    case 2:
      showCurrentParameterPage("Mod Source", String("LFO"));
      midiCCOut(CCmodSourcePhaser, CC_ON);
      break;
  }
}

void updateportVCO1() {
  showCurrentParameterPage("VCO1 Time", String(portVCO1str) + " ms/oct");
  midiCCOut(CCportVCO1, portVCO1);
}

void updateportVCO2() {
  showCurrentParameterPage("VCO2 Time", String(portVCO2str) + " ms/oct");
  midiCCOut(CCportVCO2, portVCO2);
}

void updateleadLFOPitch() {
  showCurrentParameterPage("LFO Pitch", String(leadLFOPitchstr) + " %");
  midiCCOut(CCleadLFOPitch, leadLFOPitch);
}

void updatevco1Range() {
  showCurrentParameterPage("VCO 1 Range", String(vco1Rangestr) + " Semi");
  midiCCOut(CCvco1Range, vco1Range);
}

void updatevco2Range() {
  showCurrentParameterPage("VCO 2 Range", String(vco2Rangestr) + " Semi");
  midiCCOut(CCvco2Range, vco2Range);
}

void updatevco2Tune() {
  showCurrentParameterPage("VCO 2 Tune", String(vco2Tunestr) + " Semi");
  midiCCOut(CCvco2Tune, vco2Tune);
}

void updatevco2Volume() {
  showCurrentParameterPage("VCO 2 Volume", String(vco2Volumestr) + " dB");
  midiCCOut(CCvco2Volume, vco2Volume);
}

void updateEnvtoVCF() {
  showCurrentParameterPage("Env to VCF", String(EnvtoVCFstr) + " %");
  midiCCOut(CCEnvtoVCF, EnvtoVCF);
}

void updateleadVCFRes() {
  showCurrentParameterPage("VCF Res", String(leadVCFResstr) + " %");
  midiCCOut(CCleadVCFRes, leadVCFRes);
}

void updateleadAttack() {
  showCurrentParameterPage("Attack", String(leadAttackstr) + " ms");
  midiCCOut(CCleadAttack, leadAttack);
}

void updateleadDecay() {
  showCurrentParameterPage("Decay", String(leadDecaystr) + " ms");
  midiCCOut(CCleadDecay, leadDecay);
}

void updateleadSustain() {
  showCurrentParameterPage("Sustain", String(leadSustainstr) + " %");
  midiCCOut(CCleadSustain, leadSustain);
}

void updateleadRelease() {
  showCurrentParameterPage("Release", String(leadReleasestr) + " ms");
  midiCCOut(CCleadRelease, leadRelease);
}

void updatemasterTune() {
  showCurrentParameterPage("Tune", String(masterTunestr) + " Semi");
  midiCCOut(CCmasterTune, masterTune);
}

void updatemasterVolume() {
  showCurrentParameterPage("Master Volume", String(masterVolumestr) + " %");
  midiCCOut(CCmasterVolume, masterVolume);
}


void updateleadPWMmod() {
  if (leadPWMmod == 1) {
    showCurrentParameterPage("PWM Mod", "ADSR");
    sr.writePin(LEAD_PWM_RED_LED, HIGH);
    sr.writePin(LEAD_PWM_GREEN_LED, LOW);
    midiCCOut(CCleadPWMmod, CC_ON);
    midiCCOut(CCleadPWMmod, 0);
  } else {
    sr.writePin(LEAD_PWM_GREEN_LED, HIGH);
    sr.writePin(LEAD_PWM_RED_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("PWM Mod", "LFO");
      midiCCOut(CCleadPWMmod, 127);
      midiCCOut(CCleadPWMmod, 0);
    }
  }
}

void updateleadVCFmod() {
  if (leadVCFmod == 1) {
    showCurrentParameterPage("VCF Mod", "ADSR");
    sr.writePin(LEAD_VCF_RED_LED, HIGH);
    sr.writePin(LEAD_VCF_GREEN_LED, LOW);
    midiCCOut(CCleadVCFmod, CC_ON);
    midiCCOut(CCleadVCFmod, 0);
  } else {
    sr.writePin(LEAD_VCF_GREEN_LED, HIGH);
    sr.writePin(LEAD_VCF_RED_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("VCF Mod", "LFO");
      midiCCOut(CCleadVCFmod, 127);
      midiCCOut(CCleadVCFmod, 0);
    }
  }
}

void updateleadNPlow() {
  if (!lead2ndVoice) {
    if (leadNPlow == 1) {
      showCurrentParameterPage("Note Priority", "Low");
      sr.writePin(LEAD_NP_LOW_LED, HIGH);
      sr.writePin(LEAD_NP_HIGH_LED, LOW);
      sr.writePin(LEAD_NP_LAST_LED, LOW);
      leadNPhigh = 0;
      leadNPlast = 0;
      midiCCOut(CCleadNPlow, CC_ON);
    }
  }
}

void updateleadNPhigh() {
  if (!lead2ndVoice) {
    if (leadNPhigh == 1) {
      showCurrentParameterPage("Note Priority", "High");
      sr.writePin(LEAD_NP_HIGH_LED, HIGH);
      sr.writePin(LEAD_NP_LOW_LED, LOW);
      sr.writePin(LEAD_NP_LAST_LED, LOW);
      leadNPlow = 0;
      leadNPlast = 0;
      midiCCOut(CCleadNPhigh, CC_ON);
    }
  }
}

void updateleadNPlast() {
  if (!lead2ndVoice) {
    if (leadNPlast == 1) {
      showCurrentParameterPage("Note Priority", "Last");
      sr.writePin(LEAD_NP_HIGH_LED, LOW);
      sr.writePin(LEAD_NP_LOW_LED, LOW);
      sr.writePin(LEAD_NP_LAST_LED, HIGH);
      leadNPlow = 0;
      leadNPhigh = 0;
      midiCCOut(CCleadNPlast, CC_ON);
    }
  }
}

void updateleadNoteTrigger() {
  if (leadNoteTrigger == 1) {
    showCurrentParameterPage("Note Trigger", "Single");
    sr.writePin(LEAD_NOTE_TRIGGER_RED_LED, HIGH);
    sr.writePin(LEAD_NOTE_TRIGGER_GREEN_LED, LOW);
    midiCCOut(CCleadNoteTrigger, CC_ON);
    midiCCOut(CCleadNoteTrigger, 0);
  } else {
    sr.writePin(LEAD_NOTE_TRIGGER_GREEN_LED, HIGH);
    sr.writePin(LEAD_NOTE_TRIGGER_RED_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Note Trigger", "Multi");
      midiCCOut(CCleadNoteTrigger, 127);
      midiCCOut(CCleadNoteTrigger, 0);
    }
  }
}

void updateVCO2KBDTrk() {
  if (vco2KBDTrk == 1) {
    showCurrentParameterPage("Keyboard Trk", "On");
    sr.writePin(LEAD_VCO2_KBD_TRK_LED, HIGH);  // LED on
    midiCCOut(CCvco2KBDTrk, CC_ON);
    midiCCOut(CCvco2KBDTrk, 0);
  } else {
    sr.writePin(LEAD_VCO2_KBD_TRK_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Keyboard Trk", "Off");
      midiCCOut(CCvco2KBDTrk, 127);
      midiCCOut(CCvco2KBDTrk, 0);
    }
  }
}

void updatelead2ndVoice() {
  if (lead2ndVoice == 1) {
    showCurrentParameterPage("2nd Voice", "On");
    sr.writePin(LEAD_SECOND_VOICE_LED, HIGH);  // LED on
    sr.writePin(LEAD_NP_HIGH_LED, LOW);        // LED on
    sr.writePin(LEAD_NP_LOW_LED, LOW);         // LED on
    sr.writePin(LEAD_NP_LAST_LED, LOW);        // LED on
    midiCCOut(CClead2ndVoice, 127);
    midiCCOut(CClead2ndVoice, 0);
    prevleadNPlow = leadNPlow;
    prevleadNPhigh = leadNPhigh;
    prevleadNPlast = leadNPlast;
  } else {
    sr.writePin(LEAD_SECOND_VOICE_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("2nd Voice", "Off");
      if (!recallPatchFlag) {

        leadNPlow = prevleadNPlow;
        leadNPhigh = prevleadNPhigh;
        leadNPlast = prevleadNPlast;

        updateleadNPlow();
        updateleadNPhigh();
        updateleadNPlast();
      }
      midiCCOut(CClead2ndVoice, 127);
      midiCCOut(CClead2ndVoice, 0);
    }
  }
}

void updateleadTrill() {
  if (leadTrill == 1) {
    showCurrentParameterPage("Trill", "On");
    sr.writePin(LEAD_TRILL_LED, HIGH);  // LED on
    midiCCOut(CCleadTrill, 127);
    midiCCOut(CCleadTrill, 0);
  } else {
    sr.writePin(LEAD_TRILL_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Trill", "Off");
      midiCCOut(CCleadTrill, 127);
      midiCCOut(CCleadTrill, 0);
    }
  }
}

void updateleadVCO1wave() {
  switch (leadVCO1wave) {
    case 1:
      showCurrentParameterPage("VCO1 Wave", "Square");
      midiCCOut(CCleadVCO1wave, 127);
      midiCCOut(CCleadVCO1wave, 0);
      break;

    case 2:
      showCurrentParameterPage("VCO1 Wave", "Sine");
      midiCCOut(CCleadVCO1wave, 127);
      midiCCOut(CCleadVCO1wave, 0);
      break;

    case 3:
      showCurrentParameterPage("VCO1 Wave", "Triangle");
      midiCCOut(CCleadVCO1wave, 127);
      midiCCOut(CCleadVCO1wave, 0);
      break;

    case 4:
      showCurrentParameterPage("VCO1 Wave", "Noise");
      midiCCOut(CCleadVCO1wave, 127);
      midiCCOut(CCleadVCO1wave, 0);
      break;

    case 5:
      showCurrentParameterPage("VCO1 Wave", "Saw Up");
      midiCCOut(CCleadVCO1wave, 127);
      midiCCOut(CCleadVCO1wave, 0);
      break;
  }
}

void updateVCOWaves() {
  for (int i = 0; i < leadVCO1wave; i++) {
    midiCCOut(CCleadVCO1wave, 127);
    midiCCOut(CCleadVCO1wave, 0);
  }
  for (int i = 0; i < leadVCO2wave; i++) {
    midiCCOut(CCleadVCO2wave, 127);
    midiCCOut(CCleadVCO2wave, 0);
  }
  for (int i = 0; i < polyWave; i++) {
    midiCCOut(CCpolyWave, 127);
    midiCCOut(CCpolyWave, 0);
  }
}

void updateleadVCO2wave() {
  switch (leadVCO2wave) {
    case 1:
      showCurrentParameterPage("VCO2 Wave", "Saw Up");
      midiCCOut(CCleadVCO2wave, 127);
      midiCCOut(CCleadVCO2wave, 0);
      break;

    case 2:
      showCurrentParameterPage("VCO2 Wave", "Square");
      midiCCOut(CCleadVCO2wave, 127);
      midiCCOut(CCleadVCO2wave, 0);
      break;

    case 3:
      showCurrentParameterPage("VCO2 Wave", "Sine");
      midiCCOut(CCleadVCO2wave, 127);
      midiCCOut(CCleadVCO2wave, 0);
      break;

    case 4:
      showCurrentParameterPage("VCO2 Wave", "Triangle");
      midiCCOut(CCleadVCO2wave, 127);
      midiCCOut(CCleadVCO2wave, 0);
      break;

    case 5:
      showCurrentParameterPage("VCO2 Wave", "Noise");
      midiCCOut(CCleadVCO2wave, 127);
      midiCCOut(CCleadVCO2wave, 0);
      break;

    case 6:
      showCurrentParameterPage("VCO2 Wave", "Saw Down");
      midiCCOut(CCleadVCO2wave, 127);
      midiCCOut(CCleadVCO2wave, 0);
      break;
  }
}

void updatepolyWave() {
  switch (polyWave) {
    case 1:
      showCurrentParameterPage("Poly Wave", "Square");
      midiCCOut(CCpolyWave, 127);
      midiCCOut(CCpolyWave, 0);
      break;

    case 2:
      showCurrentParameterPage("Poly Wave", "Triangle");
      midiCCOut(CCpolyWave, 127);
      midiCCOut(CCpolyWave, 0);
      break;

    case 3:
      showCurrentParameterPage("Poly Wave", "Sine");
      midiCCOut(CCpolyWave, 127);
      midiCCOut(CCpolyWave, 0);
      break;

    case 4:
      showCurrentParameterPage("Poly Wave", "Tri-Saw");
      midiCCOut(CCpolyWave, 127);
      midiCCOut(CCpolyWave, 0);
      break;

    case 5:
      showCurrentParameterPage("Poly Wave", "Lumpy");
      midiCCOut(CCpolyWave, 127);
      midiCCOut(CCpolyWave, 0);
      break;

    case 6:
      showCurrentParameterPage("Poly Wave", "Sawtooth");
      midiCCOut(CCpolyWave, 127);
      midiCCOut(CCpolyWave, 0);
      break;
  }
}

void updatepolyPWMSW() {
  if (polyPWMSW == 1) {
    showCurrentParameterPage("PWM Mod", "ADSR");
    sr.writePin(POLY_PWM_RED_LED, HIGH);
    sr.writePin(POLY_PWM_GREEN_LED, LOW);
    midiCCOut(CCpolyPWMSW, 127);
    midiCCOut(CCpolyPWMSW, 0);
  } else {
    sr.writePin(POLY_PWM_GREEN_LED, HIGH);
    sr.writePin(POLY_PWM_RED_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("PWM Mod", "LFO");
      midiCCOut(CCpolyPWMSW, 127);
      midiCCOut(CCpolyPWMSW, 0);
    }
  }
}

void updateLFOTri() {
  if (LFOTri == 1) {
    showCurrentParameterPage("LFO Wave", "Triangle");
    sr.writePin(LFO_TRI_LED, HIGH);
    sr.writePin(LFO_SAW_UP_LED, LOW);
    sr.writePin(LFO_SAW_DN_LED, LOW);
    sr.writePin(LFO_SQUARE_LED, LOW);
    sr.writePin(LFO_SH_LED, LOW);
    LFOSawUp = 0;
    LFOSawDown = 0;
    LFOSquare = 0;
    LFOSH = 0;
    midiCCOut(CCLFOTri, 127);
  }
}

void updateLFOSawUp() {
  if (LFOSawUp == 1) {
    showCurrentParameterPage("LFO Wave", "Saw Up");
    sr.writePin(LFO_TRI_LED, LOW);
    sr.writePin(LFO_SAW_UP_LED, HIGH);
    sr.writePin(LFO_SAW_DN_LED, LOW);
    sr.writePin(LFO_SQUARE_LED, LOW);
    sr.writePin(LFO_SH_LED, LOW);
    LFOTri = 0;
    LFOSawDown = 0;
    LFOSquare = 0;
    LFOSH = 0;
    midiCCOut(CCLFOSawUp, 127);
  }
}

void updateLFOSawDown() {
  if (LFOSawDown == 1) {
    showCurrentParameterPage("LFO Wave", "Saw Down");
    sr.writePin(LFO_TRI_LED, LOW);
    sr.writePin(LFO_SAW_UP_LED, LOW);
    sr.writePin(LFO_SAW_DN_LED, HIGH);
    sr.writePin(LFO_SQUARE_LED, LOW);
    sr.writePin(LFO_SH_LED, LOW);
    LFOTri = 0;
    LFOSawUp = 0;
    LFOSquare = 0;
    LFOSH = 0;
    midiCCOut(CCLFOSawDown, 127);
  }
}

void updateLFOSquare() {
  if (LFOSquare == 1) {
    showCurrentParameterPage("LFO Wave", "Square");
    sr.writePin(LFO_TRI_LED, LOW);
    sr.writePin(LFO_SAW_UP_LED, LOW);
    sr.writePin(LFO_SAW_DN_LED, LOW);
    sr.writePin(LFO_SQUARE_LED, HIGH);
    sr.writePin(LFO_SH_LED, LOW);
    LFOTri = 0;
    LFOSawUp = 0;
    LFOSawDown = 0;
    LFOSH = 0;
    midiCCOut(CCLFOSquare, 127);
  }
}

void updateLFOSH() {
  if (LFOSH == 1) {
    showCurrentParameterPage("LFO Wave", "Sample & Hold");
    sr.writePin(LFO_TRI_LED, LOW);
    sr.writePin(LFO_SAW_UP_LED, LOW);
    sr.writePin(LFO_SAW_DN_LED, LOW);
    sr.writePin(LFO_SQUARE_LED, LOW);
    sr.writePin(LFO_SH_LED, HIGH);
    LFOTri = 0;
    LFOSawUp = 0;
    LFOSawDown = 0;
    LFOSquare = 0;
    midiCCOut(CCLFOSH, 127);
  }
}

void updatestrings8() {
  if (strings8 == 1) {
    showCurrentParameterPage("Strings 8", "On");
    sr.writePin(STRINGS_8_LED, HIGH);  // LED on
    midiCCOut(CCstrings8, 127);
    midiCCOut(CCstrings8, 0);
  } else {
    sr.writePin(STRINGS_8_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Strings 8", "Off");
      midiCCOut(CCstrings8, 127);
      midiCCOut(CCstrings8, 0);
    }
  }
}

void updatestrings4() {
  if (strings4 == 1) {
    showCurrentParameterPage("Strings 4", "On");
    sr.writePin(STRINGS_4_LED, HIGH);  // LED on
    midiCCOut(CCstrings4, 127);
    midiCCOut(CCstrings4, 0);
  } else {
    sr.writePin(STRINGS_4_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Strings 4", "Off");
      midiCCOut(CCstrings4, 127);
      midiCCOut(CCstrings4, 0);
    }
  }
}

void updatepolyNoteTrigger() {
  if (polyNoteTrigger == 1) {
    showCurrentParameterPage("Note Trigger", "Single");
    sr.writePin(POLY_NOTE_TRIGGER_RED_LED, HIGH);
    sr.writePin(POLY_NOTE_TRIGGER_GREEN_LED, LOW);
    midiCCOut(CCpolyNoteTrigger, 127);
    midiCCOut(CCpolyNoteTrigger, 0);
  } else {
    sr.writePin(POLY_NOTE_TRIGGER_GREEN_LED, HIGH);
    sr.writePin(POLY_NOTE_TRIGGER_RED_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Note Trigger", "Multi");
      midiCCOut(CCpolyNoteTrigger, 127);
      midiCCOut(CCpolyNoteTrigger, 0);
    }
  }
}

void updatepolyVelAmp() {
  if (polyVelAmp == 1) {
    showCurrentParameterPage("Amp velocity", "On");
    sr.writePin(POLY_VEL_AMP_LED, HIGH);  // LED on
    midiCCOut(CCpolyVelAmp, 127);
    midiCCOut(CCpolyVelAmp, 0);
  } else {
    sr.writePin(POLY_VEL_AMP_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Amp Velocity", "Off");
      midiCCOut(CCpolyVelAmp, 127);
      midiCCOut(CCpolyVelAmp, 0);
    }
  }
}

void updatepolyDrift() {
  if (polyDrift == 1) {
    showCurrentParameterPage("Poly Drift", "On");
    sr.writePin(POLY_DRIFT_LED, HIGH);  // LED on
    midiCCOut(CCpolyDrift, 127);
    midiCCOut(CCpolyDrift, 0);
  } else {
    sr.writePin(POLY_DRIFT_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Poly Drift", "Off");
      midiCCOut(CCpolyDrift, 127);
      midiCCOut(CCpolyDrift, 0);
    }
  }
}

void updatepoly16() {
  if (poly16 == 1) {
    showCurrentParameterPage("Poly 16", "On");
    sr.writePin(POLY_16_LED, HIGH);  // LED on
    midiCCOut(CCpoly16, 127);
    midiCCOut(CCpoly16, 0);
  } else {
    sr.writePin(POLY_16_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Poly 16", "Off");
      midiCCOut(CCpoly16, 127);
      midiCCOut(CCpoly16, 0);
    }
  }
}

void updatepoly8() {
  if (poly8 == 1) {
    showCurrentParameterPage("Poly 8", "On");
    sr.writePin(POLY_8_LED, HIGH);  // LED on
    midiCCOut(CCpoly8, 127);
    midiCCOut(CCpoly8, 0);
  } else {
    sr.writePin(POLY_8_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Poly 8", "Off");
      midiCCOut(CCpoly8, 127);
      midiCCOut(CCpoly8, 0);
    }
  }
}

void updatepoly4() {
  if (poly4 == 1) {
    showCurrentParameterPage("Poly 4", "On");
    sr.writePin(POLY_4_LED, HIGH);  // LED on
    midiCCOut(CCpoly4, 127);
    midiCCOut(CCpoly4, 0);
  } else {
    sr.writePin(POLY_4_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Poly 4", "Off");
      midiCCOut(CCpoly4, 127);
      midiCCOut(CCpoly4, 0);
    }
  }
}

void updateebass16() {
  if (ebass16 == 1) {
    showCurrentParameterPage("Elec Bass 16", "On");
    sr.writePin(EBASS_16_LED, HIGH);  // LED on
    midiCCOut(CCebass16, 127);
    midiCCOut(CCebass16, 0);
  } else {
    sr.writePin(EBASS_16_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Elec Bass 16", "Off");
      midiCCOut(CCebass16, 127);
      midiCCOut(CCebass16, 0);
    }
  }
}

void updateebass8() {
  if (ebass8 == 1) {
    showCurrentParameterPage("Elec Bass 8", "On");
    sr.writePin(EBASS_8_LED, HIGH);  // LED on
    midiCCOut(CCebass8, 127);
    midiCCOut(CCebass8, 0);
  } else {
    sr.writePin(EBASS_8_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Elec Bass 8", "Off");
      midiCCOut(CCebass8, 127);
      midiCCOut(CCebass8, 0);
    }
  }
}

void updatebassNoteTrigger() {
  if (bassNoteTrigger == 1) {
    showCurrentParameterPage("Note Trigger", "Single");
    sr.writePin(BASS_NOTE_TRIGGER_RED_LED, HIGH);
    sr.writePin(BASS_NOTE_TRIGGER_GREEN_LED, LOW);
    midiCCOut(CCbassNoteTrigger, 127);
    midiCCOut(CCbassNoteTrigger, 0);
  } else {
    sr.writePin(BASS_NOTE_TRIGGER_GREEN_LED, HIGH);
    sr.writePin(BASS_NOTE_TRIGGER_RED_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Note Trigger", "Multi");
      midiCCOut(CCbassNoteTrigger, 127);
      midiCCOut(CCbassNoteTrigger, 0);
    }
  }
}

void updatestringbass16() {
  if (stringbass16 == 1) {
    showCurrentParameterPage("String Bass 16", "On");
    sr.writePin(STRINGS_BASS_16_LED, HIGH);  // LED on
    midiCCOut(CCstringbass16, 127);
    midiCCOut(CCstringbass16, 0);
  } else {
    sr.writePin(STRINGS_BASS_16_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("String Bass 16", "Off");
      midiCCOut(CCstringbass16, 127);
      midiCCOut(CCstringbass16, 0);
    }
  }
}

void updatestringbass8() {
  if (stringbass8 == 1) {
    showCurrentParameterPage("String Bass 8", "On");
    sr.writePin(STRINGS_BASS_8_LED, HIGH);  // LED on
    midiCCOut(CCstringbass8, 127);
    midiCCOut(CCstringbass8, 0);
  } else {
    sr.writePin(STRINGS_BASS_8_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("String Bass 8", "Off");
      midiCCOut(CCstringbass8, 127);
      midiCCOut(CCstringbass8, 0);
    }
  }
}

void updatehollowWave() {
  if (hollowWave == 1) {
    showCurrentParameterPage("Hollow Wave", "On");
    sr.writePin(HOLLOW_WAVE_LED, HIGH);  // LED on
    midiCCOut(CChollowWave, 127);
    midiCCOut(CChollowWave, 0);
  } else {
    sr.writePin(HOLLOW_WAVE_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Hollow Wave", "Off");
      midiCCOut(CChollowWave, 127);
      midiCCOut(CChollowWave, 0);
    }
  }
}

void updatebassPitchSW() {
  if (bassPitchSW == 1) {
    showCurrentParameterPage("Bass PB", "On");
    sr.writePin(BASS_PITCH_LED, HIGH);  // LED on
    midiCCOut(CCbassPitchSW, 127);
    midiCCOut(CCbassPitchSW, 0);
  } else {
    sr.writePin(BASS_PITCH_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Bass PB", "Off");
      midiCCOut(CCbassPitchSW, 127);
      midiCCOut(CCbassPitchSW, 0);
    }
  }
}

void updatestringsPitchSW() {
  if (stringsPitchSW == 1) {
    showCurrentParameterPage("Strings PB", "On");
    sr.writePin(STRINGS_PITCH_LED, HIGH);  // LED on
    midiCCOut(CCstringsPitchSW, 127);
    midiCCOut(CCstringsPitchSW, 0);
  } else {
    sr.writePin(STRINGS_PITCH_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Strings PB", "Off");
      midiCCOut(CCstringsPitchSW, 127);
      midiCCOut(CCstringsPitchSW, 0);
    }
  }
}

void updatepolyPitchSW() {
  if (polyPitchSW == 1) {
    showCurrentParameterPage("Poly Pitch PB", "On");
    sr.writePin(POLY_PITCH_LED, HIGH);  // LED on
    midiCCOut(CCpolyPitchSW, 127);
    midiCCOut(CCpolyPitchSW, 0);
  } else {
    sr.writePin(POLY_PITCH_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Poly Pitch PB", "Off");
      midiCCOut(CCpolyPitchSW, 127);
      midiCCOut(CCpolyPitchSW, 0);
    }
  }
}

void updatepolyVCFSW() {
  if (polyVCFSW == 1) {
    showCurrentParameterPage("Poly VCF PB", "On");
    sr.writePin(POLY_VCF_LED, HIGH);  // LED on
    midiCCOut(CCpolyVCFSW, 127);
    midiCCOut(CCpolyVCFSW, 0);
  } else {
    sr.writePin(POLY_VCF_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Poly VCF PB", "Off");
      midiCCOut(CCpolyVCFSW, 127);
      midiCCOut(CCpolyVCFSW, 0);
    }
  }
}

void updateleadPitchSW() {
  if (leadPitchSW == 1) {
    showCurrentParameterPage("Lead Pitch PB", "On");
    sr.writePin(LEAD_PITCH_LED, HIGH);  // LED on
    midiCCOut(CCleadPitchSW, 127);
    midiCCOut(CCleadPitchSW, 0);
  } else {
    sr.writePin(LEAD_PITCH_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Lead Pitch PB", "Off");
      midiCCOut(CCleadPitchSW, 127);
      midiCCOut(CCleadPitchSW, 0);
    }
  }
}

void updateleadVCFSW() {
  if (leadVCFSW == 1) {
    showCurrentParameterPage("Lead VCF PB", "On");
    sr.writePin(LEAD_VCF_LED, HIGH);  // LED on
    midiCCOut(CCleadVCFSW, 127);
    midiCCOut(CCleadVCFSW, 0);
  } else {
    sr.writePin(LEAD_VCF_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Lead VCF PB", "Off");
      midiCCOut(CCleadVCFSW, 127);
      midiCCOut(CCleadVCFSW, 0);
    }
  }
}

void updatepolyAfterSW() {
  if (polyAfterSW == 1) {
    showCurrentParameterPage("Poly AfterT", "Vol / Brill");
    sr.writePin(POLY_TOUCH_DEST_RED_LED, HIGH);
    sr.writePin(POLY_TOUCH_DEST_GREEN_LED, LOW);
    midiCCOut(CCpolyAfterSW, 127);
    midiCCOut(CCpolyAfterSW, 0);
  } else {
    sr.writePin(POLY_TOUCH_DEST_GREEN_LED, HIGH);
    sr.writePin(POLY_TOUCH_DEST_RED_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Poly AfterT", "PitchBend");
      midiCCOut(CCpolyAfterSW, 127);
      midiCCOut(CCpolyAfterSW, 0);
    }
  }
}

void updateleadAfterSW() {
  if (leadAfterSW == 1) {
    showCurrentParameterPage("Lead AfterT", "Vol / Brill");
    sr.writePin(LEAD_TOUCH_DEST_RED_LED, HIGH);
    sr.writePin(LEAD_TOUCH_DEST_GREEN_LED, LOW);
    midiCCOut(CCleadAfterSW, 127);
    midiCCOut(CCleadAfterSW, 0);
  } else {
    sr.writePin(LEAD_TOUCH_DEST_GREEN_LED, HIGH);
    sr.writePin(LEAD_TOUCH_DEST_RED_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Lead AfterT", "PitchBend");
      midiCCOut(CCleadAfterSW, 127);
      midiCCOut(CCleadAfterSW, 0);
    }
  }
}

void updatephaserBassSW() {
  if (phaserBassSW == 1) {
    showCurrentParameterPage("Bass Phaser", "On");
    sr.writePin(PHASE_BASS_LED, HIGH);  // LED on
    midiCCOut(CCphaserBassSW, 127);
    midiCCOut(CCphaserBassSW, 0);
  } else {
    sr.writePin(PHASE_BASS_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Bass Phaser", "Off");
      midiCCOut(CCphaserBassSW, 127);
      midiCCOut(CCphaserBassSW, 0);
    }
  }
}

void updatephaserStringsSW() {
  if (phaserStringsSW == 1) {
    showCurrentParameterPage("Strings Phaser", "On");
    sr.writePin(PHASE_STRINGS_LED, HIGH);  // LED on
    midiCCOut(CCphaserStringsSW, 127);
    midiCCOut(CCphaserStringsSW, 0);
  } else {
    sr.writePin(PHASE_STRINGS_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Strings Phaser", "Off");
      midiCCOut(CCphaserStringsSW, 127);
      midiCCOut(CCphaserStringsSW, 0);
    }
  }
}

void updatephaserPolySW() {
  if (phaserPolySW == 1) {
    showCurrentParameterPage("Poly Phaser", "On");
    sr.writePin(PHASE_POLY_LED, HIGH);  // LED on
    midiCCOut(CCphaserPolySW, 127);
    midiCCOut(CCphaserPolySW, 0);
  } else {
    sr.writePin(PHASE_POLY_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Poly Phaser", "Off");
      midiCCOut(CCphaserPolySW, 127);
      midiCCOut(CCphaserPolySW, 0);
    }
  }
}

void updatephaserLeadSW() {
  if (phaserLeadSW == 1) {
    showCurrentParameterPage("Lead Phaser", "On");
    sr.writePin(PHASE_LEAD_LED, HIGH);  // LED on
    midiCCOut(CCphaserLeadSW, 127);
  } else {
    sr.writePin(PHASE_LEAD_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Lead Phaser", "Off");
      midiCCOut(CCphaserLeadSW, 127);
    }
  }
}

void updatechorusBassSW() {
  if (chorusBassSW == 1) {
    showCurrentParameterPage("Bass Chorus", "On");
    sr.writePin(CHORUS_BASS_LED, HIGH);  // LED on
    midiCCOut(CCchorusBassSW, 127);
  } else {
    sr.writePin(CHORUS_BASS_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Bass Chorus", "Off");
      midiCCOut(CCchorusBassSW, 127);
    }
  }
}

void updatechorusStringsSW() {
  if (chorusStringsSW == 1) {
    showCurrentParameterPage("Strings Chorus", "On");
    sr.writePin(CHORUS_STRINGS_LED, HIGH);  // LED on
    midiCCOut(CCchorusStringsSW, 127);
  } else {
    sr.writePin(CHORUS_STRINGS_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Strings Chorus", "Off");
      midiCCOut(CCchorusStringsSW, 127);
    }
  }
}

void updatechorusPolySW() {
  if (chorusPolySW == 1) {
    showCurrentParameterPage("Poly Chorus", "On");
    sr.writePin(CHORUS_POLY_LED, HIGH);  // LED on
    midiCCOut(CCchorusPolySW, 127);
  } else {
    sr.writePin(CHORUS_POLY_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Poly Chorus", "Off");
      midiCCOut(CCchorusPolySW, 127);
    }
  }
}

void updatechorusLeadSW() {
  if (chorusLeadSW == 1) {
    showCurrentParameterPage("Lead Chorus", "On");
    sr.writePin(CHORUS_LEAD_LED, HIGH);  // LED on
    midiCCOut(CCchorusLeadSW, 127);
  } else {
    sr.writePin(CHORUS_LEAD_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Lead Chorus", "Off");
      midiCCOut(CCchorusLeadSW, 127);
    }
  }
}

void updateechoBassSW() {
  if (echoBassSW == 1) {
    showCurrentParameterPage("Bass Echo", "On");
    sr.writePin(ECHO_BASS_LED, HIGH);  // LED on
    midiCCOut(CCechoBassSW, 127);
  } else {
    sr.writePin(ECHO_BASS_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Bass Echo", "Off");
      midiCCOut(CCechoBassSW, 127);
    }
  }
}

void updateechoStringsSW() {
  if (echoStringsSW == 1) {
    showCurrentParameterPage("Strings Echo", "On");
    sr.writePin(ECHO_STRINGS_LED, HIGH);  // LED on
    midiCCOut(CCechoStringsSW, 127);
  } else {
    sr.writePin(ECHO_STRINGS_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Strings Echo", "Off");
      midiCCOut(CCechoStringsSW, 127);
    }
  }
}

void updateechoPolySW() {
  if (echoPolySW == 1) {
    showCurrentParameterPage("Poly Echo", "On");
    sr.writePin(ECHO_POLY_LED, HIGH);  // LED on
    midiCCOut(CCechoPolySW, 127);
  } else {
    sr.writePin(ECHO_POLY_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Poly Echo", "Off");
      midiCCOut(CCechoPolySW, 127);
    }
  }
}

void updateechoLeadSW() {
  if (echoLeadSW == 1) {
    showCurrentParameterPage("Lead Echo", "On");
    sr.writePin(ECHO_LEAD_LED, HIGH);  // LED on
    midiCCOut(CCechoLeadSW, 127);
  } else {
    sr.writePin(ECHO_LEAD_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Lead Echo", "Off");
      midiCCOut(CCechoLeadSW, 127);
    }
  }
}

void updatereverbBassSW() {
  if (reverbBassSW == 1) {
    showCurrentParameterPage("Bass Reverb", "On");
    sr.writePin(REVERB_BASS_LED, HIGH);  // LED on
    midiCCOut(CCreverbBassSW, 127);
  } else {
    sr.writePin(REVERB_BASS_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Bass Reverb", "Off");
      midiCCOut(CCreverbBassSW, 127);
    }
  }
}

void updatereverbStringsSW() {
  if (reverbStringsSW == 1) {
    showCurrentParameterPage("Strings Reverb", "On");
    sr.writePin(REVERB_STRINGS_LED, HIGH);  // LED on
    midiCCOut(CCreverbStringsSW, 127);
  } else {
    sr.writePin(REVERB_STRINGS_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Strings Reverb", "Off");
      midiCCOut(CCreverbStringsSW, 127);
    }
  }
}

void updatereverbPolySW() {
  if (reverbPolySW == 1) {
    showCurrentParameterPage("Poly Reverb", "On");
    sr.writePin(REVERB_POLY_LED, HIGH);  // LED on
    midiCCOut(CCreverbPolySW, 127);
  } else {
    sr.writePin(REVERB_POLY_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Poly Reverb", "Off");
      midiCCOut(CCreverbPolySW, 127);
    }
  }
}

void updatereverbLeadSW() {
  if (reverbLeadSW == 1) {
    showCurrentParameterPage("Lead Reverb", "On");
    sr.writePin(REVERB_LEAD_LED, HIGH);  // LED on
    midiCCOut(CCreverbLeadSW, 127);
  } else {
    sr.writePin(REVERB_LEAD_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Lead Reverb", "Off");
      midiCCOut(CCreverbLeadSW, 127);
    }
  }
}

void updatearpOnSW() {
  if (arpOnSW == 1) {
    showCurrentParameterPage("Arpeggiator", "On");
    sr.writePin(ARP_ON_OFF_LED, HIGH);  // LED on
    midiCCOut(CCarpOnSW, 127);
  } else {
    sr.writePin(ARP_ON_OFF_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Arpeggiator", "Off");
      midiCCOut(CCarpOnSW, 0);
    }
  }
}

void updatearpDownSW() {
  if (arpDownSW == 1) {
    showCurrentParameterPage("Arp Down", "On");
    sr.writePin(ARP_DOWN_LED, HIGH);
    sr.writePin(ARP_UP_LED, LOW);
    sr.writePin(ARP_UP_DOWN_LED, LOW);
    sr.writePin(ARP_RANDOM_LED, LOW);
    arpUpSW = 0;
    arpUpDownSW = 0;
    arpRandomSW = 0;
    midiCCOut(CCarpDownSW, 127);
  }
}

void updatearpUpSW() {
  if (arpUpSW == 1) {
    showCurrentParameterPage("Arp Up", "On");
    sr.writePin(ARP_DOWN_LED, LOW);
    sr.writePin(ARP_UP_LED, HIGH);
    sr.writePin(ARP_UP_DOWN_LED, LOW);
    sr.writePin(ARP_RANDOM_LED, LOW);
    arpDownSW = 0;
    arpUpDownSW = 0;
    arpRandomSW = 0;
    midiCCOut(CCarpUpSW, 127);
  }
}

void updatearpupDownSW() {
  if (arpUpDownSW == 1) {
    showCurrentParameterPage("Arp Up/Down", "On");
    sr.writePin(ARP_DOWN_LED, LOW);
    sr.writePin(ARP_UP_LED, LOW);
    sr.writePin(ARP_UP_DOWN_LED, HIGH);
    sr.writePin(ARP_RANDOM_LED, LOW);
    arpDownSW = 0;
    arpUpSW = 0;
    arpRandomSW = 0;
    midiCCOut(CCarpUpDownSW, 127);
  }
}

void updatearpRandomSW() {
  if (arpRandomSW == 1) {
    showCurrentParameterPage("Arp Random", "On");
    sr.writePin(ARP_DOWN_LED, LOW);
    sr.writePin(ARP_UP_LED, LOW);
    sr.writePin(ARP_UP_DOWN_LED, LOW);
    sr.writePin(ARP_RANDOM_LED, HIGH);
    arpDownSW = 0;
    arpUpSW = 0;
    arpUpDownSW = 0;
    midiCCOut(CCarpRandomSW, 127);
  }
}

void updatearpHoldSW() {
  if (arpHoldSW == 1) {
    showCurrentParameterPage("Arp Hold", "On");
    sr.writePin(ARP_HOLD_LED, HIGH);  // LED on
    midiCCOut(CCarpHoldSW, 127);
  } else {
    sr.writePin(ARP_HOLD_LED, LOW);  // LED off
    if (!recallPatchFlag) {
      showCurrentParameterPage("Arp Hold", "Off");
      midiCCOut(CCarpHoldSW, 0);
    }
  }
}

void updateleadLearn() {
  setLEDDisplay1();
  if (leadLearn == 1) {
    learningDisplayNumber = 1;
    learning = true;
    learn_timer = millis();
    showCurrentParameterPage("Lead Learn", "On");
    displayLEDNumber(1, leadBottomNote);
    midiCCOut(CCleadLearn, 127);
  } else {
    learning = false;
    showCurrentParameterPage("Lead Learn", "Off");
    setLEDDisplay1();
    display1.setBacklight(LEDintensity);
    displayLEDNumber(1, leadBottomNote);
    delay(10);
    setLEDDisplay0();
    display0.setBacklight(LEDintensity);
    displayLEDNumber(0, leadTopNote);
    // usbMIDI.sendNoteOn(leadTopNote, 127, midiOutCh);  //MIDI USB is set to Out
    // usbMIDI.sendNoteOff(leadTopNote, 0, midiOutCh);   //MIDI USB is set to Out
    // delay(5);
    // MIDI.sendNoteOn(leadTopNote, 127, midiOutCh);  //MIDI DIN is set to Out
    // MIDI.sendNoteOff(leadTopNote, 0, midiOutCh);
    midiCCOut(CCleadLearn, 0);
  }
}

void updateleadTopNote() {
  setLEDDisplay0();
  learningDisplayNumber = 0;
  learning = true;
  displayLEDNumber(0, leadTopNote);
  learn_timer = millis();
}

void updatepolyLearn() {
  setLEDDisplay3();
  if (polyLearn == 1) {
    learningDisplayNumber = 3;
    learning = true;
    learn_timer = millis();
    showCurrentParameterPage("Poly Learn", "On");
    displayLEDNumber(3, polyBottomNote);
    midiCCOut(CCpolyLearn, 127);
  } else {
    learning = false;
    showCurrentParameterPage("Poly Learn", "Off");
    setLEDDisplay3();
    display3.setBacklight(LEDintensity);
    displayLEDNumber(3, polyBottomNote);
    delay(10);
    setLEDDisplay2();
    display2.setBacklight(LEDintensity);
    displayLEDNumber(2, polyTopNote);
    midiCCOut(CCpolyLearn, 0);
  }
}

void updatepolyTopNote() {
  setLEDDisplay2();
  learningDisplayNumber = 2;
  learning = true;
  displayLEDNumber(2, polyTopNote);
  learn_timer = millis();
}

void updatestringsLearn() {
  setLEDDisplay5();
  if (stringsLearn == 1) {
    learningDisplayNumber = 5;
    learning = true;
    learn_timer = millis();
    showCurrentParameterPage("Strings Learn", "On");
    displayLEDNumber(5, stringsBottomNote);
    midiCCOut(CCstringsLearn, 127);
  } else {
    learning = false;
    showCurrentParameterPage("Strings Learn", "Off");
    setLEDDisplay5();
    display5.setBacklight(LEDintensity);
    displayLEDNumber(5, stringsBottomNote);
    delay(10);
    setLEDDisplay4();
    display4.setBacklight(LEDintensity);
    displayLEDNumber(4, stringsTopNote);
    midiCCOut(CCstringsLearn, 0);
  }
}

void updatestringsTopNote() {
  setLEDDisplay4();
  learningDisplayNumber = 4;
  learning = true;
  displayLEDNumber(4, stringsTopNote);
  learn_timer = millis();
}

void updatebassLearn() {
  setLEDDisplay7();
  if (bassLearn == 1) {
    learningDisplayNumber = 7;
    learning = true;
    learn_timer = millis();
    showCurrentParameterPage("Bass Learn", "On");
    displayLEDNumber(7, bassBottomNote);
    midiCCOut(CCbassLearn, 127);
  } else {
    learning = false;
    showCurrentParameterPage("Bass Learn", "Off");
    setLEDDisplay7();
    display7.setBacklight(LEDintensity);
    displayLEDNumber(7, bassBottomNote);
    delay(10);
    setLEDDisplay6();
    display6.setBacklight(LEDintensity);
    displayLEDNumber(6, bassTopNote);
    midiCCOut(CCbassLearn, 0);
  }
}

void updatebassTopNote() {
  setLEDDisplay6();
  learningDisplayNumber = 6;
  learning = true;
  displayLEDNumber(6, bassTopNote);
  learn_timer = millis();
}

void updatedisplayNoteRanges() {

  setLEDDisplay1();
  display1.setBacklight(LEDintensity);
  displayLEDNumber(1, leadBottomNote);
  midiCCOut(CCleadLearn, midiOutCh);
  delay(5);
  usbMIDI.sendNoteOn(leadBottomNote, 127, midiOutCh);  //MIDI USB is set to Out
  usbMIDI.sendNoteOff(leadBottomNote, 0, midiOutCh);   //MIDI USB is set to Out
  delay(5);
  usbMIDI.sendNoteOn(leadTopNote, 127, midiOutCh);  //MIDI USB is set to Out
  usbMIDI.sendNoteOff(leadTopNote, 0, midiOutCh);   //MIDI USB is set to Out
  delay(10);
  MIDI.sendNoteOn(leadBottomNote, 127, midiOutCh);  //MIDI DIN is set to Out
  MIDI.sendNoteOff(leadBottomNote, 0, midiOutCh);
  delay(10);
  MIDI.sendNoteOn(leadTopNote, 127, midiOutCh);  //MIDI DIN is set to Out
  MIDI.sendNoteOff(leadTopNote, 0, midiOutCh);

  setLEDDisplay0();
  display0.setBacklight(LEDintensity);
  displayLEDNumber(0, leadTopNote);

  setLEDDisplay3();
  display3.setBacklight(LEDintensity);
  displayLEDNumber(3, polyBottomNote);
  midiCCOut(CCpolyLearn, midiOutCh);
  delay(5);
  usbMIDI.sendNoteOn(polyBottomNote, 127, midiOutCh);  //MIDI USB is set to Out
  usbMIDI.sendNoteOff(polyBottomNote, 0, midiOutCh);   //MIDI USB is set to Out
  delay(5);
  usbMIDI.sendNoteOn(polyTopNote, 127, midiOutCh);  //MIDI USB is set to Out
  usbMIDI.sendNoteOff(polyTopNote, 0, midiOutCh);   //MIDI USB is set to Out
  delay(10);
  MIDI.sendNoteOn(polyBottomNote, 127, midiOutCh);  //MIDI DIN is set to Out
  MIDI.sendNoteOff(polyBottomNote, 0, midiOutCh);
  delay(10);
  MIDI.sendNoteOn(polyTopNote, 127, midiOutCh);  //MIDI DIN is set to Out
  MIDI.sendNoteOff(polyTopNote, 0, midiOutCh);

  setLEDDisplay2();
  display2.setBacklight(LEDintensity);
  displayLEDNumber(2, polyTopNote);

  setLEDDisplay5();
  display5.setBacklight(LEDintensity);
  displayLEDNumber(5, stringsBottomNote);
  midiCCOut(CCstringsLearn, midiOutCh);
  delay(5);
  usbMIDI.sendNoteOn(stringsBottomNote, 127, midiOutCh);  //MIDI USB is set to Out
  usbMIDI.sendNoteOff(stringsBottomNote, 0, midiOutCh);   //MIDI USB is set to Out
  delay(5);
  usbMIDI.sendNoteOn(stringsTopNote, 127, midiOutCh);  //MIDI USB is set to Out
  usbMIDI.sendNoteOff(stringsTopNote, 0, midiOutCh);   //MIDI USB is set to Out
  delay(10);
  MIDI.sendNoteOn(stringsBottomNote, 127, midiOutCh);  //MIDI DIN is set to Out
  MIDI.sendNoteOff(stringsBottomNote, 0, midiOutCh);
  delay(10);
  MIDI.sendNoteOn(stringsTopNote, 127, midiOutCh);  //MIDI DIN is set to Out
  MIDI.sendNoteOff(stringsTopNote, 0, midiOutCh);


  setLEDDisplay4();
  display4.setBacklight(LEDintensity);
  displayLEDNumber(4, stringsTopNote);

  setLEDDisplay7();
  display7.setBacklight(LEDintensity);
  displayLEDNumber(7, bassBottomNote);
  midiCCOut(CCbassLearn, midiOutCh);
  delay(5);
  usbMIDI.sendNoteOn(bassBottomNote, 127, midiOutCh);  //MIDI USB is set to Out
  usbMIDI.sendNoteOff(bassBottomNote, 0, midiOutCh);   //MIDI USB is set to Out
  delay(5);
  usbMIDI.sendNoteOn(bassTopNote, 127, midiOutCh);  //MIDI USB is set to Out
  usbMIDI.sendNoteOff(bassTopNote, 0, midiOutCh);   //MIDI USB is set to Out
  delay(10);
  MIDI.sendNoteOn(bassBottomNote, 127, midiOutCh);  //MIDI DIN is set to Out
  MIDI.sendNoteOff(bassBottomNote, 0, midiOutCh);
  delay(10);
  MIDI.sendNoteOn(bassTopNote, 127, midiOutCh);  //MIDI DIN is set to Out
  MIDI.sendNoteOff(bassTopNote, 0, midiOutCh);


  setLEDDisplay6();
  display6.setBacklight(LEDintensity);
  displayLEDNumber(6, bassTopNote);
}

void updatePatchname() {
  showPatchPage(String(patchNo), patchName);
}

void displayLEDNumber(int displayNumber, int value) {
  if (value > 0 && value <= 9) {
    setCursorPos = 3;
  }
  if (value > 9 && value <= 99) {
    setCursorPos = 2;
  }
  if (value > 99 && value <= 999) {
    setCursorPos = 1;
  }
  if (value < 0) {
    if (value < 0 && value >= -9) {
      setCursorPos = 2;
    }
    if (value < -9 && value >= -99) {
      setCursorPos = 1;
    }
  }

  switch (displayNumber) {
    case 0:
      setLEDDisplay0();
      display0.clear();
      display0.setCursor(0, setCursorPos);
      display0.print(value);
      break;

    case 1:
      setLEDDisplay1();
      display1.clear();
      display1.setCursor(0, setCursorPos);
      display1.print(value);
      break;

    case 2:
      setLEDDisplay2();
      display2.clear();
      display2.setCursor(0, setCursorPos);
      display2.print(value);
      break;

    case 3:
      setLEDDisplay3();
      display3.clear();
      display3.setCursor(0, setCursorPos);
      display3.print(value);
      break;

    case 4:
      setLEDDisplay4();
      display4.clear();
      display4.setCursor(0, setCursorPos);
      display4.print(value);
      break;

    case 5:
      setLEDDisplay5();
      display5.clear();
      display5.setCursor(0, setCursorPos);
      display5.print(value);
      break;

    case 6:
      setLEDDisplay6();
      display6.clear();
      display6.setCursor(0, setCursorPos);
      display6.print(value);
      break;

    case 7:
      setLEDDisplay7();
      display7.clear();
      display7.setCursor(0, setCursorPos);
      display7.print(value);
      break;

    case 8:
      trilldisplay.clear();
      trilldisplay.setCursor(0, setCursorPos);
      trilldisplay.print(value);
      break;
  }
}

void setLEDDisplay0() {
  digitalWrite(LED_MUX_0, LOW);
  digitalWrite(LED_MUX_1, LOW);
  digitalWrite(LED_MUX_2, LOW);
}

void setLEDDisplay1() {
  digitalWrite(LED_MUX_0, HIGH);
  digitalWrite(LED_MUX_1, LOW);
  digitalWrite(LED_MUX_2, LOW);
}

void setLEDDisplay2() {
  digitalWrite(LED_MUX_0, LOW);
  digitalWrite(LED_MUX_1, HIGH);
  digitalWrite(LED_MUX_2, LOW);
}

void setLEDDisplay3() {
  digitalWrite(LED_MUX_0, HIGH);
  digitalWrite(LED_MUX_1, HIGH);
  digitalWrite(LED_MUX_2, LOW);
}

void setLEDDisplay4() {
  digitalWrite(LED_MUX_0, LOW);
  digitalWrite(LED_MUX_1, LOW);
  digitalWrite(LED_MUX_2, HIGH);
}

void setLEDDisplay5() {
  digitalWrite(LED_MUX_0, HIGH);
  digitalWrite(LED_MUX_1, LOW);
  digitalWrite(LED_MUX_2, HIGH);
}

void setLEDDisplay6() {
  digitalWrite(LED_MUX_0, LOW);
  digitalWrite(LED_MUX_1, HIGH);
  digitalWrite(LED_MUX_2, HIGH);
}

void setLEDDisplay7() {
  digitalWrite(LED_MUX_0, HIGH);
  digitalWrite(LED_MUX_1, HIGH);
  digitalWrite(LED_MUX_2, HIGH);
}

void myControlChange(byte channel, byte control, int value) {
  switch (control) {

    case CCmodWheelinput:
      usbMIDI.sendControlChange(control, value, channel);
      break;

    case CCmodWheel:
      modWheel = value;
      updatemodWheel();
      break;

    case CCportVCO2:
      portVCO2 = value;
      portVCO2str = QUADRAPORT[value];  // for display
      updateportVCO2();
      break;

    case CCleadMix:
      leadMix = value;
      leadMixstr = QUADRA100[value];  // for display
      updateleadMix();
      break;

    case CCphaserSpeed:
      phaserSpeed = value;
      phaserSpeedstr = QUADRAPHASER[value];  // for display
      updatephaserSpeed();
      break;

    case CCpolyMix:
      polyMix = value;
      polyMixstr = QUADRA100[value];  // for display
      updatepolyMix();
      break;

    case CCphaserRes:
      phaserRes = value;
      phaserResstr = QUADRA100[value];  // for display
      updatephaserRes();
      break;

    case CCstringMix:
      stringMix = value;
      stringMixstr = QUADRA100[value];  // for display
      updatestringMix();
      break;

    case CCchorusFlange:
      chorusFlange = value;
      updatechorusFlange();
      break;

    case CCbassMix:
      bassMix = value;
      bassMixstr = QUADRA100[value];  // for display
      updatebassMix();
      break;

    case CCchorusSpeed:
      chorusSpeed = value;
      chorusSpeedstr = QUADRACHORUS[value];  // for display
      updatechorusSpeed();
      break;

    case CClfoDelay:
      lfoDelay = value;
      updatelfoDelay();
      break;

    case CCchorusDepth:
      chorusDepth = value;
      chorusDepthstr = QUADRA100[value];  // for display
      updatechorusDepth();
      break;

    case CCpolyPWM:
      polyPWM = value;
      polyPWMstr = QUADRA100LOG[value];  // for display
      updatepolyPWM();
      break;

    case CCchorusRes:
      chorusRes = value;
      chorusResstr = QUADRA100[value];  // for display
      updatechorusRes();
      break;

    case CCpolyPW:
      polyPW = value;
      polyPWstr = QUADRAINITPW[value];  // for display
      updatepolyPW();
      break;

    case CCechoSync:
      echoSync = value;
      updateechoSync();
      break;

    case CClfoSync:
      lfoSync = value;
      updatelfoSync();
      break;

    case CCpolyLFOPitch:
      polyLFOPitch = value;
      polyLFOPitchstr = QUADRA100LOG[value];
      updatepolyLFOPitch();
      break;

    case CCshSource:
      shSource = value;
      updateshSource();
      break;

    case CCpolyLFOVCF:
      polyLFOVCF = value;
      polyLFOVCFstr = QUADRA100LOG[value];
      updatepolyLFOVCF();
      break;

    case CCpolyVCFRes:
      polyVCFRes = value;
      polyVCFResstr = QUADRAVCFRES[value];
      updatepolyVCFRes();
      break;

    case CCstringOctave:
      stringOctave = value;
      stringOctave = map(stringOctave, 0, 127, 0, 2);
      updatestringOctave();
      break;

    case CCpolyVCFCutoff:
      polyVCFCutoff = value;
      polyVCFCutoffstr = QUADRACUTOFF[value];
      updatepolyVCFCutoff();
      break;

    case CCbassOctave:
      bassOctave = value;
      bassOctave = map(bassOctave, 0, 127, 0, 2);
      updatebassOctave();
      break;

    case CCpolyADSRDepth:
      polyADSRDepth = value;
      polyADSRDepthstr = QUADRA100LOG[value];
      updatepolyADSRDepth();
      break;

    case CCpolyAttack:
      polyAttack = value;
      polyAttackstr = QUADRAPOLYATTACK[value];
      updatepolyAttack();
      break;

    case CClfoSpeed:
      lfoSpeed = value;
      if (lfoSync < 63) {
        lfoSpeedstr = QUADRALFO[value];
      } else {
        lfoSpeedmap = map(lfoSpeed, 0, 127, 0, 19);
        lfoSpeedstring = QUADRAARPSYNC[lfoSpeedmap];
      }
      updatelfoSpeed();
      break;

    case CCpolyDecay:
      polyDecay = value;
      polyDecaystr = QUADRAPOLYDECAY[value];
      updatepolyDecay();
      break;

    case CCleadPW:
      leadPW = value;
      leadPWstr = QUADRAINITPW[value];
      updateleadPW();
      break;

    case CCpolySustain:
      polySustain = value;
      polySustainstr = QUADRA100[value];
      updatepolySustain();
      break;

    case CCleadPWM:
      leadPWM = value;
      leadPWMstr = QUADRA100LOG[value];
      updateleadPWM();
      break;

    case CCpolyRelease:
      polyRelease = value;
      polyReleasestr = QUADRAPOLYRELEASE[value];
      updatepolyRelease();
      break;

    case CCbassPitch:
      bassPitch = value;
      bassPitchstr = QUADRABENDPITCH[value];
      updatebassPitch();
      break;

    case CCechoTime:
      echoTime = value;
      if (echoSync < 63) {
        echoTimestr = QUADRAECHOTIME[value];
      }
      if (echoSync > 63) {
        echoTimemap = map(echoTime, 0, 127, 0, 19);
        echoTimestring = QUADRAECHOSYNC[echoTimemap];
      }
      updateechoTime();
      break;

    case CCstringPitch:
      stringPitch = value;
      stringPitchstr = QUADRABENDPITCH[value];
      updatestringPitch();
      break;

    case CCechoRegen:
      echoRegen = value;
      echoRegenstr = QUADRA100[value];
      updateechoRegen();
      break;

    case CCpolyPitch:
      polyPitch = value;
      polyPitchstr = QUADRABENDPITCH[value];
      updatepolyPitch();
      break;

    case CCechoDamp:
      echoDamp = value;
      echoDampstr = QUADRA100[value];
      updateechoDamp();
      break;

    case CCpolyVCF:
      polyVCF = value;
      polyVCFstr = QUADRA100[value];
      updatepolyVCF();
      break;

    case CCechoLevel:
      echoLevel = value;
      echoLevelstr = QUADRA100[value];
      updateechoLevel();
      break;

    case CCleadPitch:
      leadPitch = value;
      leadPitchstr = QUADRABENDPITCH[value];
      updateleadPitch();
      break;

    case CCreverbType:
      reverbType = value;
      reverbType = map(reverbType, 0, 127, 0, 2);
      updatereverbType();
      break;

    case CCleadVCF:
      leadVCF = value;
      leadVCFstr = QUADRA100[value];
      updateleadVCF();
      break;

    case CCreverbDecay:
      reverbDecay = value;
      reverbDecaystr = QUADRA100[value];
      updatereverbDecay();
      break;

    case CCpolyDepth:
      polyDepth = value;
      polyDepthstr = QUADRA100[value];
      updatepolyDepth();
      break;

    case CCreverbDamp:
      reverbDamp = value;
      reverbDampstr = QUADRA100[value];
      updatereverbDamp();
      break;

    case CCleadDepth:
      leadDepth = value;
      leadDepthstr = QUADRA100[value];
      updateleadDepth();
      break;

    case CCebassDecay:
      ebassDecay = value;
      ebassDecaystr = QUADRAEBASSDECAY[value];
      updateebassDecay();
      break;

    case CCebassRes:
      ebassRes = value;
      ebassResstr = QUADRA100[value];
      updateebassRes();
      break;

    case CCstringBassVolume:
      stringBassVolume = value;
      stringBassVolumestr = QUADRA100[value];
      updatestringBassVolume();
      break;

    case CCreverbLevel:
      reverbLevel = value;
      reverbLevelstr = QUADRA100[value];
      updatereverbLevel();
      break;

    case CCarpSync:
      arpSync = value;
      arpSyncstr = value;
      updatearpSync();
      break;

    case CCarpSpeed:
      arpSpeed = value;
      if (arpSync < 63) {
        arpSpeedstr = QUADRAARPSPEED[value];
      } else {
        arpSpeedmap = map(arpSpeed, 0, 127, 0, 19);
        arpSpeedstring = QUADRAARPSYNC[arpSpeedmap];
      }
      updatearpSpeed();
      break;

    case CCarpRange:
      arpRange = value;
      arpRange = map(arpRange, 0, 127, 0, 2);
      updatearpRange();
      break;

    case CClowEQ:
      lowEQ = value;
      lowEQstr = QUADRAEQ[value];
      updatelowEQ();
      break;

    case CChighEQ:
      highEQ = value;
      highEQstr = QUADRAEQ[value];
      updatehighEQ();
      break;

    case CCstringAttack:
      stringAttack = value;
      stringAttackstr = QUADRAESTRINGSATTACK[value];
      updatestringAttack();
      break;

    case CCstringRelease:
      stringRelease = value;
      stringReleasestr = QUADRAESTRINGSRELEASE[value];
      updatestringRelease();
      break;


    case CCmodSourcePhaser:
      modSourcePhaser = value;
      modSourcePhaser = map(modSourcePhaser, 0, 127, 0, 2);
      updatemodSourcePhaser();
      break;


    case CCportVCO1:
      portVCO1 = value;
      portVCO1str = QUADRAPORT[value];
      updateportVCO1();
      break;

    case CCleadLFOPitch:
      leadLFOPitch = value;
      leadLFOPitchstr = QUADRA100LOG[value];
      updateleadLFOPitch();
      break;

    case CCvco1Range:
      vco1Range = value;
      vco1Rangestr = QUADRASEMITONES[value];
      updatevco1Range();
      break;

    case CCvco2Range:
      vco2Range = value;
      vco2Rangestr = QUADRASEMITONES[value];
      updatevco2Range();
      break;

    case CCvco2Tune:
      vco2Tune = value;
      vco2Tunestr = QUADRAEVCO2TUNE[value];
      updatevco2Tune();
      break;

    case CCvco2Volume:
      vco2Volume = value;
      vco2Volumestr = QUADRA100[value];
      updatevco2Volume();
      break;

    case CCEnvtoVCF:
      EnvtoVCF = value;
      EnvtoVCFstr = QUADRA100LOG[value];
      updateEnvtoVCF();
      break;

    case CCleadVCFCutoff:
      leadVCFCutoff = value;
      leadVCFCutoffstr = QUADRACUTOFF[value];
      updateleadVCFCutoff();
      break;

    case CCleadVCFRes:
      leadVCFRes = value;
      leadVCFResstr = QUADRA100[value];
      updateleadVCFRes();
      break;

    case CCleadAttack:
      leadAttack = value;
      leadAttackstr = QUADRALEADATTACK[value];
      updateleadAttack();
      break;

    case CCleadDecay:
      leadDecay = value;
      leadDecaystr = QUADRALEADDECAY[value];
      updateleadDecay();
      break;

    case CCleadSustain:
      leadSustain = value;
      leadSustainstr = QUADRA100[value];
      updateleadSustain();
      break;

    case CCleadRelease:
      leadRelease = value;
      leadReleasestr = QUADRALEADRELEASE[value];
      updateleadRelease();
      break;

    case CCmasterTune:
      masterTune = value;
      masterTunestr = QUADRAETUNE[value];
      updatemasterTune();
      break;

    case CCmasterVolume:
      masterVolume = value;
      masterVolumestr = QUADRAVOLUME[value];
      updatemasterVolume();
      break;

    case CCleadPWMmod:
      value > 0 ? leadPWMmod = 1 : leadPWMmod = 0;
      updateleadPWMmod();
      break;

    case CCleadVCFmod:
      value > 0 ? leadVCFmod = 1 : leadVCFmod = 0;
      updateleadVCFmod();
      break;

    case CCpolyLearn:
      value > 0 ? polyLearn = 1 : polyLearn = 0;
      updatepolyLearn();
      break;

    case CCtrillUp:
      value > 0 ? trillUp = 1 : trillUp = 0;
      updatetrillUp();
      break;

    case CCtrillDown:
      value > 0 ? trillDown = 1 : trillDown = 0;
      updatetrillDown();
      break;

    case CCleadLearn:
      value > 0 ? leadLearn = 1 : leadLearn = 0;
      updateleadLearn();
      break;

    case CCleadNPhigh:
      value > 0 ? leadNPhigh = 1 : leadNPhigh = 0;
      updateleadNPhigh();
      break;

    case CCleadNPlow:
      value > 0 ? leadNPlow = 1 : leadNPlow = 0;
      updateleadNPlow();
      break;

    case CCleadNPlast:
      value > 0 ? leadNPlast = 1 : leadNPlast = 0;
      updateleadNPlast();
      break;

    case CCleadNoteTrigger:
      value > 0 ? leadNoteTrigger = 1 : leadNoteTrigger = 0;
      updateleadNoteTrigger();
      break;

    case CCvco2KBDTrk:
      value > 0 ? vco2KBDTrk = 1 : vco2KBDTrk = 0;
      updateVCO2KBDTrk();
      break;

    case CClead2ndVoice:
      value > 0 ? lead2ndVoice = 1 : lead2ndVoice = 0;
      updatelead2ndVoice();
      break;

    case CCleadTrill:
      value > 0 ? leadTrill = 1 : leadTrill = 0;
      updateleadTrill();
      break;

    case CCleadVCO1wave:
      leadVCO1wave = value;
      updateleadVCO1wave();
      break;

    case CCleadVCO2wave:
      leadVCO2wave = value;
      updateleadVCO2wave();
      break;

    case CCpolyWave:
      polyWave = value;
      updatepolyWave();
      break;

    case CCpolyPWMSW:
      value > 0 ? polyPWMSW = 1 : polyPWMSW = 0;
      updatepolyPWMSW();
      break;

    case CCLFOTri:
      value > 0 ? LFOTri = 1 : LFOTri = 0;
      updateLFOTri();
      break;

    case CCLFOSawUp:
      value > 0 ? LFOSawUp = 1 : LFOSawUp = 0;
      updateLFOSawUp();
      break;

    case CCLFOSawDown:
      value > 0 ? LFOSawDown = 1 : LFOSawDown = 0;
      updateLFOSawDown();
      break;

    case CCLFOSquare:
      value > 0 ? LFOSquare = 1 : LFOSquare = 0;
      updateLFOSquare();
      break;

    case CCLFOSH:
      value > 0 ? LFOSH = 1 : LFOSH = 0;
      updateLFOSH();
      break;

    case CCstrings8:
      value > 0 ? strings8 = 1 : strings8 = 0;
      updatestrings8();
      break;

    case CCstrings4:
      value > 0 ? strings4 = 1 : strings4 = 0;
      updatestrings4();
      break;

    case CCpolyNoteTrigger:
      value > 0 ? polyNoteTrigger = 1 : polyNoteTrigger = 0;
      updatepolyNoteTrigger();
      break;

    case CCpolyVelAmp:
      value > 0 ? polyVelAmp = 1 : polyVelAmp = 0;
      updatepolyVelAmp();
      break;

    case CCpolyDrift:
      value > 0 ? polyDrift = 1 : polyDrift = 0;
      updatepolyDrift();
      break;

    case CCpoly16:
      value > 0 ? poly16 = 1 : poly16 = 0;
      updatepoly16();
      break;

    case CCpoly8:
      value > 0 ? poly8 = 1 : poly8 = 0;
      updatepoly8();
      break;

    case CCpoly4:
      value > 0 ? poly4 = 1 : poly4 = 0;
      updatepoly4();
      break;

    case CCebass16:
      value > 0 ? ebass16 = 1 : ebass16 = 0;
      updateebass16();
      break;

    case CCebass8:
      value > 0 ? ebass8 = 1 : ebass8 = 0;
      updateebass8();
      break;

    case CCbassNoteTrigger:
      value > 0 ? bassNoteTrigger = 1 : bassNoteTrigger = 0;
      updatebassNoteTrigger();
      break;

    case CCstringbass16:
      value > 0 ? stringbass16 = 1 : stringbass16 = 0;
      updatestringbass16();
      break;

    case CCstringbass8:
      value > 0 ? stringbass8 = 1 : stringbass8 = 0;
      updatestringbass8();
      break;

    case CChollowWave:
      value > 0 ? hollowWave = 1 : hollowWave = 0;
      updatehollowWave();
      break;

    case CCbassLearn:
      value > 0 ? bassLearn = 1 : bassLearn = 0;
      updatebassLearn();
      break;

    case CCstringsLearn:
      value > 0 ? stringsLearn = 1 : stringsLearn = 0;
      updatestringsLearn();
      break;

    case CCbassPitchSW:
      value > 0 ? bassPitchSW = 1 : bassPitchSW = 0;
      updatebassPitchSW();
      break;

    case CCstringsPitchSW:
      value > 0 ? stringsPitchSW = 1 : stringsPitchSW = 0;
      updatestringsPitchSW();
      break;

    case CCpolyPitchSW:
      value > 0 ? polyPitchSW = 1 : polyPitchSW = 0;
      updatepolyPitchSW();
      break;

    case CCpolyVCFSW:
      value > 0 ? polyVCFSW = 1 : polyVCFSW = 0;
      updatepolyVCFSW();
      break;

    case CCleadPitchSW:
      value > 0 ? leadPitchSW = 1 : leadPitchSW = 0;
      updateleadPitchSW();
      break;

    case CCleadVCFSW:
      value > 0 ? leadVCFSW = 1 : leadVCFSW = 0;
      updateleadVCFSW();
      break;

    case CCpolyAfterSW:
      value > 0 ? polyAfterSW = 1 : polyAfterSW = 0;
      updatepolyAfterSW();
      break;

    case CCleadAfterSW:
      value > 0 ? leadAfterSW = 1 : leadAfterSW = 0;
      updateleadAfterSW();
      break;

    case CCphaserBassSW:
      value > 0 ? phaserBassSW = 1 : phaserBassSW = 0;
      updatephaserBassSW();
      break;

    case CCphaserStringsSW:
      value > 0 ? phaserStringsSW = 1 : phaserStringsSW = 0;
      updatephaserStringsSW();
      break;

    case CCphaserPolySW:
      value > 0 ? phaserPolySW = 1 : phaserPolySW = 0;
      updatephaserPolySW();
      break;

    case CCphaserLeadSW:
      value > 0 ? phaserLeadSW = 1 : phaserLeadSW = 0;
      updatephaserLeadSW();
      break;

    case CCchorusBassSW:
      value > 0 ? chorusBassSW = 1 : chorusBassSW = 0;
      updatechorusBassSW();
      break;

    case CCchorusStringsSW:
      value > 0 ? chorusStringsSW = 1 : chorusStringsSW = 0;
      updatechorusStringsSW();
      break;

    case CCchorusPolySW:
      value > 0 ? chorusPolySW = 1 : chorusPolySW = 0;
      updatechorusPolySW();
      break;

    case CCchorusLeadSW:
      value > 0 ? chorusLeadSW = 1 : chorusLeadSW = 0;
      updatechorusLeadSW();
      break;

    case CCechoBassSW:
      value > 0 ? echoBassSW = 1 : echoBassSW = 0;
      updateechoBassSW();
      break;

    case CCechoStringsSW:
      value > 0 ? echoStringsSW = 1 : echoStringsSW = 0;
      updateechoStringsSW();
      break;

    case CCechoPolySW:
      value > 0 ? echoPolySW = 1 : echoPolySW = 0;
      updateechoPolySW();
      break;

    case CCechoLeadSW:
      value > 0 ? echoLeadSW = 1 : echoLeadSW = 0;
      updateechoLeadSW();
      break;

    case CCreverbBassSW:
      value > 0 ? reverbBassSW = 1 : reverbBassSW = 0;
      updatereverbBassSW();
      break;

    case CCreverbStringsSW:
      value > 0 ? reverbStringsSW = 1 : reverbStringsSW = 0;
      updatereverbStringsSW();
      break;

    case CCreverbPolySW:
      value > 0 ? reverbPolySW = 1 : reverbPolySW = 0;
      updatereverbPolySW();
      break;

    case CCreverbLeadSW:
      value > 0 ? reverbLeadSW = 1 : reverbLeadSW = 0;
      updatereverbLeadSW();
      break;

    case CCarpOnSW:
      value > 0 ? arpOnSW = 1 : arpOnSW = 0;
      updatearpOnSW();
      break;

    case CCarpDownSW:
      value > 0 ? arpDownSW = 1 : arpDownSW = 0;
      updatearpDownSW();
      break;

    case CCarpUpSW:
      value > 0 ? arpUpSW = 1 : arpUpSW = 0;
      updatearpUpSW();
      break;

    case CCarpUpDownSW:
      value > 0 ? arpUpDownSW = 1 : arpUpDownSW = 0;
      updatearpupDownSW();
      break;

    case CCarpRandomSW:
      value > 0 ? arpRandomSW = 1 : arpRandomSW = 0;
      updatearpRandomSW();
      break;

    case CCarpHoldSW:
      value > 0 ? arpHoldSW = 1 : arpHoldSW = 0;
      updatearpHoldSW();
      break;

    case CCallnotesoff:
      allNotesOff();
      break;
  }
}

void myProgramChange(byte channel, byte program) {
  state = PATCH;
  patchNo = program + 1;
  recallPatch(patchNo);
  Serial.print("MIDI Pgm Change:");
  Serial.println(patchNo);
  state = PARAMETER;
}

void recallPatch(int patchNo) {
  allNotesOff();
  usbMIDI.sendProgramChange(0, midiOutCh);
  MIDI.sendProgramChange(0, midiOutCh);
  delay(50);
  recallPatchFlag = true;
  File patchFile = SD.open(String(patchNo).c_str());
  if (!patchFile) {
    Serial.println("File not found");
  } else {
    String data[NO_OF_PARAMS];  //Array of data read in
    recallPatchData(patchFile, data);
    setCurrentPatchData(data);
    patchFile.close();
  }
  recallPatchFlag = false;
}

void setCurrentPatchData(String data[]) {
  patchName = data[0];
  leadMix = data[1].toInt();
  polyMix = data[2].toInt();
  stringMix = data[3].toInt();
  bassMix = data[4].toInt();
  lfoDelay = data[5].toFloat();
  polyPWM = data[6].toInt();
  polyPW = data[7].toInt();
  lfoSync = data[8].toFloat();
  shSource = data[9].toFloat();
  modWheel = data[10].toFloat();
  stringOctave = data[11].toInt();
  bassOctave = data[12].toInt();
  ebass16 = data[13].toFloat();
  lfoSpeed = data[14].toInt();
  leadPW = data[15].toInt();
  leadPWM = data[16].toInt();
  bassPitch = data[17].toInt();
  stringPitch = data[18].toInt();
  polyPitch = data[19].toInt();
  polyVCF = data[20].toInt();
  leadPitch = data[21].toInt();
  leadVCF = data[22].toInt();
  polyDepth = data[23].toInt();
  leadDepth = data[24].toInt();
  ebassDecay = data[25].toInt();
  ebassRes = data[26].toInt();
  stringBassVolume = data[27].toInt();
  lowEQ = data[28].toInt();
  highEQ = data[29].toInt();
  stringAttack = data[30].toInt();
  stringRelease = data[31].toInt();
  modSourcePhaser = data[32].toInt();
  phaserSpeed = data[33].toInt();
  phaserRes = data[34].toInt();
  chorusFlange = data[35].toFloat();
  chorusSpeed = data[36].toInt();
  chorusDepth = data[37].toInt();
  chorusRes = data[38].toInt();
  echoSync = data[39].toFloat();
  polyLFOPitch = data[40].toInt();
  polyLFOVCF = data[41].toInt();
  polyVCFRes = data[42].toInt();
  polyVCFCutoff = data[43].toInt();
  polyADSRDepth = data[44].toInt();
  polyAttack = data[45].toInt();
  polyDecay = data[46].toInt();
  polySustain = data[47].toInt();
  polyRelease = data[48].toInt();
  echoTime = data[49].toInt();
  echoRegen = data[50].toInt();
  echoDamp = data[51].toInt();
  echoLevel = data[52].toInt();
  reverbType = data[53].toInt();
  reverbDecay = data[54].toInt();
  reverbDamp = data[55].toInt();
  reverbLevel = data[56].toInt();
  arpSync = data[57].toFloat();
  arpSpeed = data[58].toInt();
  arpRange = data[59].toInt();
  ebass8 = data[60].toInt();
  leadVCFmod = data[61].toInt();
  leadNPlow = data[62].toInt();
  leadNPhigh = data[63].toInt();
  leadNPlast = data[64].toInt();
  portVCO1 = data[65].toInt();
  portVCO2 = data[66].toInt();
  leadLFOPitch = data[67].toInt();
  vco1Range = data[68].toInt();
  vco2Range = data[69].toInt();
  vco2Tune = data[70].toInt();
  vco2Volume = data[71].toInt();
  EnvtoVCF = data[72].toInt();
  leadVCFCutoff = data[73].toInt();
  leadVCFRes = data[74].toInt();
  leadAttack = data[75].toInt();
  leadDecay = data[76].toInt();
  leadSustain = data[77].toInt();
  leadRelease = data[78].toInt();
  masterTune = data[79].toInt();
  masterVolume = data[80].toInt();
  leadPWMmod = data[81].toInt();
  leadNoteTrigger = data[82].toInt();
  vco2KBDTrk = data[83].toInt();
  lead2ndVoice = data[84].toInt();
  leadTrill = data[85].toInt();
  leadVCO1wave = data[86].toInt();
  leadVCO2wave = data[87].toInt();
  polyWave = data[88].toInt();
  polyPWMSW = data[89].toInt();
  LFOTri = data[90].toInt();
  LFOSawUp = data[91].toInt();
  LFOSawDown = data[92].toInt();
  LFOSquare = data[93].toInt();
  LFOSH = data[94].toInt();
  strings8 = data[95].toInt();
  strings4 = data[96].toInt();
  polyNoteTrigger = data[97].toInt();
  polyVelAmp = data[98].toInt();
  polyDrift = data[99].toInt();
  poly16 = data[100].toInt();
  poly8 = data[101].toInt();
  poly4 = data[102].toInt();
  bassNoteTrigger = data[103].toInt();
  stringbass16 = data[104].toInt();
  stringbass8 = data[105].toInt();
  hollowWave = data[106].toInt();
  bassPitchSW = data[107].toInt();
  stringsPitchSW = data[108].toInt();
  polyPitchSW = data[109].toInt();
  polyVCFSW = data[110].toInt();
  leadPitchSW = data[111].toInt();
  leadVCFSW = data[112].toInt();
  polyAfterSW = data[113].toInt();
  leadAfterSW = data[114].toInt();
  phaserBassSW = data[115].toInt();
  phaserStringsSW = data[116].toInt();
  phaserPolySW = data[117].toInt();
  phaserLeadSW = data[118].toInt();
  chorusBassSW = data[119].toInt();
  chorusStringsSW = data[120].toInt();
  chorusPolySW = data[121].toInt();
  chorusLeadSW = data[122].toInt();
  echoBassSW = data[123].toInt();
  echoStringsSW = data[124].toInt();
  echoPolySW = data[125].toInt();
  echoLeadSW = data[126].toInt();
  reverbBassSW = data[127].toInt();
  reverbStringsSW = data[128].toInt();
  reverbPolySW = data[129].toInt();
  reverbLeadSW = data[130].toInt();
  arpOnSW = data[131].toInt();
  arpDownSW = data[132].toInt();
  arpUpSW = data[133].toInt();
  arpUpDownSW = data[134].toInt();
  arpRandomSW = data[135].toInt();
  arpHoldSW = data[136].toInt();
  leadBottomNote = data[137].toInt();
  leadTopNote = data[138].toInt();
  polyBottomNote = data[139].toInt();
  polyTopNote = data[140].toInt();
  stringsBottomNote = data[141].toInt();
  stringsTopNote = data[142].toInt();
  bassBottomNote = data[143].toInt();
  bassTopNote = data[144].toInt();
  trillValue = data[145].toInt();

  //Mux1

  updateleadMix();
  updatepolyMix();
  updatestringMix();
  updatebassMix();
  updatelfoDelay();
  updatepolyPWM();
  updatepolyPW();
  updatelfoSync();
  updateshSource();
  updatemodWheel();
  updatestringOctave();
  updatebassOctave();

  updatelfoSpeed();
  updateleadPW();
  updateleadPWM();

  //MUX 2
  updatebassPitch();
  updatestringPitch();
  updatepolyPitch();
  updatepolyVCF();
  updateleadPitch();
  updateleadVCF();
  updatepolyDepth();
  updateleadDepth();
  updateebassDecay();
  updateebassRes();
  updatestringBassVolume();
  updatelowEQ();
  updatehighEQ();
  updatestringAttack();
  updatestringRelease();
  updatemodSourcePhaser();

  //MUX3
  updatephaserSpeed();
  updatephaserRes();
  updatechorusFlange();
  updatechorusSpeed();
  updatechorusDepth();
  updatechorusRes();
  updateechoSync();
  updatepolyLFOPitch();
  updatepolyLFOVCF();
  updatepolyVCFRes();
  updatepolyVCFCutoff();
  updatepolyADSRDepth();
  updatepolyAttack();
  updatepolyDecay();
  updatepolySustain();
  updatepolyRelease();

  //MUX4
  updateechoTime();
  updateechoRegen();
  updateechoDamp();
  updateechoLevel();
  updatereverbType();
  updatereverbDecay();
  updatereverbDamp();
  updatereverbLevel();
  updatearpSync();
  updatearpSpeed();
  updatearpRange();

  //MUX5
  updateportVCO1();
  updateportVCO2();
  updateleadLFOPitch();
  updatevco1Range();
  updatevco2Range();
  updatevco2Tune();
  updatevco2Volume();
  updateEnvtoVCF();
  updateleadVCFCutoff();
  updateleadVCFRes();
  updateleadAttack();
  updateleadDecay();
  updateleadSustain();
  updateleadRelease();
  updatemasterTune();
  updatemasterVolume();

  //Switches

  updateleadPWMmod();
  updateleadVCFmod();
  updateleadNPlow();
  updateleadNPhigh();
  updateleadNPlast();
  updateleadNoteTrigger();
  updateVCO2KBDTrk();
  updatelead2ndVoice();
  updateleadTrill();
  //updateleadVCO1wave();
  //updateleadVCO2wave();
  //updatepolyWave();
  updateVCOWaves();
  updatepolyPWMSW();
  updateLFOTri();
  updateLFOSawUp();
  updateLFOSawDown();
  updateLFOSquare();
  updateLFOSH();
  updatestrings8();
  updatestrings4();
  updatepolyNoteTrigger();
  updatepolyVelAmp();
  updatepolyDrift();
  updatepoly16();
  updatepoly8();
  updatepoly4();
  updateebass16();
  updateebass8();
  updatebassNoteTrigger();
  updatestringbass16();
  updatestringbass8();
  updatehollowWave();
  updatebassPitchSW();
  updatestringsPitchSW();
  updatepolyPitchSW();
  updatepolyVCFSW();
  updateleadPitchSW();
  updateleadVCFSW();
  updatepolyAfterSW();
  updateleadAfterSW();
  updatephaserBassSW();
  updatephaserStringsSW();
  updatephaserPolySW();
  updatephaserLeadSW();
  updatechorusBassSW();
  updatechorusStringsSW();
  updatechorusPolySW();
  updatechorusLeadSW();
  updateechoBassSW();
  updateechoStringsSW();
  updateechoPolySW();
  updateechoLeadSW();
  updatereverbBassSW();
  updatereverbStringsSW();
  updatereverbPolySW();
  updatereverbLeadSW();
  updatearpOnSW();
  updatearpDownSW();
  updatearpUpSW();
  updatearpupDownSW();
  updatearpRandomSW();
  updatearpHoldSW();
  updateTrills();
  delay(10);
  updatedisplayNoteRanges();


  //Patchname
  updatePatchname();

  Serial.print("Set Patch: ");
  Serial.println(patchName);
}

String getCurrentPatchData() {
  return patchName + "," + String(leadMix) + "," + String(polyMix) + "," + String(stringMix) + "," + String(bassMix) + "," + String(lfoDelay) + "," + String(polyPWM) + "," + String(polyPW) + "," + String(lfoSync) + "," + String(shSource) + "," + String(modWheel)
         + "," + String(stringOctave) + "," + String(bassOctave) + "," + String(ebass16) + "," + String(lfoSpeed) + "," + String(leadPW) + "," + String(leadPWM) + "," + String(bassPitch) + "," + String(stringPitch) + "," + String(polyPitch) + "," + String(polyVCF)
         + "," + String(leadPitch) + "," + String(leadVCF) + "," + String(polyDepth) + "," + String(leadDepth) + "," + String(ebassDecay) + "," + String(ebassRes) + "," + String(stringBassVolume) + "," + String(lowEQ) + "," + String(highEQ) + "," + String(stringAttack)
         + "," + String(stringRelease) + "," + String(modSourcePhaser) + "," + String(phaserSpeed) + "," + String(phaserRes) + "," + String(chorusFlange) + "," + String(chorusSpeed) + "," + String(chorusDepth) + "," + String(chorusRes) + "," + String(echoSync) + "," + String(polyLFOPitch)
         + "," + String(polyLFOVCF) + "," + String(polyVCFRes) + "," + String(polyVCFCutoff) + "," + String(polyADSRDepth) + "," + String(polyAttack) + "," + String(polyDecay) + "," + String(polySustain) + "," + String(polyRelease) + "," + String(echoTime) + "," + String(echoRegen)
         + "," + String(echoDamp) + "," + String(echoLevel) + "," + String(reverbType) + "," + String(reverbDecay) + "," + String(reverbDamp) + "," + String(reverbLevel) + "," + String(arpSync) + "," + String(arpSpeed) + "," + String(arpRange) + "," + String(ebass8)
         + "," + String(leadVCFmod) + "," + String(leadNPlow) + "," + String(leadNPhigh) + "," + String(leadNPlast) + "," + String(portVCO1) + "," + String(portVCO2) + "," + String(leadLFOPitch) + "," + String(vco1Range) + "," + String(vco2Range) + "," + String(vco2Tune)
         + "," + String(vco2Volume) + "," + String(EnvtoVCF) + "," + String(leadVCFCutoff) + "," + String(leadVCFRes) + "," + String(leadAttack) + "," + String(leadDecay) + "," + String(leadSustain) + "," + String(leadRelease) + "," + String(masterTune) + "," + String(masterVolume)
         + "," + String(leadPWMmod) + "," + String(leadNoteTrigger) + "," + String(vco2KBDTrk) + "," + String(lead2ndVoice) + "," + String(leadTrill) + "," + String(leadVCO1wave) + "," + String(leadVCO2wave) + "," + String(polyWave) + "," + String(polyPWMSW) + "," + String(LFOTri)
         + "," + String(LFOSawUp) + "," + String(LFOSawDown) + "," + String(LFOSquare) + "," + String(LFOSH) + "," + String(strings8) + "," + String(strings4) + "," + String(polyNoteTrigger) + "," + String(polyVelAmp) + "," + String(polyDrift) + "," + String(poly16)
         + "," + String(poly8) + "," + String(poly4) + "," + String(bassNoteTrigger) + "," + String(stringbass16) + "," + String(stringbass8) + "," + String(hollowWave) + "," + String(bassPitchSW) + "," + String(stringsPitchSW) + "," + String(polyPitchSW) + "," + String(polyVCFSW)
         + "," + String(leadPitchSW) + "," + String(leadVCFSW) + "," + String(polyAfterSW) + "," + String(leadAfterSW) + "," + String(phaserBassSW) + "," + String(phaserStringsSW) + "," + String(phaserPolySW) + "," + String(phaserLeadSW) + "," + String(chorusBassSW) + "," + String(chorusStringsSW)
         + "," + String(chorusPolySW) + "," + String(chorusLeadSW) + "," + String(echoBassSW) + "," + String(echoStringsSW) + "," + String(echoPolySW) + "," + String(echoLeadSW) + "," + String(reverbBassSW) + "," + String(reverbStringsSW) + "," + String(reverbPolySW) + "," + String(reverbLeadSW)
         + "," + String(arpOnSW) + "," + String(arpDownSW) + "," + String(arpUpSW) + "," + String(arpUpDownSW) + "," + String(arpRandomSW) + "," + String(arpHoldSW)
         + "," + String(leadBottomNote) + "," + String(leadTopNote) + "," + String(polyBottomNote) + "," + String(polyTopNote) + "," + String(stringsBottomNote) + "," + String(stringsTopNote) + "," + String(bassBottomNote) + "," + String(bassTopNote) + "," + String(trillValue);
}

void checkMux() {

  mux1Read = adc->adc1->analogRead(MUX1_S);

  if (mux1Read > (mux1ValuesPrev[muxInput] + QUANTISE_FACTOR) || mux1Read < (mux1ValuesPrev[muxInput] - QUANTISE_FACTOR)) {
    mux1ValuesPrev[muxInput] = mux1Read;
    mux1Read = (mux1Read >> resolutionFrig);  // Change range to 0-127

    switch (muxInput) {
      case MUX1_leadMix:
        myControlChange(midiChannel, CCleadMix, mux1Read);
        break;
      case MUX1_polyMix:
        myControlChange(midiChannel, CCpolyMix, mux1Read);
        break;
      case MUX1_stringMix:
        myControlChange(midiChannel, CCstringMix, mux1Read);
        break;
      case MUX1_bassMix:
        myControlChange(midiChannel, CCbassMix, mux1Read);
        break;
      case MUX1_lfoDelay:
        myControlChange(midiChannel, CClfoDelay, mux1Read);
        break;
      case MUX1_polyPWM:
        myControlChange(midiChannel, CCpolyPWM, mux1Read);
        break;
      case MUX1_polyPW:
        myControlChange(midiChannel, CCpolyPW, mux1Read);
        break;
      case MUX1_lfoSync:
        myControlChange(midiChannel, CClfoSync, mux1Read);
        break;
      case MUX1_shSource:
        myControlChange(midiChannel, CCshSource, mux1Read);
        break;
      case MUX1_modWheel:
        myControlChange(midiChannel, CCmodWheel, mux1Read);
        break;
      case MUX1_stringOctave:
        myControlChange(midiChannel, CCstringOctave, mux1Read);
        break;
      case MUX1_bassOctave:
        myControlChange(midiChannel, CCbassOctave, mux1Read);
        break;



      case MUX1_lfoSpeed:
        myControlChange(midiChannel, CClfoSpeed, mux1Read);
        break;
      case MUX1_leadPW:
        myControlChange(midiChannel, CCleadPW, mux1Read);
        break;
      case MUX1_leadPWM:
        myControlChange(midiChannel, CCleadPWM, mux1Read);
        break;
    }
  }

  mux2Read = adc->adc1->analogRead(MUX2_S);

  if (mux2Read > (mux2ValuesPrev[muxInput] + QUANTISE_FACTOR) || mux2Read < (mux2ValuesPrev[muxInput] - QUANTISE_FACTOR)) {
    mux2ValuesPrev[muxInput] = mux2Read;
    mux2Read = (mux2Read >> resolutionFrig);  // Change range to 0-127

    switch (muxInput) {
      case MUX2_bassPitch:
        myControlChange(midiChannel, CCbassPitch, mux2Read);
        break;
      case MUX2_stringPitch:
        myControlChange(midiChannel, CCstringPitch, mux2Read);
        break;
      case MUX2_polyPitch:
        myControlChange(midiChannel, CCpolyPitch, mux2Read);
        break;
      case MUX2_polyVCF:
        myControlChange(midiChannel, CCpolyVCF, mux2Read);
        break;
      case MUX2_leadPitch:
        myControlChange(midiChannel, CCleadPitch, mux2Read);
        break;
      case MUX2_leadVCF:
        myControlChange(midiChannel, CCleadVCF, mux2Read);
        break;
      case MUX2_polyDepth:
        myControlChange(midiChannel, CCpolyDepth, mux2Read);
        break;
      case MUX2_leadDepth:
        myControlChange(midiChannel, CCleadDepth, mux2Read);
        break;
      case MUX2_ebassDecay:
        myControlChange(midiChannel, CCebassDecay, mux2Read);
        break;
      case MUX2_ebassRes:
        myControlChange(midiChannel, CCebassRes, mux2Read);
        break;
      case MUX2_stringBassVolume:
        myControlChange(midiChannel, CCstringBassVolume, mux2Read);
        break;
      case MUX2_lowEQ:
        myControlChange(midiChannel, CClowEQ, mux2Read);
        break;
      case MUX2_highEQ:
        myControlChange(midiChannel, CChighEQ, mux2Read);
        break;
      case MUX2_stringAttack:
        myControlChange(midiChannel, CCstringAttack, mux2Read);
        break;
      case MUX2_stringRelease:
        myControlChange(midiChannel, CCstringRelease, mux2Read);
        break;
      case MUX2_modSourcePhaser:
        myControlChange(midiChannel, CCmodSourcePhaser, mux2Read);
        break;
    }
  }


  mux3Read = adc->adc1->analogRead(MUX3_S);

  if (mux3Read > (mux3ValuesPrev[muxInput] + QUANTISE_FACTOR) || mux3Read < (mux3ValuesPrev[muxInput] - QUANTISE_FACTOR)) {
    mux3ValuesPrev[muxInput] = mux3Read;
    mux3Read = (mux3Read >> resolutionFrig);  // Change range to 0-127

    switch (muxInput) {
      case MUX3_phaserSpeed:
        myControlChange(midiChannel, CCphaserSpeed, mux3Read);
        break;
      case MUX3_phaserRes:
        myControlChange(midiChannel, CCphaserRes, mux3Read);
        break;
      case MUX3_chorusFlange:
        myControlChange(midiChannel, CCchorusFlange, mux3Read);
        break;
      case MUX3_chorusSpeed:
        myControlChange(midiChannel, CCchorusSpeed, mux3Read);
        break;
      case MUX3_chorusDepth:
        myControlChange(midiChannel, CCchorusDepth, mux3Read);
        break;
      case MUX3_chorusRes:
        myControlChange(midiChannel, CCchorusRes, mux3Read);
        break;
      case MUX3_echoSync:
        myControlChange(midiChannel, CCechoSync, mux3Read);
        break;
      case MUX3_polyLFOPitch:
        myControlChange(midiChannel, CCpolyLFOPitch, mux3Read);
        break;
      case MUX3_polyLFOVCF:
        myControlChange(midiChannel, CCpolyLFOVCF, mux3Read);
        break;
      case MUX3_polyADSRDepth:
        myControlChange(midiChannel, CCpolyADSRDepth, mux3Read);
        break;
      case MUX3_polyVCFCutoff:
        myControlChange(midiChannel, CCpolyVCFCutoff, mux3Read);
        break;
      case MUX3_polyVCFRes:
        myControlChange(midiChannel, CCpolyVCFRes, mux3Read);
        break;
      case MUX3_polyAttack:
        myControlChange(midiChannel, CCpolyAttack, mux3Read);
        break;
      case MUX3_polyDecay:
        myControlChange(midiChannel, CCpolyDecay, mux3Read);
        break;
      case MUX3_polySustain:
        myControlChange(midiChannel, CCpolySustain, mux3Read);
        break;
      case MUX3_polyRelease:
        myControlChange(midiChannel, CCpolyRelease, mux3Read);
        break;
    }
  }

  mux4Read = adc->adc0->analogRead(MUX4_S);

  if (mux4Read > (mux4ValuesPrev[muxInput] + QUANTISE_FACTOR) || mux4Read < (mux4ValuesPrev[muxInput] - QUANTISE_FACTOR)) {
    mux4ValuesPrev[muxInput] = mux4Read;
    mux4Read = (mux4Read >> resolutionFrig);  // Change range to 0-127

    switch (muxInput) {
      case MUX4_echoTime:
        myControlChange(midiChannel, CCechoTime, mux4Read);
        break;
      case MUX4_echoRegen:
        myControlChange(midiChannel, CCechoRegen, mux4Read);
        break;
      case MUX4_echoDamp:
        myControlChange(midiChannel, CCechoDamp, mux4Read);
        break;
      case MUX4_echoLevel:
        myControlChange(midiChannel, CCechoLevel, mux4Read);
        break;
      case MUX4_reverbType:
        myControlChange(midiChannel, CCreverbType, mux4Read);
        break;
      case MUX4_reverbDecay:
        myControlChange(midiChannel, CCreverbDecay, mux4Read);
        break;
      case MUX4_reverbDamp:
        myControlChange(midiChannel, CCreverbDamp, mux4Read);
        break;
      case MUX4_reverbLevel:
        myControlChange(midiChannel, CCreverbLevel, mux4Read);
        break;
      case MUX4_arpSync:
        myControlChange(midiChannel, CCarpSync, mux4Read);
        break;
      case MUX4_arpSpeed:
        myControlChange(midiChannel, CCarpSpeed, mux4Read);
        break;
      case MUX4_arpRange:
        myControlChange(midiChannel, CCarpRange, mux4Read);
        break;
    }
  }

  mux5Read = adc->adc0->analogRead(MUX5_S);

  if (mux5Read > (mux5ValuesPrev[muxInput] + QUANTISE_FACTOR) || mux5Read < (mux5ValuesPrev[muxInput] - QUANTISE_FACTOR)) {
    mux5ValuesPrev[muxInput] = mux5Read;
    mux5Read = (mux5Read >> resolutionFrig);  // Change range to 0-127

    switch (muxInput) {
      case MUX5_portVCO1:
        myControlChange(midiChannel, CCportVCO1, mux5Read);
        break;
      case MUX5_portVCO2:
        myControlChange(midiChannel, CCportVCO2, mux5Read);
        break;
      case MUX5_leadLFOPitch:
        myControlChange(midiChannel, CCleadLFOPitch, mux5Read);
        break;
      case MUX5_vco1Range:
        myControlChange(midiChannel, CCvco1Range, mux5Read);
        break;
      case MUX5_vco2Range:
        myControlChange(midiChannel, CCvco2Range, mux5Read);
        break;
      case MUX5_vco2Tune:
        myControlChange(midiChannel, CCvco2Tune, mux5Read);
        break;
      case MUX5_vco2Volume:
        myControlChange(midiChannel, CCvco2Volume, mux5Read);
        break;
      case MUX5_EnvtoVCF:
        myControlChange(midiChannel, CCEnvtoVCF, mux5Read);
        break;
      case MUX5_leadVCFCutoff:
        myControlChange(midiChannel, CCleadVCFCutoff, mux5Read);
        break;
      case MUX5_leadVCFRes:
        myControlChange(midiChannel, CCleadVCFRes, mux5Read);
        break;
      case MUX5_leadAttack:
        myControlChange(midiChannel, CCleadAttack, mux5Read);
        break;
      case MUX5_leadDecay:
        myControlChange(midiChannel, CCleadDecay, mux5Read);
        break;
      case MUX5_leadSustain:
        myControlChange(midiChannel, CCleadSustain, mux5Read);
        break;
      case MUX5_leadRelease:
        myControlChange(midiChannel, CCleadRelease, mux5Read);
        break;
      case MUX5_masterTune:
        myControlChange(midiChannel, CCmasterTune, mux5Read);
        break;
      case MUX5_masterVolume:
        myControlChange(midiChannel, CCmasterVolume, mux5Read);
        break;
    }
  }

  muxInput++;
  if (muxInput >= MUXCHANNELS)
    muxInput = 0;

  digitalWriteFast(MUX_0, muxInput & B0001);
  digitalWriteFast(MUX_1, muxInput & B0010);
  digitalWriteFast(MUX_2, muxInput & B0100);
  digitalWriteFast(MUX_3, muxInput & B1000);
  delayMicroseconds(75);
}

void onButtonPress(uint16_t btnIndex, uint8_t btnType) {

  // to check if a specific button was pressed

  if (btnIndex == LEAD_PWM_SW && btnType == ROX_PRESSED) {
    leadPWMmod = !leadPWMmod;
    myControlChange(midiChannel, CCleadPWMmod, leadPWMmod);
  }

  if (btnIndex == LEAD_VCF_MOD_SW && btnType == ROX_PRESSED) {
    leadVCFmod = !leadVCFmod;
    myControlChange(midiChannel, CCleadVCFmod, leadVCFmod);
  }

  if (btnIndex == POLY_LEARN_SW && btnType == ROX_PRESSED) {
    polyLearn = !polyLearn;
    myControlChange(midiChannel, CCpolyLearn, polyLearn);
  }

  if (btnIndex == TRILL_UP_SW && btnType == ROX_PRESSED) {
    trillUp = 1;
    myControlChange(midiChannel, CCtrillUp, trillUp);
  }

  if (btnIndex == TRILL_DOWN_SW && btnType == ROX_PRESSED) {
    trillDown = 1;
    myControlChange(midiChannel, CCtrillDown, trillDown);
  }

  if (btnIndex == LEAD_LEARN_SW && btnType == ROX_PRESSED) {
    leadLearn = !leadLearn;
    myControlChange(midiChannel, CCleadLearn, leadLearn);
  }

  if (btnIndex == LEAD_NP_LOW_SW && btnType == ROX_PRESSED) {
    leadNPlow = !leadNPlow;
    myControlChange(midiChannel, CCleadNPlow, leadNPlow);
  }

  if (btnIndex == LEAD_NP_HIGH_SW && btnType == ROX_PRESSED) {
    leadNPhigh = !leadNPhigh;
    myControlChange(midiChannel, CCleadNPhigh, leadNPhigh);
  }

  if (btnIndex == LEAD_NP_LAST_SW && btnType == ROX_PRESSED) {
    leadNPlast = !leadNPlast;
    myControlChange(midiChannel, CCleadNPlast, leadNPlast);
  }

  if (btnIndex == LEAD_NOTE_TRIGGER_SW && btnType == ROX_PRESSED) {
    leadNoteTrigger = !leadNoteTrigger;
    myControlChange(midiChannel, CCleadNoteTrigger, leadNoteTrigger);
  }

  if (btnIndex == LEAD_VCO2_KBD_TRK_SW && btnType == ROX_PRESSED) {
    vco2KBDTrk = !vco2KBDTrk;
    myControlChange(midiChannel, CCvco2KBDTrk, vco2KBDTrk);
  }

  if (btnIndex == LEAD_SECOND_VOICE_SW && btnType == ROX_PRESSED) {
    lead2ndVoice = !lead2ndVoice;
    myControlChange(midiChannel, CClead2ndVoice, lead2ndVoice);
  }

  if (btnIndex == LEAD_TRILL_SW && btnType == ROX_PRESSED) {
    leadTrill = !leadTrill;
    myControlChange(midiChannel, CCleadTrill, leadTrill);
  }

  if (btnIndex == LEAD_VCO1_WAVE_SW && btnType == ROX_PRESSED) {
    sr.writePin(LEAD_VCO1_WAVE_LED, LOW);
    vco1wave_timer = millis();
    leadVCO1wave = leadVCO1wave + 1;
    if (leadVCO1wave > 5) {
      leadVCO1wave = 1;
    }
    myControlChange(midiChannel, CCleadVCO1wave, leadVCO1wave);
  }

  if (btnIndex == LEAD_VCO2_WAVE_SW && btnType == ROX_PRESSED) {
    sr.writePin(LEAD_VCO2_WAVE_LED, LOW);
    vco2wave_timer = millis();
    leadVCO2wave = leadVCO2wave + 1;
    if (leadVCO2wave > 6) {
      leadVCO2wave = 1;
    }
    myControlChange(midiChannel, CCleadVCO2wave, leadVCO2wave);
  }

  if (btnIndex == POLY_WAVE_SW && btnType == ROX_PRESSED) {
    sr.writePin(POLY_WAVE_LED, LOW);
    polywave_timer = millis();
    polyWave = polyWave + 1;
    if (polyWave > 6) {
      polyWave = 1;
    }
    myControlChange(midiChannel, CCpolyWave, polyWave);
  }

  if (btnIndex == POLY_PWM_SW && btnType == ROX_PRESSED) {
    polyPWMSW = !polyPWMSW;
    myControlChange(midiChannel, CCpolyPWMSW, polyPWMSW);
  }

  if (btnIndex == LFO_TRI_SW && btnType == ROX_PRESSED) {
    LFOTri = !LFOTri;
    myControlChange(midiChannel, CCLFOTri, LFOTri);
  }

  if (btnIndex == LFO_SAW_UP_SW && btnType == ROX_PRESSED) {
    LFOSawUp = !LFOSawUp;
    myControlChange(midiChannel, CCLFOSawUp, LFOSawUp);
  }

  if (btnIndex == LFO_SAW_DN_SW && btnType == ROX_PRESSED) {
    LFOSawDown = !LFOSawDown;
    myControlChange(midiChannel, CCLFOSawDown, LFOSawDown);
  }

  if (btnIndex == LFO_SQR_SW && btnType == ROX_PRESSED) {
    LFOSquare = !LFOSquare;
    myControlChange(midiChannel, CCLFOSquare, LFOSquare);
  }

  if (btnIndex == LFO_SH_SW && btnType == ROX_PRESSED) {
    LFOSH = !LFOSH;
    myControlChange(midiChannel, CCLFOSH, LFOSH);
  }

  if (btnIndex == STRINGS_8_SW && btnType == ROX_PRESSED) {
    strings8 = !strings8;
    myControlChange(midiChannel, CCstrings8, strings8);
  }

  if (btnIndex == STRINGS_4_SW && btnType == ROX_PRESSED) {
    strings4 = !strings4;
    myControlChange(midiChannel, CCstrings4, strings4);
  }

  if (btnIndex == POLY_NOTE_TRIGGER_SW && btnType == ROX_PRESSED) {
    polyNoteTrigger = !polyNoteTrigger;
    myControlChange(midiChannel, CCpolyNoteTrigger, polyNoteTrigger);
  }

  if (btnIndex == POLY_VEL_AMP_SW && btnType == ROX_PRESSED) {
    polyVelAmp = !polyVelAmp;
    myControlChange(midiChannel, CCpolyVelAmp, polyVelAmp);
  }

  if (btnIndex == POLY_DRIFT_SW && btnType == ROX_PRESSED) {
    polyDrift = !polyDrift;
    myControlChange(midiChannel, CCpolyDrift, polyDrift);
  }

  if (btnIndex == POLY_16_SW && btnType == ROX_PRESSED) {
    poly16 = !poly16;
    myControlChange(midiChannel, CCpoly16, poly16);
  }

  if (btnIndex == POLY_8_SW && btnType == ROX_PRESSED) {
    poly8 = !poly8;
    myControlChange(midiChannel, CCpoly8, poly8);
  }

  if (btnIndex == POLY_4_SW && btnType == ROX_PRESSED) {
    poly4 = !poly4;
    myControlChange(midiChannel, CCpoly4, poly4);
  }

  if (btnIndex == EBASS_16_SW && btnType == ROX_PRESSED) {
    ebass16 = !ebass16;
    myControlChange(midiChannel, CCebass16, ebass16);
  }

  if (btnIndex == EBASS_8_SW && btnType == ROX_PRESSED) {
    ebass8 = !ebass8;
    myControlChange(midiChannel, CCebass8, ebass8);
  }

  if (btnIndex == BASS_NOTE_TRIGGER_SW && btnType == ROX_PRESSED) {
    bassNoteTrigger = !bassNoteTrigger;
    myControlChange(midiChannel, CCbassNoteTrigger, bassNoteTrigger);
  }

  if (btnIndex == STRINGS_BASS_16_SW && btnType == ROX_PRESSED) {
    stringbass16 = !stringbass16;
    myControlChange(midiChannel, CCstringbass16, stringbass16);
  }

  if (btnIndex == STRINGS_BASS_8_SW && btnType == ROX_PRESSED) {
    stringbass8 = !stringbass8;
    myControlChange(midiChannel, CCstringbass8, stringbass8);
  }

  if (btnIndex == HOLLOW_WAVE_SW && btnType == ROX_PRESSED) {
    hollowWave = !hollowWave;
    myControlChange(midiChannel, CChollowWave, hollowWave);
  }

  if (btnIndex == BASS_LEARN_SW && btnType == ROX_PRESSED) {
    bassLearn = !bassLearn;
    myControlChange(midiChannel, CCbassLearn, bassLearn);
  }

  if (btnIndex == STRINGS_LEARN_SW && btnType == ROX_PRESSED) {
    stringsLearn = !stringsLearn;
    myControlChange(midiChannel, CCstringsLearn, stringsLearn);
  }

  if (btnIndex == BASS_PITCH_SW && btnType == ROX_PRESSED) {
    bassPitchSW = !bassPitchSW;
    myControlChange(midiChannel, CCbassPitchSW, bassPitchSW);
  }

  if (btnIndex == STRINGS_PITCH_SW && btnType == ROX_PRESSED) {
    stringsPitchSW = !stringsPitchSW;
    myControlChange(midiChannel, CCstringsPitchSW, stringsPitchSW);
  }

  if (btnIndex == POLY_PITCH_SW && btnType == ROX_PRESSED) {
    polyPitchSW = !polyPitchSW;
    myControlChange(midiChannel, CCpolyPitchSW, polyPitchSW);
  }

  if (btnIndex == POLY_VCF_SW && btnType == ROX_PRESSED) {
    polyVCFSW = !polyVCFSW;
    myControlChange(midiChannel, CCpolyVCFSW, polyVCFSW);
  }

  if (btnIndex == LEAD_PITCH_SW && btnType == ROX_PRESSED) {
    leadPitchSW = !leadPitchSW;
    myControlChange(midiChannel, CCleadPitchSW, leadPitchSW);
  }

  if (btnIndex == LEAD_VCF_SW && btnType == ROX_PRESSED) {
    leadVCFSW = !leadVCFSW;
    myControlChange(midiChannel, CCleadVCFSW, leadVCFSW);
  }

  if (btnIndex == POLY_TOUCH_DEST_SW && btnType == ROX_PRESSED) {
    polyAfterSW = !polyAfterSW;
    myControlChange(midiChannel, CCpolyAfterSW, polyAfterSW);
  }

  if (btnIndex == LEAD_TOUCH_DEST_SW && btnType == ROX_PRESSED) {
    leadAfterSW = !leadAfterSW;
    myControlChange(midiChannel, CCleadAfterSW, leadAfterSW);
  }

  if (btnIndex == PHASE_BASS_SW && btnType == ROX_PRESSED) {
    phaserBassSW = !phaserBassSW;
    myControlChange(midiChannel, CCphaserBassSW, phaserBassSW);
  }

  if (btnIndex == PHASE_STRINGS_SW && btnType == ROX_PRESSED) {
    phaserStringsSW = !phaserStringsSW;
    myControlChange(midiChannel, CCphaserStringsSW, phaserStringsSW);
  }

  if (btnIndex == PHASE_POLY_SW && btnType == ROX_PRESSED) {
    phaserPolySW = !phaserPolySW;
    myControlChange(midiChannel, CCphaserPolySW, phaserPolySW);
  }

  if (btnIndex == PHASE_LEAD_SW && btnType == ROX_PRESSED) {
    phaserLeadSW = !phaserLeadSW;
    myControlChange(midiChannel, CCphaserLeadSW, phaserLeadSW);
  }

  if (btnIndex == CHORUS_BASS_SW && btnType == ROX_PRESSED) {
    chorusBassSW = !chorusBassSW;
    myControlChange(midiChannel, CCchorusBassSW, chorusBassSW);
  }

  if (btnIndex == CHORUS_STRINGS_SW && btnType == ROX_PRESSED) {
    chorusStringsSW = !chorusStringsSW;
    myControlChange(midiChannel, CCchorusStringsSW, chorusStringsSW);
  }

  if (btnIndex == CHORUS_POLY_SW && btnType == ROX_PRESSED) {
    chorusPolySW = !chorusPolySW;
    myControlChange(midiChannel, CCchorusPolySW, chorusPolySW);
  }

  if (btnIndex == CHORUS_LEAD_SW && btnType == ROX_PRESSED) {
    chorusLeadSW = !chorusLeadSW;
    myControlChange(midiChannel, CCchorusLeadSW, chorusLeadSW);
  }

  if (btnIndex == ECHO_BASS_SW && btnType == ROX_PRESSED) {
    echoBassSW = !echoBassSW;
    myControlChange(midiChannel, CCechoBassSW, echoBassSW);
  }

  if (btnIndex == ECHO_STRINGS_SW && btnType == ROX_PRESSED) {
    echoStringsSW = !echoStringsSW;
    myControlChange(midiChannel, CCechoStringsSW, echoStringsSW);
  }

  if (btnIndex == ECHO_POLY_SW && btnType == ROX_PRESSED) {
    echoPolySW = !echoPolySW;
    myControlChange(midiChannel, CCechoPolySW, echoPolySW);
  }

  if (btnIndex == ECHO_LEAD_SW && btnType == ROX_PRESSED) {
    echoLeadSW = !echoLeadSW;
    myControlChange(midiChannel, CCechoLeadSW, echoLeadSW);
  }

  if (btnIndex == REVERB_BASS_SW && btnType == ROX_PRESSED) {
    reverbBassSW = !reverbBassSW;
    myControlChange(midiChannel, CCreverbBassSW, reverbBassSW);
  }

  if (btnIndex == REVERB_STRINGS_SW && btnType == ROX_PRESSED) {
    reverbStringsSW = !reverbStringsSW;
    myControlChange(midiChannel, CCreverbStringsSW, reverbStringsSW);
  }

  if (btnIndex == REVERB_POLY_SW && btnType == ROX_PRESSED) {
    reverbPolySW = !reverbPolySW;
    myControlChange(midiChannel, CCreverbPolySW, reverbPolySW);
  }

  if (btnIndex == REVERB_LEAD_SW && btnType == ROX_PRESSED) {
    reverbLeadSW = !reverbLeadSW;
    myControlChange(midiChannel, CCreverbLeadSW, reverbLeadSW);
  }

  if (btnIndex == ARP_ON_OFF_SW && btnType == ROX_PRESSED) {
    arpOnSW = !arpOnSW;
    myControlChange(midiChannel, CCarpOnSW, arpOnSW);
  }

  if (btnIndex == ARP_DOWN_SW && btnType == ROX_PRESSED) {
    arpDownSW = !arpDownSW;
    myControlChange(midiChannel, CCarpDownSW, arpDownSW);
  }

  if (btnIndex == ARP_UP_SW && btnType == ROX_PRESSED) {
    arpUpSW = !arpUpSW;
    myControlChange(midiChannel, CCarpUpSW, arpUpSW);
  }

  if (btnIndex == ARP_UP_DOWN_SW && btnType == ROX_PRESSED) {
    arpUpDownSW = !arpUpDownSW;
    myControlChange(midiChannel, CCarpUpDownSW, arpUpDownSW);
  }

  if (btnIndex == ARP_RANDOM_SW && btnType == ROX_PRESSED) {
    arpRandomSW = !arpRandomSW;
    myControlChange(midiChannel, CCarpRandomSW, arpRandomSW);
  }

  if (btnIndex == ARP_HOLD_SW && btnType == ROX_PRESSED) {
    arpHoldSW = !arpHoldSW;
    myControlChange(midiChannel, CCarpHoldSW, arpHoldSW);
  }
}

void showSettingsPage() {
  showSettingsPage(settings::current_setting(), settings::current_setting_value(), state);
}

void midiCCOut(byte cc, byte value) {
  if (midiOutCh > 0) {
    switch (ccType) {
      case 0:
        {
          switch (cc) {

            case 128:                                   // Poly learn
              usbMIDI.sendNoteOn(120, 127, midiOutCh);  //MIDI USB is set to Out
              usbMIDI.sendNoteOff(120, 0, midiOutCh);   //MIDI USB is set to Out
              MIDI.sendNoteOn(120, 127, midiOutCh);     //MIDI DIN is set to Out
              MIDI.sendNoteOff(120, 0, midiOutCh);      //MIDI USB is set to Out
              break;

            case 129:                                   // trill up
              usbMIDI.sendNoteOn(116, 127, midiOutCh);  //MIDI USB is set to Out
              usbMIDI.sendNoteOff(116, 0, midiOutCh);   //MIDI USB is set to Out
              MIDI.sendNoteOn(116, 127, midiOutCh);     //MIDI DIN is set to Out
              MIDI.sendNoteOff(116, 0, midiOutCh);      //MIDI USB is set to Out
              break;

            case 130:                                   // trill down
              usbMIDI.sendNoteOn(117, 127, midiOutCh);  //MIDI USB is set to Out
              usbMIDI.sendNoteOff(117, 0, midiOutCh);   //MIDI USB is set to Out
              MIDI.sendNoteOn(117, 127, midiOutCh);     //MIDI DIN is set to Out
              MIDI.sendNoteOff(117, 0, midiOutCh);      //MIDI USB is set to Out
              break;

            case 131:                                   // LEad learn
              usbMIDI.sendNoteOn(121, 127, midiOutCh);  //MIDI USB is set to Out
              usbMIDI.sendNoteOff(121, 0, midiOutCh);   //MIDI USB is set to Out
              MIDI.sendNoteOn(121, 127, midiOutCh);     //MIDI DIN is set to Out
              MIDI.sendNoteOff(121, 0, midiOutCh);      //MIDI USB is set to Out
              break;

            case 132:                                   // bass learn
              usbMIDI.sendNoteOn(119, 127, midiOutCh);  //MIDI USB is set to Out
              usbMIDI.sendNoteOff(119, 0, midiOutCh);   //MIDI USB is set to Out
              MIDI.sendNoteOn(119, 127, midiOutCh);     //MIDI DIN is set to Out
              MIDI.sendNoteOff(119, 0, midiOutCh);      //MIDI USB is set to Out
              break;

            case 133:                                   // strings learn
              usbMIDI.sendNoteOn(118, 127, midiOutCh);  //MIDI USB is set to Out
              usbMIDI.sendNoteOff(118, 0, midiOutCh);   //MIDI USB is set to Out
              MIDI.sendNoteOn(118, 127, midiOutCh);     //MIDI DIN is set to Out
              MIDI.sendNoteOff(118, 0, midiOutCh);      //MIDI USB is set to Out
              break;

            case 134:
              usbMIDI.sendNoteOn(0, 127, midiOutCh);  //MIDI USB is set to Out
              usbMIDI.sendNoteOff(0, 0, midiOutCh);   //MIDI USB is set to Out
              MIDI.sendNoteOn(0, 127, midiOutCh);     //MIDI DIN is set to Out
              MIDI.sendNoteOff(0, 0, midiOutCh);      //MIDI USB is set to Out
              break;

            case 135:
              usbMIDI.sendNoteOn(1, 127, midiOutCh);  //MIDI USB is set to Out
              usbMIDI.sendNoteOff(1, 0, midiOutCh);   //MIDI USB is set to Out
              MIDI.sendNoteOn(1, 127, midiOutCh);     //MIDI DIN is set to Out
              MIDI.sendNoteOff(1, 0, midiOutCh);      //MIDI USB is set to Out
              break;

            case 136:
              usbMIDI.sendNoteOn(2, 127, midiOutCh);  //MIDI USB is set to Out
              usbMIDI.sendNoteOff(2, 0, midiOutCh);   //MIDI USB is set to Out
              MIDI.sendNoteOn(2, 127, midiOutCh);     //MIDI DIN is set to Out
              MIDI.sendNoteOff(2, 0, midiOutCh);      //MIDI USB is set to Out
              break;

            case 137:
              usbMIDI.sendNoteOn(3, 127, midiOutCh);  //MIDI USB is set to Out
              usbMIDI.sendNoteOff(3, 0, midiOutCh);   //MIDI USB is set to Out
              MIDI.sendNoteOn(3, 127, midiOutCh);     //MIDI DIN is set to Out
              MIDI.sendNoteOff(3, 0, midiOutCh);      //MIDI USB is set to Out
              break;

            case 138:
              usbMIDI.sendNoteOn(4, 127, midiOutCh);  //MIDI USB is set to Out
              usbMIDI.sendNoteOff(4, 0, midiOutCh);   //MIDI USB is set to Out
              MIDI.sendNoteOn(4, 127, midiOutCh);     //MIDI DIN is set to Out
              MIDI.sendNoteOff(4, 0, midiOutCh);      //MIDI USB is set to Out
              break;

            case 139:
              usbMIDI.sendNoteOn(5, 127, midiOutCh);  //MIDI USB is set to Out
              usbMIDI.sendNoteOff(5, 0, midiOutCh);   //MIDI USB is set to Out
              MIDI.sendNoteOn(5, 127, midiOutCh);     //MIDI DIN is set to Out
              MIDI.sendNoteOff(5, 0, midiOutCh);      //MIDI USB is set to Out
              break;

            case 140:
              usbMIDI.sendNoteOn(6, 127, midiOutCh);  //MIDI USB is set to Out
              usbMIDI.sendNoteOff(6, 0, midiOutCh);   //MIDI USB is set to Out
              MIDI.sendNoteOn(6, 127, midiOutCh);     //MIDI DIN is set to Out
              MIDI.sendNoteOff(6, 0, midiOutCh);      //MIDI USB is set to Out
              break;

            case 141:
              usbMIDI.sendNoteOn(7, 127, midiOutCh);  //MIDI USB is set to Out
              usbMIDI.sendNoteOff(7, 0, midiOutCh);   //MIDI USB is set to Out
              MIDI.sendNoteOn(7, 127, midiOutCh);     //MIDI DIN is set to Out
              MIDI.sendNoteOff(7, 0, midiOutCh);      //MIDI USB is set to Out
              break;

            case 142:
              usbMIDI.sendNoteOn(8, 127, midiOutCh);  //MIDI USB is set to Out
              usbMIDI.sendNoteOff(8, 0, midiOutCh);   //MIDI USB is set to Out
              MIDI.sendNoteOn(8, 127, midiOutCh);     //MIDI DIN is set to Out
              MIDI.sendNoteOff(8, 0, midiOutCh);      //MIDI USB is set to Out
              break;

            case 143:
              usbMIDI.sendNoteOn(9, 127, midiOutCh);  //MIDI USB is set to Out
              usbMIDI.sendNoteOff(9, 0, midiOutCh);   //MIDI USB is set to Out
              MIDI.sendNoteOn(9, 127, midiOutCh);     //MIDI DIN is set to Out
              MIDI.sendNoteOff(9, 0, midiOutCh);      //MIDI USB is set to Out
              break;

            case 144:
              usbMIDI.sendNoteOn(10, 127, midiOutCh);  //MIDI USB is set to Out
              usbMIDI.sendNoteOff(10, 0, midiOutCh);   //MIDI USB is set to Out
              MIDI.sendNoteOn(10, 127, midiOutCh);     //MIDI DIN is set to Out
              MIDI.sendNoteOff(10, 0, midiOutCh);      //MIDI USB is set to Out
              break;

            case 145:
              usbMIDI.sendNoteOn(11, 127, midiOutCh);  //MIDI USB is set to Out
              usbMIDI.sendNoteOff(11, 0, midiOutCh);   //MIDI USB is set to Out
              MIDI.sendNoteOn(11, 127, midiOutCh);     //MIDI DIN is set to Out
              MIDI.sendNoteOff(11, 0, midiOutCh);      //MIDI USB is set to Out
              break;

            case 146:
              usbMIDI.sendNoteOn(12, 127, midiOutCh);  //MIDI USB is set to Out
              usbMIDI.sendNoteOff(12, 0, midiOutCh);   //MIDI USB is set to Out
              MIDI.sendNoteOn(12, 127, midiOutCh);     //MIDI DIN is set to Out
              MIDI.sendNoteOff(12, 0, midiOutCh);      //MIDI USB is set to Out
              break;

            case 147:
              // Arp start
              usbMIDI.sendNoteOn(127, 127, midiOutCh);  //MIDI USB is set to Out
              usbMIDI.sendNoteOff(127, 0, midiOutCh);   //MIDI USB is set to Out
              MIDI.sendNoteOn(127, 127, midiOutCh);     //MIDI DIN is set to Out
              MIDI.sendNoteOff(127, 0, midiOutCh);      //MIDI USB is set to Out

              //usbMIDI.sendSysEx(6, PlayCommandArray, true); //Send an array of 6 bytes representing the play command
              // usbMIDI.sendSysEx(6, PlayCommandArray, false); //Send an array of 6 bytes representing the play command
              //MIDI.sendSysEx(6, PlayCommandArray, true); //Send an array of 6 bytes representing the play command
              // MIDI.sendSysEx(6, PlayCommandArray, true); //Send an array of 6 bytes representing the play command
              break;

            case 148:
              // Arp Down
              usbMIDI.sendNoteOn(126, 127, midiOutCh);  //MIDI USB is set to Out
              MIDI.sendNoteOn(126, 127, midiOutCh);     //MIDI DIN is set to Out
              break;

            case 149:
              // Arp Up
              usbMIDI.sendNoteOn(125, 127, midiOutCh);  //MIDI USB is set to Out
              MIDI.sendNoteOn(125, 127, midiOutCh);     //MIDI DIN is set to Out
              break;

            case 150:
              // Arp UpDown
              usbMIDI.sendNoteOn(124, 127, midiOutCh);  //MIDI USB is set to Out
              MIDI.sendNoteOn(124, 127, midiOutCh);     //MIDI DIN is set to Out
              break;

            case 151:
              // Arp Random
              usbMIDI.sendNoteOn(123, 127, midiOutCh);  //MIDI USB is set to Out
              MIDI.sendNoteOn(123, 127, midiOutCh);     //MIDI DIN is set to Out
              break;

            case 152:
              // Arp Hold
              usbMIDI.sendNoteOn(122, 127, midiOutCh);  //MIDI USB is set to Out
              usbMIDI.sendNoteOff(122, 0, midiOutCh);   //MIDI USB is set to Out
              MIDI.sendNoteOn(122, 127, midiOutCh);     //MIDI DIN is set to Out
              MIDI.sendNoteOff(122, 0, midiOutCh);      //MIDI USB is set to Out
              break;

            default:
              usbMIDI.sendControlChange(cc, value, midiOutCh);  //MIDI DIN is set to Out
              MIDI.sendControlChange(cc, value, midiOutCh);     //MIDI DIN is set to Out
              break;
          }
        }
      case 1:
        {
          // usbMIDI.sendControlChange(99, 0, midiOutCh);      //MIDI DIN is set to Out
          // usbMIDI.sendControlChange(98, cc, midiOutCh);     //MIDI DIN is set to Out
          // usbMIDI.sendControlChange(38, value, midiOutCh);  //MIDI DIN is set to Out
          // usbMIDI.sendControlChange(6, 0, midiOutCh);       //MIDI DIN is set to Out

          // midi1.sendControlChange(99, 0, midiOutCh);      //MIDI DIN is set to Out
          // midi1.sendControlChange(98, cc, midiOutCh);     //MIDI DIN is set to Out
          // midi1.sendControlChange(38, value, midiOutCh);  //MIDI DIN is set to Out
          // midi1.sendControlChange(6, 0, midiOutCh);       //MIDI DIN is set to Out

          // MIDI.sendControlChange(99, 0, midiOutCh);      //MIDI DIN is set to Out
          // MIDI.sendControlChange(98, cc, midiOutCh);     //MIDI DIN is set to Out
          // MIDI.sendControlChange(38, value, midiOutCh);  //MIDI DIN is set to Out
          // MIDI.sendControlChange(6, 0, midiOutCh);       //MIDI DIN is set to Out
          break;
        }
      case 2:
        {

          break;
        }
    }
  }
}

void checkSwitches() {

  saveButton.update();
  if (saveButton.held()) {
    switch (state) {
      case PARAMETER:
      case PATCH:
        state = DELETE;
        break;
    }
  } else if (saveButton.numClicks() == 1) {
    switch (state) {
      case PARAMETER:
        if (patches.size() < PATCHES_LIMIT) {
          resetPatchesOrdering();  //Reset order of patches from first patch
          patches.push({ patches.size() + 1, INITPATCHNAME });
          state = SAVE;
        }
        break;
      case SAVE:
        //Save as new patch with INITIALPATCH name or overwrite existing keeping name - bypassing patch renaming
        patchName = patches.last().patchName;
        state = PATCH;
        savePatch(String(patches.last().patchNo).c_str(), getCurrentPatchData());
        showPatchPage(patches.last().patchNo, patches.last().patchName);
        patchNo = patches.last().patchNo;
        loadPatches();  //Get rid of pushed patch if it wasn't saved
        setPatchesOrdering(patchNo);
        renamedPatch = "";
        state = PARAMETER;
        break;
      case PATCHNAMING:
        if (renamedPatch.length() > 0) patchName = renamedPatch;  //Prevent empty strings
        state = PATCH;
        savePatch(String(patches.last().patchNo).c_str(), getCurrentPatchData());
        showPatchPage(patches.last().patchNo, patchName);
        patchNo = patches.last().patchNo;
        loadPatches();  //Get rid of pushed patch if it wasn't saved
        setPatchesOrdering(patchNo);
        renamedPatch = "";
        state = PARAMETER;
        break;
    }
  }

  settingsButton.update();
  if (settingsButton.held()) {
    //If recall held, set current patch to match current hardware state
    //Reinitialise all hardware values to force them to be re-read if different
    state = REINITIALISE;
    reinitialiseToPanel();
  } else if (settingsButton.numClicks() == 1) {
    switch (state) {
      case PARAMETER:
        state = SETTINGS;
        showSettingsPage();
        break;
      case SETTINGS:
        showSettingsPage();
      case SETTINGSVALUE:
        settings::save_current_value();
        state = SETTINGS;
        showSettingsPage();
        break;
    }
  }

  backButton.update();
  if (backButton.held()) {
    //If Back button held, Panic - all notes off
  } else if (backButton.numClicks() == 1) {
    switch (state) {
      case RECALL:
        setPatchesOrdering(patchNo);
        state = PARAMETER;
        break;
      case SAVE:
        renamedPatch = "";
        state = PARAMETER;
        loadPatches();  //Remove patch that was to be saved
        setPatchesOrdering(patchNo);
        break;
      case PATCHNAMING:
        charIndex = 0;
        renamedPatch = "";
        state = SAVE;
        break;
      case DELETE:
        setPatchesOrdering(patchNo);
        state = PARAMETER;
        break;
      case SETTINGS:
        state = PARAMETER;
        break;
      case SETTINGSVALUE:
        state = SETTINGS;
        showSettingsPage();
        break;
    }
  }

  //Encoder switch
  recallButton.update();
  if (recallButton.held()) {
    //If Recall button held, return to current patch setting
    //which clears any changes made
    state = PATCH;
    //Recall the current patch
    patchNo = patches.first().patchNo;
    recallPatch(patchNo);
    state = PARAMETER;
  } else if (recallButton.numClicks() == 1) {
    switch (state) {
      case PARAMETER:
        state = RECALL;  //show patch list
        break;
      case RECALL:
        state = PATCH;
        //Recall the current patch
        patchNo = patches.first().patchNo;
        recallPatch(patchNo);
        state = PARAMETER;
        break;
      case SAVE:
        showRenamingPage(patches.last().patchName);
        patchName = patches.last().patchName;
        state = PATCHNAMING;
        break;
      case PATCHNAMING:
        if (renamedPatch.length() < 12)  //actually 12 chars
        {
          renamedPatch.concat(String(currentCharacter));
          charIndex = 0;
          currentCharacter = CHARACTERS[charIndex];
          showRenamingPage(renamedPatch);
        }
        break;
      case DELETE:
        //Don't delete final patch
        if (patches.size() > 1) {
          state = DELETEMSG;
          patchNo = patches.first().patchNo;     //PatchNo to delete from SD card
          patches.shift();                       //Remove patch from circular buffer
          deletePatch(String(patchNo).c_str());  //Delete from SD card
          loadPatches();                         //Repopulate circular buffer to start from lowest Patch No
          renumberPatchesOnSD();
          loadPatches();                      //Repopulate circular buffer again after delete
          patchNo = patches.first().patchNo;  //Go back to 1
          recallPatch(patchNo);               //Load first patch
        }
        state = PARAMETER;
        break;
      case SETTINGS:
        state = SETTINGSVALUE;
        showSettingsPage();
        break;
      case SETTINGSVALUE:
        settings::save_current_value();
        state = SETTINGS;
        showSettingsPage();
        break;
    }
  }
}

void reinitialiseToPanel() {
  //This sets the current patch to be the same as the current hardware panel state - all the pots
  //The four button controls stay the same state
  //This reinialises the previous hardware values to force a re-read
  muxInput = 0;
  for (int i = 0; i < MUXCHANNELS; i++) {
    mux1ValuesPrev[i] = RE_READ;
    mux2ValuesPrev[i] = RE_READ;
    mux3ValuesPrev[i] = RE_READ;
    mux4ValuesPrev[i] = RE_READ;
    mux5ValuesPrev[i] = RE_READ;
  }
  patchName = INITPATCHNAME;
  showPatchPage("Initial", "Panel Settings");
}

void checkEncoder() {
  //Encoder works with relative inc and dec values
  //Detent encoder goes up in 4 steps, hence +/-3

  long encRead = encoder.read();
  if ((encCW && encRead > encPrevious + 3) || (!encCW && encRead < encPrevious - 3)) {
    switch (state) {
      case PARAMETER:
        state = PATCH;
        patches.push(patches.shift());
        patchNo = patches.first().patchNo;
        recallPatch(patchNo);
        state = PARAMETER;
        break;
      case RECALL:
        patches.push(patches.shift());
        break;
      case SAVE:
        patches.push(patches.shift());
        break;
      case PATCHNAMING:
        if (charIndex == TOTALCHARS) charIndex = 0;  //Wrap around
        currentCharacter = CHARACTERS[charIndex++];
        showRenamingPage(renamedPatch + currentCharacter);
        break;
      case DELETE:
        patches.push(patches.shift());
        break;
      case SETTINGS:
        settings::increment_setting();
        showSettingsPage();
        break;
      case SETTINGSVALUE:
        settings::increment_setting_value();
        showSettingsPage();
        break;
    }
    encPrevious = encRead;
  } else if ((encCW && encRead < encPrevious - 3) || (!encCW && encRead > encPrevious + 3)) {
    switch (state) {
      case PARAMETER:
        state = PATCH;
        patches.unshift(patches.pop());
        patchNo = patches.first().patchNo;
        recallPatch(patchNo);
        state = PARAMETER;
        break;
      case RECALL:
        patches.unshift(patches.pop());
        break;
      case SAVE:
        patches.unshift(patches.pop());
        break;
      case PATCHNAMING:
        if (charIndex == -1)
          charIndex = TOTALCHARS - 1;
        currentCharacter = CHARACTERS[charIndex--];
        showRenamingPage(renamedPatch + currentCharacter);
        break;
      case DELETE:
        patches.unshift(patches.pop());
        break;
      case SETTINGS:
        settings::decrement_setting();
        showSettingsPage();
        break;
      case SETTINGSVALUE:
        settings::decrement_setting_value();
        showSettingsPage();
        break;
    }
    encPrevious = encRead;
  }
}

void checkEEPROM() {

  if (oldLEDintensity != LEDintensity) {
    LEDintensity = LEDintensity * 10;
    trilldisplay.setBacklight(LEDintensity);
    setLEDDisplay0();
    display0.setBacklight(LEDintensity);
    setLEDDisplay1();
    display1.setBacklight(LEDintensity);
    setLEDDisplay2();
    display2.setBacklight(LEDintensity);
    setLEDDisplay3();
    display3.setBacklight(LEDintensity);
    setLEDDisplay4();
    display4.setBacklight(LEDintensity);
    setLEDDisplay5();
    display5.setBacklight(LEDintensity);
    setLEDDisplay6();
    display6.setBacklight(LEDintensity);
    setLEDDisplay7();
    display7.setBacklight(LEDintensity);
    oldLEDintensity = LEDintensity;
  }
}

void stopLEDs() {
  if ((polywave_timer > 0) && (millis() - polywave_timer > 150)) {
    sr.writePin(POLY_WAVE_LED, HIGH);
    polywave_timer = 0;
  }

  if ((vco1wave_timer > 0) && (millis() - vco1wave_timer > 150)) {
    sr.writePin(LEAD_VCO1_WAVE_LED, HIGH);
    vco1wave_timer = 0;
  }

  if ((vco2wave_timer > 0) && (millis() - vco2wave_timer > 150)) {
    sr.writePin(LEAD_VCO2_WAVE_LED, HIGH);
    vco2wave_timer = 0;
  }
}

void flashLearnLED(int displayNumber) {

  if (learning) {
    if ((learn_timer > 0) && (millis() - learn_timer >= 1000)) {
      switch (displayNumber) {
        case 0:
          setLEDDisplay0();
          display0.setBacklight(LEDintensity);
          break;

        case 1:
          setLEDDisplay1();
          display1.setBacklight(LEDintensity);
          break;

        case 2:
          setLEDDisplay2();
          display2.setBacklight(LEDintensity);
          break;

        case 3:
          setLEDDisplay3();
          display3.setBacklight(LEDintensity);
          break;

        case 4:
          setLEDDisplay4();
          display4.setBacklight(LEDintensity);
          break;

        case 5:
          setLEDDisplay5();
          display5.setBacklight(LEDintensity);
          break;

        case 6:
          setLEDDisplay6();
          display6.setBacklight(LEDintensity);
          break;

        case 7:
          setLEDDisplay7();
          display7.setBacklight(LEDintensity);
          break;
      }

      learn_timer = millis();
    } else if ((learn_timer > 0) && (millis() - learn_timer >= 500)) {
      switch (displayNumber) {
        case 0:
          setLEDDisplay0();
          display0.setBacklight(0);
          break;

        case 1:
          setLEDDisplay1();
          display1.setBacklight(0);
          break;

        case 2:
          setLEDDisplay2();
          display2.setBacklight(0);
          break;

        case 3:
          setLEDDisplay3();
          display3.setBacklight(0);
          break;

        case 4:
          setLEDDisplay4();
          display4.setBacklight(0);
          break;

        case 5:
          setLEDDisplay5();
          display5.setBacklight(0);
          break;

        case 6:
          setLEDDisplay6();
          display6.setBacklight(0);
          break;

        case 7:
          setLEDDisplay7();
          display7.setBacklight(0);
          break;
      }
    }
  }
}

void loop() {
  single();   // Keeps the lights on sliders on
  checkMux();  // Read the sliders and switches
  checkSwitches(); // Read the buttons for the program menus etc
  checkEncoder(); // check the encoder status
  octoswitch.update(); // read all the buttons for the Quadra
  sr.update(); // update all the LEDs in the buttons

  // Read all the MIDI ports
  myusb.Task();
  midi1.read();  //USB HOST MIDI Class Compliant
  MIDI.read(midiChannel);
  usbMIDI.read(midiChannel);

  checkEEPROM(); // check anything that may have changed form the initial startup
  stopLEDs(); // blink the wave LEDs once when pressed
  flashLearnLED(learningDisplayNumber); // Flash the corresponding learn LED display when button pressed
  convertIncomingNote(); // read a note when in learn mode and use it to set the values
}
