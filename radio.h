#ifndef RADIO_H
#define RADIO_H

#include <SPI.h>
#include <NRFLite.h>

#define RADIO_VERSION 1

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

#define RADIO_BITRATE NRFLite::BITRATE1MBPS         // { BITRATE2MBPS, BITRATE1MBPS, BITRATE250KBPS }
#define RADIO_CHANNEL 100 + RADIO_VERSION           // Channel hop with each version
#define RADIO_SENDPERIOD 1000                       // how often we broadcast, in millisec

class Radio;

typedef uint16_t CommandId;
typedef uint8_t TubeId;

#define MESSAGE_DATA_MAX_SIZE 25
typedef struct {
  CommandId command;
  TubeId tubeId;
  TubeId relayId;
  byte data[MESSAGE_DATA_MAX_SIZE];
  uint16_t crc = 0;
} RadioMessage;

class MessageReceiver {
  public:

  virtual void onCommand(uint8_t fromId, CommandId command, void *data) {
    // Abstract: subclasses must define
  }
};

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
  return random(10, 250); // Leave room for master
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

class Radio {
  public:
    bool alive = false;                            // true if radio booted up
    bool reported_no_radio = false;
    TubeId tubeId = 0;
    TubeId masterTubeId = 0;

    unsigned long radioFailures = 0;
    unsigned long radioRestarts = 0;

  void setup(bool isMaster) {
    if (isMaster)
      this->resetId(254);
    else
      this->resetId();
  
#ifdef USERADIO
    SPI.setSCK(PIN_RADIO_SCK);
    SPI.setMOSI(PIN_RADIO_MOSI);
    SPI.setMISO(PIN_RADIO_MISO);
    SPI.begin();
    
    this->reported_no_radio = false;
    if (_radio.init(RADIO_RX_ID, PIN_RADIO_CE, PIN_RADIO_CSN, RADIO_BITRATE, RADIO_CHANNEL)) {
      this->alive = true;
    }
    Serial.println(this->alive ? F("Radio: ok") : F("Radio: fail"));
  
    // Start the radio, but mute & listen for a bit
#endif
  }

  void resetId(uint8_t id=0) {
    if (id == 0)
      id = newTubeId();
    this->tubeId = id;
    Serial.print(F("My ID is "));
    Serial.println(this->tubeId);

    if (this->tubeId > this->masterTubeId)
      this->masterTubeId = 0;
  }

  bool sendCommand(uint32_t command, void *data=0, uint8_t size=0, TubeId relayId=0)
  {
    return this->sendCommandFrom(this->tubeId, command, data, size, relayId);
  }

  bool sendCommandFrom(TubeId id, uint32_t command, void *data=0, uint8_t size=0, TubeId relayId=0)
  {
    bool sent = 0;
    if (!this->alive)
      return sent;
  
#ifdef USERADIO
    RadioMessage message;
    if (size > sizeof(message.data)) {
      Serial.println(F("Too big to send"));
      return 0;
    }
  
    message.tubeId = id;
    message.relayId = relayId;
    message.command = command + (RADIO_VERSION << 12);
    memset(message.data, 0, sizeof(message.data));
    memcpy(message.data, data, size);
    uint16_t crc = calculate_crc(message.data, sizeof(message.data));
    message.crc = crc;

    Serial.print(F("["));
    Serial.print(message.tubeId);
    Serial.print(F(": "));
    Serial.print(message.command, HEX);
  
    sent = _radio.send(RADIO_TX_ID, &message, sizeof(message), NRFLite::NO_ACK);
    Serial.print(sent ? F(" ok] ") : F(" failed] "));
#endif

    return sent;
  }

  void receiveCommands(MessageReceiver *receiver)
  {
#ifdef USERADIO
    RadioMessage message;
  
    if (!this->alive && !this->reported_no_radio)
    {
      Serial.println(F("No radio"));
      this->reported_no_radio = true;
      return;
    }
    
    // check for incoming data
    while (_radio.hasData())
    {
      _radio.readData(&message);

      // Messages must be from a tube with the current version
      if ((message.command>>12) != RADIO_VERSION)
        return;

      // Ignore relayed messages if we already have a master
      if (message.relayId && message.relayId <= this->masterTubeId)
        return;

      // Filter out corrupt messages
      unsigned long crc = calculate_crc(message.data, sizeof(message.data));
      if (crc != message.crc) {
        // Corrupt packet... ignore it.
        Serial.print(F("Invalid CRC: "));
        Serial.print(message.crc);
        Serial.print(F(" should be "));
        Serial.println(crc);
        continue;
      }

      // If we detect an ID collision, fix it by choosing a new random one
      while (message.tubeId == this->tubeId) {
        Serial.print(F("ID collision!"));
        this->resetId();
      }

      // Ignore messages from a lower ID
      if (message.tubeId < this->tubeId) {
        // Don't need to be noisy about relayed messages
        if (message.relayId == 0) {
          Serial.print(F("Ignoring message from "));
          Serial.println(message.tubeId);
        }
        return;
      }

      if (message.tubeId != 255 && message.tubeId > this->masterTubeId) {
        // Found a new master!
        this->masterTubeId = message.tubeId;
        Serial.print(F("All hail new master "));
        Serial.println(this->masterTubeId);
      }  

      // Process the command
      receiver->onCommand(message.tubeId, message.command & 0xFFF, message.data);

      // Occcasionally relay commands - more frequently if higher ID
      uint8_t r = random8();
      if ((r % 3 == 0) && r < this->tubeId) {
        Serial.print(F("Relaying from "));
        Serial.println(this->tubeId);
        message.relayId = message.tubeId;
        message.tubeId = this->tubeId;
        _radio.send(RADIO_TX_ID, &message, sizeof(message), NRFLite::NO_ACK);
      }
    }
#endif
  }

};

#endif
