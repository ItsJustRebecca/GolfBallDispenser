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
#define S_OFF      3
#define S_ALERT    4
#define S_SET      5
#define S_WAIT     6
#define S_DISPENSE 7
#define TOP 500 // 1000*(64/16000000)

int num_balls = 1;							//number of balls that will be dispensed
uint8_t count;								//will keep count of how many balls are in dispenser

//Sam's code
void pwm_init() {
	DDRD |= (1<<6);     //Fast PWM output at OC0A pin -> D6
	OCR0A = 255;	    // Duty cycle of 100% -> update this value with desired pwm value
	TCCR0A |= (1<<COM0A1) | (1<<WGM01) | (1<<WGM00);	//Non-Inverting Fast PWM mode 3 using OCRA unit
	TCCR0B |= (1<<CS00);	//No-Prescalar
}

int empty_wait(){							//there should be a button that the golfer presses
	//int x;									//to indicate that the dispenser has been refilled
	//implement timer for 3 minutes here
	//if button was pressed
	//count = MAX;
	
	return (count == MAX) ? 1 : 0;
}

void motor_spin(int num_balls){
	//for(int i = 0; i < num_balls; i++){
		//PORTD |= (1<<PORTD6);			//assume motor is connected to D6
		//_delay_ms(7.5);					//spins 180 degrees? 
		//PORTD &= ~(1<<PORTD6);			//remove power from motor
		//_delay_ms(5);					//pause for a small amount of time between each ball being dispensed
	//}
	//Jeremy's code
	for(int i = 100; i < TOP; i++){
		OCR0A = i;
		_delay_ms(1500);
		if(OCR0A == 300){
			PORTD &= ~(1<<5);
		}
	}
	count = count - num_balls;
	eeprom_update_byte(( uint8_t *)46, count);
}

int main(void)
{	
	pwm_init();     //initialize pwm pins and setup
	DDRD |= (1<<6);								//make pin 6 as motor output
	DDRD &= ~(1<<4|1<<5);						//make pin 4 button input and pin 5 sensor input
	//button already has pull up resistor
	DDRD |= (1<<7);								//make (arbitrary) 7 pin as an output
	//set button for refilling dispenser as an input
	//set buttons for number of balls dispensed as an input
	
	int state, next_state;
    uint8_t init_power;							//keeps track of whether dispenser is being turned on for the first time
	count = eeprom_read_byte(( uint8_t *)46);	//store in memory address 46
	init_power = eeprom_read_byte(( uint8_t *)47);
	
	if(init_power == 0){						//if turned on for the first time
		count = MAX;							//dispenser is full
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
			else if(count <= (MAX/10)){
				next_state = S_ALERT;
			}
			else{
				next_state = S_SET;
			}
		}
		//state 2: empty, wait 3 minutes
		if(state == S_EMPTY){
			int x = empty_wait();
			if(x){
				next_state = S_SET;
			}
			else{
				next_state = S_OFF;
			} 
		}
		//state 3: still empty, turn off
		if(state == S_OFF){
			//do something to turn power off
			
		}
		//state 4: low count, alert golfer
		if(state == S_ALERT){
			PORTD |= (1<<PORTD7);					//assume there is an LED connected to D7
			next_state = S_SET;
		}
		//state 5: set amount of balls to be dispensed
		if(state == S_SET){
			//if button 1 pressed, num_ball = 1;
			//next_state = S_SET;
			while(!(PIND & (1<<4))){
				num_balls = 1;
				next_state = S_WAIT;	
			}
			//if button 2 pressed, num_ball = 2;
			//if button 3 pressed, num_ball = 3;
		}
		//state 6: wait for golfer to activate proximity sensor
		if(state == S_WAIT){
			//if button for ball count is pressed, go back to S_SET
			//else if proximity sensor is activated, go to S_DISPENSE
			while(!(PIND & (1<<5))){}		//keep busy wait while sensor is low
			//else stay in S_WAIT
			next_state = S_DISPENSE;
		}
		//state 7: dispense balls
		if(state == S_DISPENSE){
			motor_spin(num_balls);
			next_state = S_CHECK;
		}
		state = next_state;
		
		//implement code for LED battery life here (probably don't need for prototype)
    }
}

//motor code from video
/*int main(){
	pwm.period_ms(20);
	while(1){
		pwm.pulsewidth_us(1500);
		wait(0.5);
		pwm.pulsewidth_us(900);
		wait(0.5);
		pwm.pulsewidth_us(500);
		wait(0.5);
		pwm.pulsewidth_us(2100);
		wait(0.5);
		pwm.pulsewidth_us(2500);
		wait(0.1);
	}
}
*/
