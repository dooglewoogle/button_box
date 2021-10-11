#include <RotaryEncoder.h>
#include <PCF8574.h>
#include <Wire.h>
#include "Joystick.h"

// Currently using a local copy of the Joystick library. Ideally in the future we will deal with setting and sending multiple buttons better
uint8_t btnCount = 32;
uint8_t hatCount = 2;
Joystick__ Joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD,
                    btnCount, hatCount, false, false, false, false, false, false, false, false, false, false, false);

PCF8574 pcfs[4] = {PCF8574(0x38), PCF8574(0x39), PCF8574(0x3A), PCF8574(0x3B)};

struct EncoderHelper {
  RotaryEncoder encoder;
  int newPos;
  int pos;
  unsigned int offAt;
  bool isOn;
  int hatSwitch;
  int val1;
  int val2;
};

static int numEncoders = 3;
EncoderHelper enc1 = {RotaryEncoder(15, 14, RotaryEncoder::LatchMode::FOUR3), 0, 0, 0, false, 0, 0, 180};
EncoderHelper enc2 = {RotaryEncoder(5, 6, RotaryEncoder::LatchMode::FOUR3), 0, 0, 0, false, 0, 90, 270};
EncoderHelper enc3 = {RotaryEncoder(7, 8, RotaryEncoder::LatchMode::FOUR3), 0, 0, 0, false, 1, 0, 180};
EncoderHelper encoders[3] = {enc1, enc2, enc3};

unsigned int updateMillis;

bool buttonsHaveChanged(){
  for (uint8_t i=0; i<btnCount/8; i++){
    if( Joystick.buttonValues[i] != Joystick.oldButtonValues[i]){
      return true;
    }  
    }
    return false;
  }

void setup() {
  for (int i=0;i<4;i++){
    pcfs[i].begin();
  }
  Joystick.begin(false);
  Serial.begin(115200);
  Serial.println("Ready");
}

void loop() {

  unsigned int cMillis = millis();

//read and update encoder values

  for (int i = 0; i < numEncoders; i++) {

    // clear any on encoder buttons that have timed out
    if (encoders[i].isOn && encoders[i].offAt < cMillis) {
      Joystick.setHatSwitch(encoders[i].hatSwitch, JOYSTICK_HATSWITCH_RELEASE);
      encoders[i].isOn = false;
      Joystick.sendState();
    }

    // read encoders
    encoders[i].encoder.tick();
    encoders[i].newPos = encoders[i].encoder.getPosition();

    if (encoders[i].newPos != encoders[i].pos) {
      int d = (int)(encoders[i].encoder.getDirection());
      if (d == 1) {
        Joystick.setHatSwitch(encoders[i].hatSwitch, encoders[i].val1);
      } else {
         Joystick.setHatSwitch(encoders[i].hatSwitch, encoders[i].val2);
      }
      Joystick.sendState();
      
      encoders[i].offAt = millis() + 50;
      encoders[i].isOn = true;
      encoders[i].pos = encoders[i].newPos;
    }
  }


  // rate limited button/switch read
  if (cMillis - updateMillis >= 50 || true) {
    updateMillis = cMillis;


    // read the buttons/switches
    for (int i=0; i<4; i++) {
      byte input1 = pcfs[i].read8();
      for (int j=0, mask=1; j<8; j++, mask = mask << 1) {

        if (!bitRead(input1, j)) {
          Joystick.pressButton(j+(i*8));

        } else {
          Joystick.releaseButton(j+(i*8));
        }
      }
    }

    //Only send if something has changed
    if (buttonsHaveChanged()){
      memcpy(Joystick.oldButtonValues, Joystick.buttonValues, btnCount/8);
      Joystick.sendState();
    }


    
  }
}
