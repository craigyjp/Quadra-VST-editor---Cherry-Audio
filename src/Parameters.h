//Values below are just for initialising and will be changed when synth is initialised to current panel controls & EEPROM settings
byte midiChannel = MIDI_CHANNEL_OMNI;//(EEPROM)
byte midiOutCh = 1;//(EEPROM)
int readresdivider = 32;
int resolutionFrig = 5;
boolean recallPatchFlag = true;

int CC_ON = 127;
int CC_OFF = 127;

int MIDIThru = midi::Thru::Off;//(EEPROM)
String patchName = INITPATCHNAME;
boolean encCW = true;//This is to set the encoder to increment when turned CW - Settings Option

int leadMix = 0;
int leadMixmap = 0;
float leadMixstr = 0;

int phaserSpeed = 0;
int phaserSpeedmap = 0;
float phaserSpeedstr = 0;

int polyMix = 0;
int polyMixmap = 0;
float polyMixstr = 0;

int phaserRes = 0;
int phaserResmap = 0;
float phaserResstr = 0;

int stringMix = 0;
int stringMixmap = 0;
float stringMixstr = 0;

float chorusFlange = 0;

int bassMix = 0;
int bassMixmap = 0;
float bassMixstr = 0; // for display

int chorusSpeed = 0;
int chorusSpeedmap = 0;
float chorusSpeedstr = 0; // for display

float lfoDelay = 0;
float lfoDelaystr = 0;

int chorusDepth = 0;
int chorusDepthmap = 0;
float chorusDepthstr = 0;

int polyPWM = 0;
int polyPWMmap = 0;
float polyPWMstr = 0;

int chorusRes = 0;
int chorusResmap = 0;
float chorusResstr = 0;

int polyPW = 0;
int polyPWmap = 0;
float polyPWstr = 0;

float echoSync = 0;

float lfoSync = 0;

int polyLFOPitch =0;
int polyLFOPitchmap =0;
float polyLFOPitchstr = 0;

float shSource = 0;
float shSourcestr = 0;

int polyLFOVCF =0;
int polyLFOVCFmap =0;
float polyLFOVCFstr = 0;

float modWheel = 0;

int polyVCFRes = 0;
int polyVCFResmap = 0;
float polyVCFResstr = 0;

int stringOctave = 0;

int polyVCFCutoff =  0;
int polyVCFCutoffmap =  0;
float polyVCFCutoffstr =  0;

int bassOctave = 0;

int polyADSRDepth =  0;
int polyADSRDepthmap =  0;
float polyADSRDepthstr =  0;

int polyAttack =0;
int polyAttackmap =0;
float polyAttackstr = 0;

int lfoSpeed = 0;
int lfoSpeedmap = 0;
float lfoSpeedstr = 0;

int polyDecay =0;
int polyDecaymap =0;
float polyDecaystr = 0;

int leadPW = 0;
int leadPWmap = 0;
float leadPWstr = 0;

int polySustain = 0;
int polySustainmap = 0;
float polySustainstr = 0;

int leadPWM = 0;
int leadPWMmap = 0;
float leadPWMstr = 0;

int polyRelease = 0;
int polyReleasemap = 0;
float polyReleasestr = 0;

int bassPitch = 0;
int bassPitchmap = 0;
float bassPitchstr = 0;

int echoTime = 0;
int echoTimemap= 0;
float echoTimestr = 0;

int stringPitch = 0;
int stringPitchmap = 0;
float stringPitchstr = 0;

int echoRegen = 0;
int echoRegenmap = 0;
float echoRegenstr = 0;

int polyPitch = 0;
int polyPitchmap = 0;
float polyPitchstr = 0;

int echoDamp =0;
int echoDampmap = 0;
float echoDampstr = 0;

int polyVCF = 0;
int polyVCFmap = 0;
float polyVCFstr = 0;

int echoLevel = 0;
int echoLevelmap = 0;
float echoLevelstr = 0;

int leadPitch = 0;
int leadPitchmap = 0;
float leadPitchstr = 0;

int reverbType =0;

int leadVCF = 0;
int leadVCFmap = 0;
float leadVCFstr = 0;

int reverbDecay = 0;
int reverbDecaymap = 0;
float reverbDecaystr = 0;

int polyDepth = 0;
int polyDepthmap = 0;
float polyDepthstr = 0;

int reverbDamp = 0;
int reverbDampmap = 0;
float reverbDampstr = 0;

int leadDepth = 0;
int leadDepthmap = 0;
float leadDepthstr = 0;

int ebassDecay = 0;
int ebassDecaymap = 0;
float ebassDecaystr = 0;

int ebassRes = 0;
int ebassResmap = 0;
float ebassResstr = 0; // for display

int stringBassVolume = 0;
int stringBassVolumemap = 0;
float stringBassVolumestr = 0;

int reverbLevel = 0;
int reverbLevelmap = 0;
float reverbLevelstr = 0;

float arpSync = 0;
float arpSyncstr = 0;

int arpSpeed = 0;
int arpSpeedmap = 0;
float arpSpeedstr = 0; // for display

int arpRange = 0;

int lowEQ = 0;
int lowEQmap = 0;
float lowEQstr = 0;

int highEQ = 0;
int highEQmap = 0;
float highEQstr = 0;

int stringAttack = 0;
int stringAttackmap = 0;
float stringAttackstr = 0;

int stringRelease = 0;
int stringReleasemap = 0;
float stringReleasestr = 0;

int modSourcePhaser = 0;

int portVCO1 = 0;
int portVCO1map = 0;
float portVCO1str = 0;

int leadLFOPitch = 0;
int leadLFOPitchmap = 0;
float leadLFOPitchstr = 0;

int vco1Range = 0;
int vco1Rangemap = 0;
float vco1Rangestr = 0;

int vco2Range = 0;
int vco2Rangemap = 0;
float vco2Rangestr = 0;

int vco2Tune = 0;
int vco2Tunemap = 0;
float vco2Tunestr = 0;

int vco2Volume = 0;
int vco2Volumemap = 0;
float vco2Volumestr = 0;

int EnvtoVCF = 0;
int EnvtoVCFmap = 0;
float EnvtoVCFstr = 0;

int leadVCFCutoff = 0;
int leadVCFCutoffmap = 0;
float leadVCFCutoffstr = 0;

int leadVCFRes = 0;
int leadVCFResmap = 0;
float leadVCFResstr = 0;

int leadAttack = 0;
int leadAttackmap = 0;
float leadAttackstr = 0;

int leadDecay = 0;
int leadDecaymap = 0;
float leadDecaystr = 0;

int leadSustain = 0;
int leadSustainmap = 0;
float leadSustainstr = 0;

int leadRelease = 0;
int leadReleasemap = 0;
float leadReleasestr = 0;

int masterTune = 0;
int masterTunemap = 0;
float masterTunestr = 0;

int masterVolume = 0;
int masterVolumemap = 0;
float masterVolumestr = 0;

float chorusOn = 0;

int portVCO2 = 0;
int portVCO2map = 0;
float portVCO2str = 0;

int ebass16 = 0;
int ebass8 = 0;
int leadPWMmod = 0;
int leadVCFmod = 0;
int polyLearn = 0;
int trillUp = 0;
int trillDown= 0;
int leadLearn = 0;
int leadNPlow = 0;
int leadNPhigh = 0;
int leadNPlast = 0;
int prevleadNPlow = 0;
int prevleadNPhigh = 0;
int prevleadNPlast = 0;
int leadNoteTrigger = 0;
int vco2KBDTrk = 0;
int lead2ndVoice = 0;
int leadTrill= 0;
int leadVCO1wave = 0;
int leadVCO2wave = 0;
int polyWave = 0;
int polyPWMSW = 0;
int LFOTri= 0;
int LFOSawUp = 0;
int LFOSawDown = 0;
int LFOSquare = 0;
int LFOSH = 0;
int strings8 = 0;
int strings4 = 0;
int polyNoteTrigger = 0;
int polyVelAmp = 0;
int polyDrift = 0;
int poly16 = 0;
int poly8 = 0;
int poly4 = 0;
int bassNoteTrigger= 0;
int stringbass16 = 0;
int stringbass8 = 0;
int hollowWave = 0;
int bassLearn = 0;
int stringsLearn = 0;
int bassPitchSW = 0;
int stringsPitchSW = 0;
int polyPitchSW = 0;
int polyVCFSW = 0;
int leadPitchSW = 0;
int leadVCFSW = 0;
int polyAfterSW = 0;
int leadAfterSW = 0;
int phaserBassSW = 0;
int phaserStringsSW = 0;
int phaserPolySW = 0;
int phaserLeadSW = 0;
int chorusBassSW = 0;
int chorusStringsSW = 0;
int chorusPolySW = 0;
int chorusLeadSW = 0;
int echoBassSW = 0;
int echoStringsSW = 0;
int echoPolySW = 0;
int echoLeadSW = 0;
int reverbBassSW = 0;
int reverbStringsSW = 0;
int reverbPolySW = 0;
int reverbLeadSW = 0;

int arpOnSW = 0;
int arpDownSW = 0;
int arpUpSW = 0;
int arpUpDownSW = 0;
int arpRandomSW = 0;
int arpHoldSW = 0;

int returnvalue = 0;

//Pick-up - Experimental feature
//Control will only start changing when the Knob/MIDI control reaches the current parameter value
//Prevents jumps in value when the patch parameter and control are different values
boolean pickUp = false;//settings option (EEPROM)
boolean pickUpActive = false;
#define TOLERANCE 2 //Gives a window of when pick-up occurs, this is due to the speed of control changing and Mux reading
