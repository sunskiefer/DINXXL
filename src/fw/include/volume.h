

//SCHEMATIC
//http://i.cubeupload.com/RRgtnT.png

// --- Audio Amplifier I2C definitions ---
#define AMP_ADDRESS 0x60  // TPA6130A2 device address
// Register 1: HP_EN_L, HP_EN_R, Mode[1], Mode[0], Reserved, Reserved, Reserved, Thermal, SWS
#define AMP_REG1 0x1  // TPA6130A2 register 1 address

#define dualMono B10010000  // default configuration: 11000000 - both channels enabled
#define bridged B11100000   // default configuration: 11000000 - both channels enabled
#define stereo B10000000    // default configuration: 11000000 - both channels enabled

// Register 2: Mute_L, Mute_R, Volume[5-0]
#define AMP_REG2 0x2  // TPA6130A2 register 2 address

#define AMP_REG2_SETUP B01111111  // default configuration: 00110100 - both channels on -0.3 dB Gain
//              Bit7      Bit6        Bit5        Bit4        Bit3        Bit2         Bit1         Bit0
//  reg 1    HP_EN_L    HP_EN_R     Mode[1]    Mode[0]    Reserved     Reserved      Thermal         SWS
//  teg 2     Mute_L     Mute_R    Volume[5]   Volume[4]   Volume[3]   Volume[2]     Volume[1]     Volume[0]
void setMixVol() {
  if (!earlyProto) {
#if defined(STM32F1) || defined(STM32F4)  // Code specific to STM32
    Wire.setSDA(PB7);
    Wire.setSCL(PB6);
#endif
    Wire.begin();

    //MIX
    mux(2);
    delay(1);
    if (r3) {
      amp_writeTo(AMP_REG1, dualMono);
    } else {
      amp_writeTo(AMP_REG1, bridged);
    }
    amp_writeTo(AMP_REG2, mixVol * 7);




    mux(muxIndex);
  }
}



void setPhonesVol() {
  if (!earlyProto) {
#if defined(STM32F1) || defined(STM32F4)  // Code specific to STM32
    Wire.setSDA(PB7);
    Wire.setSCL(PB6);
#endif
    Wire.begin();
    //PHONES
    mux(1);
    delay(1);
    amp_writeTo(AMP_REG1, bridged);
    amp_writeTo(AMP_REG2, phonesVol * 7);
    mux(muxIndex);
  }
}



void initAmp() {

#if defined(STM32F1) || defined(STM32F4)  // Code specific to STM32
  Wire.setSDA(PB7);
  Wire.setSCL(PB6);
#endif

  if (earlyProto) {
    Wire.begin();

    //2=mix out
    //0=out1/2
    //1=out3/4

    //MIX
    mux(2);
    delay(1);
    amp_writeTo(AMP_REG1, stereo);
    amp_writeTo(AMP_REG2, AMP_REG2_SETUP);

    //OUT1-2
    mux(0);
    delay(1);
    amp_writeTo(AMP_REG1, stereo);
    amp_writeTo(AMP_REG2, AMP_REG2_SETUP);

    //OUT3-4
    mux(1);
    delay(1);
    amp_writeTo(AMP_REG1, stereo);
    amp_writeTo(AMP_REG2, AMP_REG2_SETUP);
  } else {

    Wire.begin();

    //0=1-4
    //1=phones
    //2=mix

    //OUT 1-4
    mux(0);
    delay(1);
    amp_writeTo(AMP_REG1, bridged);
    amp_writeTo(AMP_REG2, AMP_REG2_SETUP);

    //PHONES
    mux(1);
    delay(1);
    amp_writeTo(AMP_REG1, bridged);
    amp_writeTo(AMP_REG2, phonesVol * 7);

    //MIX
    mux(2);
    delay(1);
    if (r3) {
      amp_writeTo(AMP_REG1, dualMono);
    } else {
      amp_writeTo(AMP_REG1, bridged);
    }
    amp_writeTo(AMP_REG2, mixVol * 7);
  }
}



void amp_writeTo(uint8_t address, uint8_t val) {
  Wire.beginTransmission(AMP_ADDRESS);  //was 0x60
  Wire.write(address);                  // address high byte
  Wire.write(val);                      // address low byte
  Wire.endTransmission();
}
