/*    Serial menu for AVR/TPIC8101 based knock monitoring system allows user adjustable settings 
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>
#include "knock_watch.h"
#include "tables.h"
#include "serial_menu.h"
#define OUTPUT_LEN 128

configuration cfg;
volatile vars variables;
unsigned char top_ret;
unsigned char bot_ret;

char output[OUTPUT_LEN];

//TODO: make sure differences between 2 voltage points do not exceed 2.5v 
void print_main_menu()
{
	unsigned char i,j;
	char tmpchar;
	newline();
	for(i = 0, j = 0; i < NUM_MENU_LINES; i++, j = 0)
	{
		while((tmpchar=pgm_read_byte(&menu[i][j++] ) ) != '\0')
		{
			while(!(UCSR0A & (1 << UDRE0)));
  				UDR0 = tmpchar; 
		}
		newline();
	}

return;
}

void newline()
{
		while(!(UCSR0A & (1 << UDRE0)));
			UDR0 = '\r';

		while(!(UCSR0A & (1 << UDRE0)));
			UDR0 = '\n';
}
void seperator()
{
		print_str(cfg.seperator);
}
void print_str(char * strp)
{
	unsigned char i;
	for( i = 0; strp[i] != '\0' && i < OUTPUT_LEN; i++)
	{
		while(!(UCSR0A & (1 << UDRE0)));
			UDR0 = strp[i];

	}
return;	
}

void print_str_P( const char * strp)
{
	unsigned char i, tmp;
	for(i = 0; (tmp = pgm_read_byte(&strp[i])) != '\0' && i < 255; i++)
	{
		while(!(UCSR0A & (1 << UDRE0)));
			UDR0 = tmp;
	}
return;
}

void dump_variables( void )
{
	unsigned char i = 0;
	print_str_P(timer_variable_names);
	newline();
	snprintf_P(output ,OUTPUT_LEN, timer_variable_formats, variables.ptcnt, variables.ctcnt,variables.difference,variables.ticks_per_four_deg,variables.p_dwell_off,
								variables.p_dwell_on, variables.dwell_off, variables.dwell_on, variables.dwell_ctcnt);
	print_str(output);
	newline();


	print_str_P(knock_state_variable_names);
	newline();
	snprintf_P(output ,OUTPUT_LEN, knock_state_variable_formats, variables.cur_knock_thresh, variables.target_time_const, variables.window_integ_div, 
									variables.curr_integ_idx );
	print_str(output);
	newline();

	print_str_P(datalog_flag_variable_names);
	newline();
	snprintf_P(output ,OUTPUT_LEN, datalog_flag_variable_formats, variables.window_open, variables.do_advance, variables.do_datalog,variables.curr_datalog_counter,
									variables.timer_overflows);
	print_str(output);
	newline();

	print_str_P(error_variable_names);
	newline();
	snprintf_P(output ,OUTPUT_LEN, error_variable_formats, variables.sync_loss, variables.window_overruns, variables.input_capture_overruns);
	print_str(output);
	newline();

	print_str_P(engine_state_variable_names);
	newline();
	snprintf_P(output ,OUTPUT_LEN, engine_state_variable_formats, variables.rpm, variables.rknock, variables.current_cyl_idx, variables.isknock, variables.advance_angle);
	print_str(output);
	newline();


	for(i = 0; i < MAX_CYLS; i++)
	{
		snprintf_P(output, OUTPUT_LEN, cylinder_variables, i);
		print_str(output);
		newline();
		snprintf_P(output, OUTPUT_LEN, cylinder_variables_formats, variables.number_of_knocks[i], variables.lastknock_timer_val[i],
									 variables.lastknock_timer_overflows[i],variables.lastknock_volts[i],variables.lastknock_tpfd[i]);
		print_str(output);
		newline();
	}
}

//This will dump the configuration out the serial port
//Needs to be updated each time the configuration struct is updated


void dump_config( void ) 
{
		unsigned char i;
		char flt[8];

		newline();
		print_str_P(dump_cfg[0]);
		snprintf(output, OUTPUT_LEN, "%u", cfg.num_cyls);
		print_str(output);
		newline();

		print_str_P(dump_cfg[1]);
		snprintf(output, OUTPUT_LEN, "%u", cfg.window_length_deg);
		print_str(output);
		newline();

		print_str_P(dump_cfg[2]);
		snprintf(output, OUTPUT_LEN, "%u", cfg.window_open_deg_atdc);
		print_str(output);
		newline();

		print_str_P(dump_cfg[3]);
		snprintf(output, OUTPUT_LEN, "%u", cfg.mech_advance_deg_btdc);
		print_str(output);
		newline();
		
		print_str_P(dump_cfg[4]);
		snprintf(output, OUTPUT_LEN, "%u", cfg.window_integ_div);
		print_str(output);
		newline();

	//	include additional string for index at end of line
		print_str_P(dump_cfg[5]);
		snprintf(output, OUTPUT_LEN, "%u index: %u", pgm_read_word(&bandpass_freq_value[cfg.tpic_freq]), cfg.tpic_freq );
		print_str(output);
		newline();

		dtostrf( pgm_read_float(&gain_value[cfg.tpic_gain] ) , 5, 3, flt);
		print_str_P(dump_cfg[6]);
		snprintf(output,OUTPUT_LEN, "%s index: %u", flt, cfg.tpic_gain);
		print_str(output);
		newline();
	
		print_str_P(dump_cfg[7]);	
		for(i = 0; i < ( cfg.num_cyls -1); i++)
		{
			snprintf(output, OUTPUT_LEN, "%u,", cfg.firing_order[i]);
			print_str(output);
		}
		snprintf(output, OUTPUT_LEN, "%u", cfg.firing_order[i]);
		print_str(output);
		newline();

		print_str_P(dump_cfg[8]);
		snprintf(output,OUTPUT_LEN, "%u", MAX_DATALOGS_PER_SEC/(cfg.datalog_frequency) );
		print_str(output);
		newline();

		print_str_P(dump_cfg[9]);
		snprintf(output,OUTPUT_LEN, "%u",  cfg.datalog_header_count );
		print_str(output);
		newline();

		print_str_P(dump_cfg[10]);
		snprintf(output,OUTPUT_LEN, "%u",  cfg.wheelmode );
		print_str(output);
		newline();

		print_str_P(dump_cfg[11]);
		snprintf(output,OUTPUT_LEN, "%u",  cfg.tpic_chan + 1 );
		print_str(output);
		newline();

		print_str_P(dump_cfg[12]);
		snprintf(output,OUTPUT_LEN, "%u",  cfg.datalogmode );
		print_str(output);
		newline();

		print_str_P(dump_cfg[13]);
		snprintf(output,OUTPUT_LEN, "\"%s\"",  cfg.seperator);
		print_str(output);
		newline();

		print_str_P(dump_cfg[14]);
		snprintf(output,OUTPUT_LEN, "%u",  cfg.pulse_events);
		print_str(output);
		newline();		

		print_str_P(dump_cfg[15]);
		snprintf(output,OUTPUT_LEN, "%u",  cfg.pulse_tenms);
		print_str(output);
		newline();		

		print_str_P(dump_cfg[16]);
		snprintf(output,OUTPUT_LEN, "%u",  cfg.pulse_timer_select);
		print_str(output);
		newline();		

		for(i = 0; i < cfg.num_cyls; i++)
		{
			newline();
			snprintf_P(output, OUTPUT_LEN, table_header, cfg.firing_order[i]);
			print_str(output);
			newline();
			dump_rpm_table( i);
			newline();
			dump_voltage_table( i );
	
			newline();
			dump_fslopes( i );
			newline();
		}
		newline();
		
return;
}

void dump_rpm_table( int index)
{
	int i = 0;
	print_str_P(rpm_tbl_hdr);
	for( i = 0; i < MAX_TABLE_POINTS; i++)
	{
		snprintf(output, OUTPUT_LEN, "%05u(%05u)\t",  (unsigned int)(TICKS_PER_MIN/(cfg.rpm_points[index][i])), cfg.rpm_points[index][i]);
		print_str(output);
	}

return;
}

void dump_fslopes(int index)
{
	int i = 0;
	print_str_P(fslope_tbl_hdr); 
	for( i = 0; i < MAX_TABLE_POINTS; i++)
	{
		snprintf(output, OUTPUT_LEN, "%012d\t", cfg.fslopes[index][i] );
		print_str(output);
	}

return;
}


void dump_voltage_table(int index)
{
	int i = 0;
	print_str_P(voltage_tbl_hdr);
	for(i = 0; i < MAX_TABLE_POINTS; i++)
	{
		dtostrf( (float) (cfg.knock_voltage[index][i] * TEN_BIT_LSB), 5, 3, output);
		output[5] = '\0';
		print_str(output);
		snprintf(output, OUTPUT_LEN, "(%05u)\t", cfg.knock_voltage[index][i] );
		print_str(output);
	}
return;
}

//This function will calculate slopes with numerator 
//multipled by 128, this allows a shift to be used with
//multiplication in knock lookup, keeps division out
//of critical path and helps reduce error in calc
void calculate_fslopes( int index)
{
	unsigned char i = 0;
	int numerator = 0;
	int denominator = 0;
	cfg.fslopes[index][i] = 0;
	for(i =1; i<MAX_TABLE_POINTS; i++)
	{
		numerator = (int)((cfg.knock_voltage[index][i] - cfg.knock_voltage[index][i-1])<<7);
		denominator = (int)(cfg.rpm_points[index][i] - cfg.rpm_points[index][i-1] );
		cfg.fslopes[index][i-1] = numerator/denominator;
	}
	cfg.fslopes[index][MAX_TABLE_POINTS-1] = 0;

return;
}
void dump_datalog(void)
{
	unsigned char i = 0;
	static unsigned char datalog_count = 0;

	if(datalog_count-- <= 0  && cfg.datalog_header_count < 255)
	{
		newline();
		snprintf_P(output, OUTPUT_LEN, datalog_title, cfg.seperator, cfg.seperator, cfg.seperator);
		print_str(output);
		for(i = 0; i < cfg.num_cyls; i++)
		{
			snprintf_P(output, OUTPUT_LEN, cylinder_header, cfg.seperator, cfg.firing_order[i],  cfg.seperator, cfg.firing_order[i]);
			print_str(output);

		}
		datalog_count = cfg.datalog_header_count;
		seperator();
		print_str("knock?");
	}
	newline();
	snprintf(output, OUTPUT_LEN, "%05d", variables.rpm);
	print_str(output);
	seperator();

	dtostrf( ((float)(variables.timer_overflows )  / OVERFLOWS_PER_SEC ) + ((float)TCNT1/TICKS_PER_SEC) , 5, 3, output);
	print_str(output);
	seperator();

/*
	snprintf(output, OUTPUT_LEN, "%d", variables.rknock);
	print_str(output);
	seperator();
*/

	dtostrf( (float)(variables.rknock) * TEN_BIT_LSB, 5, 3, output);
	print_str(output);
	seperator();

	dtostrf( (float)(variables.cur_knock_thresh) * TEN_BIT_LSB, 5, 3, output);
//	snprintf(output, OUTPUT_LEN, "%d", variables.cur_knock_thresh);
	print_str(output);
//	snprintf(output, OUTPUT_LEN, " %d", variables.current_cyl_idx);
//	print_str(output);
	seperator();


	for(i = 0; i < cfg.num_cyls; i++)
	{
		dtostrf( (((float)(variables.lastknock_timer_overflows[i]) ) / OVERFLOWS_PER_SEC) + ((float)(variables.lastknock_timer_val[i])/TICKS_PER_SEC), 5, 3, output);
		print_str(output);
		snprintf(output, OUTPUT_LEN, "%s%d%s", cfg.seperator,variables.number_of_knocks[i],cfg.seperator);
		print_str(output);
	}


	if(variables.isknock)
		print_str("YES");
	else
		print_str("NO");
return;
}

void dump_fastlog( void )
{
	extern unsigned char top_ret;
	extern unsigned char bot_ret;
	newline();

	if(variables.current_cyl_idx < cfg.num_cyls && 
		variables.lastknock_tpfd[variables.current_cyl_idx] < 1500 && 
		variables.lastknock_tpfd[variables.current_cyl_idx] )
	{
		snprintf(output, OUTPUT_LEN, "%05lu", TICKS_PER_MIN/((unsigned long int) variables.lastknock_tpfd[variables.current_cyl_idx] *90));
		print_str(output);
		seperator();

		snprintf(output, OUTPUT_LEN, "%u", cfg.firing_order[variables.current_cyl_idx]);

	}
	else
	{
		snprintf(output, OUTPUT_LEN, "00000");
		print_str(output);
		seperator();

		snprintf(output, OUTPUT_LEN, "0");
	}

	print_str(output);
	seperator();

	dtostrf( (float) (variables.lastknock_volts[variables.current_cyl_idx] * TEN_BIT_LSB), 5, 3, output);
	print_str(output);

	seperator();
	snprintf(output, OUTPUT_LEN, "%u - %02X - %02X ", variables.rknock, top_ret, bot_ret);
	print_str(output);
return;
}

unsigned char menu_input(unsigned char cmd)
{
	static unsigned char state = 'q';
	static unsigned char substate = 0;
	static unsigned char input_idx=0;
	static unsigned char cyl_counter =0;
	unsigned int tmpi = 0;
	float tmpf = 0;

	unsigned char i = 0;
	static unsigned char subst = 0;
	
	//just dump the menu if we're at the top level
	//and user hit enter
	if(state == 'q' || state == 'm' )
	{
	switch( cmd )
		{	
			//set rpm points for knock table
			case 'r':	state = 'r'; cyl_counter = 0; substate = 0; break;
			//set voltage points for knock table
			//will be dumped to eeprom when done
			case 'v':	state = 'v'; cyl_counter = 0; substate = 0; break;
			//get the current engine settings
			case 'g':	state = 'g'; break;
			//configure the engine settings 
			//will be dumped to eeprom when done
			case 'c':	state = 'c'; break;
			//dump the status of internal variables
			case 'd':	state = 'd'; break;
			//if we get a 'q' shut down the menu
			case 'q':  return 0;
			//set the state to main menu
			default:	state = 'm'; print_main_menu(); break;
		}

	return 1;
	}
	else
	{

		switch(state)
		{
		
			case 'g':
			dump_config();
			newline();
			print_main_menu();
			state = 'm';
			break;

			case 'd':
			dump_variables();
			newline();
			print_main_menu();
			state = 'm';
			break;

			case 'r':
			switch(substate)
			{
				case 0:
				newline();
				snprintf_P(output, OUTPUT_LEN, cylinder_prompt, cfg.firing_order[cyl_counter]);
				print_str(output);
				substate++;
				input_idx = 0;
				break;

				case 1:
				//set value from substate 0 query
				//if \r\n our work here is done
				if(cmd == '\n' || cmd == '\r')
				{
					if(input_idx > 0)
					{

						output[input_idx] = '\0';
						subst = 0;
						input_idx = 0;
						for(i = 0; i < OUTPUT_LEN && output[i] != '\0'; i++)
						{
								if(output[i] == ',')
								{
									output[i] = '\0';
									//this is actually stored in timer ticks to avoid division in code
									//introduces +/- 8 rpm error around 10k rpm
									cfg.rpm_points[cyl_counter][input_idx++] = (TICKS_PER_MIN / atoi( (output+subst) ) );
									subst = i+1;
								}	
	
						}
						cfg.rpm_points[cyl_counter][input_idx] = (TICKS_PER_MIN / atoi( (output+subst) ) );
					} 
					if(++cyl_counter == cfg.num_cyls )
					{
						print_str_P(table_layout);
						for(i = 0; i < cfg.num_cyls; i++)
						{
							calculate_fslopes(i);
							newline();
							newline();
							snprintf_P(output, OUTPUT_LEN, table_header, cfg.firing_order[i]);
							print_str(output);
							newline();
							dump_rpm_table(i);
							newline();
							dump_voltage_table(i);
							newline();
							dump_fslopes(i);
						}
						print_str_P(prompt);
						substate++;
						input_idx = 0;
					}
					else
					{
						newline();
						input_idx = 0;
						snprintf_P(output, OUTPUT_LEN, cylinder_prompt, cfg.firing_order[cyl_counter]);
						print_str(output);
					}
				}
				else if( input_idx < OUTPUT_LEN -1  && (isdigit(cmd) || cmd == ',' ) )
					output[input_idx++]=cmd;
				else if ( input_idx > 0 && ( cmd == '\b' || cmd == 0x7F || cmd == 0x08 ) )
					input_idx--;

				break;

				case 2:
				if( cmd == 'y' || cmd == 'Y')
				{
					write_cfg_to_eep();
					setup();
				}
				else if( cmd == 'n' || cmd == 'N' )
				{
					substate = 0;
					input_idx = 0;
					cyl_counter = 0;
					break;
				}
				else if( cmd == 'q' || cmd == 'Q')
				{	
					read_cfg_from_eep();
				}
				else
				{ 
					print_str_P(prompt);
					break;
				}
				state = 'm';

				default: substate =0; input_idx = 0; cyl_counter = 0; break;
			}	
			break;
		

			case 'v':
			switch(substate)
			{
				case 0:
				newline();
				snprintf_P(output, OUTPUT_LEN, voltage_prompt , cfg.firing_order[cyl_counter]);
				print_str(output);
				substate++;
				break;
				
				
				case 1:
				//set value from substate 0 query
				//if \n our work here is done
				if(cmd == '\n' || cmd == '\r')
				{
					if(input_idx > 0)
					{
						output[input_idx] = '\0';
						subst = 0;
						input_idx = 0;
						for(i = 0; i < OUTPUT_LEN && output[i] != '\0'; i++)
						{
								if(output[i] == ',')
								{
									output[i] = '\0';
									cfg.knock_voltage[cyl_counter][input_idx++] = (unsigned int ) (atof((output+subst)) /TEN_BIT_LSB) ;
									subst = i+1;
								}	
						} 
							cfg.knock_voltage[cyl_counter][input_idx] = (unsigned int ) (atof((output+subst)) /TEN_BIT_LSB) ;
					}
					if(++cyl_counter == cfg.num_cyls)
					{
						print_str_P(table_layout);
						for(i = 0; i < cfg.num_cyls; i++)
						{
							calculate_fslopes(i);
							newline();
							newline();
							snprintf_P(output, OUTPUT_LEN, table_header, cfg.firing_order[i]);
							print_str(output);
							newline();
							dump_rpm_table(i);
							newline();
							dump_voltage_table(i);
							newline();
							dump_fslopes(i);
						}
						print_str_P(prompt);
						substate++;
						input_idx = 0;
					}
					else
					{
						newline();
						snprintf_P(output, OUTPUT_LEN, voltage_prompt, cfg.firing_order[cyl_counter]);
						print_str(output);
						input_idx = 0;
					}
				}
				else if( input_idx < OUTPUT_LEN -1  && (isdigit(cmd) || cmd == ','|| cmd == '.' ) )
					output[input_idx++]=cmd;
				else if ( input_idx > 0 && ( cmd == '\b' || cmd == 0x7F || cmd == 0x08 ) )
					input_idx--;

				
				break;

				case 2:
				if( cmd == 'y' || cmd == 'Y')
				{
					write_cfg_to_eep();
					setup();
				}
				else if( cmd == 'n' || cmd == 'N' )
				{
					substate = 0;
					input_idx = 0;
					cyl_counter = 0;
					break;
				}
				else if( cmd == 'q' || cmd == 'Q')
				{	
					read_cfg_from_eep();
				}
				else 
				{
					print_str_P(prompt);
					break;
				}
				state = 'm';

				default: substate = 0; input_idx = 0; cyl_counter = 0; break;					

			}	
			break;

			case 'c':
			switch(substate)
			{
				case 0:
				newline();
				print_str_P(dump_cfg[substate++] );
				break;

				case 1:
				if(cmd == '\n' || cmd == '\r')
				{   
					if(input_idx > 0)
					{
						output[input_idx] = '\0';
						cfg.num_cyls = (unsigned char)atoi(output);
                        			input_idx=0;    
                    			}   
					newline();
					print_str_P(dump_cfg[substate++]);
				}
               			else if( input_idx < OUTPUT_LEN  && isdigit(cmd) )
               			 	output[input_idx++]=cmd;
				else if ( input_idx > 0 && ( cmd == '\b' || cmd == 0x7F || cmd == 0x08 ) )
					input_idx--;
				break;

				case 2:
                                if(cmd == '\n' || cmd == '\r')
                                {
					if(input_idx > 0)
					{
						output[input_idx] = '\0';
                                        cfg.window_length_deg = (unsigned char)atoi(output);
                                        input_idx=0;
                                	}
					newline();
					print_str_P(dump_cfg[substate++]);
				}
                                else if( input_idx < OUTPUT_LEN  && isdigit(cmd) )
                                                output[input_idx++]=cmd;
				else if ( input_idx > 0 && ( cmd == '\b' || cmd == 0x7F || cmd == 0x08 ) )
					input_idx--;
                                
                                break;

				case 3:
				if(cmd == '\n' || cmd == '\r')
                                {
					if(input_idx > 0)
					{
						output[input_idx] = '\0';
                                        cfg.window_open_deg_atdc = (unsigned char)atoi(output);
                                        input_idx=0;
                                	}
					newline();
					print_str_P(dump_cfg[substate++]);
				}
                                else if( input_idx < OUTPUT_LEN  && isdigit(cmd) )
                                                output[input_idx++]=cmd;
				else if ( input_idx > 0 && ( cmd == '\b' || cmd == 0x7F || cmd == 0x08 ) )
					input_idx--;
                                
                                break;

				case 4:
				if(cmd == '\n' || cmd == '\r')
                                {
					if(input_idx > 0)
					{
						output[input_idx] = '\0';
                                        cfg.mech_advance_deg_btdc = (unsigned char)atoi(output);
                                        input_idx=0;
                                	}
					newline();
					print_str_P(dump_cfg[substate++]);
				}
                                else if( input_idx < OUTPUT_LEN  && isdigit(cmd) )
                                                output[input_idx++]=cmd;
				else if ( input_idx > 0 && ( cmd == '\b' || cmd == 0x7F || cmd == 0x08 ) )
					input_idx--;
                                
                                break;

				case 5:
				if(cmd == '\n' || cmd == '\r')
				{
					if(input_idx > 0)
					{
						output[input_idx] = '\0';
					cfg.window_integ_div = (unsigned char)atoi(output);
       		             		input_idx=0;
					}
					newline();
					print_str_P(dump_cfg[substate++]);
				}
				else if( input_idx < OUTPUT_LEN  && isdigit(cmd) )
                			output[input_idx++]=cmd;
				else if ( input_idx > 0 && ( cmd == '\b' || cmd == 0x7F || cmd == 0x08 ) )
					input_idx--;
                		break;


				case 6:
				if(cmd == '\n' || cmd == '\r')
				{
					if(input_idx > 0)
					{
						output[input_idx] = '\0';
					tmpi = (unsigned int)atoi(output);
					//search for the next largest bandpass frequenc and 
					//set the index of the current center frequency in the cfg section
					for(i = 0; i < NUM_BANDPASS_FREQS && pgm_read_word(&bandpass_freq_value[i]) < tmpi ; i++);
					cfg.tpic_freq = i;
					input_idx=0;
					}
					newline();
					print_str_P(dump_cfg[substate++]);
				}
				else if( input_idx < OUTPUT_LEN	&& isdigit(cmd) )
					output[input_idx++]=cmd;
				else if ( input_idx > 0 && ( cmd == '\b' || cmd == 0x7F || cmd == 0x08 ) )
					input_idx--;
				break;


				case 7:
				if(cmd == '\n' || cmd == '\r' )
				{
					if(input_idx > 0)
					{
						output[input_idx] = '\0';
						tmpf = (float)atof(output);
						//search for the next smallest gain
						//set the index of the gain in the cfg section
						for(i = 0; i < NUM_GAIN && pgm_read_float(&gain_value[i]) > tmpf ; i++);
						cfg.tpic_gain = i;
						input_idx=0;
					}
					newline();
					print_str_P(dump_cfg[substate++]);
				}
				else if( input_idx < OUTPUT_LEN && (isdigit(cmd) || cmd == '.' ) )
					output[input_idx++]=cmd;
				else if ( input_idx > 0 && ( cmd == '\b' || cmd == 0x7F || cmd == 0x08 ) )
					input_idx--;
				break;

				case 8:
				if(cmd == '\n' || cmd == '\r')
				{	
					if(input_idx > 0)
					{
						output[input_idx] = '\0';
						subst = 0;
					input_idx = 0;
						for(i = 0; i < OUTPUT_LEN && output[i] != '\0'; i++)
					{
							if(output[i] == ',')
						{
								output[i] = '\0';
								cfg.firing_order[input_idx++] = atoi( (output+subst) );
								subst = i+1;
						}

					}
						cfg.firing_order[input_idx] = atoi( (output+subst) );
                    			input_idx=0;
					}
					newline();
					print_str_P(dump_cfg[substate++]);
				}
				else if( input_idx < OUTPUT_LEN  && (isdigit(cmd) || cmd == ',' ) )
                			output[input_idx++]=cmd;
				else if ( input_idx > 0 && ( cmd == '\b' || cmd == 0x7F || cmd == 0x08 ) )
					input_idx--;
              			break;
				case 9:
				if(cmd == '\n' || cmd == '\r')
                                {
					if(input_idx > 0)
					{
						output[input_idx] = '\0';
                                        cfg.datalog_frequency = MAX_DATALOGS_PER_SEC/(unsigned char)atoi(output);
										if(cfg.datalog_frequency < 2)
											cfg.datalog_frequency = 1;
										else
											cfg.datalog_frequency--;
                                        input_idx=0;
	                                }
					newline();
					print_str_P(dump_cfg[substate++]);

				}
                                else if( input_idx < OUTPUT_LEN  && isdigit(cmd) )
                                                output[input_idx++]=cmd;
				else if ( input_idx > 0 && ( cmd == '\b' || cmd == 0x7F || cmd == 0x08 ) )
					input_idx--;
                                
                                break;

				case 10:
				if(cmd == '\n' || cmd == '\r')
                                {
					if(input_idx > 0)
					{
						output[input_idx] = '\0';
                                       		cfg.datalog_header_count = (unsigned char)atoi(output);

						if(cfg.datalog_header_count == 0)
							cfg.datalog_header_count = 1;

                                       		input_idx=0;
                                	}
					newline();
					print_str_P(dump_cfg[substate++]);
				}
                                else if( input_idx < OUTPUT_LEN  && isdigit(cmd) )
                                                output[input_idx++]=cmd;
				else if ( input_idx > 0 && ( cmd == '\b' || cmd == 0x7F || cmd == 0x08 ) )
					input_idx--;
                                
                                break;

				case 11:
				if(cmd == '\n' || cmd == '\r')
                                {
					if(input_idx > 0)
					{
						output[input_idx] = '\0';
                                       		cfg.wheelmode = (unsigned char)atoi(output);
						if(cfg.wheelmode != HEP && cfg.wheelmode != FOURTWENTYA )
							cfg.wheelmode = 0;

                                       		input_idx=0;
                                	}
					newline();
					print_str_P(dump_cfg[substate++]);
				}
                                else if( input_idx < OUTPUT_LEN  && isdigit(cmd) )
                                                output[input_idx++]=cmd;
				else if ( input_idx > 0 && ( cmd == '\b' || cmd == 0x7F || cmd == 0x08 ) )
					input_idx--;
                                
                                break;

				case 12:
				if(cmd == '\n' || cmd == '\r')
                                {
					if(input_idx > 0)
					{
						output[input_idx] = '\0';
                                       		cfg.tpic_chan = ( (unsigned char)atoi(output) -1 );
						if(cfg.tpic_chan != 1 )
							cfg.tpic_chan = 0;

                                       		input_idx=0;
                                	}
					newline();
					print_str_P(dump_cfg[substate++]);
				}
                                else if( input_idx < OUTPUT_LEN  && isdigit(cmd) )
                                                output[input_idx++]=cmd;
				else if ( input_idx > 0 && ( cmd == '\b' || cmd == 0x7F || cmd == 0x08 ) )
					input_idx--;
                                
                                break;

				case 13:
				if(cmd == '\n' || cmd == '\r')
                {
					if(input_idx > 0)
					{
						output[input_idx] = '\0';
                        cfg.datalogmode = ( (unsigned char)atoi(output)  );
						if( cfg.datalogmode )
							cfg.datalogmode = 1;

                        input_idx=0;
                    }
					newline();
					print_str_P(dump_cfg[substate++]);
				}
                else if( input_idx < OUTPUT_LEN  && isdigit(cmd) )
					output[input_idx++]=cmd;
				else if ( input_idx > 0 && ( cmd == '\b' || cmd == 0x7F || cmd == 0x08 ) )
					input_idx--;
                                
				break;

				case 14:
				if(cmd == '\n' || cmd == '\r')
                {
					if(input_idx > 0)
					{
						output[input_idx] = '\0';
						strncpy(cfg.seperator, output, MAX_SEP_LEN-1);
						cfg.seperator[MAX_SEP_LEN - 1] = '\0';
 						input_idx=0;
					}
					newline();
					print_str_P(dump_cfg[substate++]);

				}
                else if( input_idx < OUTPUT_LEN )
                    output[input_idx++]=cmd;
				else if ( input_idx > 0 && ( cmd == '\b' || cmd == 0x7F || cmd == 0x08 ) )
					input_idx--;
				break;

				case 15:
				if(cmd == '\n' || cmd == '\r')
                {
					if(input_idx > 0)
					{
						output[input_idx] = '\0';
						cfg.pulse_events = (unsigned char) atoi(output);
 						input_idx=0;
					}
					newline();
					print_str_P(dump_cfg[substate++]);

				}
                else if( input_idx < OUTPUT_LEN  && isdigit(cmd) )
                    output[input_idx++]=cmd;
				else if ( input_idx > 0 && ( cmd == '\b' || cmd == 0x7F || cmd == 0x08 ) )
					input_idx--;
				break;


				case 16:
				if(cmd == '\n' || cmd == '\r')
                {
					if(input_idx > 0)
					{
						output[input_idx] = '\0';
						cfg.pulse_tenms = (unsigned char) atoi(output);
 						input_idx=0;
					}
					newline();
					print_str_P(dump_cfg[substate++]);

				}
                else if( input_idx < OUTPUT_LEN  && isdigit(cmd) )
                    output[input_idx++]=cmd;
				else if ( input_idx > 0 && ( cmd == '\b' || cmd == 0x7F || cmd == 0x08 ) )
					input_idx--;
				break;


				case 17:
				if(cmd == '\n' || cmd == '\r')
                                {
					if(input_idx > 0)
					{
						output[input_idx] = '\0';
						cfg.pulse_timer_select = (unsigned char) atoi(output);
						if(cfg.pulse_timer_select)
							cfg.pulse_timer_select = 1;
 						input_idx=0;
					}
					print_str_P(config_done);
					dump_config();
					print_str_P(prompt);
					newline();
					substate++;
				}
                else if( input_idx < OUTPUT_LEN  && isdigit(cmd) )
                    output[input_idx++]=cmd;
				else if ( input_idx > 0 && ( cmd == '\b' || cmd == 0x7F || cmd == 0x08 ) )
					input_idx--;
				break;



				case 18:
				if( cmd == 'y' || cmd == 'Y')
				{
					write_cfg_to_eep();
					setup();
				}
				else if( cmd == 'n' || cmd == 'N' )
				{
					substate = 0;
					input_idx = 0;
					cyl_counter = 0;
					break;
				}
				else if( cmd == 'q' || cmd == 'Q')
				{	
					read_cfg_from_eep();
				}
				else
				{
					print_str_P(prompt);
					break;
				}
				state = 'm';

				default: substate = 0; input_idx = 0; cyl_counter = 0; break;					

			}//switch(substate)
			
		}//switch(state)

	}//else
return 1;
}//void menu_input()

