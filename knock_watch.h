/*    Set of symbolic constants and pin mappings for AVR/TPIC8101 based knock monitoring system
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
#include <avr/eeprom.h>
#define SCK 						5
#define MISO 						4
#define MOSI 						3
#define CS 							2
#define TPIC_INTHOLD 				1
#define TRIG_1 						7
#define TRIG_2 						2
#define COIL_INT 					3
#define SERIAL_RX 					0
#define SERIAL_TX 					1
#define TIMER1_DIVISOR 				64
#define MAX_DATALOGS_PER_SEC		100
#define MAX_CYLS					8
#define MAX_TABLE_POINTS			8
#define MAX_SEP_LEN					8
#define OUTPUT_DIGITAL				6
#define TPIC_CLOCK_SCALER_8MHZ 		0x46
#define TPIC_CLOCK_SCALER_16MHZ 	0x4C
#define TPIC_CHANNEL_ONE 			0xE0
#define TPIC_CHANNEL_TWO 			0xE1
#define TPIC_FREQ_6370HZ 			0x27
#define TPIC_GAIN_1 				0x0E
#define TPIC_TIME_CONST_100USEC 	0x0E
#define TPIC_ADV_MODE 				0x71
#define TPIC_AAF_TEST 				0xF0
#define CLOCK_IDX					0
#define CHANNEL_IDX					1
#define FREQ_IDX					2
#define GAIN_IDX					3
#define TIME_CONST_IDX				4
#define ADV_IDX						5
#define TPIC_BOT_BYTE 				CLOCK_IDX
#define TPIC_TOP_BYTE 				CHANNEL_IDX
#define TIME_CONST_BYTE				TIME_CONST_IDX
#define NUM_SPI_SETUP_CMDS 			6
#define TIMER_MAX					0xFFFF
#define HEP							0
#define FOURTWENTYA					1
#define USECS_PER_SEC				1000000UL
#define	USECS_PER_TICK				( USECS_PER_SEC / (F_CPU >> 6) )
//#define TICKS_PER_MIN				((F_CPU >> 6) * 60 )UL
#define TICKS_PER_SEC				((F_CPU >> 6) )
#define TICKS_PER_MIN				15000000UL
#define OVERFLOWS_PER_SEC			((F_CPU >> 6) / TIMER_MAX)
#define TEN_BIT_LSB				0.004882812F
//use the calc below if divisor for timer1 isn't 64
//#define	USECS_PER_TICK			( USECS_PER_SEC / (F_CPU / TIMER1_DIVISOR) )
//use the calc below if divisor for timer1 isn't 64
//#define TICKS_PER_MIN				((F_CPU /TIMER1_DIVISOR) * 60)

typedef struct  
{
		unsigned char		num_cyls;
		unsigned char		window_length_deg;
		unsigned char		window_open_deg_atdc;
		unsigned char		mech_advance_deg_btdc;
		unsigned char		datalog_frequency;
		unsigned char		datalog_header_count;
		unsigned char		window_integ_div;
		unsigned char		tpic_freq;
		unsigned char		tpic_gain;
		unsigned char		tpic_chan;
		unsigned char		firing_order[MAX_CYLS];
		unsigned int		rpm_points[MAX_CYLS][MAX_TABLE_POINTS];
		unsigned int		knock_voltage[MAX_CYLS][MAX_TABLE_POINTS];
				 int		fslopes[MAX_CYLS][MAX_TABLE_POINTS];
		unsigned char		wheelmode;
		unsigned char		datalogmode;
				 char		seperator[MAX_SEP_LEN];
		unsigned char		pulse_events;
				 int		pulse_tenms;
        unsigned char		pulse_timer_select;
}configuration;

typedef struct
		{
		//Timer/counter values to be used in calculations
		unsigned int		ptcnt;
		unsigned int		ctcnt;
		unsigned int		difference;
		unsigned int		ticks_per_four_deg;
		unsigned int		dwell_t;
		unsigned int		anti_dwell_t;
		unsigned int		p_dwell_off;
		unsigned int		p_dwell_on;
		unsigned int		dwell_off;
		unsigned int		dwell_on;
		unsigned int		dwell_ctcnt;

		// 420a specific
		unsigned char		falls;
		unsigned char		sync;

		//variables relating to current knock monitoring state
		unsigned int		cur_knock_thresh;
		unsigned int		target_time_const;
		unsigned char		window_integ_div;
		unsigned char		curr_integ_idx;
		unsigned long long	lastknock_timer_overflows[MAX_CYLS];
		unsigned int		lastknock_timer_val[MAX_CYLS];
		unsigned int		number_of_knocks[MAX_CYLS];
		unsigned int		lastknock_volts[MAX_CYLS];
		unsigned int		lastknock_tpfd[MAX_CYLS];

		//Flags for calcuations or monitoring
		unsigned char		window_open;
		unsigned char		do_advance;
		unsigned char		do_datalog;

		//datalog variables
		unsigned char		curr_datalog_counter;
		unsigned long long	timer_overflows;

		//internal error counters
		unsigned int		sync_loss;
		unsigned char		window_overruns;
		unsigned char		input_capture_overruns;

		//Engine state variables
		unsigned int 		rpm;
		unsigned int		rknock;
		unsigned char		isknock;
		unsigned char		current_cyl_idx;
		unsigned char		advance_angle;
		}vars;

void read_cfg_from_eep( void );
void write_cfg_to_eep( void );
void setup( void );
