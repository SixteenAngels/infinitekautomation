/*
  xdrv_31_Infinitek_client.ino - Support for external microcontroller on serial

  Copyright (C) 2021  Andre Thomas and Theo Arends

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef USE_Infinitek_CLIENT
/*********************************************************************************************\
 * Infinitek to microcontroller
\*********************************************************************************************/

#define XDRV_31                         31

#ifndef USE_Infinitek_CLIENT_FLASH_SPEED
#define USE_Infinitek_CLIENT_FLASH_SPEED  57600     // Usually 57600 for 3.3V variants and 115200 for 5V variants
#endif
#ifndef USE_Infinitek_CLIENT_SERIAL_SPEED
#define USE_Infinitek_CLIENT_SERIAL_SPEED 57600     // Depends on the sketch that is running on the Uno/Pro Mini
#endif

#define Infinitek_CLIENT_LIB_VERSION      20191129
#define Infinitek_CLIENT_TIMEOUT          250       // mSeconds

#define CONST_STK_CRC_EOP               0x20

#define CMND_STK_GET_SYNC               0x30
#define CMND_STK_SET_DEVICE             0x42
#define CMND_STK_SET_DEVICE_EXT         0x45
#define CMND_STK_ENTER_PROGMODE         0x50
#define CMND_STK_LEAVE_PROGMODE         0x51
#define CMND_STK_LOAD_ADDRESS           0x55
#define CMND_STK_PROG_PAGE              0x64

/*************************************************\
 * Infinitek Client Specific Commands
\*************************************************/

#define CMND_START                      0xFC
#define CMND_END                        0xFD

#define CMND_FEATURES                   0x01
#define CMND_GET_JSON                   0x02
#define CMND_FUNC_EVERY_SECOND          0x03
#define CMND_FUNC_EVERY_100_MSECOND     0x04
#define CMND_CLIENT_SEND                0x05
#define CMND_PUBLISH_TELE               0x06
#define CMND_EXECUTE_CMND               0x07

#define PARAM_DATA_START                0xFE
#define PARAM_DATA_END                  0xFF

#include <InfinitekSerial.h>

struct SimpleHexParse {
  uint8_t FlashPage[128];
  uint8_t layoverBuffer[16];
  uint8_t FlashPageIdx;
  uint8_t layoverIdx;
  uint8_t ptr_l;
  uint8_t ptr_h;
  bool firstrun;
  bool EndOfFile;
} SHParse;

uint8_t SimpleHexParseGetByte(char* hexline, uint8_t idx) {
  char buff[3];
  buff[3] = '\0';
  memcpy(&buff, &hexline[(idx*2)-2], 2);
  return strtol(buff, 0, 16);
}

uint32_t SimpleHexParseLine(char *hexline) {
  if (SHParse.layoverIdx) {
    memcpy(&SHParse.FlashPage, &SHParse.layoverBuffer, SHParse.layoverIdx);
    SHParse.FlashPageIdx = SHParse.layoverIdx;
    SHParse.layoverIdx = 0;
  }

  // 10 00 00 00 0C945D000C9485000C9485000C948500 84
  uint8_t len = SimpleHexParseGetByte(hexline, 1);
  uint8_t addr_h = SimpleHexParseGetByte(hexline, 2);
  uint8_t addr_l = SimpleHexParseGetByte(hexline, 3);
  uint8_t rectype = SimpleHexParseGetByte(hexline, 4);

//  AddLog(LOG_LEVEL_DEBUG, PSTR("DBG: Hexline |%s|, Len %d, Address 0x%02X%02X, RecType %d"), hexline, len, addr_h, addr_l, rectype);

  if (len > 16) { return 5; }                // Error: Line too long
  if (rectype > 1) { return 6; }             // Error: Invalid record type

  for (uint32_t idx = 0; idx < len; idx++) {
    if (SHParse.FlashPageIdx < sizeof(SHParse.FlashPage)) {
      SHParse.FlashPage[SHParse.FlashPageIdx] = SimpleHexParseGetByte(hexline, idx+5);
      SHParse.FlashPageIdx++;
    } else { // We have layover bytes
      SHParse.layoverBuffer[SHParse.layoverIdx] = SimpleHexParseGetByte(hexline, idx+5);
      SHParse.layoverIdx++;
    }
  }

  if (1 == rectype) {
    SHParse.EndOfFile = true;
    while (SHParse.FlashPageIdx < sizeof(SHParse.FlashPage)) {
      SHParse.FlashPage[SHParse.FlashPageIdx] = 0xFF;
      SHParse.FlashPageIdx++;
    }
  }

  if (SHParse.FlashPageIdx == sizeof(SHParse.FlashPage)) {
    if (SHParse.firstrun) {
      SHParse.firstrun = false;
    } else {
      SHParse.ptr_l += 0x40;
      if (SHParse.ptr_l == 0) {
        SHParse.ptr_h++;
      }
    }
  }
  return 0;
}

struct TCLIENT {
  uint8_t inverted = LOW;
  bool type = false;
  bool SerialEnabled = false;
  uint8_t waitstate = 0;            // We use this so that features detection does not slow down other stuff on startup
  bool unsupported = false;
} TClient;

typedef union {
  uint32_t data;
  struct {
    uint32_t func_json_append : 1;              // Client supports providing a JSON for TELEPERIOD
    uint32_t func_every_second : 1;             // Client supports receiving a FUNC_EVERY_SECOND callback with no response
    uint32_t func_every_100_msecond : 1;        // Client supports receiving a FUNC_EVERY_100_MSECOND callback with no response
    uint32_t func_client_send : 1;              // Client supports receiving commands with "client send xxx"
    uint32_t spare4 : 1;
    uint32_t spare5 : 1;
    uint32_t spare6 : 1;
    uint32_t spare7 : 1;
    uint32_t spare8 : 1;
    uint32_t spare9 : 1;
    uint32_t spare10 : 1;
    uint32_t spare11 : 1;
    uint32_t spare12 : 1;
    uint32_t spare13 : 1;
    uint32_t spare14 : 1;
    uint32_t spare15 : 1;
    uint32_t spare16 : 1;
    uint32_t spare17 : 1;
    uint32_t spare18 : 1;
    uint32_t spare19 : 1;
    uint32_t spare20 : 1;
    uint32_t spare21 : 1;
    uint32_t spare22 : 1;
    uint32_t spare23 : 1;
    uint32_t spare24 : 1;
    uint32_t spare25 : 1;
    uint32_t spare26 : 1;
    uint32_t spare27 : 1;
    uint32_t spare28 : 1;
    uint32_t spare29 : 1;
    uint32_t spare30 : 1;
    uint32_t spare31 : 1;
  };
} TClientFeatureCfg;

/*
 * The structure below must remain 4 byte aligned to be compatible with
 * Infinitek as master
 */

struct TCLIENT_FEATURES {
  uint32_t features_version;
  TClientFeatureCfg features;
} TClientSettings;

struct TCLIENT_COMMAND {
  uint8_t command;
  uint8_t parameter;
  uint8_t unused2;
  uint8_t unused3;
} TClientCommand;

InfinitekSerial *InfinitekClient_Serial;

void InfinitekClient_Reset(void) {
  if (TClient.SerialEnabled) {
    digitalWrite(Pin(GPIO_InfinitekCLIENT_RST), !TClient.inverted);
    delay(1);
    digitalWrite(Pin(GPIO_InfinitekCLIENT_RST), TClient.inverted);
    delay(1);
    digitalWrite(Pin(GPIO_InfinitekCLIENT_RST), !TClient.inverted);
    delay(5);
  }
}

uint8_t InfinitekClient_waitForSerialData(int dataCount, int timeout) {
  int timer = 0;
  while (timer < timeout) {
    if (InfinitekClient_Serial->available() >= dataCount) {
      return 1;
    }
    delay(1);
    timer++;
  }
  return 0;
}

uint8_t InfinitekClient_receiveData(char* buffer, int size) {
  uint8_t index = 255;
  int timer = 0;
  while (timer < Infinitek_CLIENT_TIMEOUT) {
    int data = InfinitekClient_Serial->read();
    if (data >= 0) {
      if (PARAM_DATA_START == data) { index = 0; }  // Start of data
      else if (PARAM_DATA_END == data) { break; }   // End of data
      else if (index < 255) {
        buffer[index++] = (char)data;               // Data
        if (index == size) { break; }               // No EoD received or done
      }
    } else {
      delay(1);
      timer++;
    }
  }
  if (255 == index) { index = 0; }

//  AddLog(LOG_LEVEL_DEBUG, PSTR("TCL: ReceiveData %*_H"), index, (uint8_t*)buffer);

  return index;
}

uint8_t InfinitekClient_sendBytes(uint8_t* bytes, int count) {
//  AddLog(LOG_LEVEL_DEBUG, PSTR("TCL: SendBytes %*_H"), count, (uint8_t*)&bytes);

  InfinitekClient_Serial->write(bytes, count);
  InfinitekClient_waitForSerialData(2, Infinitek_CLIENT_TIMEOUT);
  uint8_t sync = InfinitekClient_Serial->read();
  uint8_t ok = InfinitekClient_Serial->read();
  if ((sync == 0x14) && (ok == 0x10)) {
    return 1;
  }
  return 0;
}

uint8_t InfinitekClient_execCmd(uint8_t cmd) {
  uint8_t bytes[] = { cmd, CONST_STK_CRC_EOP };
  return InfinitekClient_sendBytes(bytes, 2);
}

uint8_t InfinitekClient_execParam(uint8_t cmd, uint8_t* params, int count) {
  uint8_t bytes[32];
  bytes[0] = cmd;
  int i = 0;
  while (i < count) {
    bytes[i + 1] = params[i];
    i++;
  }
  bytes[i + 1] = CONST_STK_CRC_EOP;
  return InfinitekClient_sendBytes(bytes, i + 2);
}

uint32_t InfinitekClient_Flash(uint8_t* data, size_t size) {
/*
  // Don't do this as there is no re-init configured
  InfinitekClient_Serial->end();
  delay(10);

  InfinitekClient_Serial->begin(USE_Infinitek_CLIENT_FLASH_SPEED);
  if (InfinitekClient_Serial->hardwareSerial()) {
    ClaimSerial();
  }
*/
  InfinitekClient_Reset();

  uint8_t timeout = 0;
  while (timeout <= 50) {
    if (InfinitekClient_execCmd(CMND_STK_GET_SYNC)) {
      break;
    }
    timeout++;
    delay(1);
  }
  if (timeout > 50) { return 1; }            // Error: Bootloader could not be found

  AddLog(LOG_LEVEL_INFO, PSTR("TCL: Found bootloader"));

  uint8_t ProgParams[] = {0x86, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x03, 0xff, 0xff, 0xff, 0xff, 0x00, 0x80, 0x04, 0x00, 0x00, 0x00, 0x80, 0x00};
  if (!InfinitekClient_execParam(CMND_STK_SET_DEVICE, ProgParams, sizeof(ProgParams))) {
    return 2;                                // Error: Could not configure device for programming (1)
  }

  uint8_t ExtProgParams[] = {0x05, 0x04, 0xd7, 0xc2, 0x00};
  if (!InfinitekClient_execParam(CMND_STK_SET_DEVICE_EXT, ExtProgParams, sizeof(ExtProgParams))) {
    return 3;                                // Error: Could not configure device for programming (2)
  }

  if (!InfinitekClient_execCmd(CMND_STK_ENTER_PROGMODE)) {
    return 4;                                // Error: Failed to put bootloader into programming mode
  }

  uint8_t header[] = {CMND_STK_PROG_PAGE, 0x00, 0x80, 0x46};

  SHParse.FlashPageIdx = 0;
  SHParse.layoverIdx = 0;
  SHParse.ptr_l = 0;
  SHParse.ptr_h = 0;
  SHParse.firstrun = true;
  SHParse.EndOfFile = false;

  char flash_buffer[512];
  char thishexline[50];
  uint32_t processed = 0;
  uint32_t position = 0;
  uint32_t error = 0;

  uint32_t read = 0;
  while (read < size) {
    memcpy(flash_buffer, data + read, sizeof(flash_buffer));
    read = read + sizeof(flash_buffer);

//    AddLog(LOG_LEVEL_DEBUG, PSTR("TCL: Flash %32_H"), (uint8_t*)flash_buffer);

    for (uint32_t ca = 0; ca < sizeof(flash_buffer); ca++) {
      processed++;
      if ((processed <= size) && (!SHParse.EndOfFile)) {
        // flash_buffer = :100000000C945D000C9485000C9485000C94850084<0x0D><0x0A>
        if (':' == flash_buffer[ca]) {
          position = 0;
        }
        else if (0x0D == flash_buffer[ca]) {
          // 100000000C945D000C9485000C9485000C94850084
          thishexline[position] = 0;
          error = SimpleHexParseLine(thishexline);
          if (error) { break; }              // Error 5 and 6
          if (SHParse.FlashPageIdx == sizeof(SHParse.FlashPage)) {
            uint8_t params[] = {SHParse.ptr_l, SHParse.ptr_h};
            InfinitekClient_execParam(CMND_STK_LOAD_ADDRESS, params, sizeof(params));

            InfinitekClient_Serial->write(header, sizeof(header));

            for (uint32_t i = 0; i < sizeof(SHParse.FlashPage); i++) {
              InfinitekClient_Serial->write(SHParse.FlashPage[i]);
            }
            InfinitekClient_Serial->write(CONST_STK_CRC_EOP);

            InfinitekClient_waitForSerialData(2, Infinitek_CLIENT_TIMEOUT);
            InfinitekClient_Serial->read();
            InfinitekClient_Serial->read();

            SHParse.FlashPageIdx = 0;
          }
        }
        else if (0x0A != flash_buffer[ca]) {
          if (!isalnum(flash_buffer[ca])) {
//            AddLog(LOG_LEVEL_DEBUG, PSTR("DBG: Size %d, Processed %d"), size, processed);
            error = 7;                       // Error: Invalid data
            break;
          }
          if (position < sizeof(thishexline) -2) {
            thishexline[position++] = flash_buffer[ca];
          }
        }
      }
    }
    if (error) { break; }
  }
  InfinitekClient_execCmd(CMND_STK_LEAVE_PROGMODE);

  return error;                              // Error or Flash done!
}

void InfinitekClient_Init(void) {
  if (TClient.type) {
    return;
  }
  if (10 > TClient.waitstate) {
    TClient.waitstate++;
    return;
  }
  if (!TClient.SerialEnabled) {
    if (PinUsed(GPIO_InfinitekCLIENT_RXD) && PinUsed(GPIO_InfinitekCLIENT_TXD) &&
        (PinUsed(GPIO_InfinitekCLIENT_RST) || PinUsed(GPIO_InfinitekCLIENT_RST_INV))) {
      InfinitekClient_Serial = new InfinitekSerial(Pin(GPIO_InfinitekCLIENT_RXD), Pin(GPIO_InfinitekCLIENT_TXD), 1, 0, 200);
      if (InfinitekClient_Serial->begin(USE_Infinitek_CLIENT_SERIAL_SPEED)) {
        if (InfinitekClient_Serial->hardwareSerial()) {
          ClaimSerial();
        }
#ifdef ESP32
        AddLog(LOG_LEVEL_DEBUG, PSTR("TCL: Serial UART%d"), InfinitekClient_Serial->getUart());
#endif
        if (PinUsed(GPIO_InfinitekCLIENT_RST_INV)) {
          SetPin(Pin(GPIO_InfinitekCLIENT_RST_INV), AGPIO(GPIO_InfinitekCLIENT_RST));
          TClient.inverted = HIGH;
        }
        pinMode(Pin(GPIO_InfinitekCLIENT_RST), OUTPUT);
        TClient.SerialEnabled = true;
        InfinitekClient_Reset();
        AddLog(LOG_LEVEL_INFO, PSTR("TCL: Enabled"));
      }
    }
  }
  if (TClient.SerialEnabled) {  // All go for hardware now we need to detect features if there are any
    InfinitekClient_sendCmnd(CMND_FEATURES, 0);

    char buffer[sizeof(TClientSettings)];
    uint8_t len = InfinitekClient_receiveData(buffer, sizeof(buffer));  // 99 17 34 01 02 00 00 00
    if (len == sizeof(TClientSettings)) {
      memcpy(&TClientSettings, &buffer, sizeof(TClientSettings));
      if (Infinitek_CLIENT_LIB_VERSION == TClientSettings.features_version) {
        TClient.type = true;
        AddLog(LOG_LEVEL_INFO, PSTR("TCL: Version %u"), TClientSettings.features_version);
      } else {
        if ((!TClient.unsupported) && (TClientSettings.features_version > 0)) {
          AddLog(LOG_LEVEL_INFO, PSTR("TCL: Version %u not supported!"), TClientSettings.features_version);
          TClient.unsupported = true;
        }
      }
    }
  }
}

bool InfinitekClient_Available(void) {
  return TClient.SerialEnabled;
}

void InfinitekClient_Show(void) {
  if ((TClient.type) && (TClientSettings.features.func_json_append)) {
    InfinitekClient_sendCmnd(CMND_GET_JSON, 0);

    char buffer[250];  // Keep size below 255 to stay within 8-bits index and len
    uint8_t len = InfinitekClient_receiveData(buffer, sizeof(buffer) -1);

    if (len) {
      buffer[len] = '\0';
      ResponseAppend_P(PSTR(",\"InfinitekClient\":%s"), buffer);
    }
  }
}

void InfinitekClient_sendCmnd(uint8_t cmnd, uint8_t param) {
  TClientCommand.command = cmnd;
  TClientCommand.parameter = param;
  char buffer[sizeof(TClientCommand)+2];
  buffer[0] = CMND_START;
  memcpy(&buffer[1], &TClientCommand, sizeof(TClientCommand));
  buffer[sizeof(TClientCommand)+1] = CMND_END;

//  AddLog(LOG_LEVEL_DEBUG, PSTR("TCL: SendCmnd %*_H"), sizeof(buffer), (uint8_t*)&buffer);

  for (uint32_t ca = 0; ca < sizeof(buffer); ca++) {
    InfinitekClient_Serial->write(buffer[ca]);
  }
}

#define D_PRFX_CLIENT "Client"
#define D_CMND_CLIENT_RESET "Reset"
#define D_CMND_CLIENT_SEND "Send"

const char kInfinitekClientCommands[] PROGMEM = D_PRFX_CLIENT "|"
  D_CMND_CLIENT_RESET "|" D_CMND_CLIENT_SEND;

void (* const InfinitekClientCommand[])(void) PROGMEM = {
  &CmndClientReset, &CmndClientSend };

void CmndClientReset(void) {
  InfinitekClient_Reset();
  TClient.type = false;        // Force redetection
  TClient.waitstate = 7;       // give it at least 3 seconds to restart from bootloader
  TClient.unsupported = false; // Reset unsupported flag
  ResponseCmndDone();
}

void CmndClientSend(void) {
  if (TClient.SerialEnabled) {
    if (0 < XdrvMailbox.data_len) {
      InfinitekClient_sendCmnd(CMND_CLIENT_SEND, XdrvMailbox.data_len);
      InfinitekClient_Serial->write(char(PARAM_DATA_START));
      for (uint32_t idx = 0; idx < XdrvMailbox.data_len; idx++) {
        InfinitekClient_Serial->write(XdrvMailbox.data[idx]);
      }
      InfinitekClient_Serial->write(char(PARAM_DATA_END));
    }
    ResponseCmndDone();
  }
}

void InfinitekClient_ProcessIn(void) {
  uint8_t cmnd = InfinitekClient_Serial->read();
  if (CMND_START == cmnd) {
    InfinitekClient_waitForSerialData(sizeof(TClientCommand), 50);
    uint8_t buffer[sizeof(TClientCommand)];
    for (uint32_t idx = 0; idx < sizeof(TClientCommand); idx++) {
      buffer[idx] = InfinitekClient_Serial->read();
    }
    InfinitekClient_Serial->read(); // read trailing byte of command
    memcpy(&TClientCommand, &buffer, sizeof(TClientCommand));
    char inbuf[TClientCommand.parameter+1];
    InfinitekClient_waitForSerialData(TClientCommand.parameter, 50);
    InfinitekClient_Serial->read(); // Read leading byte
    for (uint32_t idx = 0; idx < TClientCommand.parameter; idx++) {
      inbuf[idx] = InfinitekClient_Serial->read();
    }
    InfinitekClient_Serial->read(); // Read trailing byte
    inbuf[TClientCommand.parameter] = '\0';

    if (CMND_PUBLISH_TELE == TClientCommand.command) { // We need to publish stat/ with incoming stream as content
      Response_P(PSTR("{\"InfinitekClient\":"));
      ResponseAppend_P("%s", inbuf);
      ResponseJsonEnd();
      MqttPublishPrefixTopicRulesProcess_P(RESULT_OR_TELE, PSTR("InfinitekClient"));
    }
    if (CMND_EXECUTE_CMND == TClientCommand.command) { // We need to execute the incoming command
      ExecuteCommand(inbuf, SRC_TCL);
    }
  }
}

/*********************************************************************************************\
 * Interface
\*********************************************************************************************/

bool Xdrv31(uint32_t function) {
  bool result = false;

  switch (function) {
    case FUNC_EVERY_100_MSECOND:
      if (TClient.type) {
        if (InfinitekClient_Serial->available()) {
          InfinitekClient_ProcessIn();
        }
        if (TClientSettings.features.func_every_100_msecond) {
          InfinitekClient_sendCmnd(CMND_FUNC_EVERY_100_MSECOND, 0);
        }
      }
      break;
    case FUNC_EVERY_SECOND:
      if ((TClient.type) && (TClientSettings.features.func_every_second)) {
        InfinitekClient_sendCmnd(CMND_FUNC_EVERY_SECOND, 0);
      }
      InfinitekClient_Init();
      break;
    case FUNC_JSON_APPEND:
      if ((TClient.type) && (TClientSettings.features.func_json_append)) {
        InfinitekClient_Show();
      }
      break;
    case FUNC_COMMAND:
      result = DecodeCommand(kInfinitekClientCommands, InfinitekClientCommand);
      break;
    case FUNC_ACTIVE:
      result = true;
      break;
  }
  return result;
}

#endif  // USE_Infinitek_CLIENT
