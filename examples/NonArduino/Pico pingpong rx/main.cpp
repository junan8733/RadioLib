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
#define SPI_MOSI 15
#define SPI_SCK 14

#define RFM_NSS 13
#define RFM_RST 9
#define RFM_DIO0 10 // irq? : 모듈의 d1
#define RFM_DIO1 7 // busy? : 모듈의 d4

// uncomment the following only on one
// of the nodes to initiate the pings
//#define INITIATING_NODE

#include <pico/stdlib.h>

// include the library
#include <RadioLib.h>

// include the hardware abstraction layer
#include "PicoHal.h"

// create a new instance of the HAL class
PicoHal* hal = new PicoHal(SPI_PORT, SPI_MISO, SPI_MOSI, SPI_SCK);

// now we can create the radio module
LLCC68 radio = new Module(hal, RFM_NSS, RFM_DIO0, RFM_RST, RFM_DIO1);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//LLCC68 radio = RadioShield.ModuleA;

// or using CubeCell
//LLCC68 radio = new Module(RADIOLIB_BUILTIN_MODULE);

// save transmission states between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate transmission or reception state
bool transmitFlag = false;

// flag to indicate that a packet was sent or received
volatile bool operationDone = false;

// this function is called when a complete packet
// is transmitted or received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!

void setFlag(void) {
  // we sent or received a packet, set the flag
  operationDone = true;
}

// Pico W devices use a GPIO on the WIFI chip for the LED,
// so when building for Pico W, CYW43_WL_GPIO_LED_PIN will be defined
#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 250
#endif

// Perform initialisation
int pico_led_init(void) {
#if defined(PICO_DEFAULT_LED_PIN)
    // A device like Pico that uses a GPIO for the LED will define PICO_DEFAULT_LED_PIN
    // so we can use normal GPIO functionality to turn the led on and off
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return PICO_OK;
#elif defined(CYW43_WL_GPIO_LED_PIN)
    // For Pico W devices we need to initialise the driver etc
    return cyw43_arch_init();
#endif
}

// Turn the led on or off
void pico_set_led(bool led_on) {
#if defined(PICO_DEFAULT_LED_PIN)
    // Just set the GPIO on or off
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);
#elif defined(CYW43_WL_GPIO_LED_PIN)
    // Ask the wifi "driver" to set the GPIO on or off
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
#endif
}

void led_blink(int blink_count){
  for (int i = 0; i < blink_count; i++){
    pico_set_led(true);
    sleep_ms(LED_DELAY_MS);
    pico_set_led(false);
    sleep_ms(LED_DELAY_MS);
  }
}

int main() {
  // initialize PICO LED
  int rc = pico_led_init();
  hard_assert(rc == PICO_OK);

  // initialize just like with Arduino
  printf("[LLCC68] Initializing ... ");
  int state = radio.begin();
  if (state != RADIOLIB_ERR_NONE) {
    printf("failed, code %d\n", state);
    return(1);
  }
  printf("success!\n");

  // set output power to 22 dBm (accepted range is -17 - 22 dBm)
  if (radio.setOutputPower(22) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
    printf("Selected output power is invalid for this module!\n");
    while (true) { hal->delay(10); }
  }
  printf("22dBm success!\n");

  // set over current protection limit to 120 mA (accepted range is 45 - 240 mA)
  // NOTE: set value to 0 to disable overcurrent protection
  if (radio.setCurrentLimit(120) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
    printf("Selected current limit is invalid for this module!\n");
    while (true) { hal->delay(10); }
  }
  printf("120mA success!\n");

  // set the function that will be called
  // when new packet is received
  radio.setDio1Action(setFlag);

  int count = 0;

  #if defined(INITIATING_NODE)
    // send the first packet on this node
    printf("[LLCC68] Sending first packet ... ");
    char str_to_transmit[25] = {0};
    snprintf(str_to_transmit, sizeof(str_to_transmit), "Hello World! %d", count);
    transmissionState = radio.startTransmit(str_to_transmit);
    transmitFlag = true;
  #else
    // start listening for LoRa packets on this node
    printf("[LLCC68] Starting to listen ... ");
    state = radio.startReceive();
    if (state == RADIOLIB_ERR_NONE) {
      printf("success!\n");
    } else {
      printf("failed, code %d\n", state);
      while (true) { hal->delay(10); }
    }
  #endif

  // loop forever
  for(;;) {
    // check if the previous operation finished
    if(operationDone) {
      // reset flag
      operationDone = false;

      if(transmitFlag) {
        // the previous operation was transmission, listen for response
        // print the result
        if (transmissionState == RADIOLIB_ERR_NONE) {
          // packet was successfully sent
          printf("transmission finished!\n");

        } else {
          printf("failed, code %d\n", transmissionState);

        }

        // listen for response
        radio.startReceive();
        transmitFlag = false;

        // 5초 딜레이.
        //hal->delay(5000);

      } 
      else {
        // the previous operation was reception
        // print data and send another packet
        uint8_t byteArr_str[25] = { NULL };  // 모든 포인터를 NULL로 초기화
        size_t len = sizeof(byteArr_str);
        int state = radio.readData(byteArr_str, len);

        if (state == RADIOLIB_ERR_NONE) {
          // packet was successfully received
          printf("[LLCC68] Received packet!\n");

          // print data of the packet
          printf("[LLCC68] Data:\t\t");
          printf("%s\n", (char*)byteArr_str);

          // print RSSI (Received Signal Strength Indicator)
          printf("[LLCC68] RSSI:\t\t");
          printf("%f", radio.getRSSI());
          printf(" dBm\n");

          // print SNR (Signal-to-Noise Ratio)
          printf("[LLCC68] SNR:\t\t");
          printf("%f", radio.getSNR());
          printf(" dB\n");

          led_blink(5);

        }

        // wait a second before transmitting again 원래는 여기가 1초였다.
        hal->delay(1000);

        count++;

        // send another one
        printf("[LLCC68] Sending another packet ... ");
        char str_to_transmit[25] = {0};
        snprintf(str_to_transmit, sizeof(str_to_transmit), "Hello World! %d", count);
        transmissionState = radio.startTransmit(str_to_transmit);
        transmitFlag = true;
      }

      // 10초 딜레이.
      hal->delay(10000);

    }

  }

  return(0);
}
