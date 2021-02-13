/*
  Test.h - Test library for Wiring - description
  Copyright (c) 2006 John Doe.  All right reserved.
*/

// ensure this library description is only included once
#ifndef PMS_h
#define PMS_h

#include <SoftwareSerial.h>
#include <Print.h>
#include "Stream.h"

// library interface description
class PMS
{
  // user-accessible "public" interface
  public:
    PMS(bool displayMsg=false,int baudRate=9600);
    //void begin(int baudRate=9600);

    static void setOutput(Print& debugOut, bool verbose = true);
    void PMS_Init(void);
    void PMS_Init(int,int,int,int);
    void PMS_Init(int,int,int,int,int);

    bool _debugMsg;


    //PMS VARIABLES PUBLIC_START
    static const uint16_t SINGLE_RESPONSE_TIME = 1000;
    static const uint16_t TOTAL_RESPONSE_TIME = 1000 * 10;
    static const uint16_t STEADY_RESPONSE_TIME = 1000 * 30;

    static const uint16_t BAUD_RATE = 9600;

    struct DATA {
      // Standard Particles, CF=1
      uint16_t PM_SP_UG_1_0;
      uint16_t PM_SP_UG_2_5;
      uint16_t PM_SP_UG_10_0;

      // Atmospheric environment
      uint16_t PM_AE_UG_1_0;
      uint16_t PM_AE_UG_2_5;
      uint16_t PM_AE_UG_10_0;
    };

    void start(Stream&);
    void sleep();
    void wakeUp();
    void reset();
    void activeMode();
    void passiveMode();

    void requestRead();
    bool read_PMS(DATA& data);
    bool readUntil(DATA& data, uint16_t timeout = SINGLE_RESPONSE_TIME);
    const char* getPM2();
    int getPM2_Raw();
    const char* getPM1();
    int getPM1_Raw();
    const char* getPM10();
    int getPM10_Raw();
    void clearSerial();
    
    //PMS VARIABLES PUBLIC_END


    //MQ135 VARIABLES PUBLIC_START
    //MQ135 VARIABLES PUBLIC_END



  // library-accessible "private" interface
  private:
    int value;
    int _setPin;
    int _resetPin;

     //PMS VARIABLES PRIVATE START
    enum STATUS { STATUS_WAITING, STATUS_OK };
    enum MODE { MODE_ACTIVE, MODE_PASSIVE };

    uint8_t _payload[12];
    Stream* _stream;
    DATA* _data;
    STATUS _PMSstatus;
    MODE _mode = MODE_ACTIVE;

    uint8_t _index = 0;
    uint16_t _frameLen;
    uint16_t _checksum;
    uint16_t _calculatedChecksum;
    SoftwareSerial *_SoftSerial_PMS;
    void loop();
    char Char_PM2[10];
    char Char_PM1[10];
    char Char_PM10[10];
    //PMS VARIABLES PRIVATE END

};


#endif

