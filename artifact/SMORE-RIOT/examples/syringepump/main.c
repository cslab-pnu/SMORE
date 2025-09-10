#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "clk.h"
#include "cpu.h"
#include "periph/gpio.h"
#include "periph/pwm.h"
#include "periph/uart.h"
#include "xtimer.h"
#include "board.h"
#include "periph_conf.h"

/* -- Constants -- */
#define SYRINGE_VOLUME_ML 30.0
#define SYRINGE_BARREL_LENGTH_MM 80.0

#define THREADED_ROD_PITCH 1.25
#define STEPS_PER_REVOLUTION 200.0
#define MICROSTEPS_PER_STEP 16.0

#define SPEED_MICROSECONDS_DELAY 100 //longer delay = lower speed

/* Calculate steps per unit */
#define USTEPS_PER_MM (MICROSTEPS_PER_STEP * STEPS_PER_REVOLUTION / THREADED_ROD_PITCH)
#define USTEPS_PER_ML ((MICROSTEPS_PER_STEP * STEPS_PER_REVOLUTION * SYRINGE_BARREL_LENGTH_MM) / (SYRINGE_VOLUME_ML * THREADED_ROD_PITCH))

/* -- Pin definitions -- */
#define MOTOR_DIR_PIN GPIO_PIN(PORT_A, 2)  // Port A, Pin 2
#define MOTOR_STEP_PIN GPIO_PIN(PORT_A, 3) // Port A, Pin 3
#define TRIGGER_PIN GPIO_PIN(PORT_A, 5)    // Port A, Pin 5 (changed from Pin 3 to avoid conflict)
#define BIG_TRIGGER_PIN GPIO_PIN(PORT_A, 4) // Port A, Pin 4

/* -- GPIO modes -- */
#define GPIO_IN_PU GPIO_IN_PU
#define GPIO_OUT GPIO_OUT

/* -- Keypad states -- */
#define KEY_RIGHT 0
#define KEY_UP 1
#define KEY_DOWN 2
#define KEY_LEFT 3
#define KEY_SELECT 4
#define KEY_NONE 5

#define NUM_KEYS 5
static const int adc_key_val[5] = {30, 150, 360, 535, 760};

#define DWT_CYCCNT (*(uint32_t *) 0xe0001004)

/* -- Enums and constants -- */
enum {PUSH, PULL}; //syringe movement direction
enum {MAIN, BOLUS_MENU}; //UI states

const int mLBolusStepsLength = 9;
static const float mLBolusSteps[9] = {0.001, 0.005, 0.010, 0.050, 0.100, 0.500, 1.000, 5.000, 10.000};

/* -- Default Parameters -- */
float mLBolus = 0.500; //default bolus size
float mLBigBolus = 1.000; //default large bolus size
float mLUsed = 0.0;
int mLBolusStepIdx = 3; //0.05 mL increments at first
float mLBolusStep = 0.050; // Initial value, will be updated in main()

long stepperPos = 0; //in microsteps

//debounce params
long lastKeyRepeatAt = 0;
long keyRepeatDelay = 400;
long keyDebounce = 125;
int prevKey = KEY_NONE;
        
//menu stuff
int uiState = MAIN;

//triggering
uint8_t prevBigTrigger = 1;
uint8_t prevTrigger = 1;

//serial
char serialStr[40] = {0};
uint8_t serialStrReady = 0;
size_t serialStrLen = 0;

/* Function prototypes */
void checkTriggers(void);
void readSerial(void);
void processSerial(void);
void bolus(int direction);
void readKey(void);
void doKeyAction(unsigned int key);
void updateScreen(void);
int get_key(unsigned int input);
void decToString(float decNumber, char *buf, size_t size);

static void delay(unsigned d) {
	uint32_t loops = coreclk()*d / 20000;
	for (volatile uint32_t i=0; i<loops; i++) {}
}

void loop(int count) {
	mLBolus = 0.001 * count;
	serialStr[0] = '+';
	serialStrReady = true;

	enable_dwt();
  __asm__ volatile(
	"mov	r9, #0x8000	\n"
	"movt	r9, #0x2000	\n"
	"msr	psplim, r9	\n"
	"mov	r9, #0		\n"
	"msr	msplim, r9	\n"
	"movt	r9, #0x2003	\n"
	"msr	psp, r9		\n"
  );


	if (serialStrReady) {
		processSerial();
	}
  unsigned psplim, msplim, psp;
  __asm__ volatile(
	"mrs	%0, psplim	\n"
	"mrs	%1, msplim	\n"
	"mrs	%2, psp		\n"
	:"=r"(psplim), "=r"(msplim), "=r"(psp)::
  );
  printf("psplim: 0x%lx\tmsplim: %u\tpsp: %u\n", psplim, msplim, psp);
  printf("stack peak: %u\tplaceRedzone: %u\tshiftRedzone: %u\n", 0x20008000-psplim, msplim/8, (0x20030000-psp)/8);


	uint32_t clock = DWT_CYCCNT;
	printf("syringepump clock cycle: %u\n", clock);

}

int main(void) {
    printf("SyringePump v2.0\n");

    /* Initialize mLBolusStep */
    mLBolusStep = mLBolusSteps[mLBolusStepIdx];

    /* Triggering setup */
    gpio_init(TRIGGER_PIN, GPIO_IN_PU);
    gpio_init(BIG_TRIGGER_PIN, GPIO_IN_PU);
    
    /* Motor Setup */ 
    gpio_init(MOTOR_DIR_PIN, GPIO_OUT);
    gpio_init(MOTOR_STEP_PIN, GPIO_OUT);

	int count = 1;
	while (count < 62) {
		count += 10;
		loop(count);
	}

    /*for (int i=0; i<5; i++) {
        readKey();
        checkTriggers();
        readSerial();
        if(serialStrReady) {
            processSerial();
        }
    }*/
    return 0;
}

void checkTriggers(void) {
    //check low-reward trigger line
    int pushTriggerValue = gpio_read(TRIGGER_PIN);
    if(pushTriggerValue == 1 && prevTrigger == 0) {
        bolus(PUSH);
        updateScreen();
    }
    prevTrigger = pushTriggerValue;
    
    //check high-reward trigger line
    int bigTriggerValue = gpio_read(BIG_TRIGGER_PIN);
    if(bigTriggerValue == 1 && prevBigTrigger == 0) {
        //push big reward amount
        float mLBolusTemp = mLBolus;
        mLBolus = mLBigBolus;
        bolus(PUSH);
        mLBolus = mLBolusTemp;
        updateScreen();
    }
    prevBigTrigger = bigTriggerValue;
}

void processSerial(void) {
    if (serialStr[0] == '+') {
        bolus(PUSH);
        updateScreen();
    }
    else if (serialStr[0] == '-') {
        bolus(PULL);
        updateScreen();
    }
    else {
        int uLbolus = atoi(serialStr);
        if (uLbolus != 0) {
            mLBolus = (float)uLbolus / 1000.0;
            updateScreen();
        }
        else {
            printf("Invalid command: [%s]\n", serialStr);
        }
    }
    serialStrReady = 0;
    serialStrLen = 0;
}

void bolus(int direction) {
    long steps = (mLBolus * USTEPS_PER_ML);
    if(direction == PUSH) {
		LED0_TOGGLE;
        gpio_write(MOTOR_DIR_PIN, 1);
        steps = mLBolus * USTEPS_PER_ML;
        mLUsed += mLBolus;
    }
    else if(direction == PULL) {
		LED0_TOGGLE;
        gpio_write(MOTOR_DIR_PIN, 0);
        if((mLUsed-mLBolus) > 0) {
            mLUsed -= mLBolus;
        }
        else {
            mLUsed = 0;
        }
    }    

    float usDelay = SPEED_MICROSECONDS_DELAY;
    
    for(long i=0; i < steps; i++) { 
        gpio_write(MOTOR_STEP_PIN, 1);
		delay(usDelay);
        //xtimer_usleep(usDelay);
        gpio_write(MOTOR_STEP_PIN, 0);
		delay(usDelay);
        //xtimer_usleep(usDelay);
    }
}

void readKey(void) {
    // Simulate keypad input - always return KEY_RIGHT
    int key = KEY_RIGHT;

    long currentTime = xtimer_now_usec() / 1000; // Convert to milliseconds
    long timeSinceLastPress = (currentTime - lastKeyRepeatAt);
    
    uint8_t processThisKey = 0;
    if (prevKey == key && timeSinceLastPress > keyRepeatDelay) {
        processThisKey = 1;
    }
    if (prevKey == KEY_NONE && timeSinceLastPress > keyDebounce) {
        processThisKey = 1;
    }
    if (key == KEY_NONE) {
        processThisKey = 0;
    }  
    
    prevKey = key;
    
    if (processThisKey) {
        doKeyAction(key);
        lastKeyRepeatAt = currentTime;
    }
}

void doKeyAction(unsigned int key){
	if(key == KEY_NONE){
        return;
    }

	if(key == KEY_SELECT){
		if(uiState == MAIN){
			uiState = BOLUS_MENU;
		}
		else if(BOLUS_MENU){
			uiState = MAIN;
		}
	}

	if(uiState == MAIN){
		if(key == KEY_LEFT){
			bolus(PULL);
		}
		if(key == KEY_RIGHT){
			bolus(PUSH);
		}
		if(key == KEY_UP){
			mLBolus += mLBolusStep;
		}
		if(key == KEY_DOWN){
			if((mLBolus - mLBolusStep) > 0){
			  mLBolus -= mLBolusStep;
			}
			else{
			  mLBolus = 0;
			}
		}
	}
	else if(uiState == BOLUS_MENU){
		if(key == KEY_LEFT){
			//nothin'
		}
		if(key == KEY_RIGHT){
			//nothin'
		}
		if(key == KEY_UP){
			if(mLBolusStepIdx < mLBolusStepsLength-1){
				mLBolusStepIdx++;
				mLBolusStep = mLBolusSteps[mLBolusStepIdx];
			}
		}
		if(key == KEY_DOWN){
			if(mLBolusStepIdx > 0){
				mLBolusStepIdx -= 1;
				mLBolusStep = mLBolusSteps[mLBolusStepIdx];
			}
		}
	}

	updateScreen();
}

void updateScreen(void) {
    if (uiState == MAIN) {
        printf("\rUsed %.3f mL | Bolus %.3f mL", mLUsed, mLBolus);
    }
    else if (uiState == BOLUS_MENU) {
        printf("\rMenu> BolusStep: %.3f", mLBolusStep);
    }
    fflush(stdout);
}

// Convert ADC value to key number
int get_key(unsigned int input){
  int k;
  for (k = 0; k < NUM_KEYS; k++){
    if (input < adc_key_val[k]){
      return k;
    }
  }
  if (k >= NUM_KEYS){
    k = KEY_NONE;     // No valid key pressed
  }
  return k;
}

void decToString(float decNumber, char *buf, size_t size) {
    //not a general use converter! Just good for the numbers we're working with here.
    int wholePart = decNumber; //truncate
    int decPart = round(abs(decNumber*1000)-abs(wholePart*1000)); //3 decimal places
    char strZeros[4] = "";
    
    if(decPart < 10) {
        strcpy(strZeros, "00");
    }  
    else if(decPart < 100) {
        strcpy(strZeros, "0");
    }
    
    snprintf(buf, size, "%d.%s%d", wholePart, strZeros, decPart);
}

void readSerial(void) {
    if (scanf("%39s", serialStr) == 1) {
        serialStrReady = 1;
        serialStrLen = strlen(serialStr);
    }
}
