/*
  InfinitekLEDPusher.cpp - Implementation to push Leds via hardware acceleration

  Copyright (C) 2024  Stephan Hadinger

  This library is free software: you can redistribute it and/or modify
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

#ifdef ESP32

#include "InfinitekLEDPusher.h"
#include "InfinitekLED.h"

//**************************************************************************************************************
// enable AddLog support within a C++ library
extern void AddLog(uint32_t loglevel, PGM_P formatP, ...);
enum LoggingLevels {LOG_LEVEL_NONE, LOG_LEVEL_ERROR, LOG_LEVEL_INFO, LOG_LEVEL_DEBUG, LOG_LEVEL_DEBUG_MORE};
//**************************************************************************************************************


// convert to the appropriate hardware acceleration based on capacities of the SOC
uint32_t InfinitekLEDPusher::ResolveHardware(uint32_t hw_input) {
  // Step 1. discard any unsupported hardware, and replace with InfinitekLed_HW_Default
  uint32_t hw = hw_input & 0xFF0000;     // discard bits 0..15
#if !InfinitekLED_HARDWARE_RMT
  hw &= ~InfinitekLed_RMT;                // remove RMT flag if not supported by hardware
#endif // InfinitekLED_HARDWARE_RMT
#if !InfinitekLED_HARDWARE_SPI
  hw &= ~InfinitekLed_SPI;                // remove SPI flag if not supported by hardware
#endif // InfinitekLED_HARDWARE_SPI
#if !InfinitekLED_HARDWARE_I2S
  hw &= ~InfinitekLed_I2S;                // remove I2S flag if not supported by hardware
#endif // InfinitekLED_HARDWARE_I2S

  // Step 2. If InfinitekLed_HW_Default, find a suitable scheme, RMT preferred
#if InfinitekLED_HARDWARE_RMT
  if (hw == InfinitekLed_HW_Default) {
    hw |= InfinitekLed_RMT;
  }
#endif // InfinitekLED_HARDWARE_RMT
#if InfinitekLED_HARDWARE_I2S
  if (hw == InfinitekLed_HW_Default) {
    hw |= InfinitekLed_I2S;
  }
#endif // InfinitekLED_HARDWARE_I2S
#if InfinitekLED_HARDWARE_SPI
  if (hw == InfinitekLed_HW_Default) {
    hw |= InfinitekLed_SPI;
  }
#endif // InfinitekLED_HARDWARE_SPI
  return hw;
}


InfinitekLEDPusher * InfinitekLEDPusher::Create(uint32_t hw, int8_t gpio) {
  InfinitekLEDPusher * pusher = nullptr;

  hw = InfinitekLEDPusher::ResolveHardware(hw);

#if InfinitekLED_HARDWARE_RMT
  if (pusher == nullptr && (hw & InfinitekLed_RMT)) {
    pusher = new InfinitekLEDPusherRMT(gpio);
    if (pusher->Initialized()) {
      AddLog(LOG_LEVEL_DEBUG, "LED: RMT gpio %i", gpio);
    } else {
      AddLog(LOG_LEVEL_INFO, "LED: Error create %s bus failed %i err=%i", "RMT", gpio, pusher->Error());
      delete pusher;
      pusher = nullptr;
    }
  }
#endif // InfinitekLED_HARDWARE_RMT
#if InfinitekLED_HARDWARE_SPI
  if (pusher == nullptr && (hw & InfinitekLed_SPI)) {
    pusher = new InfinitekLEDPusherSPI(gpio);
    if (pusher->Initialized()) {
      AddLog(LOG_LEVEL_DEBUG, "LED: SPI gpio %i", gpio);
    } else {
      AddLog(LOG_LEVEL_INFO, "LED: Error create %s bus failed %i err=%i", "SPI", gpio, pusher->Error());
      delete pusher;
      pusher = nullptr;
    }
  }
#endif // InfinitekLED_HARDWARE_SPI
  return pusher;
}

#endif // ESP32
