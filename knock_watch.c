/*    Main source for AVR/TPIC8101 based knock monitoring system, handles actual running of system
 *    Copyright (C) 2010-2015  Keith Daigle
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

/* 
layout for MCU:
PC6 (PCINT14/RESET)
PD0 (PCINT16/RXD) --- serial rx
PD1 (PCINT17/TXD) --- serial tx
PD2 (PCINT18/INT0) --- cam/secondary trigger
PD3 (PCINT19/OC2B/INT1) -- coil signal for timing calcs
PD4 (PCINT20/XCK/T0)
PB6 (PCINT6/XTAL1/TOSC1) --- 16 mhz xtal
PB7 (PCINT7/XTAL2/TOSC2) --- 16 mhz xtal
PD5 (PCINT21/OC0B/T1)
PD6 (PCINT22/OC0A/AIN0) --- LCD Enable
PD7 (PCINT23/AIN1)  --- crank/main trigger
PB0 (PCINT0/CLKO/ICP1) --- TPIC Xin
PC5 (ADC5/SCL/PCINT13) --- LCD rs
PC4 (ADC4/SDA/PCINT12) --- LCD r/w
PC3 (ADC3/PCINT11) --- LCD data 3
PC2 (ADC2/PCINT10) --- LCD data 2
PC1 (ADC1/PCINT9) --- LCD data 1
PC0 (ADC0/PCINT8) --- LCD data 0
PB5 (SCK/PCINT5)  --- TPIC SCK
PB4 (MISO/PCINT4) --- TPIC MISO
PB3 (MOSI/OC2A/PCINT3) ---- TPIC MOSI
PB2 (SS/OC1B/PCINT2) --- TPIC CS
PB1 (OC1A/PCINT1) --- TPIC INT/HOLD 
*/

//#define WITH_LCDP

#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <stdio.h>

#ifdef WITH_LCDP
#include "lcd.h"
#endif

#include "knock_watch.h"
#include "tables.h"
#include "serial_menu.h"

//TODO: error checking for difference in voltage points > 2.5v 
//TODO: consider how to put hooks into for other wheel modes
//TODO: shut down montiroing window if overrun is found, need to double ckeck on scope.


configuration cfg;
volatile vars variables;


//these are just defaults for the spi array,
//they are mostly reset in the setup function
unsigned char spi_cmds[NUM_SPI_SETUP_CMDS] = { 
				TPIC_CLOCK_SCALER_16MHZ ,
				TPIC_CHANNEL_ONE ,
				TPIC_FREQ_6370HZ ,
				TPIC_GAIN_1 ,
				TPIC_TIME_CONST_100USEC ,
				TPIC_ADV_MODE };

/*
//This will read the knock value out of the integrator
//and set the time constant
static inline void get_knock(void)
{
	unsigned char tmp;
	PORTB &= ~(1<<CS);
	SPDR = spi_cmds[TPIC_BOT_BYTE];
		while(!(SPSR & (1<<SPIF)));
	PORTB |= (1<<CS);
	tmp = SPDR;		

	PORTB &= ~(1<<CS);
	SPDR = spi_cmds[TPIC_TOP_BYTE];
		while(!(SPSR & (1<<SPIF)));
	PORTB |= (1<<CS);
	variables.rknock = SPDR;


	PORTB &= ~(1<<CS);
	SPDR = spi_cmds[TIME_CONST_BYTE];
		while(!(SPSR & (1<<SPIF)));
	PORTB |= (1<<CS);
	variables.rknock += SPDR * 256;		

return;
}
*/


//This will read the knock value out of the integrator
//and set the time constant
static inline void get_knock(void)
{
	unsigned char tmp;
	extern unsigned char top_ret;
	extern unsigned char bot_ret;
	PORTB &= ~(1<<CS);
	SPDR = spi_cmds[TPIC_TOP_BYTE];
		while(!(SPSR & (1<<SPIF)));
	PORTB |= (1<<CS);
	tmp = SPDR;		

	PORTB &= ~(1<<CS);
	SPDR = spi_cmds[TPIC_BOT_BYTE];
		while(!(SPSR & (1<<SPIF)));
	PORTB |= (1<<CS);
	top_ret = SPDR>>6;
	variables.rknock =  top_ret * 256;


	PORTB &= ~(1<<CS);
	SPDR = spi_cmds[TIME_CONST_BYTE];
		while(!(SPSR & (1<<SPIF)));
	PORTB |= (1<<CS);
	bot_ret = SPDR;
	variables.rknock += bot_ret;		

return;
}

//Initial setup for the tpic chip.
static inline void setup_tpic8101(void)
{
	unsigned char i = 0;
	spi_cmds[CLOCK_IDX] = TPIC_CLOCK_SCALER_16MHZ;
	spi_cmds[CHANNEL_IDX] = (0xE0 | (cfg.tpic_chan));
	//spi_cmds[CHANNEL_IDX] = TPIC_CHANNEL_TWO;
	spi_cmds[FREQ_IDX] = cfg.tpic_freq;
	spi_cmds[GAIN_IDX] = (0x80 | cfg.tpic_gain);
	spi_cmds[TIME_CONST_IDX] = (0xC0 | variables.curr_integ_idx);
	spi_cmds[ADV_IDX] = TPIC_ADV_MODE;

	//this sends the bytes to the chip;
	for( i = 0; i < NUM_SPI_SETUP_CMDS; i++)
	{	
		//Select chip by driving the CS line low
		PORTB &= ~(1<<CS);
		SPDR = spi_cmds[i];
			while(!(SPSR & (1<<SPIF)));
		PORTB |= (1<<CS);

	}
	
	
return;
}

//This gets the target time constant based upon the number of usec in the monitoring window
static inline void set_time_constant(void)
{


	//The number of ticks in the window is then divided by the window divisor and 4 since the
	//ticks per degree is for every 4 degrees for better resolution the data is actually sent to chip	
	//during get knock function as 3rd byte sent
	variables.target_time_const = (((cfg.window_length_deg * variables.ticks_per_four_deg * USECS_PER_TICK ) >> cfg.window_integ_div) >> 4);

	//Linear search, my compsci profs could kill me right now
	for(variables.curr_integ_idx = 0; 
		variables.curr_integ_idx < NUM_INTEG_CONST && 
		pgm_read_byte(&timeconstants_value[variables.curr_integ_idx]) < variables.target_time_const;
		variables.curr_integ_idx++ );

	//Always back up 1 place or just give the 0th index
	if(variables.curr_integ_idx >= 1)
		variables.curr_integ_idx--;
	else
		variables.curr_integ_idx = 0;

	spi_cmds[TIME_CONST_IDX] = (0XC0 | variables.curr_integ_idx);
return;
}

//This will interpolate the slope between the 
//points, starts at hightest rpm works backwards
//It actually compares inverse rpm (ticks per 4 degrees * 90) and starts at highest rpm point
static inline void interpolate_fslope(void)
{
	int i = 0;
	unsigned int ticks_per_rev = variables.ticks_per_four_deg * 90 ;
	//this function should only be called if the number of cylinders
	//current index is a valid one, but just to be safe, we'll only do the lookup if it's valid
	if (variables.current_cyl_idx < cfg.num_cyls)
	{


		for(i = MAX_TABLE_POINTS-1; i > 0 &&  ticks_per_rev > cfg.rpm_points[variables.current_cyl_idx][i]; i--);
			
	
		//if the current rpm is less than the 1st table point, give flat slope
		if( i == 0 && ticks_per_rev > cfg.rpm_points[variables.current_cyl_idx][i])
				variables.cur_knock_thresh = cfg.knock_voltage[variables.current_cyl_idx][0];
		
		//if the current rpm is greater than the max rpm, give highest point
		else if( i == MAX_TABLE_POINTS-1)
				variables.cur_knock_thresh = cfg.knock_voltage[variables.current_cyl_idx][MAX_TABLE_POINTS-1];
	
		else if( ticks_per_rev == cfg.rpm_points[variables.current_cyl_idx][i] )
				variables.cur_knock_thresh = cfg.knock_voltage[variables.current_cyl_idx][i];
		
		else
		{
				variables.cur_knock_thresh = (((int)(ticks_per_rev - cfg.rpm_points[variables.current_cyl_idx][i] ) * 
						cfg.fslopes[variables.current_cyl_idx][i]) >> 7 ) + cfg.knock_voltage[variables.current_cyl_idx][i];
		}
	}
return;
}

void read_cfg_from_eep( void )
{
	eeprom_read_block( (void*) &cfg,0,sizeof(configuration));
}

void write_cfg_to_eep( void )
{
	eeprom_write_block( (const void*) &cfg,0, sizeof(configuration));
}

ISR(TIMER1_CAPT_vect)
{
	unsigned int tmptcnt = ICR1;
	unsigned int Q;
	//I'm not sure why the test below is needed but it seems that 
	//there's more noise than the noise cancler is catching
	//Gets worse with higher clock speed, maybe just my bench rig but it's 
	//nonexistant at 1 mhz, very rare at 8 mhz, 1 every 10-30s error @ 16mhz
	//Force there to be at least one timer tick before actually doing anything
	//Just the equality test was enough to stop error from occuring on bench
	//the force of 1 tick should completely stop it, 10k rpm is 750 ticks between falling edges on hep
	// and ~75 ticks on 420a so 1 tick isn't unreasonable
	if( tmptcnt == variables.ctcnt || (variables.ctcnt + 1) == tmptcnt) 
		return;

	if(cfg.wheelmode == HEP )
	{
		//if this interrupt is caused by the sync window
		//just sync the cylinder counter and return
		if( (PIND & (1<<TRIG_2) )&& !(PIND & (1<<TRIG_1) ) )
		{
			if(	variables.current_cyl_idx == cfg.num_cyls || variables.current_cyl_idx == 0 )
					variables.current_cyl_idx = 0;
	
			else
			{
				variables.sync_loss++;
				variables.current_cyl_idx = cfg.num_cyls;
			}
			return;
		}
	
		variables.ctcnt = tmptcnt;
		//if not synced don't do any of the work for the
		//cylinder monitoring
		if(variables.current_cyl_idx < cfg.num_cyls)
		{
			//this will get the difference in timer ticks
			//overflow of timer doesn't matter due to unsigned variables
			variables.difference = variables.ctcnt - variables.ptcnt;
	
			//we only get in trouble if it's less than 1 degree / tick
			//Have to find upper limit of rpm to stop that from happening
			//and set an error code to keep from screwing up
			//Using timer ticks per four degrees to keep resolution
			variables.ticks_per_four_deg = variables.difference/45;
	
			//always set previous = current 
			variables.ptcnt = variables.ctcnt;
	
			//overflows here work to our advantge to keep the timer in sync with the 
			//trigger
			//set compare match and enable the oc1a channel output
			OCR1A = variables.ctcnt + (((cfg.mech_advance_deg_btdc + cfg.window_open_deg_atdc) * variables.ticks_per_four_deg)>>2);
			TCCR1A |= (1<<COM1A0);
			TIMSK1 |= (1<<OCIE1A);
			variables.window_open = 1;
		}

	}
	if(cfg.wheelmode == FOURTWENTYA && variables.sync)
	{
		//if the sync flag is up, count the number of falling edges
		//on the 3rd falling edge stash the current timer count
		//on the 4th falling edge, calcuate the difference, number of ticks per
		//four degrees, reset the number of edges, and set the monitoring window
		//to open Should track all 4 interrupts in cycle to monitor sync and loss thereof
		//
		//This code brought to you by dogfishead and shadows fall :)
		//
		switch(variables.falls++)
		{
			case 2: variables.ptcnt = tmptcnt; return;
			case 3:
				variables.ctcnt = tmptcnt;
				variables.difference = variables.ctcnt - variables.ptcnt;
				//to do fast division by 5, should only take 20 cycles or so
				//if + and << are 1 cycle instructions, maybe faster depending on register usage
				//http://www.cs.uiowa.edu/~jones/bcd/divide.html
				/* approximate A/5 */
				Q = ((variables.difference >>	1) + variables.difference) >> 1; /* Q = A*0.11 */
				Q = ((Q >> 4) + Q)	; /* Q = A*0.110011 */
				Q = ((Q >> 8) + Q) >> 2; /* Q = A*0.0011001100110011 */
				/* either Q = A/5 or Q+1 = A/5 for all A < 185,365 */
				//variables.ticks_per_four_deg = variables.difference/5;
				variables.ticks_per_four_deg = Q;
				variables.falls = 0;
				OCR1A = variables.ctcnt + 
					(((cfg.mech_advance_deg_btdc + cfg.window_open_deg_atdc) *
					variables.ticks_per_four_deg)>>2);
				TCCR1A |= (1<<COM1A0);
				TIMSK1 |= (1<<OCIE1A);
				variables.window_open = 1;
			default: return;
                }
	}

return;
}

ISR(TIMER1_COMPA_vect)
{
	//if the window is currently open, that means we're in this ISR
	//because the first compare matched, so set the next compare match
	//to hit the end of the window and mark the window as closed at that compare
	if(variables.window_open)
	{	
		OCR1A += ((cfg.window_length_deg * variables.ticks_per_four_deg)>>2);
		variables.window_open = 0;

		//This will attempt to catch the overruns in turning off the integration window
		//Fails if the current counter has overflown and is less than the compare reg
		if(TCNT1 > OCR1A)
		{
			variables.window_overruns++;
			//Consider setting a FOC here to shut down window
		}
	}
	//Otherwise, this is the close of the window, disable the compare
	//and it's interrupt, and get the value from the tpic chip
	else
	{
		//disable the oc1a output channel
		TCCR1A &= ~(1<<COM1A0);
		TIMSK1 &= ~(1<<OCIE1A);

		//call to check the output from the tpic via the serial channel
		get_knock();

		//Statsh this cylinder's knock voltage and tick count
		variables.lastknock_volts[variables.current_cyl_idx]  = variables.rknock;
		variables.lastknock_tpfd[variables.current_cyl_idx] = variables.ticks_per_four_deg;

		//this will set the current knock threshold for the comparison below
		//is indexed for currently firing cylinder
		interpolate_fslope();

		if( variables.rknock > variables.cur_knock_thresh )
		{
				variables.lastknock_timer_overflows[variables.current_cyl_idx] = variables.timer_overflows;
				variables.lastknock_timer_val[variables.current_cyl_idx] = OCR1A;
				variables.number_of_knocks[variables.current_cyl_idx]++;

				//start output pulse for the digital output of knock
				//This is decremented further down, so if set to 0
				//the pulse will go down next cylinder after there's no knock
				if(!variables.isknock )
				{
					PORTD |= (1<<OUTPUT_DIGITAL);
					if(cfg.pulse_timer_select)
						variables.isknock = cfg.pulse_events+1;
					else
						variables.isknock = cfg.pulse_tenms;
				}

		}

		//Set the time constant on the TPIC chip for next reading
		//will be loaded upon call to get_knock, always runs 1 iteration behind current
		set_time_constant();

		//move to the next cylinder before cycle starts all over again
		if(++variables.current_cyl_idx == cfg.num_cyls)
			variables.current_cyl_idx = 0;

		//Shut off the digital knock output if the
		//number of events has elapsed and timer mode is set to
		//cylinder events
		if(cfg.pulse_timer_select)
		{
			if(variables.isknock)
				variables.isknock--;
			else
				PORTD &= ~(1<<OUTPUT_DIGITAL);
		}

		//This will track the overrun of this interrupt if a ICP has happened that hasn't
		//been handled
		if(TIFR1 & (1<<ICF1) )
			variables.input_capture_overruns++;

	}
return;
}
//This ISR is for dwell calculations
//catches changing edges on each edge to get dwell on and off times
//I'm not sure how well this runs in 420a mode or lately, haven't
//had coil line hooked up in quite some time
ISR(INT1_vect)
{
	unsigned int tmp = TCNT1;

	
	//If the current firing cylinder isn't in the 
	//list of cyls to watch, return
	//if(!( (1<<cylinder) & CYLINDER_MASK) )
	//	return;

	//If the pin is high, mark the time as when the dwell
	//came back on
	if( PIND & (1<<COIL_INT))
	{
		variables.p_dwell_on = variables.dwell_on;
		variables.dwell_on = tmp;
		//this subtract is done without regard to overflows
		//due to un-signedness of variable
		variables.anti_dwell_t = variables.dwell_on - variables.dwell_off;
		variables.do_advance = 1;
	}
	else
	{
		variables.p_dwell_off = variables.dwell_off;
		variables.dwell_off = tmp;
		//this subtract is done without regard to overflows
		//due to un-signedness of variable
		variables.dwell_t = variables.dwell_off - variables.dwell_on;
		variables.dwell_ctcnt = variables.ctcnt;
	}
	
return;
}

//This ISR handles the sync edge on the cam trigger for the 420a mode
//It should only do the sync step 1 time per rev, but run 2x per rev
ISR(INT0_vect)
{
	//if the edge on the cam trigger has fallen and
	//the crank trigger is high, reset the firing order index
	if ( PIND & (1<<TRIG_1) )
	{
		variables.current_cyl_idx = 0;
		//This will check to see how many times the sync has been
		//lost, variables.falls should be 0 when this is called
		if(variables.falls !=0 )
			variables.sync_loss++;

		variables.falls = 0;
		variables.sync = 1;
	}
	//Consider de sync-ing here and unsetting current_cyl_idx

return;
}

//this overflows around 4x/s
//is used for timestamping events and knocks
ISR(TIMER1_OVF_vect)
{
	variables.timer_overflows++;
	return;
}
//This runs about 100x/s
//is used to spit out datalog line whien counter goes to 0
ISR(TIMER0_OVF_vect)
{
	if( variables.curr_datalog_counter <= 0)
	{
		
		variables.curr_datalog_counter = cfg.datalog_frequency;
		variables.do_datalog = 1;
	}
	else
		variables.curr_datalog_counter--;

	//This will decrement the pulse length each overflow
	if( !cfg.pulse_timer_select)
	{
		if( variables.isknock)
			variables.isknock--;
		else
			PORTD &= ~(1<<OUTPUT_DIGITAL);
	}
		
	return;
}
void setup(void)
{
	unsigned char * varp;
	unsigned int i;
	varp =  (unsigned char *) &variables;

	cli();

	//MUST SET CLOCK OUT FUSE FOR PORTB0 to send clock to TPIC chip
	DDRB = (1<<TPIC_INTHOLD)|(1<<SCK)|(0<<MISO)|(1<<MOSI)|(1<<CS);
	PORTB = (0<<TPIC_INTHOLD)|(0<<SCK)|(0<<MISO)|(0<<MOSI)|(1<<CS); 
	DDRD = (0<<TRIG_1)|(0<<TRIG_2)|(0<<COIL_INT)|(1<<OUTPUT_DIGITAL);
	PORTD = (1<<TRIG_1)|(1<<TRIG_2)|(1<<COIL_INT)|(0<<OUTPUT_DIGITAL);

	//load configuration from eeprom segment
	//this is needed to get cfg.wheelmode set properly and early
	read_cfg_from_eep();

	//zero out the variables struct
	for(i = 0; i < sizeof(vars); i++)
		*varp = 0;

	//set the cyl idx to be non-synced
	variables.current_cyl_idx = cfg.num_cyls;

	//Setup the analog comparator and ADC for input capture
	ADCSRA = (0<<ADEN); //disable ADC, redundant, but explicit
	ADCSRB = (0<<ACME);

	//Setup the external interrupt for the timing capture
	//Hopefully will give a good indication of advance
	//Setup INT1 to trigger on any logical change of the pin
	//Setup INT0 to trigger on a falling edge if a 2.0/2.4 wheel
	EICRA = (0<<ISC11)|(1<<ISC10)|(1<<ISC01)|(0<<ISC00);
	if(cfg.wheelmode == FOURTWENTYA )
		EIMSK = (1<<INT1)|(1<<INT0);
	else
		EIMSK = (1<<INT1)|(0<<INT0);


	//use AIN1 for negative line for analog comparator
	//Set bandgap reference for positive
	//connect output to input capture
	//disable digital input buffer for the ain1 pin
	ACSR  = (1<<ACBG)|(1<<ACIC)|(0<<ACD)|(1<<ACO);

	//Digital input for the crank trigger needs to be enabled
	//for 420a mode for comparison in ISR
	if(cfg.wheelmode != FOURTWENTYA)
		DIDR1 = (1<<AIN1D);

	//Setup Timer1 for input capture
	//run interrupts on input capture and overflow
	//the ocr1a channel will be used for int/hold on tpic chip
	//knock chip so it's on/off are done in hardware, matches will toggle
	//from testing on the 420A version the ICES bit seems to be inverted on the
	//analog comparator input, left to see how hep acts with edges like that
	//TCCR1B = (0<<CS12)|(1<<CS11)|(1<<CS10)|(0<<ICES1)|(1<<ICNC1);
	TCNT1 = 0;
	TCCR1B = (0<<CS12)|(1<<CS11)|(1<<CS10)|(1<<ICES1)|(1<<ICNC1);
	TIMSK1 = (1<<ICIE1)|(1<<TOV1);


	//Setup timer0 for datalog frequency flag
	//Setting the scaler to 1024 and the top to 157
	//will give a maximum freq of 100 overflows/s
	//the overflows will then be counted to get the proper 
	//datalog rate
	TCNT0 = 0;
	TCCR0A = (1<<WGM00)|(1<<WGM01);
	TCCR0B = (1<<WGM02)|(1<<CS00)|(1<<CS02);
	OCR0A = 157;
	TIMSK0 = (1<<TOV0);

	//Setup SPI for transfers to/from the TPIC chip
	SPCR = (1<<SPE)|(1<<MSTR)|(0<<CPOL)|(1<<CPHA);
	SPSR = (1<<SPI2X); 

	//Setup usart, enable tx and rx and set to 115200-8,n,1 for 16mhz
	UCSR0A = (1<<U2X0);
	UCSR0B = (1<<TXEN0)|(1<<RXEN0);
	UCSR0C = (0<<UMSEL01)|(0<<UMSEL00)|(0<<UPM01)|(0<<UPM00)|(0<<USBS0)|(1<<UCSZ01)|(1<<UCSZ00);
	UBRR0 = 16;

	#ifdef WITH_LCDP
	lcd_init(LCD_DISP_ON_CURSOR_BLINK);
	#endif

	//setup the knock chip
	setup_tpic8101();


	//enable global interrupt flag
	sei();

return;
}
int main(int argc, char * argv[])
{
	unsigned char i = 0;
	unsigned char do_menu = 0;
	unsigned int advance_t = 0;
	char lcd_line[20];


	setup();

	while(1)
	{
		if(variables.current_cyl_idx < cfg.num_cyls)
		{
			//This does the advance calc per gets captured in the ISR
			//moved to main becasue of division don't want to tie up 
			//the processor in 1 isr too long
			//if( ((1<<cylinder) & CYLINDER_MASK) && do_advance)
			if(variables.do_advance)
			{	

				/*  //This calc should match the difference between 2 falling edges
				//of the HEP pickup (as long as it's not the sync window)
				time_bet_ign = dwell_off - p_dwell_off;
				*/

				//Need to use counter capture from falling edge
				//Just before dwell which is stashed in ISR
				//again ignoring overflows due to unsigned int variables
				advance_t = (variables.dwell_off - variables.dwell_ctcnt);
				variables.advance_angle = ((advance_t / variables.ticks_per_four_deg)<<2) - cfg.mech_advance_deg_btdc;
				variables.do_advance = 0;
			}


		}
		
		//This will give the rpm by calculating the number of clock ticks in a minute (numerator)
		//and dividing it by the number of ticks it takes the crank to go 360* at the last sample (divisor)
		//Since the distance between 2 falling edges on the HEP is 180 crank degrees, the diff just has to be 
		//doubled to be 1 rev.  420a is 20 deg between last 2 falling edges tracked
		if(cfg.wheelmode == FOURTWENTYA && variables.difference > 0)
			variables.rpm =  TICKS_PER_MIN / (variables.difference * 18) ; 
		else if(cfg.wheelmode == HEP && variables.difference > 0)
			variables.rpm =  TICKS_PER_MIN / (variables.difference * 2);

		if(UCSR0A & (1<<RXC0) )
		{
			i=UDR0;
			//echo back input
			while(!(UCSR0A & (1 << UDRE0)));
			UDR0 = i;

			do_menu = menu_input(i );
		}
		if(!do_menu && variables.do_datalog)
		{
			variables.do_datalog = 0;

			if(!cfg.datalogmode)
				dump_datalog( );
			else
				dump_fastlog();



			#ifdef WITH_LCDP
			//Clear display and set cursor to home position.
			lcd_clrscr();
			snprintf(lcd_line, 20, "%s %05u %s %05u", "rpm", variables.rpm, "dif", variables.difference);
			lcd_gotoxy(0,0); 
			lcd_puts(lcd_line);

			snprintf(lcd_line, 20, "%s %05u %s %05u", "atm", advance_t, "aan", variables.advance_angle);
			lcd_gotoxy(0,1); 
			lcd_puts(lcd_line);

			snprintf(lcd_line, 20, "%s %02x %02x %s %05u", "itc", pgm_read_byte(&timeconstants_value[variables.curr_integ_idx]), variables.curr_integ_idx, "dt ", variables.dwell_t);
			lcd_gotoxy(0,2); 
			lcd_puts(lcd_line);

			sprintf(lcd_line, 20, "%s %05u", "adt", variables.anti_dwell_t);
			lcd_gotoxy(0,3); 
			lcd_puts(lcd_line);
		
			#endif
		}

		//_delay_ms(150);
	}

return 1;
}
