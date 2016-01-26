/*
 * lab4.c
 *
 * Created: 10/8/2015 2:47:02 PM
 * Author : Bahram Banisadr
 */ 

/************************************************************
Header
************************************************************/
#include <avr/io.h>
#include "m_general.h"
#include "m_bus.h"
#include "m_rf.h"
#include "m_usb.h"

/************************************************************
Definitions
************************************************************/
#define CHANNEL 1
#define RXADDRESS 0x7C
#define PACKET_LENGTH 3
#define POINTS 100
#define STABLEMAX 100
#define SYSTEM_SPEED 16000000
#define TIMER_ONE_PRESCALER 8
#define TIMER_THREE_PRESCALER 1024
#define FREQ 180

/************************************************************
Prototype Functions
************************************************************/
void init(void);
void enable_timer_one(void);
void enable_timer_three(void);
void enable_wireless(void);
void enable_usb(void);

/************************************************************
Global Variables
************************************************************/
char buffer[PACKET_LENGTH] = {0,0,0}; // Wifi input
int counter = 0; // Tracks location in sine table
volatile int duration = 0; // Play duration
char duration_buffer[2] = {0,0}; // Used to convert duration to int
float frequency = 180; // frequency to play
int sTable[POINTS] = // Sine table: values: 100, max = 100
   {50,53,56,59,62,65,68,71,
	   74,77,79,82,84,86,89,90,
	   92,94,95,96,98,98,99,100,
	   100,100,100,100,99,98,98,96,
	   95,94,92,90,89,86,84,82,
	   79,77,74,71,68,65,62,59,
	   56,53,50,47,44,41,38,35,
	   32,29,26,23,21,18,16,14,
	   11,10,8,6,5,4,2,2,
	   1,0,0,0,0,0,1,2,
	   2,4,5,6,8,10,11,14,
	   16,18,21,23,26,29,32,35,
	   38,41,44,47};

/************************************************************
Main Loop
************************************************************/
int main(void)
{
	/* Confirm Power */
	m_red(ON);

	/* Initializations */
	init(); // Set pins, clock speed, enable interrupts 
	enable_timer_one(); // Configure timer one
	enable_timer_three(); // Configure timer three
	enable_wireless(); // Configure wireless module
	//enable_usb(); // Configure USB Communications

	/* Confirm successful initialization(s) */
	m_green(ON);
	m_red(OFF);

	/* Run */
	while (1){}
}

/************************************************************
Initialization of Subsystem Components
************************************************************/

/* Initialization of Pins and System Clock */
void init(void){
	
	m_clockdivide(0); // Set to 16 MHz
	
	set(DDRB,6); // Set B6 to output
	set(DDRB,0); // Set B0 to output
	
	sei(); // Enable global interrupts
}

/* Initialization of Timer1 */
void enable_timer_one(void){

	OCR1A = SYSTEM_SPEED/TIMER_ONE_PRESCALER/FREQ/POINTS; // Set OCR1A for initial Hz
	OCR1B = OCR1A/2; // Set OCR1B to half OCR1B
	
	set(TCCR1B,WGM13); // Set timer mode to mode 15: up to OCR1A, PWM mode
	set(TCCR1B,WGM12);
	set(TCCR1A,WGM11);
	set(TCCR1A,WGM10);
	
	set(TCCR1A,COM1B1); // clear B6 at TCNT1 = OCR1B, set B6 at rollover
	clear(TCCR1A,COM1B0);
	
	clear(TCCR1B,CS12); // Set prescaler to /8
	set(TCCR1B,CS11);
	clear(TCCR1B,CS10);
	
	set(TIMSK1,OCIE1A); // Enable interrupt TIMER1_COMPA when  TCNT1 = OCR1A
}

/* Initialization of Timer3 */
void enable_timer_three(void){

	OCR3A = SYSTEM_SPEED/TIMER_THREE_PRESCALER; // Initialize Duration to 1 second

	clear(TCCR3B, WGM33); // Set timer mode to 4: up to OCR3A
	set(TCCR3B,WGM32);
	clear(TCCR3A,WGM31);
	clear(TCCR3A,WGM30);

	set(TCCR3B,CS32); // Set prescalet to /1024
	clear(TCCR3B,CS31);
	set(TCCR3B,CS30);

	set(TIMSK3,OCIE3A); // Ebable interrupt TIMER3_COMPA when TCNT3 = OCR3A
}

/* Initialization of wireless */
void enable_wireless(void){
	
	m_bus_init(); // Enable mBUS
	
	m_rf_open(CHANNEL,RXADDRESS,PACKET_LENGTH); // Configure mRF
}

/* Initialization of USB */
void enable_usb(void){
	
	m_usb_init();
	while(!m_usb_isconnected());
	
}

/************************************************************
Interrupts
************************************************************/

/* Timer1 OCR1A interrupt */
ISR(TIMER1_COMPA_vect){

	if(duration){ // If there is a duration, create sine wave
		OCR1B = (sTable[counter]*OCR1A)/STABLEMAX;
		counter++;
		if (counter>=POINTS){
			counter = 0;
		}
	} else{ // If there is no duration, silence the speaker
		OCR1B = OCR1A/2;
	}
}

/* Timer3 OCR3A interrupt */
ISR(TIMER3_COMPA_vect){

	duration = 0; // Set duration to 0: turn speaker off
	m_red(OFF);
	clear(PORTB,0);
	//m_usb_tx_string("\n Timer3 Reached");
	
}

/* Wireless interrupt */
ISR(INT2_vect){

	m_rf_read(buffer,PACKET_LENGTH); // Read RF Signal
	frequency = (*(int*)&buffer[0])/10; // Set frequency from buffer
	duration_buffer [0] = *&buffer[2]; // Read duration into transfer buffer
	duration = (*(int*)&duration_buffer[0]); // Set duration from buffer
	//duration = 255;
	OCR1A = SYSTEM_SPEED/TIMER_ONE_PRESCALER/frequency/POINTS; // Adjust sine wave frequency
	OCR3A = (SYSTEM_SPEED/TIMER_THREE_PRESCALER)*((float)duration/100); // Adjust sine wave duration
	TCNT3 = 0x0000; // Reset Timer3 to begin from 0
	m_red(ON);
	set(PORTB,0);
	/*m_usb_tx_string("\n duration: ");
	m_usb_tx_int(duration);
	m_usb_tx_string("\n frequency: ");
	m_usb_tx_int(frequency);
	m_usb_tx_string("\n TCNT3: ");
	m_usb_tx_int(TCNT3);*/
	
	

}

/************************************************************
End of Program
************************************************************/