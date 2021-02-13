/*
  Test.h - Test library for Wiring - implementation
  Copyright (c) 2006 John Doe.  All right reserved.
*/

// include this library's description file
#include "PMS.h"

// include description files for other libraries used (if any)
#include <SoftwareSerial.h>
#include "Arduino.h"
#include <Wire.h>
#include <math.h>

// Constructor /////////////////////////////////////////////////////////////////
// Function that handles the creation and setup of instances

const int STATUS_NO_RESPONSE = -2;
const int STATUS_CHECKSUM_MISMATCH = -3;
const int STATUS_INCOMPLETE = -4;
const int STATUS_NOT_READY = -5;
const int STATUS_PWM_NOT_CONFIGURED = -6;
const int STATUS_serial_MHZ19_NOT_CONFIGURED = -7;

unsigned long lastRequest = 0;

bool SerialConfigured = true;
bool PwmConfigured = true;

PMS::PMS(bool displayMsg, int baudRate)
{
  _debugMsg = displayMsg;
  Wire.begin();
  Serial.begin(baudRate);
  if (_debugMsg)
  {
    Serial.println("AirGradiant Library instantiated successfully.");
  }
}

// Public Methods //////////////////////////////////////////////////////////////
// Functions available in Wiring sketches, this library, and other libraries

void PMS::PMS_Init()
{
  if (_debugMsg)
  {
    Serial.println("Initializing PMS...");
  }
  PMS_Init(D5, D6, D8, D3);
}
void PMS::PMS_Init(int rx_pin, int tx_pin, int set_pin, int reset_pin)
{
  PMS_Init(rx_pin, tx_pin, set_pin, reset_pin, 9600);
}
void PMS::PMS_Init(int rx_pin, int tx_pin, int set_pin, int reset_pin, int baudRate)
{
  _SoftSerial_PMS = new SoftwareSerial(rx_pin, tx_pin);
  start(*_SoftSerial_PMS);
  _SoftSerial_PMS->begin(baudRate);
  _setPin = set_pin;
  _resetPin = reset_pin;
  pinMode(_setPin, OUTPUT);    // sets the digital pin _setPin as output
  pinMode(_resetPin, OUTPUT);    // sets the digital pin _resetPin as output

  if (getPM2() <= 0)
  {

    if (_debugMsg)
    {
      Serial.println("PMS Sensor Failed to Initialize ");
    }
    else
    {
      Serial.println("PMS Successfully Initialized. Heating up for 10s");
      delay(10000);
    }
  }
}

const char *PMS::getPM2()
{
  if (getPM2_Raw())
  {
    int result_raw = getPM2_Raw();
    sprintf(Char_PM2, "%d", result_raw);
    return Char_PM2;
  }
  else
  {
    //Serial.println("no PMS data");
    Char_PM2[0] = 'N';
    Char_PM2[1] = 'U';
    Char_PM2[2] = 'L';
    Char_PM2[3] = 'L';
    return Char_PM2;
  }
}

int PMS::getPM2_Raw()
{
  int pm02;
  DATA data;
  requestRead();
  if (readUntil(data))
  {
    pm02 = data.PM_AE_UG_2_5;
    return pm02;
  }
  else
  {
    return 0;
  }
}

const char *PMS::getPM1()
{
  if (getPM1_Raw())
  {
    int result_raw = getPM1_Raw();
    sprintf(Char_PM1, "%d", result_raw);
    return Char_PM1;
  }
  else
  {
    //Serial.println("no PMS data");
    Char_PM1[0] = 'N';
    Char_PM1[1] = 'U';
    Char_PM1[2] = 'L';
    Char_PM1[3] = 'L';
    return Char_PM1;
  }
}

int PMS::getPM1_Raw()
{
  int pm01;
  DATA data;
  requestRead();
  if (readUntil(data))
  {
    pm01 = data.PM_AE_UG_1_0;
    return pm01;
  }
  else
  {
    return 0;
  }
}

const char *PMS::getPM10()
{
  if (getPM10_Raw())
  {
    int result_raw = getPM10_Raw();
    sprintf(Char_PM10, "%d", result_raw);
    return Char_PM10;
  }
  else
  {
    //Serial.println("no PMS data");
    Char_PM10[0] = 'N';
    Char_PM10[1] = 'U';
    Char_PM10[2] = 'L';
    Char_PM10[3] = 'L';
    return Char_PM10;
  }
}

int PMS::getPM10_Raw()
{
  int pm10;
  DATA data;
  requestRead();
  if (readUntil(data))
  {
    pm10 = data.PM_AE_UG_10_0;
    return pm10;
  }
  else
  {
    return 0;
  }
}

//MQ135 VARIABLES PUBLIC_START
//MQ135 VARIABLES PUBLIC_END

// Private Methods /////////////////////////////////////////////////////////////
// Functions only available to other functions in this library

//START PMS FUNCTIONS //

void PMS::start(Stream &stream)
{
  this->_stream = &stream;
}

// Standby mode. For low power consumption and prolong the life of the sensor.
void PMS::sleep()
{
  // uint8_t command[] = {0x42, 0x4D, 0xE4, 0x00, 0x00, 0x01, 0x73};
  // _stream->write(command, sizeof(command));
  // _stream->flush();
  digitalWrite(_setPin, LOW); // set Active pin high
}

// Operating mode. Stable data should be got at least 30 seconds after the sensor wakeup from the sleep mode because of the fan's performance.
void PMS::wakeUp()
{
  // uint8_t command[] = {0x42, 0x4D, 0xE4, 0x00, 0x01, 0x01, 0x74};
  // _stream->write(command, sizeof(command));
  // _stream->flush();
  digitalWrite(_setPin, HIGH); // set Active pin high
}

// Reset the sensor
void PMS::reset()
{
    digitalWrite(_resetPin, LOW); // set reset pin high
    delay(100);
    digitalWrite(_resetPin, HIGH); // set reset pin low
}


// Active mode. Default mode after power up. In this mode sensor would send serial data to the host automatically.
void PMS::activeMode()
{

  uint8_t command[] = {0x42, 0x4D, 0xE1, 0x00, 0x01, 0x01, 0x71};
  _stream->write(command, sizeof(command));
  _mode = MODE_ACTIVE;
}

// Passive mode. In this mode sensor would send serial data to the host only for request.
void PMS::passiveMode()
{
  uint8_t command[] = {0x42, 0x4D, 0xE1, 0x00, 0x00, 0x01, 0x70};
  _stream->write(command, sizeof(command));
  _mode = MODE_PASSIVE;
}

// Request read in Passive Mode.
void PMS::requestRead()
{
  if (_mode == MODE_PASSIVE)
  {
    uint8_t command[] = {0x42, 0x4D, 0xE2, 0x00, 0x00, 0x01, 0x71};
    _stream->write(command, sizeof(command));
  }
}

// Non-blocking function for parse response.
bool PMS::read_PMS(DATA &data)
{
  _data = &data;
  loop();

  return _PMSstatus == STATUS_OK;
}

// Blocking function for parse response. Default timeout is 1s.
bool PMS::readUntil(DATA &data, uint16_t timeout)
{
  _data = &data;
  uint32_t start = millis();
  do
  {
    loop();
    if (_PMSstatus == STATUS_OK)
      break;
  } while (millis() - start < timeout);

  return _PMSstatus == STATUS_OK;
}

void PMS::clearSerial(){
    // Clear buffer (removes potentially old data) before read. Some data could have been also sent before switching to passive mode.
  while (_stream->available()) { _stream->read(); }
}

void PMS::loop()
{
  _PMSstatus = STATUS_WAITING;
  if (_stream->available())
  {
    uint8_t ch = _stream->read();

    switch (_index)
    {
    case 0:
      if (ch != 0x42)
      {
        return;
      }
      _calculatedChecksum = ch;
      break;

    case 1:
      if (ch != 0x4D)
      {
        _index = 0;
        return;
      }
      _calculatedChecksum += ch;
      break;

    case 2:
      _calculatedChecksum += ch;
      _frameLen = ch << 8;
      break;

    case 3:
      _frameLen |= ch;
      // Unsupported sensor, different frame length, transmission error e.t.c.
      if (_frameLen != 2 * 9 + 2 && _frameLen != 2 * 13 + 2)
      {
        _index = 0;
        return;
      }
      _calculatedChecksum += ch;
      break;

    default:
      if (_index == _frameLen + 2)
      {
        _checksum = ch << 8;
      }
      else if (_index == _frameLen + 2 + 1)
      {
        _checksum |= ch;

        if (_calculatedChecksum == _checksum)
        {
          _PMSstatus = STATUS_OK;

          // Standard Particles, CF=1.
          _data->PM_SP_UG_1_0 = makeWord(_payload[0], _payload[1]);
          _data->PM_SP_UG_2_5 = makeWord(_payload[2], _payload[3]);
          _data->PM_SP_UG_10_0 = makeWord(_payload[4], _payload[5]);

          // Atmospheric Environment.
          _data->PM_AE_UG_1_0 = makeWord(_payload[6], _payload[7]);
          _data->PM_AE_UG_2_5 = makeWord(_payload[8], _payload[9]);
          _data->PM_AE_UG_10_0 = makeWord(_payload[10], _payload[11]);
        }

        _index = 0;
        return;
      }
      else
      {
        _calculatedChecksum += ch;
        uint8_t payloadIndex = _index - 4;

        // Payload is common to all sensors (first 2x6 bytes).
        if (payloadIndex < sizeof(_payload))
        {
          _payload[payloadIndex] = ch;
        }
      }

      break;
    }

    _index++;
  }
}

//END PMS FUNCTIONS //