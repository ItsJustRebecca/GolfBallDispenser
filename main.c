/*
 * SeniorDesign.c
 *
 * Created: 3/20/2023 6:25:55 PM
 * Author : rebecca
 */ 

#define F_CPU 16000000UL //clock rate
#define BAUD 9600 //define baud
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <ctype.h>

#define MAX 50					//arbitrary max of balls that can fit in dispenser
#define S_CHECK    1			//defining each state
#define S_EMPTY    2
#define S_ALERT    3
#define S_SIGNAL   4
#define S_DISPENSE 5
#define MIN_PULSE_WIDTH 500		// Minimum pulse width in us for the servo
#define MAX_PULSE_WIDTH 2400	// Maximum pulse width in us for the servo

int num_balls = 1;				//number of balls that will be dispensed
uint8_t count;					//will keep count of how many balls are in dispenser

//Sam's pwm functions
void init() {
	DDRD |= (1 << 6);				// Motor 1 output
	DDRD |= (1 << 5);				// Motor 2 output
	DDRB |= (1<<5);					// led output
	DDRB &= ~(1<<0|1<<3|1<<2|1<<1);	// sensor input on B0, button inputs at B3, B2, B1
	PORTB |= (1<<3|1<<2|1<<1);      // pull up resistor
	//Need to include input for 4th button
	PORTB &= ~(1<<5);				// LED off
}

void servo_min() {				// Start motors 0 degrees
	PORTD |= (1 << 6);			// Turn on Motor 1
	PORTD |= (1 << 5);			// Turn on Motor 2
	_delay_us(MIN_PULSE_WIDTH);	// set to 0 degrees
	PORTD &= ~(1 << 6);			// Turn off motor 1
	PORTD &= ~(1 << 5);			// Turn off motor 2
}

void servo_max() {
	PORTD |= (1 << 6);			// Turn on Motor 1
	PORTD |= (1 << 5);			// Turn on Motor 2
	_delay_us(MAX_PULSE_WIDTH);	// set to 180 degrees
	PORTD &= ~(1 << 6);			// Turn off motor 1
	PORTD &= ~(1 << 5);			// Turn off motor 2
}

int main(void)
{	
	init();     //initialize pwm pins and setup
	
	int state, next_state;
    uint8_t init_power;									//keeps track of whether dispenser is being turned on for the first time
	count = eeprom_read_byte(( uint8_t *)46);			//store in memory address 46
	init_power = eeprom_read_byte(( uint8_t *)47);
	
	if(init_power == 0){								//if turned on for the first time
		count = MAX;									//dispenser is full
		init_power = 1;							
	}
	eeprom_update_byte(( uint8_t *)46, count);			//store value from count in memory address 46
	eeprom_update_byte(( uint8_t *)47, init_power);		//store init_power value
	state = 1;
    
	while (1) 
    {
		//state 1: check if machine is empty
		if(state == S_CHECK){
			if(count == 0){
				next_state = S_EMPTY;
			}
			else if(count <= (MAX/10)){			//arbitrary "low count" number
				next_state = S_ALERT;
			}
			else{
				//LED strip set to green
				next_state = S_SIGNAL;
			}
		}
		
		//state 2: empty, wait 3 minutes
		if(state == S_EMPTY){
			//LED strip turns red
			while(/*not refilled*/){
				next_state = S_EMPTY;
			}
			next_state = S_CHECK;
		}
		
		//state 3: low count, alert golfer
		if(state == S_ALERT){
			//Set LED strip to orange here
			//maybe could make the orange progress slowly to red
			//so this state is a little more useful
			next_state = S_SIGNAL;
		}
		
		//state 4: set amount of balls to be dispensed
		if(state == S_SIGNAL){
			next_state = S_SIGNAL;					//stay in this state until button and sensor have been activated
			//if button 1 pressed, num_ball = 1;
			while(!(PINB & (1<<3))){				//button on pin B3
				num_balls = 1;	
			}
			//if button 2 pressed, num_ball = 2;
			while(!(PINB & (1<<2))){				//button on pin B2
				num_balls = 2;
			}
			//if button 3 pressed, num_ball = 3;
			while(!(PINB & (1<<1))){				//button on pin B1
				num_balls = 3;
			}
			while(!(PINB & (1<<0))){			   //sensor at B0 activated
				next_state = S_DISPENSE;
			}
		}
		
		//state 5: dispense balls
		if(state == S_DISPENSE){
			for(int i = 0; i < num_balls; i++){			//dispense desired amount of golf balls
				servo_min();
				servo_max();
			}
			count = count - num_balls;					//update internal counter
			eeprom_update_byte(( uint8_t *)46, count);	//store value in memory
			next_state = S_CHECK;
		}
		state = next_state;
    }
}
