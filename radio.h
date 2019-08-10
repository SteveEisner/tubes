#ifndef RADIO_H
#define RADIO_H

#include <SPI.h>
#include <NRFLite.h>
#include "timer.h"

#define USERADIO
#define RADIO_VERSION 6

#ifdef USERADIO
NRFLite _radio(Serial);
#endif

const static uint8_t RADIO_TX_ID = 0;               // Radio ID for both trans
const static uint8_t RADIO_RX_ID = 0;               // Radio ID for both recv

const static uint8_t PIN_RADIO_CE = 9;              // hardware pins
const static uint8_t PIN_RADIO_CSN = 10;            // hardware pins
const static uint8_t PIN_RADIO_MOSI = 11;           // hardware pins
const static uint8_t PIN_RADIO_MISO = 12;           // hardware pins
const static uint8_t PIN_RADIO_SCK = 13;            // hardware pins
bool radioAlive = false;                            // true if radio booted up

#define RADIO_SENDPERIOD 1000                       // how often we broadcast, in millisec

Timer radioSendTimer;
Timer radioMuteTimer;

class Radio;

typedef struct {
  CommandId command;
  TubeId tubeId;
  byte data[26];
  uint16_t crc = 0;
} RadioMessage;

uint16_t calculate_crc( byte *data, byte len ) {

  const unsigned long crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
  };

  unsigned long crc = ~0L;

  for ( unsigned int index = 0 ; index < len  ; ++index ) {
    crc = crc_table[( crc ^ data[index] ) & 0x0f] ^ (crc >> 4);
    crc = crc_table[( crc ^ ( data[index] >> 4 )) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
  }
  return crc & 65535;
}

uint8_t newTubeId() {
#ifdef MASTERCONTROL
  return random(250, 255);
#else
  return random(10, 250);
#endif
}

void printMessageData(RadioMessage &message, int size) {
  Serial.print(sizeof(message.data));
  Serial.print(F(":"));
  for (unsigned int i = 0; i < sizeof(message.data); i++) {
    if (message.data[i] < 16)
      Serial.print(F("0"));
    Serial.print(message.data[i], HEX);
    Serial.print(F(" "));
  }
  Serial.print(F("["));
  Serial.print(size);
  Serial.print(F("] "));
}

bool sendRadioMessage(uint32_t command, void *data=0, uint8_t size=0)
{
  bool sent = 0;
#ifdef USERADIO
  RadioMessage message;
  if (size > sizeof(message.data)) {
    Serial.println(F("Too big to send"));
    return 0;
  }

  message.tubeId = tubeId;
  message.command = command + (RADIO_VERSION << 12);
  memset(message.data, 0, sizeof(message.data));
  memcpy(message.data, data, size);
  uint16_t crc = calculate_crc(message.data, sizeof(message.data));
  message.crc = crc;

  Serial.print(F("Sending "));
  Serial.print(message.command, HEX);
  Serial.print(F(" "));
  Serial.print(message.tubeId);
  // Serial.print(" ");
  // printMessageData(message, size);

  sent = _radio.send(RADIO_TX_ID, &message, sizeof(message), NRFLite::NO_ACK);
  Serial.println(sent ? F(" ok") : F(" failed"));
#endif
  return sent;
}

Timer mTimer;

void receiveRadioMessage(PatternController *controller)
{
  if (mTimer.ended()) {
    Serial.println(F("I have no master"));
    masterTubeId = 0;
    mTimer.snooze(100000);
  }

#ifdef USERADIO
  RadioMessage message;

  if (!radioAlive)
  {
    Serial.println(F("No radio"));
    return;
  }
  
  // check for incoming data
  while (_radio.hasData())
  {
    _radio.readData(&message);

    if ((message.command>>12) != RADIO_VERSION)
      return;

    unsigned long crc = calculate_crc(message.data, sizeof(message.data));
    if (crc != message.crc) {
      // Corrupt packet... ignore it.
      Serial.print(F("Invalid CRC: "));
      Serial.print(message.crc);
      Serial.print(F(" should be "));
      Serial.println(crc);
      continue;
    }

    if (message.tubeId > tubeId && message.tubeId > masterTubeId) {
      // Found a new master!
      masterTubeId = message.tubeId;
      Serial.print(F("All hail new master "));
      Serial.println(masterTubeId);
    }  

    if (message.tubeId == tubeId) {
      // fix the ID collision by choosing a new random one
      Serial.println(F("ID collision!"));
      tubeId = newTubeId();
    } else if (message.tubeId > tubeId) {
      // We know someone is higher ID than us, so stop and listen for a bit
      radioMuteTimer.start(RADIO_SENDPERIOD * 5);
    }
    
    if (message.tubeId == masterTubeId) {
      // Track the last time we received a message from our master
      mTimer.start(RADIO_SENDPERIOD * 8);
    }

    // Process the command
    controller->onCommandReceived(message.tubeId, message.command & 0xFFF, message.data);
  }
#endif
}

// initialize the radio code
void setupRadio()
{
  tubeId = newTubeId();

#ifdef USERADIO
  SPI.setSCK(PIN_RADIO_SCK);
  SPI.setMOSI(PIN_RADIO_MOSI);
  SPI.setMISO(PIN_RADIO_MISO);
  SPI.begin();
  
  if (_radio.init(RADIO_RX_ID, PIN_RADIO_CE, PIN_RADIO_CSN))
    radioAlive = true;
  Serial.println(radioAlive ? F("Radio: ok") : F("Radio: fail"));

  // Start the radio, but mute & listen for a bit
  radioSendTimer.start(RADIO_SENDPERIOD);
  radioMuteTimer.start(RADIO_SENDPERIOD * 3);

  sendRadioMessage(COMMAND_HELLO);
#endif
}

class Radio {
  public:
    PatternController *controller;

    unsigned long radioFailures = 0;
    unsigned long radioRestarts = 0;

  Radio (PatternController *controller) {
    this->controller = controller;
  }

  void setup() {
    setupRadio();
  }

  void update() {
    this->sendRadioUpdate();
    receiveRadioMessage(this->controller);
  }

  void sendRadioUpdate()
  {
    if (!radioAlive)
      return;
  
    // check mute timer
    if (!radioMuteTimer.ended())
      return;
  
    // run periodic timer
    if (!radioSendTimer.ended())
      return;
  
    Serial.print(F("Update "));
    printState(&currentState);
    Serial.print(F(" "));
    
    if (sendRadioMessage(COMMAND_UPDATE, &currentState, sizeof(currentState)))
    {
      this->radioFailures = 0;
      if (currentState.timer < RADIO_SENDPERIOD) {
        radioSendTimer.start(RADIO_SENDPERIOD / 4);
      } else {
        radioSendTimer.start(RADIO_SENDPERIOD);
      }
    }
    else
    {
      // might have been a collision.  Back off by a small amount determined by ID
      Serial.println(F("Radio update failed"));
      radioSendTimer.snooze( (tubeId & 0x7F) * 1000 );
      this->radioFailures++;
      if (radioFailures > 100) {
        setupRadio();
        this->radioRestarts++;
      }
    }
  }

};

#endif
