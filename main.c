// SeniorDesign.c

#define F_CPU 16000000UL //clock rate
#define BAUD 9600 //define baud

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <ctype.h>

#define MAX 20					//arbitrary max of balls that can fit in dispenser
#define S_CHECK    1			//defining each state
#define S_EMPTY    2
#define S_ALERT    3
#define S_SIGNAL   4
#define S_DISPENSE 5

// motor related definitions
#define MIN_PULSE_WIDTH 500		// Minimum pulse width in us for the servo
#define MAX_PULSE_WIDTH 2400	// Maximum pulse width in us for the servo
#define SERVO_FREQ 50							// motors operate at 50Hz
#define SERVO_PERIOD (1000000UL / SERVO_FREQ)	// servo period in us
#define TIMER_TOP ((F_CPU / 256) / SERVO_FREQ) // Timer0 TOP value for Fast PWM

// int num_balls = 1;				//number of balls that will be dispensed
int8_t count;					//will keep count of how many balls are in dispenser

/*Motor 1    -> D5
  Motor 2    -> D3
  Sensor 1   -> B0
  Sensor 2   -> B4
  Button 1   -> B2
  Button 2   -> B3			//i think???
  Button 3   -> B1
  Button 4   -> D4
  RED pin    -> D2
  ORANGE pin -> D7*/

void delay_ms(int ms)
{
	for(int i = 0; i < ms; i++)
	{
		_delay_ms(1);
	}
}

void servo_min(int motor) {
	if (motor == 1) {
		OCR0B = MIN_PULSE_WIDTH * TIMER_TOP / SERVO_PERIOD; // rotate back to 0 degrees (D5)
	}
	else if (motor == 2) {
		OCR2B = MIN_PULSE_WIDTH * TIMER_TOP / SERVO_PERIOD; // rotate back to 0 degrees (D3)
	}
}

void servo_max(int motor) {
	if (motor == 1) {
		OCR0B = MAX_PULSE_WIDTH * TIMER_TOP / SERVO_PERIOD; // rotate 180 degrees (D5)
	}
	else if (motor == 2) {
		OCR2B = MAX_PULSE_WIDTH * TIMER_TOP / SERVO_PERIOD; // rotate 180 degrees (D3)
	}
}

void init_motors() {
	servo_min(1);
	servo_min(2);
}


void init_timers() {
	// set up timer0 for motor at pin D5
	TCCR0A |= (1<<WGM01)|(1<<WGM00);	// Fast PWM (mode 3)
	TCCR0B |= (1<<CS02);				// Prescaler of 256
	TCCR0A |= (1<<COM0B1);				// clear OC0B on compare match
	
	// set up timer2 for motor at pin D3
	TCCR2A |= (1<<WGM21)|(1<<WGM20);	// Fast PWM (mode 3)
	TCCR2B |= (1<<CS22)|(1<<CS21);		// Prescaler of 256
	TCCR2A |= (1<<COM2B1);				// clear OC2B on compare match when up-counting
	
	// start both motors at 0 degrees
	init_motors();
}

void init() {
	DDRD |= (1 << 5);						// Motor 1 output
	DDRD |= (1 << 3);						// Motor 2 output
	DDRB |= (1<<5);							// led output
	PORTB &= ~(1<<5);						// LED off
	DDRB &= ~(1<<0|1<<4);					// sensor input on B0 and B4
	DDRB &= ~(1<<1|1<<2|1<<3);				// button inputs at B1, B2, B3
	PORTB |= (1<<1|1<<2|1<<3);				// pull up resistor
	DDRD &= ~(1<<4);						// dump all button on D4
	PORTD |= (1<<4);						// pull up resistor
	DDRD |= (1<<2|1<<7);					// LED strip signals to arduino
	PORTD &= ~(1<<2|1<<7);					// start 

	init_timers();
}


int main(void)
{	
	init();     //initialize pwm pins and setup
	
	int state, next_state;
	count = MAX;
	int num_balls = 0;
	int delay = 0;
	int ready = 0;
    uint8_t init_power;									//keeps track of whether dispenser is being turned on for the first time
	count = eeprom_read_byte(( uint8_t *)46);			//store in memory address 46
	init_power = eeprom_read_byte(( uint8_t *)47);
	
	if(init_power == 0){								//if turned on for the first time
		count = MAX;									//dispenser is full
		init_power = 1;							
	}
	eeprom_update_byte(( uint8_t *)46, count);			//store value from count in memory address 46
	eeprom_update_byte(( uint8_t *)47, init_power);		//store init_power value
	state = S_CHECK;
    
	while (1) 
    {
		while(!(PIND & (1<<4))){
			servo_max(1);
			servo_max(2);
			delay_ms(450*count);							//dump all should be a variable based on # of balls left in dispenser
			servo_min(1);
			servo_min(2);
			count = 0;
			state = S_CHECK;
			//next_state = S_CHECK;
			eeprom_update_byte(( uint8_t *)46, count);			//store value from count in memory address 46
		}
			
		//state 1: check if machine is empty
		if(state == S_CHECK){
			if(count <= 0){
				next_state = S_EMPTY;
			}
			else if(count <= (MAX/4)){					//arbitrary "low count" number
				next_state = S_ALERT;
			}
			else{
				PORTD &= ~(1<<2);						//red and orange off
				PORTD &= ~(1<<7);
				next_state = S_SIGNAL;
			}
			eeprom_update_byte(( uint8_t *)46, count);	//store value in memory
		}
		
		//state 2: empty, wait 3 minutes
		if(state == S_EMPTY){
			next_state = S_EMPTY;
			PORTD &= ~(1<<7);
			PORTD |= (1<<2);		//LED strip is RED
			while(!(PINB & (1<<4))){	
				count = MAX;
				next_state = S_CHECK;
			}
		}
		
		//state 3: low count, alert golfer
		if(state == S_ALERT){
			PORTD &= ~(1<<2);		
			PORTD |= (1<<7);		//LED strip is ORANGE
			next_state = S_SIGNAL;
		}
		
		//state 4: set amount of balls to be dispensed
		if(state == S_SIGNAL){
			next_state = S_SIGNAL;					//stay in this state until button and sensor have been activated
			//if button 1 pressed, num_ball = 1;
			while(!(PINB & (1<<2))){				//button pressed on pin B1
				num_balls = 1;
				delay = 200;
				ready = 1;	
			}
			//if button 2 pressed, num_ball = 2;
			while(!(PINB & (1<<3))){				//button on pin B2
				num_balls = 2;
				delay = 420;
				ready = 1;
			}
			//if button 3 pressed, num_ball = 3;
			while(!(PINB & (1<<1))){				//button on pin B3
				num_balls = 3;
				delay = 650;
				ready = 1;
			}
			
			while(!(PINB & (1<<0))){			   //sensor at B0 activated
				next_state = S_DISPENSE;
			}
		}
		
		//state 5: dispense balls
		if(state == S_DISPENSE){
			while (ready) {
				servo_max(1);
				delay_ms(delay);
				servo_min(1);
				
				if (num_balls == 3){
					_delay_ms(150);
					delay = 1300;
				}
				else{
					_delay_ms(250);
				}

				servo_max(2);
				delay_ms(delay);
				servo_min(2);
			
				ready = 0;
				count = count - num_balls;					//update internal counter
				num_balls = 0;
			}
			next_state = S_CHECK;
			eeprom_update_byte(( uint8_t *)46, count);	//store value in memory
			
		}
		state = next_state;
    }
}
