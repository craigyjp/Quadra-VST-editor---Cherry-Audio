// This optional setting causes Encoder to use more optimized code,
// It must be defined before Encoder.h is included.
#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>
#include <Bounce.h>
#include "TButton.h"
#include <ADC.h>
#include <ADC_util.h>

ADC *adc = new ADC();

//Teensy 3.6 - Mux Pins
#define MUX_0 28
#define MUX_1 32
#define MUX_2 30
#define MUX_3 31

#define MUX1_S A2 // ADC1
#define MUX2_S A0 // ADC1
#define MUX3_S A1 // ADC1
#define MUX4_S A11 // ADC0
#define MUX5_S A10 // ADC0


//Mux 1 Connections
#define MUX1_leadMix 0
#define MUX1_polyMix 1
#define MUX1_stringMix 2
#define MUX1_bassMix 3
#define MUX1_lfoDelay 4
#define MUX1_polyPWM 5
#define MUX1_polyPW 6
#define MUX1_lfoSync 7
#define MUX1_shSource 8
#define MUX1_modWheel 9
#define MUX1_stringOctave 10
#define MUX1_bassOctave 11
#define MUX1_hpfCutoff1 12
#define MUX1_lfoSpeed 13
#define MUX1_leadPW 14
#define MUX1_leadPWM 15

//Mux 2 Connections
#define MUX2_bassPitch 0
#define MUX2_stringPitch 1
#define MUX2_polyPitch 2
#define MUX2_polyVCF 3
#define MUX2_leadPitch 4
#define MUX2_leadVCF 5
#define MUX2_polyDepth 6
#define MUX2_leadDepth 7
#define MUX2_ebassDecay 8
#define MUX2_ebassRes 9
#define MUX2_stringBassVolume 10
#define MUX2_lowEQ 11
#define MUX2_highEQ 12
#define MUX2_stringAttack 13
#define MUX2_stringRelease 14
#define MUX2_modSourcePhaser 15

//Mux 3 Connections
#define MUX3_phaserSpeed 0
#define MUX3_phaserRes 1
#define MUX3_chorusFlange 2
#define MUX3_chorusSpeed 3
#define MUX3_chorusDepth 4
#define MUX3_chorusRes 5
#define MUX3_echoSync 6
#define MUX3_polyLFOPitch 7
#define MUX3_polyLFOVCF 8
#define MUX3_polyADSRDepth 9
#define MUX3_polyVCFCutoff 10
#define MUX3_polyVCFRes 11
#define MUX3_polyAttack 12
#define MUX3_polyDecay 13
#define MUX3_polySustain 14
#define MUX3_polyRelease 15

//Mux 4 Connections
#define MUX4_echoTime 0
#define MUX4_echoRegen 1
#define MUX4_echoDamp 2
#define MUX4_echoLevel 3
#define MUX4_reverbType 4
#define MUX4_reverbDecay 5
#define MUX4_reverbDamp 6
#define MUX4_reverbLevel 7
#define MUX4_arpSync 8
#define MUX4_arpSpeed 9
#define MUX4_arpRange 10


//Mux 5 Connections
#define MUX5_portVCO1 0
#define MUX5_portVCO2 1
#define MUX5_leadLFOPitch 2
#define MUX5_vco1Range 3
#define MUX5_vco2Range 4
#define MUX5_vco2Tune 5
#define MUX5_vco2Volume 6
#define MUX5_EnvtoVCF 7
#define MUX5_leadVCFCutoff 8
#define MUX5_leadVCFRes 9
#define MUX5_leadAttack 10
#define MUX5_leadDecay 11
#define MUX5_leadSustain 12
#define MUX5_leadRelease 13
#define MUX5_masterTune 14
#define MUX5_masterVolume 15

// Buttons
#define LEAD_PWM_SW 0
#define LEAD_VCF_MOD_SW 1
#define POLY_LEARN_SW 2
#define TRILL_UP_SW 3
#define TRILL_DOWN_SW 4
#define LEAD_LEARN_SW 5

#define LEAD_NP_HIGH_SW 8
#define LEAD_NP_LAST_SW 9
#define LEAD_NOTE_TRIGGER_SW 10
#define LEAD_VCO2_KBD_TRK_SW 11
#define LEAD_SECOND_VOICE_SW 12
#define LEAD_TRILL_SW 13
#define LEAD_VCO1_WAVE_SW 14
#define LEAD_VCO2_WAVE_SW 15

#define POLY_WAVE_SW 16
#define POLY_PWM_SW 17
#define LFO_TRI_SW 18
#define LFO_SAW_UP_SW 19
#define LFO_SAW_DN_SW 20
#define LFO_SQR_SW 21
#define LFO_SH_SW 22
#define LEAD_NP_LOW_SW 23

#define POLY_VEL_AMP_SW 24
#define POLY_NOTE_TRIGGER_SW 25
#define STRINGS_4_SW 26
#define STRINGS_8_SW 27
#define POLY_DRIFT_SW 28
#define POLY_16_SW 29
#define POLY_8_SW 30
#define POLY_4_SW 31

#define EBASS_16_SW 32
#define EBASS_8_SW 33
#define BASS_NOTE_TRIGGER_SW 34
#define STRINGS_BASS_16_SW 35
#define STRINGS_BASS_8_SW 36
#define HOLLOW_WAVE_SW 37
#define BASS_LEARN_SW 38
#define STRINGS_LEARN_SW 39

#define BASS_PITCH_SW 40
#define STRINGS_PITCH_SW 41
#define POLY_PITCH_SW 42
#define POLY_VCF_SW 43
#define LEAD_PITCH_SW 44
#define LEAD_VCF_SW 45
#define POLY_TOUCH_DEST_SW 46
#define LEAD_TOUCH_DEST_SW 47

#define PHASE_BASS_SW 48
#define PHASE_STRINGS_SW 49
#define PHASE_POLY_SW 50
#define PHASE_LEAD_SW 51
#define CHORUS_BASS_SW 52
#define CHORUS_STRINGS_SW 53
#define CHORUS_POLY_SW 54
#define CHORUS_LEAD_SW 55

#define ECHO_BASS_SW 56
#define ECHO_STRINGS_SW 57
#define ECHO_POLY_SW 58
#define ECHO_LEAD_SW 59
#define REVERB_BASS_SW 60
#define REVERB_STRINGS_SW 61
#define REVERB_POLY_SW 62
#define REVERB_LEAD_SW 63

#define ARP_ON_OFF_SW 64
#define ARP_DOWN_SW 65
#define ARP_UP_SW 66
#define ARP_UP_DOWN_SW 67
#define ARP_RANDOM_SW 68
#define ARP_HOLD_SW 69

// LEDS
// 1
#define LEAD_PWM_RED_LED 0
#define LEAD_VCF_RED_LED 1
#define LEAD_PWM_GREEN_LED 2
#define LEAD_VCF_GREEN_LED 3
#define LEAD_NOTE_TRIGGER_GREEN_LED 4
#define POLY_PWM_GREEN_LED 5

//2
#define LEAD_NP_HIGH_LED 8
#define LEAD_NP_LAST_LED 9
#define LEAD_NOTE_TRIGGER_RED_LED 10
#define LEAD_VCO2_KBD_TRK_LED 11
#define LEAD_SECOND_VOICE_LED 12
#define LEAD_TRILL_LED 13
#define LEAD_VCO1_WAVE_LED 14
#define LEAD_VCO2_WAVE_LED 15
//3
#define POLY_WAVE_LED 16
#define POLY_PWM_RED_LED 17
#define LFO_TRI_LED 18
#define LFO_SAW_UP_LED 19
#define LFO_SAW_DN_LED 20
#define LFO_SQUARE_LED 21
#define LFO_SH_LED 22
#define LEAD_NP_LOW_LED 23
//4
#define STRINGS_8_LED 24
#define STRINGS_4_LED 25
#define POLY_NOTE_TRIGGER_RED_LED 26
#define POLY_VEL_AMP_LED 27
#define POLY_DRIFT_LED 28
#define POLY_16_LED 29
#define POLY_8_LED 30
#define POLY_4_LED 31
//5
#define EBASS_16_LED 32
#define EBASS_8_LED 33
#define BASS_NOTE_TRIGGER_RED_LED 34
#define STRINGS_BASS_16_LED 35
#define STRINGS_BASS_8_LED 36
#define HOLLOW_WAVE_LED 37
#define POLY_NOTE_TRIGGER_GREEN_LED 38
#define BASS_NOTE_TRIGGER_GREEN_LED 39
//6
#define BASS_PITCH_LED 40
#define STRINGS_PITCH_LED 41
#define POLY_PITCH_LED 42
#define POLY_VCF_LED 43
#define LEAD_PITCH_LED 44
#define LEAD_VCF_LED 45
#define POLY_TOUCH_DEST_RED_LED 46
#define POLY_TOUCH_DEST_GREEN_LED 47
//7
#define PHASE_BASS_LED 48
#define PHASE_STRINGS_LED 49
#define PHASE_POLY_LED 50
#define PHASE_LEAD_LED 51
#define CHORUS_BASS_LED 52
#define CHORUS_STRINGS_LED 53
#define CHORUS_POLY_LED 54
#define CHORUS_LEAD_LED 55
//8
#define ECHO_BASS_LED 56
#define ECHO_STRINGS_LED 57
#define ECHO_POLY_LED 58
#define ECHO_LEAD_LED 59
#define REVERB_BASS_LED 60
#define REVERB_STRINGS_LED 61
#define REVERB_POLY_LED 62
#define REVERB_LEAD_LED 63
//9
#define ARP_ON_OFF_LED 64
#define ARP_DOWN_LED 65
#define ARP_UP_LED 66
#define ARP_UP_DOWN_LED 67
#define ARP_RANDOM_LED 68
#define ARP_HOLD_LED 69
#define LEAD_TOUCH_DEST_RED_LED 70
#define LEAD_TOUCH_DEST_GREEN_LED 71
//Teensy 3.6 Pins

#define RECALL_SW 17
#define SAVE_SW 41
#define SETTINGS_SW 12
#define BACK_SW 10

#define ENCODER_PINA 5
#define ENCODER_PINB 4

#define MUXCHANNELS 16
#define QUANTISE_FACTOR 31

#define DEBOUNCE 30

static byte muxInput = 0;
static int mux1ValuesPrev[MUXCHANNELS] = {};
static int mux2ValuesPrev[MUXCHANNELS] = {};
static int mux3ValuesPrev[MUXCHANNELS] = {};
static int mux4ValuesPrev[MUXCHANNELS] = {};
static int mux5ValuesPrev[MUXCHANNELS] = {};

static int mux1Read = 0;
static int mux2Read = 0;
static int mux3Read = 0;
static int mux4Read = 0;
static int mux5Read = 0;

static long encPrevious = 0;

//These are pushbuttons and require debouncing

Bounce recallButton = Bounce(RECALL_SW, DEBOUNCE); //On encoder
boolean recall = true; //Hack for recall button
Bounce saveButton = Bounce(SAVE_SW, DEBOUNCE);
boolean del = true; //Hack for save button
//Bounce settingsButton = Bounce(SETTINGS_SW, DEBOUNCE);
TButton settingsButton{SETTINGS_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION};
boolean reini = true; //Hack for settings button
Bounce backButton = Bounce(BACK_SW, DEBOUNCE);
boolean panic = true; //Hack for back button
Encoder encoder(ENCODER_PINB, ENCODER_PINA);//This often needs the pins swapping depending on the encoder

void setupHardware()
{
  //Volume Pot is on ADC0
  adc->adc0->setAveraging(32); // set number of averages 0, 4, 8, 16 or 32.
  adc->adc0->setResolution(12); // set bits of resolution  8, 10, 12 or 16 bits.
  adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED); // change the conversion speed
  adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::MED_SPEED); // change the sampling speed

  //MUXs on ADC1
  adc->adc1->setAveraging(32); // set number of averages 0, 4, 8, 16 or 32.
  adc->adc1->setResolution(12); // set bits of resolution  8, 10, 12 or 16 bits.
  adc->adc1->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED); // change the conversion speed
  adc->adc1->setSamplingSpeed(ADC_SAMPLING_SPEED::MED_SPEED); // change the sampling speed

  //Mux address pins

  pinMode(MUX_0, OUTPUT);
  pinMode(MUX_1, OUTPUT);
  pinMode(MUX_2, OUTPUT);
  pinMode(MUX_3, OUTPUT);

  digitalWrite(MUX_0, LOW);
  digitalWrite(MUX_1, LOW);
  digitalWrite(MUX_2, LOW);
  digitalWrite(MUX_3, LOW);

    //Mux ADC
  pinMode(MUX1_S, INPUT_DISABLE);
  pinMode(MUX2_S, INPUT_DISABLE);
  pinMode(MUX3_S, INPUT_DISABLE);
  pinMode(MUX4_S, INPUT_DISABLE);
  pinMode(MUX5_S, INPUT_DISABLE);

  //Switches
  pinMode(RECALL_SW, INPUT_PULLUP); //On encoder
  pinMode(SAVE_SW, INPUT_PULLUP);
  pinMode(SETTINGS_SW, INPUT_PULLUP);
  pinMode(BACK_SW, INPUT_PULLUP);

}
