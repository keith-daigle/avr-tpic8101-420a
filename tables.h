/*    Data tables and display information for AVR/TPIC8101 based knock monitoring system
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

#include <avr/pgmspace.h>
#define NUM_INTEG_CONST 		32
#define NUM_BANDPASS_FREQS		64
#define NUM_GAIN				64
#define NUM_MENU_LINES			7
#define NUM_CFG_LINES			17
static const unsigned int PROGMEM timeconstants_value[NUM_INTEG_CONST] =   
								{40, 45, 50, 55, 60, 65, 70, 75, 80, 90, 100,
								110, 120, 130, 140, 150, 160, 180, 200, 220,
								240, 260, 280, 300, 320, 360, 400, 440, 480,
								520, 560, 600};



static const unsigned int PROGMEM bandpass_freq_value[NUM_BANDPASS_FREQS] = 
							{1220, 1260, 1310, 1350, 1400, 1450, 1510, 1570, 1630, 1710, 1780, 1870, 1960, 2070, 2180, 2310, 2460, 2540, 2620, 2710, 2810, 2920, 3030, 3150, 3280, 3430, 3590, 3760, 3950, 4160, 4390, 4660, 4950, 5120, 5290, 5480, 5680, 5900, 6120,
							6370, 6640, 6940, 7270, 7630, 8020, 8460, 8950, 9500, 10120, 10460, 10830, 
							11220, 11650, 12100, 12600, 13140, 13720, 14360, 15070, 15840, 16710, 17670, 
							18760, 19980};


static const float PROGMEM gain_value[NUM_GAIN] = 	{2.0, 1.882, 1.778, 1.684, 1.600, 1.523, 1.455, 1.391, 1.333, 1.28,
							1.231, 1.185, 1.143, 1.063, 1, .944, .895, .85, .81, .773, .739, 
							.708, .68, .654, .63, .607, .586, .567, .548, .5, .471, .444, .421,
							.4, .381, .364, .348, .333, .32, .308, .296, .286, .276, .258, .25,
							.236, .222, .211, .2, .19, .182, .174, .167, .16, .154, .148, .143, 
							.138, .133, .129, .125, .118, .111}; 


static const char PROGMEM menu[NUM_MENU_LINES][52] = {"Knock Box menu:", 
					"\t (r) \t Set RPM points for knock table lookup",
					"\t (v) \t Set voltage points for knock table lookup",
					"\t (g) \t Get current configuration",
					"\t (c) \t Set current configuration",
					"\t (d) \t Dump internal variables to screen",
					"\t (q) \t Quit back to datalog" };

static const char PROGMEM dump_cfg[NUM_CFG_LINES][52] = {	
						"Number of cylinders: ",
						"Length of knock window in crank degrees: ",
						"Knock window opening @ in degrees atdc: ", 
						"Mechanical advance in degrees btdc: ", 
						"Integ time const - # of times to /2:  ", 
						"Center bandpass frequency value: ",
						"Gain value: ",
						"Firing order, sync cyl 1st as #,#,#..: ", 
						"Number of datalog lines per second: ",
						"Datalog lines for header 255=off : ",
						"Wheel decoder - 0=HEP 1=420A: ",
						"Knock Input Channel: ",
						"Datalog mode 0=normal 1=fast: ",
						"Datalog field seperator: ",
						"Number of cycles for knock output sig: ",
						"Number of ms(x10) for output sig: " ,
						"Counter for knock output: 0=MS 1=cycle: "};

static const char PROGMEM prompt[] = "\r\nEntry complete.\r\n y to accept, n to re-enter, q to discard changes: ";
static const char PROGMEM table_layout[] = "\r\nTable format is:\r\n1st line - RPM point\r\n2nd line - threshold for knock Voltage\r\n3rd line - fslope between points\r\n";
static const char PROGMEM config_done[]= "\r\nDone. \r\nPrinting current cfg\r\n";
static const char PROGMEM cylinder_header[] = "%slast knock cyl: %u%sknock count cyl: %u";
static const char PROGMEM cylinder_prompt[] = "Enter 8 rpm points for cyl %u as 1,2,3,..: ";
static const char PROGMEM voltage_prompt[] = "Enter 8 knock voltages for cly %u as 1.2,2.3,4.4...: ";
static const char PROGMEM rpm_tbl_hdr[] = "RPM(INV)   :\t";
static const char PROGMEM voltage_tbl_hdr[] = "Volts(ADC) :\t";
static const char PROGMEM fslope_tbl_hdr[] =  "SLOPE      :\t";
static const char PROGMEM table_header[] = "Knock table for cylinder: %u";

static const char PROGMEM timer_variable_names[] = "ptcnt\t ctcnt\t difference\t ticks_per_four_deg\t p_dwell_off\t p_dwell_on\t dwell_off\t dwell_on\t dwell_ctcnt";
static const char PROGMEM timer_variable_formats[] = "%05u\t %05u\t %011u\t %017u\t %011u\t %010u\t %09u\t %08u\t %011u";

static const char PROGMEM knock_state_variable_names[] = "cur_knock_thresh\t target_time_const\t window_integ_div\t curr_integ_idx";
static const char PROGMEM knock_state_variable_formats[] = "%016u\t %017u\t %016u\t %014u";

static const char PROGMEM cylinder_variables[] = "cyl idx: %d number_of_knocks\t lastknock_timer_val\t lastknock_timer_overflows\t lastknock_volts\t lastknock_tpfd";
static const char PROGMEM cylinder_variables_formats[] = "\t%017lu\t %019lu\t\t %015lu\t %017lu\t %017u";

static const char PROGMEM datalog_flag_variable_names[] = "window_open\t do_advance\t do_datalog\t curr_datalog_counter\t timer_overflows";
static const char PROGMEM datalog_flag_variable_formats[] = "%011u\t %010u\t %010u\t %018u\t %015lu";

static const char PROGMEM error_variable_names[] = "sync_loss\t window_overruns\t input_capture_overruns";
static const char PROGMEM error_variable_formats[] = "%09u\t %015u\t %021u";

static const char PROGMEM engine_state_variable_names[] = "  rpm\t rknock\t current_cyl_idx\t isknock?\t advance_angle";
static const char PROGMEM engine_state_variable_formats[] = "%05u\t %06u\t %015u\t %08u\t %012u";

static const char PROGMEM datalog_title[] = "rpm%stimestamp%sknock volts%sknock thresh"; 

