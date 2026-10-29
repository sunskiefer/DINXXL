#pragma once

#include "scene_types.h"
#include "packed_storage.h"

#define DEV_ADR 0x20
#define PIN_SDA PB7
#define PIN_SCL PB6

#define DAC2_CS PA6
#define DAC1_CS PB9

//VERSION TO DISPLAY
const uint8_t versionTop = 6;     //      value :     0 1 2 3 4 5 6 7 8 9
const uint8_t versionBottom = 0;  //   silksceen :    1 2 3 4 5 6 7 8 9 0

bool OnePpq;
bool lastOnePpq = false;
bool notPc;

bool r3;

enum MidiInputSource : uint8_t {
  MIDI_INPUT_NONE = 0,
  MIDI_INPUT_DIN,
  MIDI_INPUT_USB,
};

MidiInputSource activeMidiInputSource = MIDI_INPUT_NONE;
bool suppressIncomingMidiForwarding = false;

#if defined(__MK20DX256__)  // Macro for Teensy 3.1/3.2
  // Code specific to Teensy 3.1/3.2
#define CPU_RESTART_ADDR (uint32_t *)0xE000ED0C
#define CPU_RESTART_VAL 0x5FA0004
#define CPU_RESTART (*CPU_RESTART_ADDR = CPU_RESTART_VAL);

#include <uClock.h>
#include <OPL2.h>
#include "BlastLedMatrix.h"
#include <SPI.h>
#include <SD.h>
#include <MIDI.h>
IntervalTimer tickTimer;
#define MUX1_PIN A10
#define MUX2_PIN A11
#define MUX3_PIN A12
#define MUX4_PIN A6
#define MUX5_PIN A15
#define MUX6_PIN A16
#define SYNC_DETECT_PIN 10
#define SYNC_IN_PIN 14
#define SYNC_OUT_PIN 15
#define DATA_0_PIN 28
#define DATA_1_PIN 29
#define DATA_2_PIN 30
#define DATA_3_PIN 31
#define DATA_4_PIN 33
#define DATA_5_PIN 3
#define DATA_6_PIN 4
#define DATA_7_PIN 5
#define SD_CS_PIN 6
#define RUN_PIN 11

#define FM_RW_PIN 2
#define FM_RESET_PIN 12
#define FM_CS_PIN 32
#define A0_PIN 24
#define A1_PIN 25

#define SETTINGS_NEWLENGTH 25

BlastLedMatrix mx = BlastLedMatrix(7, 13, 9, 1);  //din clock cs

#elif defined(STM32F1) || defined(STM32F4)  // Code specific to STM32
  // Code specific to STM32

uint8_t EEPROMArray[2048];
bool pulseCount;


#include <uClock-st.h>
#include <OPL2BBST.h>
#include "BlastLedMatrix.h"
#include <SPI.h>
#include <SD.h>
#include <MIDI.h>
#include "STM32TimerInterrupt.h"

HardwareSerial Serial1(PA10, PA9);
// Init STM32 timer TIM1
STM32Timer ITimer0(TIM4);




#define MUX1_PIN PA2
#define MUX2_PIN PA3
#define MUX3_PIN A0
#define MUX4_PIN PB13
#define MUX5_PIN PB14
#define MUX6_PIN PA4
#define SYNC_DETECT_PIN PB8
#define SYNC_IN_PIN PB9
#define SYNC_OUT_PIN PB15
#define DATA_0_PIN PC0
#define DATA_1_PIN PC1
#define DATA_2_PIN PC2
#define DATA_3_PIN PC3
#define DATA_4_PIN PC4
#define DATA_5_PIN PC5
#define DATA_6_PIN PC6
#define DATA_7_PIN PC7
#define SD_CS_PIN PA8
#define RUN_PIN PB0

#define FM_RW_PIN PB5
#define FM_RESET_PIN PC8
#define FM_CS_PIN PC11
#define A0_PIN PC9
#define A1_PIN PC10

BlastLedMatrix mx = BlastLedMatrix(PA7, PA5, PB11, 1);  //din clock cs

#endif



uint8_t vgmFolder;
bool vgmState;

#define MODE_VGM 4
#define MODE_SONG 3
#define MODE_PATT_KIT 0
#define MODE_FUNCTION 2
#define MODE_STEP 1

#define LED_kitPat 40
#define LED_step 41
#define LED_function 42
#define LED_run 50
#define LED_tempo 51
#define LED_rec 52

#define BUTTON_KIT 30
#define BUTTON_STEP 26
#define BUTTON_FUNCTION 27
#define BUTTON_TEMPO 24
#define BUTTON_REC 25

#define FUNC_SAVE 0
#define FUNC_LENGTH 1
#define FUNC_CLEAR 2
#define FUNC_WIGGLE 3
#define FUNC_SWING 4
#define FUNC_MUTE 5
#define FUNC_SOLO 6
#define FUNC_PROB 7
#define FUNC_STUTTER 8
#define FUNC_BEND 9
#define FUNC_FREEZE 10
#define FUNC_TREM 11
#define FUNC_VIBR 12
#define FUNC_PHONES 13
#define FUNC_MIX 14
#define FUNC_ROUTE 15


const float vibratoPowers[10] = { .1, .2, .5, 1, 2, 3, 4, 8, 16, 32 };

volatile int16_t tremTickCounter;
volatile int16_t vibTickCounter;
volatile int16_t tremSubCounter;
volatile int16_t vibSubCounter;
uint8_t vibratoPower[16];
const uint8_t vibratoRates[] = { 1, 2, 3, 4, 6, 8, 12, 16, 24, 32 };
uint8_t lastStepPoly[16][10];  //last step of channels 2 and up
uint8_t polyStep[10];          //step of individual tracks
bool changedSelectedVoice;
bool quitStutterFlag;
bool alreadyClearedAuto;
bool justTransposed = false;  //don't toggle the synth step if we pressed it to transpose it
uint8_t buffy[512];
uint8_t sdSessionDepth = 0;  // protects nested SD use / SPI takeover
bool sdCardReady = false;
bool sdWriteOk = true;
bool sdTimerPrimed = false;   // STM32 timer is ready to stop/restart around SD
volatile bool sdLedsMuted = false;  // block LED SPI updates while SD owns the bus
bool ledsReady = false;             // set once mx.begin() has run
int16_t buffCounter;
bool toolMode;  //enabled by sending 1982 twice (noteOff 19 velocity 82 Ch16)
bool noFuncAction;
bool chainFreeze;
volatile bool newFlash;  //sets to true when we flash something (to update the leds)
uint8_t saveKitAlso;
bool folderChanged;
uint8_t pendingFolder;
int16_t pickNote;  //note of the picked step (hold rec to pick/place)
bool pickStep;
uint8_t pickStepNumber;  //step number that we picked
uint8_t randomTable[8];
uint8_t randomIndex;
uint8_t lastPattern;
bool seedNewRandomTable = true;
int16_t tremoloSRR;
int16_t vibratoSRR;
bool drill;
//prevent from reacting to too many buttons at once (jumps to other modes?!)
int16_t bottomButtonLimitCounter;
bool chaseAllPages;
const uint8_t bottomButtonLimit = 6;
bool muteHeld;
bool pageSkip[4];
bool wonkySwing;
uint8_t tremoloLast;
//when true we are saving the song to a new slot and need to save everything
bool saveToNewSlot;
bool flipStepFlag;
bool ignoreNotes;
bool synthHotOffOn[15];
bool synthHotOff[15];
bool justChangedToTrickStepEdit;

volatile int16_t doubleClickTimer;  //timer for double click
volatile uint8_t lastDouble;        //latest button for double click 0=kit 1=step 2=function 4=any swing top buttons
//5=top buttons in step mode (to toggle monitorSynth)
//6 = clear (for resetting all faders);
int saveCounter;

//when we hold a bottom button in step mode for x time, show the nudge (delay)amount on the top ten
uint8_t nudgeDelay[16][5][64];  //0-9 value stored in nibbles (voice 1-5 in low nibble, voice 6-10 in high nibble)
uint8_t nudgeDelayUR[5][64];
bool EEPROMChanged;
uint32_t EEPROMLastChangeMs = 0;
volatile int16_t nudgeDelayCountDown[10];
volatile int16_t nudgeDelayTimer;
volatile uint8_t nudgeMode;     // 0=idle 1=bottom held/entering nudge 2=show/input nudge on top buttons
volatile bool nudged[10];       //if set high, nudge time reached, time to fire
uint8_t drumNotes[6];  //store the note numbers of each drum part
const uint8_t drumNotesFactory[] = { 36, 38, 45, 42, 46, 49 };
uint8_t prob[10][10];
uint8_t selectedSynth;  //0-3 keep track of last selected Synth so fader don't jump to synth 1 when we select a drum voice
int offsetOct[16][10];

// Shared drum parameter precedence (which voice last wrote the shared multiplier on ch7 op1)
enum : uint8_t {
  DRUM_SHARED_MULT_HATS = 0,
  DRUM_SHARED_MULT_SNARE = 1,
};
bool changeUndo;
bool currentLed[53];       //current status of LED (for EZ flipping)
uint8_t flipLedTimer[53];  //flips led when reaches 0
bool flipped[53];
unsigned long visualiserTimeout;  //if no buttons are touched for X time, blink the top leds
volatile uint8_t chaseStep;
uint8_t fat;
const float fatMultipliers[] = { .25, .5, .75, .97, .98, .99, 1.01, 1.02, 1.04, 1.06, 1.5, 2, 2.5, 3, 4, 8 };
uint8_t op4Mode;  //used for algo

uint8_t trickStepEdit;  // 1=stutter 2=bend 3=freeze

//wipe leds from left to right when wiping is true
uint8_t wipeStep;
volatile bool wiping;
volatile uint8_t wipeCounter;  //advnce to next wipe step when reaches X
uint8_t startSkip;
bool algo[4];
uint8_t multBase[4];
uint8_t multLast[4];
uint8_t multLfo[4];
bool bendHeld;
//IntervalTimer tickTimer;
const int16_t analogMax = 8000;  //analog timeout
uint8_t analogCounts;
bool stutterHeld[10];
int16_t stutterPressed;
bool tempoHeld;
bool resetRegisters;
uint8_t pendingKit;
uint8_t pageCopyTarget;  //when 4 we are selecting a page to copy, when <4 (page number) we are pasting

uint8_t temp16[16];  //array of bytes used for pattern rotation

bool tremSync = 1;     //sync to pulse and MIDI
bool vibratoSync = 1;  //sync to pulse and MIDI

bool midiHot[6];  //when high send drum MIDI

uint8_t midiHotSynth[5];  //when high send drum MIDI

volatile uint8_t midiTimeout[11];  //Note off when reached zero
uint8_t midiLastNoteSynth[6];
uint8_t kitVariation[16][10];
uint8_t copyVariation;
uint8_t tempoPhase;  //1 = hundreds 2=tens 3=decimals
int16_t tempTempo1;
int16_t tempTempo2;
uint8_t topHeldCounter, synthCopy;
bool loadingDefault;
File sdFile;
int16_t blinkTimer;
int16_t lastBottomButton;
bool stepHeld;
bool saveFlag, saveKitFlag;
bool saveAuto;
bool kitHeld;
bool bankChooseMode;
volatile bool flip;
bool oplReady = false;
bool flipBit;
uint8_t flipStep;
bool autoBendEnabled = true;
bool autoFreezeEnabled = true;
bool freezeStepLast;
bool freezeHeld;
bool tremHeld;
bool vibHeld;
bool bendStepLast;
volatile uint8_t plocStep;  //holding a bottom button=this step will have ploc assigned when fader is moved.
bool plocked;
bool firstStutter;
uint8_t stutterLast;
uint8_t fireStep[10];
volatile uint8_t stutterStepPoly[10];
uint8_t vibAuto[16][32];
uint8_t vibAutoWav[16][32];
uint8_t tremAuto[16][32];
uint8_t tremAutoWav[16][32];
uint8_t swingAuto[16][32];
uint8_t kitAuto[16][32];
uint8_t wiggleAuto[16][32];
RouteAutoStorage routeAuto;
uint8_t phonesAuto[16][32];
uint8_t mixAuto[16][32];
PackedBitMatrix<16, 64> bendSteps;
PackedBitMatrix<16, 64> freezeSteps;
uint8_t stutterSteps[16][64];
bool stutterEnabled = true;

uint8_t stutterNumber;  //current stutter rate

RouteAutoUndoBuffer routeAutoUR;
uint8_t stutterStepsUR[64];
uint8_t vibAutoUR[32];
uint8_t tremAutoUR[32];
uint8_t tremWavPatt[16];
uint8_t vibWavPatt[16];
uint8_t vibAutoWavUR[32];
uint8_t tremAutoWavUR[32];
uint8_t swingAutoUR[32];
uint8_t kitAutoUR[32];
uint8_t wiggleAutoUR[32];

PackedBitset<64> bendStepsUR;
PackedBitset<64> freezeStepsUR;
uint8_t phonesAutoUR[32];
uint8_t mixAutoUR[32];
uint8_t undoPattern;  //don't undo/redo over another pattern
uint8_t stutterStepLast;
uint8_t currentFolder;
uint8_t currentSong;
bool funcHeld;
bool specialMode;
bool safe = true;  //keep decays within user-firendly ranges
bool channelSetup = false;
uint8_t ch1 = 1;
uint8_t ch2 = 2;
uint8_t ch3 = 3;
uint8_t ch4 = 4;
uint8_t ch10 = 10;
uint8_t chMuteMidi = 16;
uint8_t chSoloMidi = 16;
uint8_t chProgramChange = 16;
uint8_t probTarget;
bool probHeld;
bool probFire[10];
uint8_t midiSetupTarget;
uint8_t probStack[20];
uint8_t probCounter = 10;

SceneBank scenes;
uint16_t sceneValidMask[16];
bool sceneModeActive = false;
bool sceneMorphActive = false;
uint8_t sceneMorphPattern = 0;
uint8_t sceneMorphSlot = 0xFF;
SceneState sceneMorphStartState{};
SceneState sceneMorphTargetState{};
uint32_t sceneMorphTicksRemaining = 0;
uint32_t sceneMorphTicksTotal = 0;
uint8_t sceneMorphLastMix = 0;
uint8_t sceneMorphLastPhones = 0;
uint8_t sceneMorphActiveSlot = 0xFF;
uint8_t sceneMorphLengthSteps = 4;
uint32_t sceneMorphElapsedTicks = 0;
uint16_t sceneMorphLastTickCounter = 0;

uint8_t kit[16];          // [kit number]
uint8_t pattChain[256];   // [kit number][next kit countdown in steps]
uint8_t pattChainLength;  // in slots
uint8_t pattChainStep;    // in slots
bool patternVisualizerDirty = false;
uint32_t patternVisualizerLastSendMs = 0;
uint32_t patternVisualizerLastActivityMs = 0;
const uint8_t doubleClick_timeout = 60;
const int ISRCLOCK = 5000;
byte loadLeds;
byte loadLedsLast;
bool midiSlave = false;
volatile bool newStepHot;
volatile bool analogSlave;
uint8_t syncOutCounter;
volatile uint8_t tempoTaps;
volatile int16_t pulseCounter;
bool pulseAlternator;
uint8_t brightness = 8;

const uint8_t wav[10][64] = {
  { 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60,
    64, 68, 72, 76, 80, 84, 88, 92, 96, 100, 104, 108, 112, 116, 120, 127,
    127, 120, 116, 112, 108, 104, 100, 96, 92, 88, 84, 80, 76, 72, 68, 64,
    60, 56, 52, 48, 44, 40, 36, 32, 28, 24, 20, 16, 12, 8, 4, 0 },
  { 0, 4, 8, 12, 16, 24, 36, 48, 56, 68, 76, 88, 96, 104, 116, 124, 120, 112, 104, 96, 92, 80, 72, 68, 60, 52, 44, 36, 28, 20, 8, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 4, 12, 28, 40, 52, 64, 76, 88, 96, 104, 112, 120, 120, 124, 124, 120, 116, 112, 104, 96, 84, 76, 68, 56, 48, 40, 32, 16, 8, 4, 0, 4, 12, 24, 36, 44, 56, 72, 80, 88, 96, 104, 112, 116, 120, 124, 124, 124, 120, 116, 112, 108, 100, 96, 88, 80, 72, 60, 52, 32, 24, 16, 0 },
  { 0, 4, 8, 12, 16, 20, 24, 32, 40, 44, 52, 56, 64, 76, 80, 92, 100, 112, 116, 120, 124, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 12, 16, 20, 28, 32, 36, 44, 52, 64, 72, 80, 92, 100, 108, 112, 120, 124, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 4, 5, 7, 11, 15, 17, 19, 23, 27, 29, 31, 34, 38, 42, 46, 50, 54, 58, 64, 70,
    76, 80, 82, 85, 89, 92, 96, 100, 104, 108, 112, 116, 120, 124, 103, 79, 66,
    45, 30, 18, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 8, 12, 32, 32, 44, 52, 64, 84, 104, 124, 116, 108, 72, 60, 28, 20, 8, 4, 16, 36, 52, 72, 96, 120, 124, 112, 100, 80, 60, 44, 32, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 124, 116, 108, 100, 92, 88, 84, 76, 68, 60, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 52, 48, 44, 40, 32, 28, 20, 16, 12, 8, 0 },
  { 124, 20, 88, 80, 40, 36, 36, 92, 88, 68, 116, 80, 76, 72, 68, 64, 96, 92, 92, 104, 48, 40, 24, 8, 56, 68, 112, 112, 108, 104, 20, 20, 20, 20, 84, 104, 44, 24, 24, 24, 24, 52, 48, 44, 44, 4, 44, 124, 124, 124, 20, 20, 92, 20, 20, 68, 44, 44, 96, 64, 68, 72, 64, 0 },
  { 76, 80, 84, 36, 40, 48, 0, 68, 72, 76, 80, 0, 52, 88, 92, 100, 0, 64, 64, 96, 100, 104, 0, 36, 32, 92, 96, 116, 116, 120, 0, 0, 0, 0, 84, 76, 48, 40, 84, 84, 80, 76, 0, 0, 96, 96, 96, 96, 0, 0, 76, 48, 44, 40, 0, 96, 100, 60, 64, 64, 116, 116, 120, 0 }
};

const float note2freq[] = {
  8.1758, 8.6620, 9.1770, 9.7227, 10.3009, 10.9134, 11.5623, 12.2499,
  12.9783, 13.7500, 14.5676, 15.4339, 16.3516, 17.3239, 18.3540, 19.4454,
  20.6017, 21.8268, 23.1247, 24.4997, 25.9565, 27.5000, 29.1352, 30.8677,
  32.7032, 34.6478, 36.7081, 38.8909, 41.2034, 43.6535, 46.2493, 48.9994,
  51.9131, 55.0000, 58.2705, 61.7354, 65.4064, 69.2957, 73.4162, 77.7817,
  82.4069, 87.3071, 92.4986, 97.9989, 103.8262, 110.0000, 116.5409, 123.4708,
  130.8128, 138.5913, 146.8324, 155.5635, 164.8138, 174.6141, 184.9972, 195.9977,
  207.6523, 220.0000, 233.0819, 246.9417, 261.6256, 277.1826, 293.6648, 311.1270,
  329.6276, 349.2282, 369.9944, 391.9954, 415.3047, 440.0000, 466.1638, 493.8833,
  523.2511, 554.3653, 587.3295, 622.2540, 659.2551, 698.4565, 739.9888, 783.9909,
  830.6094, 880.0000, 932.3275, 987.7666, 1046.5023, 1108.7305, 1174.6591, 1244.5079,
  1318.5102, 1396.9129, 1479.9777, 1567.9817, 1661.2188, 1760.0000, 1864.6550, 1975.5332,
  2093.0045, 2217.4610, 2349.3181, 2489.0159, 2637.0205, 2793.8259, 2959.9554, 3135.9635,
  3322.4376, 3520.0000, 3729.3101, 3951.0664, 4186.0090, 4434.9221, 4698.6363, 4978.0317,
  5274.0409, 5587.6517, 5919.9108, 6271.9270, 6644.8752, 7040.0000, 7458.6202, 7902.1328,
  8372.0181, 8869.8442, 9397.2726, 9956.0635, 10548.0818, 11175.3034, 11839.8215, 12543.8540
};

const int16_t x256[] = { 0, 256, 512, 768, 1024, 1280, 1536, 1792, 2048, 2304, 2560, 2816, 3072, 3328, 3584, 3840 };
const uint8_t x16[] = { 0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240 };
uint8_t heldTop;
const uint8_t scan[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
uint8_t scanCount, scanCount2;
volatile bool engineTickHot;
uint8_t muxIndex;
bool clearHeld;
volatile bool bend;
bool autoDone;
volatile bool killTempoLed;
volatile bool showModeFlag;

uint8_t currentPattern = 0;
uint8_t currentKit = 0;
volatile int16_t bendCounter;
volatile bool newBend[16];
uint8_t selectedPage;
bool freeze;
int16_t masterTempo;
const int16_t defaultTempo = 200;

bool visu;  //high when visua is on

uint8_t kitHeldCounter;  //how many kit buttons held (to detect first)
uint8_t kitCopy;         //clone target

uint8_t pattHeldCounter;  //how many patt buttons held (to detect first)
uint8_t pattCopy;         //clone target

PackedBitset<152> wiggledOnThisStep;
uint8_t opl[40][152];
uint8_t oplLast[152];
PackedBitMatrix<16, 152> randomer;

uint8_t oplUR[10][152];
PackedBitset<152> randomerUR;

uint8_t octaveHeld;
uint8_t synthTrig;
uint8_t selectedVoice;
int16_t pendingPattern = -1;
bool lastStepMode;
bool routeHeld;
bool wiggleOnEnabledStep;
uint8_t heldNotes[4];
uint8_t vibratoRateMultiplier[16];
uint8_t tremRateMultiplier[16];

uint8_t vibratoRateMultiplierUR;
uint8_t tremRateMultiplierUR;
uint8_t vibratoPowerUR;
bool wiggleNudge;
uint8_t swiping = 0;
//DATA
//PATTERN
uint8_t pattern[16][2][64];  //
uint8_t synth[16][4][64];    //
int16_t pages[16][10];
uint8_t pagesLegacy[16];
bool analogPulseLast;
int16_t midiClockTicks;
//when 1 the bottomrow buttons are used to play the synth instead of toggle the steps
bool monitorSynth;
uint8_t musicRoot;
uint8_t patternUR[2][64];  //
uint8_t synthUR[4][64];    //
uint8_t pagesUR[10];
uint8_t lastStepUR[10];
bool allStepsZeroFlag;
//AUTO
uint8_t autoData[3000];
uint8_t autoDataUR[500];


const int16_t autoThird = 1000;
const int16_t autoTwoThirds = 2000;
bool quantise;      //when high we write the nudge during punch
uint8_t nudgeStep;  //convert step timer to nudge value

bool recHeld, noRecAction, stepRec;
volatile int16_t targetStep;
int16_t nudgeLast;
int16_t stepTickLast;
int16_t maxStep;
int16_t fNumberRoot[15];
volatile int16_t fNumberBend[15];
uint8_t pitchFallCounter;
uint8_t bendAlternator;
uint8_t punchOffset = 12;
bool rec;
bool rec2;
uint8_t showing;
volatile uint8_t tempoFlick;
bool tempoAlt;
volatile int16_t flashTimer;
const uint8_t fallRateFast = 1;
const uint8_t fallRateSlow = 5;

uint8_t pitchFallRate = fallRateFast;
volatile bool flasher;
volatile int16_t tapCounter;
int16_t tempo = 100;
volatile int16_t stepTicks = 40;  //number of ticks in a step
volatile int16_t stepTickCounter;
volatile int16_t analogQuarter;
volatile int16_t analogHalf;
volatile int16_t analogThreeQuarters;

const uint8_t flickerDuration = 8;
volatile int16_t analogCounter = analogMax;
int16_t pitch = 600;
int16_t hatvol;
int16_t counter;
uint8_t bendy = 1;
int16_t shape;
bool sound = true;
int16_t subCounter;
uint8_t hatDecay = 4;
uint8_t hatDecayOpen = 4;


uint8_t mode = 0;
int16_t showTimer;  //briefly show a number
int16_t volBase[4];
volatile bool run;
bool newVol;
uint8_t lastNotePlayed[4];
uint8_t lastOctPlayed[4];

const uint8_t opMin = 3;
const uint8_t opMax = 9;


//TRIANGLE
const uint8_t tremRates[]{ 64, 32, 24, 16, 12, 8, 4, 3, 1, 0 };



uint8_t tremWav;
uint8_t vibWav;
uint8_t tremIndex;
int16_t tremRate[16];
bool newTrem;
int16_t tremolo;
int16_t vibratoRate[16];
int16_t vIndex;
bool newTremRate;
bool newVibRate;
bool newV;
bool earlyProto;  //amps wired differently on early prototypes
uint8_t tremRateUR;
uint8_t vibratoRateUR;

// SYNTH parameters
float vibratoDepth[8];
float tremDepth[8];
float tune[4];  //multiplier must range from .5 to 2
float tuneBase[4];
float freqNext[12];
float freqLast[12];
int16_t midiPitchBendValue[4] = { 0, 0, 0, 0 };
float midiPitchBendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };


int16_t fileCount=0;

uint8_t currentNote[6];
bool kitArmed;
bool pattArmed;
int16_t vibrato;
uint8_t volAlternator;
int16_t volLast[22];
int16_t volNext[22];
uint8_t stutterDefault = 5;
OPL2 opl2;
int16_t i = 0;
volatile int16_t getReady;  //trigger step when zero;
int16_t swing = 0;
int16_t swingSetting[16];
uint8_t swingSettingUR;
bool voice3Alternator;
bool voice4Alternator;

int16_t lastWiggle[152];

bool detachedAFader;

uint32_t funcHeldTime;
volatile int16_t flicker;  //flicker the step
int16_t pitchRoot[10];
int16_t pitchFall[10];
int16_t pitchNow[10];
int16_t pitchNowLast[10];
uint8_t lastTop;
bool mute[10];
bool firstHeld;
int16_t randomDepth[16];
bool metro;
volatile bool metroStep;
int16_t function;
int16_t faderLast[64];
uint8_t moved;
uint8_t phonesVol;
uint8_t mixVol;
bool buttLast[32];
bool runButtonLast;
volatile uint8_t runBounce;
uint8_t solo;
volatile bool wiggleHeld;
uint8_t routeTarget;  //selected channel to route
uint8_t routing[7];   // XXXX4321
bool keyChangeEdit;

uint8_t routingPreset[10][7];  // XXXX4321

bool routeMode;
int16_t bendSubCounter;
volatile byte stutter;  //1=manual 2=automation
volatile int16_t stutterRate;
volatile int16_t stutterCounter;
volatile bool newStep;
bool dontChangePage;
uint8_t randyWrite;
uint8_t randyRead;
int16_t randy[16];
int16_t freezeTimer;
uint8_t song = 0;
const int16_t visualiserTimeoutMS = 2000;  //how long since last button press before we show visualizer led
const uint8_t visualiserDuration = 15;     //how long to show visualizer led

uint8_t keyRoot[16];  //key of the pattern unless keyChanges are recorded
uint8_t keyChange[16][32];
uint8_t keyChangeUR[32];
uint8_t musicModes[16];
uint8_t key;  //current key
uint8_t timer = 255;
