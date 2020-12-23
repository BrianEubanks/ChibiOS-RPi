/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011,2012 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ch.h"
#include "hal.h"
#include "chprintf.h"





uint8_t rawDigits[9];
/*
//0
rawDigits[0]=0x0C;
rawDigits[1]=0x3F;
//1
rawDigits[2]=0x00;
rawDigits[3]=0x06;
//2
rawDigits[4]=0x00;
rawDigits[5]=0x01;
//3
rawDigits[6]=0x00;
rawDigits[7]=0x8F;
//4
rawDigits[8]=0x00;
rawDigits[9]=0xEB;
//5
rawDigits[10]=0x20;
rawDigits[11]=0x69;
//6
rawDigits[12]=0x00;
rawDigits[13]=0xFD;
//7
rawDigits[14]=0x00;
rawDigits[15]=0x07;
*/


static const uint8_t ht16k33_address1 = 0x70;
static const uint8_t ht16k33_address2 = 0x74;




static void init_display(uint8_t displayAddress) {


  uint8_t request[3];
  request[0] = 0x21;
  request[1] = 0x81;
  request[2] = 0xE7;

    //0x21 Enable Oscillator
  i2cAcquireBus(&I2C1);

  msg_t status = i2cMasterTransmitTimeout(
    &I2C1, displayAddress, &request[0], 1,
    NULL, 0, MS2ST(1000));

  i2cReleaseBus(&I2C1);
    
      //0x81 Turn on LED
    i2cAcquireBus(&I2C1);

    status = i2cMasterTransmitTimeout(
      &I2C1, displayAddress, &request[1], 1,
      NULL, 0, MS2ST(1000));

    i2cReleaseBus(&I2C1);
    
      //0xc7 bright ness
    i2cAcquireBus(&I2C1);

    status = i2cMasterTransmitTimeout(
      &I2C1, displayAddress, &request[2], 1,
      NULL, 0, MS2ST(1000));

    i2cReleaseBus(&I2C1);
    

  if (status != RDY_OK)
    chprintf((BaseSequentialStream *)&SD1, "Error while setting UP: %d\r\n", status);
}





static void set_display(uint8_t displayAddress) {

  i2cAcquireBus(&I2C1);

  msg_t status = i2cMasterTransmitTimeout(
    &I2C1, displayAddress, rawDigits, 9,
    NULL, 0, MS2ST(1000));

  i2cReleaseBus(&I2C1);

  if (status != RDY_OK)
    chprintf((BaseSequentialStream *)&SD1, "Error while setting voltage: %d\r\n", status);
}



//Thread 1
//Display 1
static WORKING_AREA(waThread1, 128);

static msg_t Thread1(void *p) {
  UNUSED(p);
  chRegSetThreadName("Display 1 Monitor");
    init_display(ht16k33_address1);

    uint16_t shifter = 1;
    
    rawDigits[0] = 0x00;
    for(int i = 1; i<9;i++){
        rawDigits[i] = shifter && 0xFF;
        i++;
        rawDigits[i] = shifter >> 8;
    }
    chThdSleepMilliseconds(2000);
    

  while (TRUE) {

    chprintf((BaseSequentialStream *)&SD1, "Display 1: %d %d %d\r\n", shifter, (shifter && 0xFF), (shifter >> 8));
    set_display(ht16k33_address1);
      
    chThdSleepMilliseconds(2000);
      
      shifter = (shifter << 1);
      if(shifter>=32768){
          shifter = 1;
      }
      
      for(int i = 1; i<9;i++){
          rawDigits[i] = shifter & 0xFF;
          i++;
          rawDigits[i] = shifter >> 8;
      }

  }
  return 0;
}



//Thread 2
//Display 2
static WORKING_AREA(waThread2, 128);

static msg_t Thread2(void *p) {
  UNUSED(p);
  chRegSetThreadName("Display 2 Monitor");
    init_display(ht16k33_address2);

    uint16_t shifter = 1;
    
    rawDigits[0] = 0x00;
    for(int i = 1; i<9;i++){
        rawDigits[i] = shifter && 0xFF;
        i++;
        rawDigits[i] = shifter >> 8;
    }
    chThdSleepMilliseconds(1000);
    

  while (TRUE) {

    chprintf((BaseSequentialStream *)&SD1, "Display 2: %d %d %d\r\n", shifter, (shifter && 0xFF), (shifter >> 8));
    set_display(ht16k33_address2);
      
    chThdSleepMilliseconds(1000);
      
      shifter = (shifter << 1);
      if(shifter>=32768){
          shifter = 1;
      }
      
      for(int i = 1; i<9;i++){
          rawDigits[i] = shifter & 0xFF;
          i++;
          rawDigits[i] = shifter >> 8;
      }

  }
  return 0;
}




/*
 * Application entry point.
 */
int main(void) {
  halInit();
  chSysInit();

  /*
   * Serial port initialization.
   */
  sdStart(&SD1, NULL); 
  chprintf((BaseSequentialStream *)&SD1, "BCM2835 I2C Demonstration\r\n");

  /*
   * I2C initialization.
   */
  I2CConfig i2cConfig;
  i2cStart(&I2C1, &i2cConfig);

  /*
   * Creates the Display  threads.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);
    
    chThdCreateStatic(waThread2, sizeof(waThread2), NORMALPRIO, Thread2, NULL);

  /*
   * Events servicing loop.
   */
  chThdWait(chThdSelf());

  return 0;
}
