#include "o3.h"
#include "gpio.h"
#include "systick.h"

#define LED_PORT GPIO_PORT_E
#define LED_PIN 2
#define PB0_PORT GPIO_PORT_B
#define PB0_PIN 9
#define PB1_PORT GPIO_PORT_B
#define PB1_PIN 10

#define SET_SECONDS 0
#define SET_MINUTES 1
#define SET_HOURS 2
#define COUNTDOWN 3
#define ALARM 4

/**************************************************************************//**
 * @brief Konverterer nummer til string
 * Konverterer et nummer mellom 0 og 99 til string
 *****************************************************************************/
void int_to_string(char *timestamp, unsigned int offset, int i) {
    if (i > 99) {
        timestamp[offset]   = '9';
        timestamp[offset+1] = '9';
        return;
    }

    while (i > 0) {
	    if (i >= 10) {
		    i -= 10;
		    timestamp[offset]++;

	    } else {
		    timestamp[offset+1] = '0' + i;
		    i=0;
	    }
    }
}

/**************************************************************************//**
 * @brief Konverterer 3 tall til en timestamp-string
 * timestamp-argumentet må være et array med plass til (minst) 7 elementer.
 * Det kan deklareres i funksjonen som kaller som "char timestamp[7];"
 * Kallet blir dermed:
 * char timestamp[7];
 * time_to_string(timestamp, h, m, s);
 *****************************************************************************/
void time_to_string(char *timestamp, int h, int m, int s) {
    timestamp[0] = '0';
    timestamp[1] = '0';
    timestamp[2] = '0';
    timestamp[3] = '0';
    timestamp[4] = '0';
    timestamp[5] = '0';
    timestamp[6] = '\0';

    int_to_string(timestamp, 0, h);
    int_to_string(timestamp, 2, m);
    int_to_string(timestamp, 4, s);
}

/*Initialaze the register structs*/
gpio_map_t* gpio_pointer = (gpio_map_t *)GPIO_BASE;
systick_map_t* systick_pointer = (systick_map_t *)SYSTICK_BASE;

/*Initialize a time struct with all values at zero*/
count_time_t time_struct = {0,0,0};

/*Initialize a string to be manipulated by time_to_string*/
char string[8] = "0000000\0";

/*Initialize state*/
int state = SET_SECONDS;

void set4bitValue(volatile word *reg, int position, int value){
    /*
    * Call register (reg) by reference
    * Eg
    * From this:
    * xxxx xxxx xxxx xxxx xxxx xxxx xxxx
    * To this:
    * xxxx xxxx xxxx xxxx 0010 xxxx xxxx
    */

    /*
    * Start by reseting the 4-bits in position to 0000
    * ~(0b1111 << position*4);
    * Eg
    * 1111 1111 1111 1111 0000 1111 1111
    * And bitwise AND this into the register
    * register is now Eg
    * xxxx xxxx xxxx xxxx 0000 xxxx xxxx
    */
    *reg &= ~(0b1111 << position*4);
    /*
    * Now we insert the value into the position
    * by bitshifting the value by position*4
    * and bitwise OR into the register
    */
    *reg |= value << position*4;
    /*register is now Eg
    * xxxx xxxx xxxx xxxx 0010 xxxx xxxx
    * Success!
    */
}

void initialize_IO(void){
    /*
    * Set input and output mode
    */
    set4bitValue(&gpio_pointer->ports[LED_PORT].MODEL, LED_PIN, GPIO_MODE_OUTPUT);
    set4bitValue(&gpio_pointer->ports[PB0_PORT].MODEH, PB0_PIN-8, GPIO_MODE_INPUT);
    set4bitValue(&gpio_pointer->ports[PB1_PORT].MODEH, PB1_PIN-8, GPIO_MODE_INPUT);

    /*
    * Set up interrupt on PB0
    */
    set4bitValue(&gpio_pointer->EXTIPSELH, PB0_PIN-8, 0b0001);
    gpio_pointer->EXTIFALL |= 1 << PB0_PIN;
    gpio_pointer->IEN |= 1 << PB0_PIN;

    /*
    * Set up interrupt on PB1
    */
    set4bitValue(&gpio_pointer->EXTIPSELH, PB1_PIN-8, 0b0001);
    gpio_pointer->EXTIFALL |= 1 << PB1_PIN;
    gpio_pointer->IEN |= 1 << PB1_PIN;

    /*
    * Set up SYSTICK
    */
    systick_pointer->LOAD = FREQUENCY;
    systick_pointer->CTRL = SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_CLKSOURCE_Msk;
    systick_pointer->CTRL |= SysTick_CTRL_TICKINT_Msk;
}

void GPIO_ODD_IRQHandler(void){
   switch(state){
        case SET_SECONDS:
            incrementSeconds();
            break;

        case SET_MINUTES:
            incrementMinutes();
            break;

        case SET_HOURS:
            incrementHours();
            break;

        case COUNTDOWN:
            break;

        case ALARM:
            break;
   }
    gpio_pointer->IFC |= 1 << PB0_PIN;
}

void GPIO_EVEN_IRQHandler(void){
   switch(state){
        case SET_SECONDS:
            state = SET_MINUTES;
            break;

        case SET_MINUTES:
            state = SET_HOURS;
            break;

        case SET_HOURS:
            state = COUNTDOWN;
            break;

        case COUNTDOWN:
            break;

        case ALARM:
            gpio_pointer->ports[LED_PORT].DOUTCLR |= 1 << LED_PIN;
            state = SET_SECONDS;
            break;
   }
    gpio_pointer->IFC |= 1 << PB1_PIN;
}

void SysTick_Handler(void){
    if(state == COUNTDOWN){
        decrementTime();
    }
}

void updateDisplay(void){
    time_to_string(string,time_struct.h,time_struct.m,time_struct.s);
    lcd_write(string);
}

void decrementTime(void){
    if(time_struct.s <= 0){
        if(time_struct.m <= 0){
            if(time_struct.h <= 0){
                updateDisplay();
                state = ALARM;
                gpio_pointer->ports[LED_PORT].DOUTSET |= 1 << LED_PIN;
                return;
            }
            time_struct.h--;
            time_struct.m = 59;
        }
        time_struct.m--;
        time_struct.s = 59;
    }else{
        time_struct.s--;
    }
    updateDisplay();
}

void incrementSeconds(void){
    if(time_struct.s >= 60){
        time_struct.s = 0;
        incrementMinutes();
    }else{
        time_struct.s++;
        updateDisplay();
    }
}

void incrementMinutes(void){
    if(time_struct.m >= 60){
        time_struct.m = 0;
        incrementHours();
    }else{
        time_struct.m++;
        updateDisplay();
    }
}

void incrementHours(void){
    if(time_struct.h >= 99){
        time_struct.h = 0;
        time_struct.m = 0;
        time_struct.s = 0;
        updateDisplay();
    }else{
        time_struct.h++;
        updateDisplay();
    }
}

int main(void) {
    init();
    initialize_IO();
    updateDisplay();

    return 0;
}
