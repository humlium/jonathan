#include <EEPROM.h>

#define ENTER_MODE_TIME (5000)
#define SHOW_MODE_TIME (3000)
#define DELAY_INC_TIME (2000)

#define CHAR_F (10)
#define CHAR_E (11)

#define EEPROM_SIZE     (64)
#define SET_DATA_ADD1   (0)
#define SET_DATA_ADD2   (4)
#define SET_DATA_ADD3   (8)
#define SET_DATA_ADD4   (12)
#define COIN_TOTAL_ADD  (16)
#define COIN_CUR_ADD    (20)

//pin define
const int pinSw1 = 34; //switch 1
const int pinSw2 = 35; //switch 2
const int pinSw3 = 32; //switch 3
const int pinSw4 = 33; //switch 4
const int pinCoin = 5; //pin recieve coin pulse
const int pinOnCoin = 22; // on read coin
const int pinRelay1 = 12;
const int pinRelay2 = 14;
const int pinRelay3 = 27;
const int pinRelay4 = 26;
const int pinFoamSensor = 21;
const int pinPower = 13;
//74595
const int pinLatch = 2;
const int pinClock = 4;
const int pinData = 15;
const int pinDigi1 = 18;
const int pinDigi2 = 19;
const int pinDigi3 = 16;
const int pinDigi4 = 17;
// char 7 segment [ 0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    F,    E]
const int num[] = {0x5f, 0x44, 0x9d, 0xd5, 0xc6, 0xd3, 0xdb, 0x45, 0xdf, 0xd7, 0x8b, 0x9b};

//define debounce switch variable
unsigned long lastDebounceCoin = 0;
const unsigned long debounceDelayCoin = 10;
int lastCoinState = HIGH;
int coinState = HIGH;
const unsigned long debounceDelaySw = 20;
unsigned long lastDebounceSw1 = 0;
int lastStateSw1 = HIGH;
int stateSw1 = HIGH;
unsigned long lastDebounceSw2 = 0;
int lastStateSw2 = HIGH;
int stateSw2 = HIGH;
unsigned long lastDebounceSw3 = 0;
int lastStateSw3 = HIGH;
int stateSw3 = HIGH;
unsigned long lastDebounceSw4 = 0;
int lastStateSw4 = HIGH;
int stateSw4 = HIGH;
unsigned long showDigiTimer = 0;
unsigned long delayIncDataTimer = 0;
enum machineMode {
  rCoin,
  mSw1,
  mSw2,
  mSw3,
  mSw4,
  f1,
  f2,
  f3,
  f4
};
machineMode mMode = rCoin;

//variable
unsigned int coinCounter = 0;
unsigned int coinTotal = 0;
unsigned long coinTimer = 0;
unsigned long runTimer = 0;
unsigned long startTime = 0;
unsigned long pressTimeSw1 = 0;
unsigned long pressTimeSw2 = 0;
unsigned long pressTimeSw3 = 0;
unsigned long pressTimeSw4 = 0;
unsigned long showModeTime = 0;
unsigned long coinInTime = 0;
bool saveCoinData = true;
bool pressSw1 = false;
bool pressSw2 = false;
bool pressSw3 = false;
bool pressSw4 = false;
unsigned int settingData1 = 20;
unsigned int settingData2 = 20;
unsigned int settingData3 = 20;
unsigned int settingData4 = 20;


void setup() {
  Serial.begin(115200);
  delay(500);
  initPin();
  lastCoinState = digitalRead(pinCoin);
  digitalWrite(pinOnCoin, HIGH);
  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("failed to initialise EEPROM"); delay(1000000);
  }
  EEPROM.get(SET_DATA_ADD1, settingData1);
  EEPROM.get(SET_DATA_ADD2, settingData2);
  EEPROM.get(SET_DATA_ADD3, settingData3);
  EEPROM.get(SET_DATA_ADD4, settingData4);
  EEPROM.get(COIN_TOTAL_ADD, coinTotal);
}

void loop() {
  sw1Check();
  sw2Check();
  sw3Check();
  sw4Check();
  coinCheck();
  if((saveCoinData==false)&&((millis()-coinInTime)>500)){
    saveCoinData=true;
    Serial.println(coinTotal);
    EEPROM.put(COIN_TOTAL_ADD, coinTotal);
    EEPROM.commit();
  }
  if ((pressSw1 == true) && (pressSw2 == true)) {
    unsigned long t = millis();
    if (((t - pressTimeSw1) > ENTER_MODE_TIME)
        && ((t - pressTimeSw2) > ENTER_MODE_TIME)
        && ((t - pressTimeSw3) > ENTER_MODE_TIME)) {
      coinTotal = 0;
    }
  }

  //enter setting mode
  if ((pressSw1 == true) && (pressSw4 == true)) {
    unsigned long t = millis();
    if (((t - pressTimeSw1) > ENTER_MODE_TIME) && ((t - pressTimeSw4) > ENTER_MODE_TIME)) {
      if (mMode < f1) {
        mMode = f1;
      } else if (mMode < f2) {
        EEPROM.put(SET_DATA_ADD1, settingData1);
        EEPROM.commit();
        mMode = f2;
        Serial.println("save data1");
      } else if (mMode < f3) {
        EEPROM.put(SET_DATA_ADD2, settingData2);
        EEPROM.commit();
        mMode = f3;
        Serial.println("save data2");
      } else if (mMode < f4) {
        EEPROM.put(SET_DATA_ADD3, settingData3);
        EEPROM.commit();
        mMode = f4;
        Serial.println("save data3");
      } else {
        EEPROM.put(SET_DATA_ADD4, settingData4);
        EEPROM.commit();
        Serial.println("save data4");
        mMode = rCoin;
      }
      showModeTime = t;
      pressTimeSw1 = t;
      pressTimeSw4 = t;
      digitalWrite(pinRelay1, LOW);
      digitalWrite(pinRelay2, LOW);
      digitalWrite(pinRelay3, LOW);
      digitalWrite(pinRelay4, LOW);
    }
  }
  // handle mode
  switch (mMode) {
    case rCoin:
      if (pressSw1 && pressSw2) {
        printCounter(coinTotal);
      } else {
        printCounter(coinCounter);
      }

      break;
    case mSw1:
      if ((millis() - startTime) >= 100) {
        runTimer--;
        startTime = millis();
      }
      if (runTimer <= 0) {
        digitalWrite(pinRelay1, LOW);
        mMode = rCoin;
        coinCounter = 0;
      }
      printCounter(runTimer);
      break;
    case mSw2:
      if ((millis() - startTime) >= 100) {
        runTimer--;
        startTime = millis();
      }
      if (runTimer <= 0) {
        digitalWrite(pinRelay2, LOW);
        mMode = rCoin;
        coinCounter = 0;
      }
      printCounter(runTimer);
      break;
    case mSw3:
      if ((millis() - startTime) >= 100) {
        runTimer--;
        startTime = millis();
      }
      if (runTimer <= 0) {
        digitalWrite(pinRelay3, LOW);
        mMode = rCoin;
        coinCounter = 0;
      }
      printCounter(runTimer);
      break;
    case mSw4:
      if ((millis() - startTime) >= 100) {
        runTimer--;
        startTime = millis();
      }
      if (runTimer <= 0) {
        digitalWrite(pinRelay4, LOW);
        mMode = rCoin;
        coinCounter = 0;
      }
      printCounter(runTimer);
      break;
    case f1:
      if ((millis() - delayIncDataTimer > 100) && pressSw2 == true) {
        settingData1++;
        delayIncDataTimer = millis();
      } else if ((millis() - delayIncDataTimer > 100) && pressSw3 == true) {
        settingData1--;
        delayIncDataTimer = millis();
      }
      showMode(1);
      break;
    case f2:
      if ((millis() - delayIncDataTimer > 100) && pressSw2 == true) {
        settingData2++;
        delayIncDataTimer = millis();
      } else if ((millis() - delayIncDataTimer > 100) && pressSw3 == true) {
        settingData2--;
        delayIncDataTimer = millis();
      }
      showMode(2);
      break;
    case f3:
      if ((millis() - delayIncDataTimer > 200) && pressSw2 == true) {
        settingData3++;
        delayIncDataTimer = millis();
      } else if ((millis() - delayIncDataTimer > 200) && pressSw3 == true) {
        settingData3--;
        delayIncDataTimer = millis();
      }
      showMode(3);
      break;
    case f4:
      if ((millis() - delayIncDataTimer > 100) && pressSw2 == true) {
        settingData4++;
        delayIncDataTimer = millis();
      } else if ((millis() - delayIncDataTimer > 100) && pressSw3 == true) {
        settingData4--;
        delayIncDataTimer = millis();
      }
      showMode(4);
      break;
    default: break;
  }
}

void incSettingValue() {

  switch (mMode) {
    case f1:
      if (settingData1 < 1000) {
        settingData1++;
      }
      break;
    case f2:
      if (settingData2 < 1000) {
        settingData2++;
      }
      break;
    case f3:
      if (settingData3 < 1000) {
        settingData3++;
      }
      break;
    case f4:
      if (settingData4 < 1000) {
        settingData4++;
      }
      break;
    default: break;
  }
}
void decSettingValue() {
  switch (mMode) {
    case f1:
      if (settingData1 > 0) {
        settingData1--;
      }
      break;
    case f2:
      if (settingData2 > 0) {
        settingData2--;
      }
      break;
    case f3:
      if (settingData3 > 0) {
        settingData3--;
      }
      break;
    case f4:
      if (settingData4 > 0) {
        settingData4--;
      }
      break;
    default: break;
  }
}

void showMode(int m) {
  unsigned long t = millis();
  if ((t - showModeTime) < SHOW_MODE_TIME) {
    if (t - showDigiTimer < 10) {    //show digi4 10 ms
      showDigi(num[CHAR_F], pinDigi4);
    } else if (t - showDigiTimer < 20) {//show digi3 10 ms
      showDigi(num[m], pinDigi3);
    } else {
      showDigiTimer = t;
    }
  } else {
    switch (m) {
      case 1:
        printCounter(settingData1);
        break;
      case 2:
        printCounter(settingData2);
        break;
      case 3:
        printCounter(settingData3);
        break;
      case 4:
        printCounter(settingData4);
        break;
      default: break;
    }
  }
}

void sw1Check() {
  int readSw1 = digitalRead(pinSw1);
  if (readSw1 != lastStateSw1) {
    lastDebounceSw1 = millis();
  }
  if ((millis() - lastDebounceSw1) > debounceDelaySw) {
    if (readSw1 != stateSw1) {
      stateSw1 = readSw1;
      if (stateSw1 == LOW) {
        pressTimeSw1 = millis();
        pressSw1 = true;
      }
      if (stateSw1 == HIGH) {
        pressSw1 = false;
        if (mMode == rCoin) {
          digitalWrite(pinRelay1, HIGH);
          startTime = millis();
          runTimer = coinCounter * settingData1;
          mMode = mSw1;
        } else if (mMode < f1) {
          mMode = rCoin;
          digitalWrite(pinRelay1, LOW);
          coinCounter = runTimer / settingData1;
        }
      }
    }
  }
  lastStateSw1 = readSw1;
}

void sw2Check() {
  int readSw2 = digitalRead(pinSw2);
  if (readSw2 != lastStateSw2) {
    lastDebounceSw2 = millis();
  }
  if ((millis() - lastDebounceSw2) > debounceDelaySw) {
    if (readSw2 != stateSw2) {
      stateSw2 = readSw2;
      if (stateSw2 == LOW) {
        pressTimeSw2 = millis();
        delayIncDataTimer = millis();
        pressSw2 = true;
        incSettingValue();
      }
      if (stateSw2 == HIGH) {
        pressSw2 = false;
        if (mMode == rCoin) {
          if (digitalRead(pinFoamSensor) == 1) {
            mMode = mSw2;
            digitalWrite(pinRelay2, HIGH);
            startTime = millis();
            runTimer = coinCounter * settingData2;
          }
        } else if (mMode < f1) {
          mMode = rCoin;
          digitalWrite(pinRelay2, LOW);
          coinCounter = runTimer / settingData2;
        }
      }
    }
  }
  lastStateSw2 = readSw2;
}

void sw3Check() {
  int readSw3 = digitalRead(pinSw3);
  if (readSw3 != lastStateSw3) {
    lastDebounceSw3 = millis();
  }
  if ((millis() - lastDebounceSw3) > debounceDelaySw) {
    if (readSw3 != stateSw3) {
      stateSw3 = readSw3;
      if (stateSw3 == LOW) {
        pressTimeSw3 = millis();
        delayIncDataTimer = millis();
        pressSw3 = true;
        decSettingValue();
      }
      if (stateSw3 == HIGH) {
        pressSw3 = false;
        if (mMode == rCoin) {
          mMode = mSw3;
          digitalWrite(pinRelay3, HIGH);
          startTime = millis();
          runTimer = coinCounter * settingData3;
        } else if (mMode < f1) {
          mMode = rCoin;
          digitalWrite(pinRelay3, LOW);
          coinCounter = runTimer / settingData3;
        }
      }
    }
  }
  lastStateSw3 = readSw3;
}

void sw4Check() {
  int readSw4 = digitalRead(pinSw4);
  if (readSw4 != lastStateSw4) {
    lastDebounceSw4 = millis();
  }
  if ((millis() - lastDebounceSw4) > debounceDelaySw) {
    if (readSw4 != stateSw4) {
      stateSw4 = readSw4;
      if (stateSw4 == LOW) {
        pressTimeSw4 = millis();
        pressSw4 = true;
      }
      if (stateSw4 == HIGH) {
        pressSw4 = false;
        if (mMode == rCoin) {
          mMode = mSw4;
          digitalWrite(pinRelay4, HIGH);
          startTime = millis();
          runTimer = coinCounter * settingData4;
        } else if (mMode < f1) {
          mMode = rCoin;
          digitalWrite(pinRelay4, LOW);
          coinCounter = runTimer / settingData4;
        }
      }
    }
  }
  lastStateSw4 = readSw4;
}

void coinCheck() {
  int readCoin = digitalRead(pinCoin);
  if (readCoin != lastCoinState) {
    lastDebounceCoin = millis();
  }
  if ((millis() - lastDebounceCoin) > debounceDelayCoin) {
    if (readCoin != coinState) {
      coinState = readCoin;
      if (coinState == LOW) {
        coinCounter++;
        coinTotal++;
        coinInTime = millis();
        saveCoinData = false;
      }
    }
  }
  lastCoinState = readCoin;
}

void printCounter(unsigned long counter) {
  if (counter < 10) {
    showDigi(num[counter], pinDigi1);
  } else if (counter < 100) {
    unsigned long t = millis();
    int tmp = counter / 10;
    if (t - showDigiTimer < 10) {    //show digi2 10 ms
      showDigi(num[tmp], pinDigi2);
    } else if (t - showDigiTimer < 20) {//show digi1 10 ms
      tmp = counter - (tmp * 10);
      showDigi(num[tmp], pinDigi1);
    } else {
      showDigiTimer = t;
    }
  } else if (counter < 1000) {
    unsigned long t = millis();
    if (t - showDigiTimer < 5) {    //show digi3 5 ms
      int tmp = counter / 100;
      showDigi(num[tmp], pinDigi3);
    } else if (t - showDigiTimer < 10) {//show digi2 5 ms
      int tmp = counter / 100;
      tmp = counter - (tmp * 100);
      tmp = tmp / 10;
      showDigi(num[tmp], pinDigi2);
    } else if (t - showDigiTimer < 15) {//show digi1 5 ms
      int tmp = counter / 10;
      tmp = counter - (tmp * 10);
      showDigi(num[tmp], pinDigi1);
    } else {
      showDigiTimer = t;
    }
  } else {
    unsigned long t = millis();
    if ((t - showDigiTimer) < 3) {
      int tmp = counter / 1000;
      showDigi(num[tmp], pinDigi4);
    } else if (t - showDigiTimer < 6) {    //show digi3 5 ms
      int tmp = counter / 1000;
      tmp = counter - (tmp * 1000);
      tmp = tmp / 100;
      showDigi(num[tmp], pinDigi3);
    } else if (t - showDigiTimer < 9) {//show digi2 5 ms
      int tmp = counter / 100;
      tmp = counter - (tmp * 100);
      tmp = tmp / 10;
      showDigi(num[tmp], pinDigi2);
    } else if (t - showDigiTimer < 12) {//show digi1 5 ms
      int tmp = counter / 10;
      tmp = counter - (tmp * 10);
      showDigi(num[tmp], pinDigi1);
    } else {
      showDigiTimer = t;
    }
  }
}
void showDigi(byte data, int digi) {
  digitalWrite(digi, HIGH);
  digitalWrite(pinLatch, LOW);
  shiftOut(pinData, pinClock, MSBFIRST, data);
  digitalWrite(pinLatch, HIGH);
  digitalWrite(digi, LOW);
}

void initPin() {
  pinMode(pinLatch, OUTPUT);
  pinMode(pinClock, OUTPUT);
  pinMode(pinData, OUTPUT);
  pinMode(pinDigi1, OUTPUT);
  pinMode(pinDigi2, OUTPUT);
  pinMode(pinDigi3, OUTPUT);
  pinMode(pinDigi4, OUTPUT);
  pinMode(pinOnCoin, OUTPUT);
  pinMode(pinRelay1, OUTPUT);
  pinMode(pinRelay2, OUTPUT);
  pinMode(pinRelay3, OUTPUT);
  pinMode(pinRelay4, OUTPUT);

  pinMode(pinSw1, INPUT);
  pinMode(pinSw2, INPUT);
  pinMode(pinSw3, INPUT);
  pinMode(pinSw4, INPUT);
  pinMode(pinCoin, INPUT);
  pinMode(pinFoamSensor, INPUT);
}
