/*    Header for for AVR/TPIC8101 based knock monitoring system serial menu functions
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

void newline(void);
void comma(void);
void print_str_P( const char *);
void print_str(char *);
void print_main_menu(void);
void dump_config( void );
void dump_variables( void );
void dump_rpm_table(int);
void dump_voltage_table(int);
void dump_fslopes(int);
void dump_datalog( void );
void dump_fastlog( void );
void calculate_fslopes(int);
unsigned char menu_input(unsigned char );




