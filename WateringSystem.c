/* 
 * File:   Tutorial.c
 * Author: rs
 *
 * Created on 25 de Setembro de 2017, 22:19
 */



//-----------------------------------------------------------------------------
// INCLUDES
//-----------------------------------------------------------------------------
#include <xc.h>

// BEGIN CONFIG
#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT enabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable bit (BOR enabled)
#pragma config LVP = OFF        // Low-Voltage (Single-Supply) In-Circuit Serial Programming Enable bit (RB3 is digital I/O, HV on MCLR must be used for programming)
#pragma config CPD = OFF        // Data EEPROM Memory Code Protection bit (Data EEPROM code protection off)
#pragma config WRT = OFF        // Flash Program Memory Write Enable bits (Write protection off; all program memory may be written to by EECON control)
#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)
//END CONFIG




//-----------------------------------------------------------------------------
// Global and system defines
//-----------------------------------------------------------------------------
#define _XTAL_FREQ   4000000 

static const char MENU_CONFIGURACOES[] = "CONFIG";
static const char MENU_MANUAL[] = "MANUAL";
static const char MENU_ZONA[] = "ZONE";
static const char MENU_HORA[] = "HOUR";
static const char MENU_DURACAO[] = "DURATION";
static const char MENU_HUMIDADE[] = "HUMIDITY";
static const char MENU_ESTADO[] = "STATE";
static const char MENU_LIGAR[] = "ENABLE";



//-----------------------------------------------------------------------------
// STATES
//-----------------------------------------------------------------------------
typedef enum {
    WATERING_SETUP,
    WATERING_IDLE,
    WATERING_ON
} WATERINGSTATES;

typedef enum {
    FIRST_MENU,
    MANUAL_MENU,
    CONFIG_MENU
            
} MENUSTATES;



//-----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//-----------------------------------------------------------------------------
void HardwareConfiguration(void);                   
void SoftwareConfiguration(void);
void Init1msecTimerInterrupt(void);
void UpdateTimeCounters(void);
void LedProcess(void);
void InitExternal_INT(void);
void ProcessWATERINGStates(void);
void ProcessMENUStates(void);


//-----------------------------------------------------------------------------
// STRUCTURES
//-----------------------------------------------------------------------------
typedef struct {
    unsigned int enable;              // ENABLE/DISABLE 
    unsigned int beginTIME;           //START TIME - HOUR
    unsigned int duration;            //DURATION IN MINUTES
    unsigned int humidityLIMIT;       //HUMIDITY LIMIT TO START WATERING
} ZONE;

//-----------------------------------------------------------------------------
// GLOBAL VARIABLES
//-----------------------------------------------------------------------------
ZONE Z1;
ZONE Z2;
ZONE Z3;
ZONE Z4;


char bootDONE = 0;          //BOOT FLAG - 1 WHEN BOOT DONE

int LED_ON = 0;             //LED DEVICE ON STATE
int LED_Z1 = 0;             //LED ZONE 1 WATERING STATE
int LED_Z2 = 0;             //LED ZONE 2 WATERING STATE
int LED_Z3 = 0;             //LED ZONE 3 WATERING STATE
int LED_Z4 = 0;             //LED ZONE 4 WATERING STATE

int STATE_Z1 = 0;           //ZONE 1 RELAY STATE
int STATE_Z2 = 0;           //ZONE 2 RELAY STATE
int STATE_Z3 = 0;           //ZONE 3 RELAY STATE
int STATE_Z4 = 0;           //ZONE 4 RELAY STATE

unsigned int msCounter = 0;             //MILISECOND COUNTER
unsigned int secCounter = 0;            //SECOND COUNTER
unsigned int minCounter = 0;            //MINUTE COUNTER
unsigned int hrCounter = 0;             //HOUR COUNTER
unsigned int dayCounter = 0;            //DAY COUNTER

unsigned int timer20ms = 0;             //PROCESSING STATES COUNTER - RESTART EVERY 20 MILISECONDS

WATERINGSTATES wateringSTATES = WATERING_SETUP;             //WATERING STATE AFTER BOOT
MENUSTATES menuSTATES = FIRST_MENU;                         //MENU STATE AFTER BOOT



int main() {
    HardwareConfiguration();                //HARDWARE CONFIGURATION
    SoftwareConfiguration();                //SOFTWARE CONFIGURATION
    Init1msecTimerInterrupt();
    InitExternal_INT();
    
    
    while (1){
        
        
        LedProcess();               //PROCESS LED STATES - TURN ON/OFF ACCORDING TO FLAGS
        
        
        
        //----------------------------------------------------------------------
        // STATE MACHINE EVERY 20 MILISECONDS
        //----------------------------------------------------------------------
        if(timer20ms == 20){
            ProcessWATERINGStates();            //PROCESS WATERING STATE
            ProcessMENUStates();                //PROCESS MENU STATE
            timer20ms=0;
        }
        
        
        //----------------------------------------------------------------------
        // RUN ONE TIME 1 SECOND AFTER BOOT
        //----------------------------------------------------------------------
        if(msCounter == 1000 && !bootDONE){
            bootDONE = 1;           //SET BOOT FLAG
            
            LED_Z1 = 0;             //TURN OFF ZONE 1 LED
            LED_Z2 = 0;             //TURN OFF ZONE 2 LED
            LED_Z3 = 0;             //TURN OFF ZONE 3 LED
            LED_Z4 = 0;             //TURN OFF ZONE 4 LED
        }
        
       
        UpdateTimeCounters();       //UPDATE TIME
    }
    
    
    return (1);
}


void HardwareConfiguration(void){
    PORTD = 0x00 ;
    TRISD = 0x00 ;
    
    TRISB = 0xFF;
    
    
    LED_ON = 1;         //TURN DEVICE STATE LED ON
    LED_Z1 = 1;         //TURNO ZONE 1 STATE LED ON
    LED_Z2 = 1;         //TURNO ZONE 2 STATE LED ON
    LED_Z3 = 1;         //TURNO ZONE 3 STATE LED ON
    LED_Z4 = 1;         //TURNO ZONE 4 STATE LED ON

}


void SoftwareConfiguration(void){
    
    //ZONE 1 SETTINGS
    Z1.enable = 0;                   //START DISABLED
    Z1.duration = 30;                //DEFAULT DURATION - 30 MIN
    Z1.beginTIME = 1;                //DEFAULT START TIME - 1 AM 
    Z1.humidityLIMIT = 100;          //DEFAULT HUMIDITY LIMIT - 100% - ALLWAYS ON
    
    //ZONE 2 SETTINGS
    Z2.enable = 0;
    Z2.duration = 1;
    Z2.beginTIME = 1;
    Z2.humidityLIMIT = 100;
    
    //ZONE 3 SETTINGS
    Z3.enable = 0;
    Z3.duration = 1;
    Z3.beginTIME = 1;
    Z3.humidityLIMIT = 100;
    
    //ZONE 4 SETTINGS
    Z4.enable = 0;
    Z4.duration = 1;
    Z4.beginTIME = 1;
    Z4.humidityLIMIT = 100;
    
}


//----------------------------------------------------------------------
// WATERING STATE MACHINE
//----------------------------------------------------------------------
void ProcessWATERINGStates(void){
    switch(wateringSTATES){
        case(WATERING_SETUP): 
            wateringSTATES=WATERING_IDLE;
            break;
        case(WATERING_IDLE): 
            
            break;
        case(WATERING_ON): break;
    }
}

//----------------------------------------------------------------------
// MENU STATE MACHINE
//----------------------------------------------------------------------
void ProcessMENUStates(void){
    switch(menuSTATES){
        case(FIRST_MENU): 
            break;
        case(MANUAL_MENU): 
            
            break;
        case(CONFIG_MENU): break;
    }
}






//----------------------------------------------------------------------
// LED PROCESSING - TURN ON/OFF LEDS ACCORDING TO FLAGS
//----------------------------------------------------------------------
void LedProcess(void){
    RD0 = LED_ON;       
    RD1 = LED_Z1;
    RD2 = LED_Z2;
    RD3 = LED_Z3;
    RD4 = LED_Z4;
}



//----------------------------------------------------------------------
// INITIATE EXTERNAL INTERRUPT
//----------------------------------------------------------------------
void InitExternal_INT(void){
    
    INTCON |= 0X90;
    OPTION_REG |= 0X40;
}



void interrupt ISR(void){
    
    //TIME INTERRUPT - EVERY 1 MILISECOND
    if(T0IF){
        TMR0 = 0x08;
        T0IF = 0;
        msCounter++;
        timer20ms++;
    }
    
    //EXTERNAL INTERRUPT - WHEN PUSH BUTTON PRESSED
    if(INTF){
        
        //UP PUSH BUTTON PRESSED
        if(RB1){
            //LED_Z1 = 1;
        }
        //DOWN PUSH BUTTON PRESSED
        if(RB2){
            //LED_Z2 = 1;
        }
        //GO BACK PUSH BUTTON PRESSED
        if(RB3){
            //LED_Z3 = 1;
        }
        //ENTER PUSH BUTTON PRESSED
        if(RB4){
            //LED_Z4 = 1;
        }
        
        
        
        
        
        INTF = 0;       //RESTART EXTERNAL INTERRUPT FLAG
    }
}


//----------------------------------------------------------------------
// INITIATE TIMER INTERRUPT
//----------------------------------------------------------------------
void InitTimer0(void){
    OPTION_REG &= 0xC1;
    
    T0IE = 1;
    GIE = 1;
}

void Init1msecTimerInterrupt(void){
    InitTimer0();
}


//----------------------------------------------------------------------
// UPDATE TIME VARIABLES
//----------------------------------------------------------------------
void UpdateTimeCounters(void)
{
	if (msCounter==1000)
	{
		secCounter++;
		msCounter=0;
	}

	if(secCounter==60)
	{
		minCounter++;
		secCounter=0;
	}

	if(minCounter==60)
	{
		hrCounter++;
		minCounter=0;
	}

	if(hrCounter==24)
	{
		hrCounter = 0;
        dayCounter++;
	}
    if(dayCounter==6)
	{
        dayCounter = 0;
	}
}





