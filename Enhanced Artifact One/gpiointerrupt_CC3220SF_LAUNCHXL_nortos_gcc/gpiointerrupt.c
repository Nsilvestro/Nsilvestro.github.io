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

/* Defined tasks function structure */
typedef void (*TaskFct)(void);

/* Task scheduler task structures */
typedef struct task {
    unsigned long period;       // The task period
    unsigned long elapsedTime;  // Elapsed time since task
    TaskFct taskFct;
} task;

/* Creating a list of tasks */
int tasksNum = 3;
task tasks[3];

/* Global task timer variables */
const unsigned long tasksPeriodGCD = 100000;
const unsigned long buttonCheck_Period = 200000;
const unsigned long compareTemperatures_Period = 500000;
const unsigned long outputData_Period = 1000000;

/* Global temperature variables */
unsigned long tempSetPoint;
int16_t currentTemp;


/*
 * I2C Peripheral: Global Variables
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

I2C_Handle i2c; // Driver Handles


/*
 * UART Peripheral: Global Variables
 */
char output[64];
int bytesToSend;
UART2_Handle uart; // Driver Handles


/*
 * Timer: Global Variables
 */
Timer_Handle timer0; // Driver Handles
volatile unsigned char TimerFlag = 0;

/* Timer callback function (manipulates flag) */
void timerCallback(Timer_Handle myHandle, int_fast16_t status)
{
    TimerFlag = 1;
}


/* Buttons: Global Variables */
volatile unsigned char ButtonFlagIncrease = 0;
volatile unsigned char ButtonFlagDecrease = 0;

/* Button callback functions (manipulates flag) */
void buttonCallbackIncrease(uint_least8_t index)
{
    ButtonFlagIncrease = 1;
}
void buttonCallbackDecrease(uint_least8_t index)
{
    ButtonFlagDecrease = 1;
}


/*
 * Initialize I2C Peripheral
 * Make sure to call initUART() before calling this function.
 */
void initI2C(void)
{
    int8_t i, found;
    I2C_Params i2cParams;
    DISPLAY(snprintf(output, 64, "Initializing I2C Driver - "))

    /* Init the driver */
    I2C_init();

    /* Configure the driver */
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;

    /* Open the driver */
    i2c = I2C_open(CONFIG_I2C_0, &i2cParams);
    if (i2c == NULL)
    {
        DISPLAY(snprintf(output, 64, "Failed\n\r"))
        while (1);
    }

    DISPLAY(snprintf(output, 32, "Passed\n\r"))

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


/*
 * Function to read temperature via I2C
 */
int16_t readTemp(void)
{
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


/*
 * Initialize UART Peripheral
 */
void initUART(void)
{
    UART2_Params uartParams;

    /* Init the driver */
    UART2_Params_init(&uartParams);

    /* Configure the driver */
    UART2_Params_init(&uartParams);
    uartParams.writeMode = UART2_Mode_BLOCKING;
    uartParams.readMode = UART2_Mode_BLOCKING;
    uartParams.readReturnMode = UART2_ReadReturnMode_FULL;
    uartParams.baudRate = 115200;

    /* Open the driver */
    uart = UART2_open(CONFIG_UART2_0, &uartParams);

    if (uart == NULL)
    {
        /* UART_open() failed */
        while (1);
    }
}


/*
 * Initialize Timer
 */
void initTimer(void)
{
    Timer_Params params;

    /* Init the driver */
    Timer_init();

    /* Configure the driver */
    Timer_Params_init(&params);
    params.period = 100000; // Set timer period to 100,000 microseconds(100ms) -> GCD of 200ms, 500ms
    params.periodUnits = Timer_PERIOD_US;
    params.timerMode = Timer_CONTINUOUS_CALLBACK;
    params.timerCallback = timerCallback;

    /* Open the driver */
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
 * Task Function: button check
 */
void buttonCheck(void) { ////CHANGED TO VOID FROM UNSIGNED LONG IF ISSUE
    if (ButtonFlagIncrease == 1) { // If increase temp button pushed
        tempSetPoint += 1;         // Raise temp set point
        ButtonFlagIncrease = 0;
    }
    if (ButtonFlagDecrease == 1) { // If decrease temp button pushed
        tempSetPoint -= 1;         // Decrease temp set point
        ButtonFlagDecrease = 0;
    }
}


/*
 * Task Function: compare temperatures
 */
void compareTemperatures(void) {
    currentTemp = readTemp();

    if (currentTemp > tempSetPoint) {                       //If temp greater than set point, turn off system (LED)
        GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
    }
    else if (currentTemp < tempSetPoint) {                  //If temp less than set point, turn on system (LED)
        GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
    }
    else {
        GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
    }
}


/*
 * Task Function: output data
 */
void outputData(void) {
    float seconds = 1.0000; // NS:Issues with DISPLAY and current format, attempted to provide float with change d -> f
    int heat = 0;
    currentTemp = readTemp();
    if(currentTemp < tempSetPoint) {
        heat = 1;
    }
    else {
        heat = 0;
    }
    DISPLAY(snprintf(output, 64, "<%02d, %02d, %d, %04f>\n\r", currentTemp, tempSetPoint, heat, seconds))
}


/*
 * Initialize tasks list with tasks
 */
void initTasks(void) {
    unsigned char i = 0;

    /* Button check task */
    tasks[i].period = buttonCheck_Period;
    tasks[i].elapsedTime = 200000;
    tasks[i].taskFct = &buttonCheck;
    i++;

    /* Compare temperatures task */
    tasks[i].period = compareTemperatures_Period;
    tasks[i].elapsedTime = 500000;
    tasks[i].taskFct = &compareTemperatures;
    i++;

    /* Output data task */
    tasks[i].period = outputData_Period;
    tasks[i].elapsedTime = 1000000;
    tasks[i].taskFct = &outputData;
}


/*
 * Task scheduler main function
 * Cycles through tasks in list
 * If enough time has elapsed, calls respective function
 */
void taskScheduler(void) {
    for (unsigned char i =0; i < tasksNum; i++) {
        if (tasks[i].elapsedTime >= tasks[i].period) {
            tasks[i].taskFct();
            tasks[i].elapsedTime = 0;
        }
        tasks[i].elapsedTime += tasksPeriodGCD;
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

    /* System temperature set point */
    tempSetPoint = readTemp();

    /* Configure the LED and button pins */
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_BUTTON_0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

    /* Turn off user LEDs */
    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);

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
        initTasks();
        taskScheduler();

        /* Wait for timer period */
        while (!TimerFlag) {}
        TimerFlag = 0; // Lower flag
        ++TimerFlag;

        //timerCounter += timerPeriod;
    }
    return (NULL);
}

