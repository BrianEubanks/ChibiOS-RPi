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

#include "text.h"


static uint8_t rawDigits1[9];
static uint8_t rawDigits2[9];



static const uint8_t display_address1 = 0x70;
static const uint8_t display_address2 = 0x74;


//Timer values in milliseconds
//timeToDisplay converts milliseconds to 4 digit time value
//              and stores in digit buffer


//60000 = 1 minutes
//300000 = 5 minutes

static uint32_t callback_count_1 = 300000;

static uint32_t callback_count_2 = 30000;



static void gptcallback1(GPTDriver *gptp) {
    UNUSED(gptp);
    chSysLockFromIsr();
        if(callback_count_1 != 0){
          callback_count_1-=10;
      }
    timeToDisplay(rawDigits1, callback_count_1);
    //timeToDisplay(rawDigits2, callback_count_1);
    
    chSysUnlockFromIsr();
}


static void gptcallback2(GPTDriver *gptp) {
    UNUSED(gptp);
    chSysLockFromIsr();
    if(callback_count_2 != 0){
        callback_count_2-=10;
    }
    timeToDisplay(rawDigits2, callback_count_2);
    chSysUnlockFromIsr();
}







static void init_display(uint8_t displayAddress) {


    uint8_t request[3];
    request[0] = 0x21;
    request[1] = 0x81;
    request[2] = 0xE7;
    
    for(int i = 0; i < 9; i++){
        rawDigits1[i]=0x00;
        rawDigits2[i]=0x00;
    }

    //0x21 Enable Oscillator
    
    i2cAcquireBus(&I2C1);

    msg_t osc_status = i2cMasterTransmitTimeout(&I2C1, displayAddress, &request[0], 1,NULL, 0, MS2ST(1000));

    
    //Clear Display before turning on Display
    
    msg_t clr_status = i2cMasterTransmitTimeout(
      &I2C1, displayAddress, rawDigits1, 9,
      NULL, 0, MS2ST(1000));
    
    //0x81 Turn on LED

    msg_t led_status = i2cMasterTransmitTimeout(&I2C1, displayAddress, &request[1], 1,NULL, 0, MS2ST(1000));

    //0xc7 bright ness
    
    msg_t bright_status = i2cMasterTransmitTimeout(
      &I2C1, displayAddress, &request[2], 1,
      NULL, 0, MS2ST(1000));

    i2cReleaseBus(&I2C1);
    
    
    
    if (osc_status != RDY_OK){
        chprintf((BaseSequentialStream *)&SD1, "Error while setting Display Oscillator: %d\r\n", osc_status);
    }
    if (clr_status != RDY_OK){
        chprintf((BaseSequentialStream *)&SD1, "Error while Clearing Display: %d\r\n", osc_status);
    }
    if (led_status != RDY_OK){
        chprintf((BaseSequentialStream *)&SD1, "Error while setting Display LED: %d\r\n", led_status);
    }
    if (bright_status != RDY_OK){
        chprintf((BaseSequentialStream *)&SD1, "Error while setting Display Brightness: %d\r\n", bright_status);
    }
    
    
}



static void set_display(uint8_t displayAddress1, uint8_t displayAddress2, uint8_t * rawD1, uint8_t * rawD2) {

    i2cAcquireBus(&I2C1);

    msg_t dsp1_status = i2cMasterTransmitTimeout(&I2C1, displayAddress1, rawD1, 9,NULL, 0, MS2ST(1000));
    
    msg_t dsp2_status = i2cMasterTransmitTimeout(&I2C1, displayAddress2, rawD2, 9,NULL, 0, MS2ST(1000));

    i2cReleaseBus(&I2C1);

    if (dsp1_status != RDY_OK){
        chprintf((BaseSequentialStream *)&SD1, "Error while setting Display1: %d\r\n", dsp1_status);
    }
    if (dsp2_status != RDY_OK){
        chprintf((BaseSequentialStream *)&SD1, "Error while setting Display2: %d\r\n", dsp2_status);
    }
}



//Display Thread
//Updates the displays every 10ms

static WORKING_AREA(waThreadDSP, 128);
static msg_t ThreadDSP(void *p) {
    UNUSED(p);
    chRegSetThreadName("Displays Thread");

    while (TRUE) {

        //chprintf((BaseSequentialStream *)&SD1, "Display 1: %d %d %d\r\n", shifter, (shifter && 0xFF), (shifter >> 8));
        set_display(display_address1, display_address2, rawDigits1, rawDigits2);
      
        chThdSleepMilliseconds(10);

    }
    return 0;
    
}






//Timer Thread
static WORKING_AREA(waThreadTimer, 128);

static msg_t ThreadTimer(void *p) {
    UNUSED(p);
    chRegSetThreadName("Timer Thread");
    
    timeToDisplay(rawDigits1, callback_count_1);
    timeToDisplay(rawDigits2, callback_count_2);
    
    chThdSleepMilliseconds(10000);
    
    // Start both timers
    while(TRUE){
        
        uint32_t T1button_state = palReadPad(GPIO21_PORT, GPIO21_PAD);
        uint32_t T2button_state = palReadPad(GPIO20_PORT, GPIO20_PAD);
        uint32_t Rbutton_state = palReadPad(GPIO16_PORT, GPIO16_PAD);
        
        chThdSleepMilliseconds(100);
        
        if (!T1button_state){
            gptStop(&GPTD1);
            gptStartContinuous(&GPTD2, 10000);
        }
        

        else if (!T2button_state){
            gptStop(&GPTD2);
            gptStartContinuous(&GPTD1, 10000);
        }
        
        
        else if (!Rbutton_state){
            chThdSleepMilliseconds(50);
            gptStop(&GPTD1);
            gptStop(&GPTD2);
            callback_count_1 = 30000;
            callback_count_2 = 30000;
            timeToDisplay(rawDigits1, callback_count_1);
            timeToDisplay(rawDigits2, callback_count_2);
            
            chThdSleepMilliseconds(1000);

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
    chprintf((BaseSequentialStream *)&SD1, "BCM2835 Chess Timer: BE\r\n");

    /*
     * I2C initialization.
     */
    I2CConfig i2cConfig;
    i2cStart(&I2C1, &i2cConfig);
    
    // Initialize Displays
    init_display(display_address1);
    init_display(display_address2);
    
    
    // Initialize GPIO
    
    palSetPadMode(GPIO21_PORT, GPIO21_PAD, PAL_MODE_INPUT_PULLUP);
    palSetPadMode(GPIO20_PORT, GPIO20_PAD, PAL_MODE_INPUT_PULLUP);
    palSetPadMode(GPIO16_PORT, GPIO16_PAD, PAL_MODE_INPUT_PULLUP);
    
    /*
     * Serial General Purpose Timer (GPT) #1 initialization.
     */
    GPTConfig gptConfig1;
    gptConfig1.callback = gptcallback1;
    gptStart(&GPTD1, &gptConfig1);

    /*
     * Serial General Purpose Timer (GPT) #2 initialization.
     */
    GPTConfig gptConfig2;
    gptConfig2.callback = gptcallback2;
    gptStart(&GPTD2, &gptConfig2);
    
    
    /*
     * Creates the Display and timer thread.
     */
    
    chThdCreateStatic(waThreadDSP, sizeof(waThreadDSP), NORMALPRIO, ThreadDSP, NULL);
    
    chThdCreateStatic(waThreadTimer, sizeof(waThreadTimer), NORMALPRIO, ThreadTimer, NULL);


    

    /*
     * Events servicing loop.
     */
    chThdWait(chThdSelf());
    

    return 0;
}
