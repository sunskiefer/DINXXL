uint8_t getKeyChange(uint8_t step) {
  step = constrain(step, 0, 63);
  if (step < 32) {
    //first half of the byte
    return (keyChange[currentPattern][step] >> 4);
  } else {
    //second half of the byte
    return (keyChange[currentPattern][step - 32] & 0x0F);
  }
}


void setKeyChange(uint8_t step, uint8_t key) {
  step = constrain(step, 0, 63);
  saveAuto = true;
  if (step < 32) {
    bitWrite(keyChange[currentPattern][step], 4, bitRead(key, 0));
    bitWrite(keyChange[currentPattern][step], 5, bitRead(key, 1));
    bitWrite(keyChange[currentPattern][step], 6, bitRead(key, 2));
    bitWrite(keyChange[currentPattern][step], 7, bitRead(key, 3));
  } else {
    bitWrite(keyChange[currentPattern][step - 32], 0, bitRead(key, 0));
    bitWrite(keyChange[currentPattern][step - 32], 1, bitRead(key, 1));
    bitWrite(keyChange[currentPattern][step - 32], 2, bitRead(key, 2));
    bitWrite(keyChange[currentPattern][step - 32], 3, bitRead(key, 3));
  }
}