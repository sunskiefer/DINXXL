#include "OPL2.h"


const byte oplDelay=10;


void write2(uint8_t data)
{
digitalWrite(28,bitRead(data,0));//DATA0
digitalWrite(29,bitRead(data,1));//DATA1
digitalWrite(30,bitRead(data,2));//DATA2
digitalWrite(31,bitRead(data,3));//DATA3
digitalWrite(33,bitRead(data,4));//DATA4
digitalWrite(3,bitRead(data,5));//DATA5
digitalWrite(4,bitRead(data,6));//DATA6
digitalWrite(5,bitRead(data,7));//DATA7

}



/**
 * Instantiate the OPL2 library with default pin setup.
 */
OPL2::OPL2() {


}


/**
 * Instantiate the OPL2 library with custom pin setup.
 */
OPL2::OPL2(byte reset, byte address, byte latch) {

}



void OPL2::init() {

for (int i=0;i<255;i++){
  lastSentData[i]=-1;
}

    cs=32;//default value for blastbeats


	reset();

}

void OPL2::setCS(byte data) {//change cs pin if multiple chips are needed

pinMode(data,OUTPUT);
    cs=data;

}


/**
 * Hard reset the OPL2 chip. This should be done before sending any register data to the chip.
 */
void OPL2::reset() {

    digitalWrite(12, LOW);
    delay(100);
    digitalWrite(12, HIGH);

}


/**
 * Send the given byte of data to the given register of the OPL2 chip.
 */

 void OPL2::setCrazy(boolean data) {

   crayCray=data;
 }

void OPL2::freeze(boolean data) {

  frz=data;
}

int highestReg;




void OPL2::write(uint8_t reg, uint8_t data) {

//don't want to resend the same data twice..
// is this safe?!?!?!?!?seems to be OK
if(lastSentData[reg]!=data){
  lastSentData[reg]=data;


if((!frz)||((frz)&&(crayCray))){

if(crayCray){
data+=random(50);
data-=random(50);
}
digitalWrite(25, LOW);//A1 LOW
digitalWrite(24, LOW);//A0 LOW

    write2(reg);
    digitalWrite(2, LOW);//WR LOW


    digitalWrite(cs, LOW);//CS LOW (OPEN_DRAIN : HIGH)

    delayMicroseconds(oplDelay);//was 33

    digitalWrite(2, HIGH);//WR HIGH
    digitalWrite(cs, HIGH);//CS HIGH (OPEN_DRAIN : LOW)

    digitalWrite(24, HIGH);//A0 HIGH
    write2(data);

    digitalWrite(2, LOW);//WR LOW
    digitalWrite(cs, LOW);//CS LOW (OPEN_DRAIN : HIGH)


    delayMicroseconds(oplDelay);// was33

    digitalWrite(2, HIGH);//WR HIGH
    digitalWrite(cs, HIGH);//CS HIGH (OPEN_DRAIN : LOW)


}}}

void OPL2::writeHigh(uint8_t reg, uint8_t data) {

if(!frz){

digitalWrite(25, HIGH); //A1

    digitalWrite(24, LOW);                    //A0 LOW
    write2(reg);
    digitalWrite(2, LOW);                        //WR LOW



    digitalWrite(cs, LOW);                         //CS LOW (OPEN_DRAIN : HIGH)

    delayMicroseconds(oplDelay);// was33

    digitalWrite(2, HIGH);                          //WR HIGH
    digitalWrite(cs, HIGH);                       //CS HIGH (OPEN_DRAIN : LOW)

   digitalWrite(24, HIGH);                         //A0 HIGH
    write2(data);

    digitalWrite(2, LOW);                         //WR LOW


    digitalWrite(cs, LOW);                         //CS LOW (OPEN_DRAIN : HIGH)


    delayMicroseconds(oplDelay);// was33

   digitalWrite(2, HIGH);                            //WR HIGH


    digitalWrite(cs, HIGH);                       //CS HIGH (OPEN_DRAIN : LOW)
}}


/**
 * Get the current value of the given register.
 */
byte OPL2::getRegister(byte reg) {
	return oplRegisters[reg];
}

byte OPL2::getRegister2(byte reg) {
	return oplRegisters2[reg];
}


/**
 * Sets the given register to the given value.
 */
byte OPL2::setRegister(byte reg, byte value) {
  oplRegisters[reg] = value;
  write(reg, value);
	return reg;
}

byte OPL2::setRegister2(byte reg, byte value) {
  oplRegisters2[reg] = value;
  writeHigh(reg, value);
	return reg;
}


/**
 * Calculate register offet based on channel and operator.
 */
byte OPL2::getRegisterOffset(byte channel, byte operatorNum) {
	channel = max(ZERO, min(channel, CHANNEL_MAX));
	operatorNum = max(ZERO, min(operatorNum, ONE));
	return registerOffsets[operatorNum][channel];
}


/**
 * Get the F-number for the given frequency for a given channel. When the F-number is calculated the current frequency
 * block of the channel is taken into account.
 */
short OPL2::getFrequencyFNumber(byte channel, float frequency) {
	float fInterval = getFrequencyStep(channel);
	return max(F_NUM_MIN, min((short)(frequency / fInterval), F_NUM_MAX));
}


/**
 * Get the F-Number for the given note. In this case the block is assumed to be the octave.
 */
short OPL2::getNoteFNumber(byte note) {
	return noteFNumbers[max(ZERO, min(note, NOTE_MAX))];
}

/**
 * Get the frequency step per F-number for the current block on the given channel.
 */
float OPL2::getFrequencyStep(byte channel) {
	return fIntervals[getBlock(channel)];
}


/**
 * Get the optimal frequency block for the given frequency.
 */
byte OPL2::getFrequencyBlock(float frequency) {
	for (byte i = 0; i < 8; i ++) {
		if (frequency < blockFrequencies[i]) {
			return i;
		}
	}
	return 7;
}
/**
 * Enable wave form selection for each operator.
 */
byte OPL2::setWaveFormSelect(bool enable) {
	if (enable) {
		return setRegister(0x01, oplRegisters[0x01] | 0x20);
	} else {
		return setRegister(0x01, oplRegisters[0x01] & 0xDF);
	}
}
/**
 * When set to true the sustain level of the voice is maintained until released. When false the sound begins to decay
 * immediately after hitting the sustain phase.
 */
byte OPL2::setMaintainSustain(byte channel, byte operatorNum, bool enable) {
  byte reg;

if(channel<9){
   reg = 0x20 + getRegisterOffset(channel, operatorNum);
	if (enable) {
		return setRegister(reg, oplRegisters[reg] | 0x20);
	} else {
		return setRegister(reg, oplRegisters[reg] & 0xDF);
	}}else{
    reg = 0x20 + getRegisterOffset(channel-9, operatorNum);
   if (enable) {
     return setRegister2(reg, oplRegisters2[reg] | 0x20);
   } else {
     return setRegister2(reg, oplRegisters2[reg] & 0xDF);
   }
  }
}




/**
 * Set frequency multiplier for the given channel. Note that a multiplier of 0 will apply a 0.5 multiplication.
 */
byte OPL2::setMultiplier(byte channel, byte operatorNum, byte multiplier) {
multiplier=constrain(multiplier,0,15);byte reg;

if(channel<9){
	 reg = 0x20 + getRegisterOffset(channel, operatorNum);
	return setRegister(reg, (oplRegisters[reg] & 0xF0) | (multiplier & 0x0F));
}else{reg = 0x20 + getRegisterOffset(channel-9, operatorNum);
return setRegister2(reg, (oplRegisters2[reg] & 0xF0) | (multiplier & 0x0F));}
}
/**
 * Set the volume of the channel.
 * Note that the scale is inverted! 0x00 for loudest, 0x3F for softest.
 */
byte OPL2::setVolume(byte channel, byte operatorNum, int volume) {
	volume=constrain(volume,0,63);byte reg;
	if(channel<9){
    reg = 0x40 + getRegisterOffset(channel, operatorNum);
	return setRegister(reg, (oplRegisters[reg] & 0xC0) | (volume & 0x3F));
}else{reg = 0x40 + getRegisterOffset(channel-9, operatorNum);
return setRegister2(reg, (oplRegisters2[reg] & 0xC0) | (volume & 0x3F));}
}
/**
 * Attack rate. 0x00 is slowest, 0x0F is fastest.
 */
byte OPL2::setAttack(byte channel, byte operatorNum, int attack) {
attack=constrain(attack,0,15);byte reg;
	if(channel<9){reg = 0x60 + getRegisterOffset(channel, operatorNum);
	return setRegister(reg, (oplRegisters[reg] & 0x0F) | ((attack & 0x0F) << 4));
}else{reg = 0x60 + getRegisterOffset(channel-9, operatorNum);
return setRegister2(reg, (oplRegisters2[reg] & 0x0F) | ((attack & 0x0F) << 4));}
}
/**
 * Decay rate. 0x00 is slowest, 0x0F is fastest.
 */
byte OPL2::setDecay(byte channel, byte operatorNum, int decay) {
decay=constrain(decay,0,15);byte reg;
	if(channel<9){ reg = 0x60 + getRegisterOffset(channel, operatorNum);
	return setRegister(reg, (oplRegisters[reg] & 0xF0) | (decay & 0x0F));}
  else{reg = 0x60 + getRegisterOffset(channel-9, operatorNum);
	return setRegister2(reg, (oplRegisters2[reg] & 0xF0) | (decay & 0x0F));}
}
/**
 * Sustain level. 0x00 is loudest, 0x0F is softest.
 */
byte OPL2::setSustain(byte channel, byte operatorNum, int sustain) {
sustain=constrain(sustain,0,15);byte reg;
if(channel<9){
 reg = 0x80 + getRegisterOffset(channel, operatorNum);
	return setRegister(reg, (oplRegisters[reg] & 0x0F) | ((sustain & 0x0F) << 4));
}else{reg = 0x80 + getRegisterOffset(channel-9, operatorNum);
 return setRegister2(reg, (oplRegisters2[reg] & 0x0F) | ((sustain & 0x0F) << 4));}
}
/**
 * Release rate. 0x00 is slowest, 0x0F is fastest.
 */
byte OPL2::setRelease(byte channel, byte operatorNum, int release) {
release=constrain(release,0,15);byte reg;

if(channel<9){
	 reg = 0x80 + getRegisterOffset(channel, operatorNum);
	return setRegister(reg, (oplRegisters[reg] & 0xF0) | (release & 0x0F));
}else{
  reg = 0x80 + getRegisterOffset(channel-9, operatorNum);
  return setRegister2(reg, (oplRegisters2[reg] & 0xF0) | (release & 0x0F));


}
}
/**
 * Get the frequenct F-number of the given channel.
 */
short OPL2::getFNumber(byte channel) {
byte offset;

  if(channel<9){
	offset = max(ZERO, min(channel, CHANNEL_MAX));
	return ((oplRegisters[0xB0 + offset] & 0x03) << 8) + oplRegisters[0xA0 + offset];
}else{
  offset = max(ZERO, min(channel-9, CHANNEL_MAX));
	return ((oplRegisters2[0xB0 + offset] & 0x03) << 8) + oplRegisters2[0xA0 + offset];
}
}
/**
 * Set frequency F-number [0, 1023] for the given channel.
 */
byte OPL2::setFNumber(byte channel, short fNumber) {
fNumber=constrain(fNumber,0,1023);

byte reg;

if(channel<9){
	reg = 0xA0 + max(ZERO, min(channel, CHANNEL_MAX));
	setRegister(reg, fNumber & 0x00FF);
	setRegister(reg + 0x10, (oplRegisters[reg + 0x10] & 0xFC) | ((fNumber & 0x0300) >> 8));
	return reg;
}else{
  reg = 0xA0 + max(ZERO, min(channel-9, CHANNEL_MAX));
	setRegister2(reg, fNumber & 0x00FF);
	setRegister2(reg + 0x10, (oplRegisters2[reg + 0x10] & 0xFC) | ((fNumber & 0x0300) >> 8));
	return reg;
}
}
/**
 * Get the frequency for the given channel.
 */
float OPL2::getFrequency(byte channel) {
	float fInterval = getFrequencyStep(channel);
	return getFNumber(channel) * fInterval;
}
/**
 * Set the frequenct of the given channel and if needed switch to a different block.
 */
byte OPL2::setFrequency(byte channel, float frequency) {

	unsigned char block = getFrequencyBlock(frequency);
	if (getBlock(channel) != block) {
		setBlock(channel, block);
	}
	short fNumber = getFrequencyFNumber(channel, frequency);
	return setFNumber(channel, fNumber);
}
/**
 * Get the frequency block of the given channel.
 */
byte OPL2::getBlock(byte channel) {
  byte offset;
  if(channel<9){
 offset = max(ZERO, min(channel, CHANNEL_MAX));
  	return (oplRegisters[0xB0 + offset] & 0x1C) >> 2;
  }else{
 offset = max(ZERO, min(channel-9, CHANNEL_MAX));
	return (oplRegisters2[0xB0 + offset] & 0x1C) >> 2;
}

}


/**
 * Set frequency block. 0x00 is lowest, 0x07 is highest. This determines the frequency interval between notes.
 * 0 - 0.048 Hz, Range: 0.047 Hz ->   48.503 Hz
 * 1 - 0.095 Hz, Range: 0.094 Hz ->   97.006 Hz
 * 2 - 0.190 Hz, Range: 0.189 Hz ->  194.013 Hz
 * 3 - 0.379 Hz, Range: 0.379 Hz ->  388.026 Hz
 * 4 - 0.759 Hz, Range: 0.758 Hz ->  776.053 Hz
 * 5 - 1.517 Hz, Range: 1.517 Hz -> 1552.107 Hz
 * 6 - 3.034 Hz, Range: 3.034 Hz -> 3104.215 Hz
 * 7 - 6.069 Hz, Range: 6.068 Hz -> 6208.431 Hz
 */
byte OPL2::setBlock(byte channel, byte block) {

  byte reg;

  if(channel<9){
	 reg = 0xB0 + max(ZERO, min(channel, CHANNEL_MAX));
	return setRegister(reg, (oplRegisters[reg] & 0xE3) | ((block & 0x07) << 2));
}else{
  reg = 0xB0 + max(ZERO, min(channel-9, CHANNEL_MAX));
 return setRegister2(reg, (oplRegisters2[reg] & 0xE3) | ((block & 0x07) << 2));


}
}


/**
 * Enable voice on channel.
 */
byte OPL2::setKeyOn(byte channel, bool keyOn) {

  byte reg;
if(channel<9){
 reg = 0xB0 + max(ZERO, min(channel, CHANNEL_MAX));
	if (keyOn) {
		return setRegister(reg, oplRegisters[reg] | 0x20);
	} else {
		return setRegister(reg, oplRegisters[reg] & 0xDF);
	}
}else{
  reg = 0xB0 + max(ZERO, min(channel-9, CHANNEL_MAX));
 	if (keyOn) {
 		return setRegister2(reg, oplRegisters2[reg] | 0x20);
 	} else {
 		return setRegister2(reg, oplRegisters2[reg] & 0xDF);
 	}



  }
}


/**
 * Set feedback strength. 0x00 is no feedback, 0x07 is strongest.
 */
byte OPL2::setFeedback(byte channel, int feedback) {
feedback=constrain(feedback,0,7);

byte reg;

if(channel<9){
  reg = 0xC0 + max(ZERO, min(channel, CHANNEL_MAX));
	bitWrite(oplRegisters[reg],1,bitRead(feedback,0));
	bitWrite(oplRegisters[reg],2,bitRead(feedback,1));
	bitWrite(oplRegisters[reg],3,bitRead(feedback,2));
	return setRegister(reg, oplRegisters[reg]);
}else{
  reg = 0xC0 + max(ZERO, min(channel-9, CHANNEL_MAX));
  bitWrite(oplRegisters2[reg],1,bitRead(feedback,0));
  bitWrite(oplRegisters2[reg],2,bitRead(feedback,1));
  bitWrite(oplRegisters2[reg],3,bitRead(feedback,2));
  return setRegister2(reg, oplRegisters2[reg]);
}
}


byte OPL2::setOutput(byte channel, byte data) {
  byte reg;

  if(channel<9){
  reg = 0xC0 + max(ZERO, min(channel, CHANNEL_MAX));
  bitWrite(oplRegisters[reg],4,bitRead(data,4));
	bitWrite(oplRegisters[reg],5,bitRead(data,5));
	bitWrite(oplRegisters[reg],6,bitRead(data,6));
  bitWrite(oplRegisters[reg],7,bitRead(data,7));
	return setRegister(reg, oplRegisters[reg]);
}else{

  reg = 0xC0 + max(ZERO, min(channel-9, CHANNEL_MAX));
  bitWrite(oplRegisters2[reg],4,bitRead(data,4));
  bitWrite(oplRegisters2[reg],5,bitRead(data,5));
  bitWrite(oplRegisters2[reg],6,bitRead(data,6));
  bitWrite(oplRegisters2[reg],7,bitRead(data,7));
  return setRegister2(reg, oplRegisters2[reg]);

}

}

void OPL2::set4Op(byte data) {
//set register 0x04 on bank two to 0x3F

writeHigh(0x04, data);
}

byte OPL2::setCnt(byte channel, byte data) {
byte reg;
if(channel<9){
	 reg = 0xC0 + max(ZERO, min(channel, CHANNEL_MAX));
  bitWrite(oplRegisters[reg],0,data);
	return setRegister(reg, oplRegisters[reg]);
}else{
  reg = 0xC0 + max(ZERO, min(channel-9, CHANNEL_MAX));
  bitWrite(oplRegisters2[reg],0,data);
  return setRegister2(reg, oplRegisters2[reg]);

}
}



/**
 * Set decay algorithm. When false operator 1 modulates operator 2 (operator 2 is the only one to produce sounde). If
 * set to true both operator 1 and operator 2 will produce sound.
 */
byte OPL2::setSynthMode(byte channel, bool isAdditive) {
	byte reg;



if(channel<9){

  reg= 0xC0 + max(ZERO, min(channel, CHANNEL_MAX));
	if (isAdditive) {
		return setRegister(reg, oplRegisters[reg] | 0x01);
	} else {
		return setRegister(reg, oplRegisters[reg] & 0xFE);
	}
}else{

  reg= 0xC0 + max(ZERO, min(channel-9, CHANNEL_MAX));
	if (isAdditive) {
		return setRegister2(reg, oplRegisters2[reg] | 0x01);
	} else {
		return setRegister2(reg, oplRegisters2[reg] & 0xFE);
	}}
}

/**
 * Enable or disable percussion mode. When set to false there are 9 melodic voices, when true there are 6 melodic
 * voices and channels 6 through 8 are used for drum sounds. KeyOn for these channels must be off.
 */
byte OPL2::setPercussion(bool enable) {
	if (enable) {
		return setRegister(0xBD, oplRegisters[0xBD] | 0x20);
	} else {
		return setRegister(0xBD, oplRegisters[0xBD] & 0xDF);
	}
}


/**
 * Enable or disable various drum sounds.
 * Note that keyOn for channels 6, 7 and 8 must be false in order to use rhythms.
 */
byte OPL2::setDrums(bool bass, bool snare, bool tom, bool cymbal, bool hihat) {
	byte drums = 0;
	drums += bass   ? DRUM_BASS   : 0x00;
	drums += snare  ? DRUM_SNARE  : 0x00;
	drums += tom    ? DRUM_TOM    : 0x00;
	drums += cymbal ? DRUM_CYMBAL : 0x00;
	drums += hihat  ? DRUM_HI_HAT : 0x00;
	setRegister(0xBD, oplRegisters[0xBD] & ~drums);
	return setRegister(0xBD, oplRegisters[0xBD] | drums);
}



/**
 * Select the wave form to use.
 */
byte OPL2::setWaveForm(byte channel, byte operatorNum, int waveForm) {
byte reg;

if(channel<9){
	reg = 0xE0 + getRegisterOffset(channel, operatorNum);
	return setRegister(reg, waveForm );
}else{
  reg = 0xE0 + getRegisterOffset(channel-9, operatorNum);
  return setRegister2(reg, waveForm );

}
}
