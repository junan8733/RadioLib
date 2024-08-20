/*
  RadioLib Non-Arduino Raspberry Pi Pico library example

  Licensed under the MIT License

  Copyright (c) 2024 Cameron Goddard

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

// define pins to be used
#define SPI_PORT spi1
#define SPI_MISO 12
#define SPI_MOSI 11
#define SPI_SCK 14

#define RFM_NSS 13
#define RFM_RST 9
#define RFM_DIO0 10
#define RFM_DIO1 7

#include <pico/stdlib.h>

// include the library
#include <RadioLib.h>

// include the hardware abstraction layer
#include "PicoHal.h"

// create a new instance of the HAL class
PicoHal* hal = new PicoHal(SPI_PORT, SPI_MISO, SPI_MOSI, SPI_SCK);

// now we can create the radio module
// NSS pin:  26
// DIO0 pin:  14
// RESET pin:  22
// DIO1 pin:  15
LLCC68 radio = new Module(hal, RFM_NSS, RFM_DIO0, RFM_RST, RFM_DIO1);

int main() {
  // initialize just like with Arduino
  printf("[LLCC68] Initializing ... ");
  int state = radio.begin();
  if (state != RADIOLIB_ERR_NONE) {
    printf("failed, code %d\n", state);
    return(1);
  }
  printf("success!\n");

  // set output power to 18 dBm (accepted range is -17 - 22 dBm)
  if (radio.setOutputPower(18) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
    printf("Selected output power is invalid for this module!\n");
    while (true) { hal->delay(10); }
  }
  printf("18dBm success!\n");

  // set over current protection limit to 110 mA (accepted range is 45 - 240 mA)
  // NOTE: set value to 0 to disable overcurrent protection
  if (radio.setCurrentLimit(110) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
    printf("Selected current limit is invalid for this module!\n");
    while (true) { hal->delay(10); }
  }
  printf("110mA success!\n");

  // loop forever
  for(;;) {
    printf("[LLCC68] Waiting for incoming transmission ... ");

    // you can receive data as an Arduino String
    // String str;
    // int state = radio.receive(str);

    // you can also receive data as byte array
    uint8_t byteArr[20] = { NULL };  // 모든 포인터를 NULL로 초기화
    size_t len = sizeof(byteArr);
    int state = radio.receive(byteArr, len);

    if (state == RADIOLIB_ERR_NONE) {
      // packet was successfully received
      printf("success!\n");

      // print the data of the packet
      printf("[LLCC68] Data:\t\t");
      printf("%s\n", (char*)byteArr);

      // print the RSSI (Received Signal Strength Indicator)
      // of the last received packet
      printf("[LLCC68] RSSI:\t\t");
      printf("%f", radio.getRSSI());
      printf(" dBm\n");

      // print the SNR (Signal-to-Noise Ratio)
      // of the last received packet
      printf("[LLCC68] SNR:\t\t");
      printf("%f", radio.getSNR());
      printf(" dB\n");

      // print frequency error
      printf("[LLCC68] Frequency error:\t");
      printf("%f", radio.getFrequencyError());
      printf(" Hz\n");

    } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
      // timeout occurred while waiting for a packet
      printf("timeout!\n");

    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      // packet was received, but is malformed
      printf("CRC error!\n");

    } else {
      // some other error occurred
      printf("failed, code %d\n", state);

    }
    // 임의로 넣어준 딜레이 9초.
    hal->delay(9000);
  }

  return(0);
}
