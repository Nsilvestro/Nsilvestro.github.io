/*
 * Copyright (c) 2015-2020, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== gpiointerrupt.c ========
 */
#include <stdint.h>
#include <stddef.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h> // I2C driver
#include <ti/drivers/UART2.h> // UART driver (question on UART vs UART2)
#include <ti/drivers/Timer.h> // Timer driver

#define DISPLAY(x) UART2_write(uart, &output, 64, x);

/* Driver configuration */
#include "ti_drivers_config.h"

// NS: added lines 51-77 to attempt to create task scheduler following zyBooks example
// NS: overall was able to acheive desired functionality with code below, attempted task
// scheduler included
//typedef struct task {
    //unsigned long period;
    //unsigned long elapsedTime;
    //void (*TickFct);
//} task;

//task tasks[3];
//const unsigned char tasksNum = 3;
//const unsigned long tasksPeriodGCD = 100000;
//const unsigned long periodButton = 200000;
//const unsigned long periodTemp = 500000;
//const unsgined long periodOutput = 1000000;

//void TickFct_Button();
//void TickFct_Temp();
//void TickFct_Output();

//void TimerISR() {
    //unsigned char i;
    //for (i =0; i < tasksNum; ++i) {
        //if (tasks[i].elapsedTime >= tasks[i].period) {
            //tasks[i] = tasks[i].TickFct();
            //tasks[i].elapsedTime = 0;
        //}
        //tasks[i].elapsedTime += tasksPEriodGCD;
        //}
//}

/*
 * I2C Global Variables
 */
static const struct {
    uint8_t address;
    uint8_t resultReg;
char *id;
} sensors[3] = {
                { 0x48, 0x0000, "11X" },
                { 0x49, 0x0000, "116" },
                { 0x41, 0x0001, "006" }
};
uint8_t txBuffer[1];
uint8_t rxBuffer[2];
I2C_Transaction i2cTransaction;

// Driver Handles - Global variables
I2C_Handle i2c;


/*
 * UART Global Variables
 */
char output[64];
int bytesToSend;
// Driver Handles - Global variables
UART2_Handle uart;


/*
 * Timer Global Variables
 */
// Driver Handles - Global variables
Timer_Handle timer0;

volatile unsigned char TimerFlag = 0;
void timerCallback(Timer_Handle myHandle, int_fast16_t status)
{
    TimerFlag = 1;
}


// NS Button flags
volatile unsigned char ButtonFlagIncrease = 0;
volatile unsigned char ButtonFlagDecrease = 0;

void buttonCallbackIncrease(uint_least8_t index)
{
    ButtonFlagIncrease = 1;
}
void buttonCallbackDecrease(uint_least8_t index)
{
    ButtonFlagDecrease = 1;
}


// Initialize I2C
// Make sure you call initUART() before calling this function.
void initI2C(void)
{
    int8_t i, found;
    I2C_Params i2cParams;
    DISPLAY(snprintf(output, 64, "Initializing I2C Driver - "))

    // Init the driver
    I2C_init();

    // Configure the driver
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;

    // Open the driver
    i2c = I2C_open(CONFIG_I2C_0, &i2cParams);
    if (i2c == NULL)
    {
        DISPLAY(snprintf(output, 64, "Failed\n\r"))
        while (1);
    }

    DISPLAY(snprintf(output, 32, "Passed\n\r"))

    // Boards were shipped with different sensors.
    // Welcome to the world of embedded systems.
    // Try to determine which sensor we have.
    // Scan through the possible sensor addresses
    /* Common I2C transaction setup */
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = rxBuffer;
    i2cTransaction.readCount = 0;

    found = false;
    for (i=0; i<3; ++i)
    {
        i2cTransaction.targetAddress = sensors[i].address;
        txBuffer[0] = sensors[i].resultReg;

    DISPLAY(snprintf(output, 64, "Is this %s? ", sensors[i].id))
    if (I2C_transfer(i2c, &i2cTransaction))
    {
    DISPLAY(snprintf(output, 64, "Found\n\r"))
    found = true;
        break;
        }
        DISPLAY(snprintf(output, 64, "No\n\r"))
    }

    if(found)
    {
        DISPLAY(snprintf(output, 64, "Detected TMP%s I2C address: %x\n\r", sensors[i].id, i2cTransaction.targetAddress))
    }
    else
    {
        DISPLAY(snprintf(output, 64, "Temperature sensor not found, contact professor\n\r"))
    }
}


// Function to read temperature
int16_t readTemp(void)
{
    int j;
    int16_t temperature = 0;
    i2cTransaction.readCount = 2;

    if (I2C_transfer(i2c, &i2cTransaction))
    {
        /*
         * Extract degrees C from the received data;
         * see TMP sensor datasheet
         */
        temperature = (rxBuffer[0] << 8) | (rxBuffer[1]);
        temperature *= 0.0078125;

        /*
         * If the MSB is set '1', then we have a 2's complement
         * negative value which needs to be sign extended
         */
        if (rxBuffer[0] & 0x80)
        {
            temperature |= 0xF000;
        }
    }
    else
    {
        DISPLAY(snprintf(output, 64, "Error reading temperature sensor (%d)\n\r",i2cTransaction.status))
        DISPLAY(snprintf(output, 64, "Please power cycle your board by unplugging USB and plugging back in.\n\r"))
    }
    return temperature;
}


// Initialize UART
void initUART(void)
{
    UART2_Params uartParams;

    // Init the driver
    UART2_Params_init(&uartParams);

    // Configure the driver
    UART2_Params_init(&uartParams);
    uartParams.writeMode = UART2_Mode_BLOCKING;
    uartParams.readMode = UART2_Mode_BLOCKING;
    uartParams.readReturnMode = UART2_ReadReturnMode_FULL;
    uartParams.baudRate = 115200;

    // Open the driver
    uart = UART2_open(CONFIG_UART2_0, &uartParams);

    if (uart == NULL)
    {
        /* UART_open() failed */
        while (1);
    }
}


// Initialize Timer
void initTimer(void)
{
    Timer_Params params;

    // Init the driver
    Timer_init();

    // Configure the driver
    Timer_Params_init(&params);
    params.period = 100000; // NS set timer period to 100,000 microseconds(100ms) -> GCD of 200ms, 500ms
    params.periodUnits = Timer_PERIOD_US;
    params.timerMode = Timer_CONTINUOUS_CALLBACK;
    params.timerCallback = timerCallback;

    // Open the driver
    timer0 = Timer_open(CONFIG_TIMER_0, &params);
    if (timer0 == NULL)
    {
        /* Failed to initialized timer */
        while (1) {}
    }

    if (Timer_start(timer0) == Timer_STATUS_ERROR)
    {
        /* Failed to start timer */
        while (1) {}
    }
}


/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    /* Call driver init functions */
    GPIO_init();
    initUART();
    initI2C();
    initTimer();

    // NS: Following zybooks example set tasks
    //unsigned char i = 0;
    //tasks[i].period = 200000;
    //tasks[i].elapsedTime = 200000;
    //tasks[i].TickFct = &TickFct_Button;
    //i++;
    //tasks[i].period = 500000;
    //tasks[i].elapsedTime = 500000;
    //tasks[i].TickFct = &TickFct_Temp;
    //i++;
    //tasks[i].period = 1000000;
    //tasks[i].elapsedTime = 1000000;
    //tasks[i].TickFct = &TickFct_Output;

    // NS added times for checks on button/temp flags, output
    unsigned long button_elapsedTime = 200000;
    unsigned long temp_elapsedTime = 500000;
    unsigned long outputInfo_elapsedTime = 1000000;
    //int timerCounter = 0;
    const unsigned long timerPeriod = 100000;

    // NS added a starting temp set point
    unsigned long tempSetPoint = readTemp();
    int16_t currentTemp = 0;

    /*
     * NOTE TO SELF: had added button flags, callbacks like timer flag/callback above
     * had just added elapsed times above, need to add conditionals in while loop
     * below to check for elapsed times, add functionality if needed to callbacks
     * -> then review next section if works on how to switch to the for loop for multiple
     * tasks
     */


    /* Configure the LED and button pins */
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    //GPIO_setConfig(CONFIG_GPIO_LED_1, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_BUTTON_0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

    /* Turn off user LEDs */
    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
    //GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);

    /* Install Button callback */
    GPIO_setCallback(CONFIG_GPIO_BUTTON_0, buttonCallbackIncrease);

    /* Enable interrupts */
    GPIO_enableInt(CONFIG_GPIO_BUTTON_0);

    /*
     *  If more than one input pin is available for your device, interrupts
     *  will be enabled on CONFIG_GPIO_BUTTON1.
     */
    if (CONFIG_GPIO_BUTTON_0 != CONFIG_GPIO_BUTTON_1)
    {
        /* Configure BUTTON1 pin */
        GPIO_setConfig(CONFIG_GPIO_BUTTON_1, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

        /* Install Button callback */
        GPIO_setCallback(CONFIG_GPIO_BUTTON_1, buttonCallbackDecrease);
        GPIO_enableInt(CONFIG_GPIO_BUTTON_1);
    }

    while(1) {
        //Sleep(); -> for zybooks example

        // Check button flags every 200ms
        if (button_elapsedTime >= 200000) { // 200ms period
            if (ButtonFlagIncrease == 1) { // NS:If increase temp button pushed
                tempSetPoint += 1;         // NS:Raise temp set point
                ButtonFlagIncrease = 0;
            }
            if (ButtonFlagDecrease == 1) { // NS:If decrease temp button pushed
                tempSetPoint -= 1;         // NS:Decrease temp set point
                ButtonFlagDecrease = 0;
            }

            button_elapsedTime = 0;
        }

        // Read temp, update LED every 500ms
        if (temp_elapsedTime >= 500000) { // 500ms period
            currentTemp = readTemp();

            if (currentTemp > tempSetPoint) { //NS: If temp greater than set point, turn off system (LED)
                GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
            }
            else if (currentTemp <= tempSetPoint) { //NS: If temp less than set point, turn on system (LED)
                GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
            }

            temp_elapsedTime = 0;
        }


        // Every second output following to UART
        if (outputInfo_elapsedTime >= 1000000) { // 1s period
            //DISPLAY(snprintf(output, 64, "<%02d, %02d, %d, %04d>\n\r", temperature, setpoint, heat, seconds))
            float seconds = 1.0000; // NS:Issues with DISPLAY and current format, attempted to provide float with change d -> f
            int heat = 0;
            if(currentTemp <= tempSetPoint) {
                heat = 1;
            }
            else {
                heat = 0;
            }
            DISPLAY(snprintf(output, 64, "<%02d, %02d, %d, %04f>\n\r", currentTemp, tempSetPoint, heat, seconds))

            outputInfo_elapsedTime = 0;
        }
        // Review zybooks converting different period tasks to c
        // Configure timer period above
        while (!TimerFlag) {} // Wait for timer period
        TimerFlag = 0; // Lower flag
        ++TimerFlag;

        button_elapsedTime += timerPeriod;
        temp_elapsedTime += timerPeriod;
        outputInfo_elapsedTime += timerPeriod;
        //timerCounter += timerPeriod;
    }

    return (NULL);
    //}
}
//NS: following zybooks example task scheduler -> they use switch cases similiar to state machines
// I did not believe I would need states from the code above, would move respective conditionals to
// tick functions
// void TickFct_Button() {
//}
// void TickFct_Temp() {
//}
// void TickFct_Output() {
//}
