#include "SettingsService.h"

void settingsMIDICh();
void settingsMIDIOutCh();
void settingsEncoderDir();
void settingsCCType();


int currentIndexMIDICh();
int currentIndexMIDIOutCh();
int currentIndexEncoderDir();
int currentIndexCCType();

void settingsMIDICh(int index, const char *value) {
  if (strcmp(value, "ALL") == 0) {
    midiChannel = MIDI_CHANNEL_OMNI;
  } else {
    midiChannel = atoi(value);
  }
  storeMidiChannel(midiChannel);
}

void settingsMIDIOutCh(int index, const char *value) {
  if (strcmp(value, "Off") == 0) {
    midiOutCh = 0;
  } else {
    midiOutCh = atoi(value);
  }
  storeMidiOutCh(midiOutCh);
}

void settingsEncoderDir(int index, const char *value) {
  if (strcmp(value, "Type 1") == 0) {
    encCW = true;
  } else {
    encCW =  false;
  }
  storeEncoderDir(encCW ? 1 : 0);
}

void settingsCCType(int index, const char *value) {
  if (strcmp(value, "CC") == 0 ) {
    ccType = 0;
  } else {
    if (strcmp(value , "NRPN") == 0 ) {
      ccType = 1;
    } else {
      if (strcmp(value , "SYSEX") == 0 ) {
        ccType = 2;
      }
    }
  }
  storeCCType(ccType);
}

int currentIndexMIDICh() {
  return getMIDIChannel();
}

int currentIndexMIDIOutCh() {
  return getMIDIOutCh();
}

int currentIndexEncoderDir() {
  return getEncoderDir() ? 0 : 1;
}

int currentIndexCCType() {
  return getCCType();
}

// add settings to the circular buffer
void setUpSettings() {
  settings::append(settings::SettingsOption{"MIDI Ch.", {"All", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "\0"}, settingsMIDICh, currentIndexMIDICh});
  settings::append(settings::SettingsOption{"MIDI Out Ch.", {"Off", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "\0"}, settingsMIDIOutCh, currentIndexMIDIOutCh});
  settings::append(settings::SettingsOption{"Encoder", {"Type 1", "Type 2", "\0"}, settingsEncoderDir, currentIndexEncoderDir});
  settings::append(settings::SettingsOption{"Control Type", {"CC", "NRPN", "SYSEX", "\0"}, settingsCCType, currentIndexCCType});
}
