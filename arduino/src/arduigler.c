/*Done by S. Tahir Ali (tahir00ali@gmail.com).
 *The files from Teensy RawHID example have been used/modified to be used on
 *ATmega16u2 on Arduino UNO R3 running HoodLoader2 https://github.com/NicoHood/HoodLoader2  
 *All credit to original authors.
 *Goal: was to port Arduigler code and run it has HID instead of RS232 for speed,
 *but for some reasons the speed of jtag operation remained slow/same
 *Myabe its timing issue?
 *Feel free to make any changes and use the code.
 */

/* Teensy RawHID example
 * http://www.pjrc.com/teensy/rawhid.html
 * Copyright (c) 2009 PJRC.COM, LLC
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above description, website URL and copyright notice and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "usb_rawhid.h"

#define CPU_PRESCALE(n)	(CLKPR = 0x80, CLKPR = (n))

const uint8_t CMD_RESET  = 0x74; //ASCII for t
const uint8_t CMD_SEND   = 0x73; //ASCII for s
const uint8_t CMD_READ   = 0x72; //ASCII for r

const uint8_t STATUS_OK   = 0x6B; //ASCII for k
const uint8_t STATUS_ERR = 0x65; //ASCII for e

uint8_t buffer[16];

int main(void)
{
	int8_t r;
	uint8_t cmd;
	// ******************************************************************************
	// PORTD on ATmega16u2 is ICSP header.
	// You need to solder the additional 4 pin header to access all 7 PORTB I/O pins
	// PB4 to PB7 is on the additional 4 pin header.
	//                                               |GND|PC1|
	// Additional 4 pin header JP2 ---->>    |PB6|PB7|PB2|PB1|   <--- ICSP Header
	//                                       |PB4|PB5|+5v|PB3|
	// PB0 = NC
	// PB1 = 
	// PB2 = RST
	// PB3 = TMS
	// PB4 = TCK
	// PB5 = TDI
	// PB6 = TRST
	// PB7 = TDO
	// ******************************************************************************
	
	DDRD = DDRD | 0x30; //DDRD has RX and TX LEDs bit4 RX, bit5 TX
	
	//  Pins bit D7 TDO Input, D6 TRST, D5 TDI, D4 TCK, D3 TMS, D2 WRST Output 
	// 1 Output 0 Input
	DDRB = 0x7F; // 0x7F is 0111 1111 // bit 7 input, 0 - 6 Output 
	PORTB &= 0x80; // make D0 to D6 low (0x80 is 1000 0000)
	
	// set for 16 MHz clock
	CPU_PRESCALE(0);

	// Initialize the USB, and then wait for the host to set configuration.
	// If the Arduino is powered without a PC connected to the USB port,
	// this will wait forever.
	usb_init();

	while (!usb_configured()) /* wait */ ;


	// Wait an extra second for the PC's operating system to load drivers
	// and do whatever it does to actually be ready for input
	_delay_ms(1000);

        

	while (1) { // infinite loop
		// if received data, do something with it
		r = usb_rawhid_recv(buffer, 0);
		if (r > 0) {
	
			PORTD &= ~(1<<4); // Turn RX Led on by setting it LOW
			
			cmd = (uint8_t) buffer[0]; // buffer[0] contains Commands and Status while buffer[1] contains data
			
			if(cmd == CMD_RESET){			// RESET
				PORTB &= 0x80; // make D0 to D6 low (0x80 is 1000 0000)
				buffer[0] = STATUS_OK;
				usb_rawhid_send(buffer, 50);				
			} else if(cmd == CMD_READ){		// READ
				// report back status of TDO 
				// send back bit 7 as 1 if high or 0 if low
				PORTD |= (1<<4); // Turn Off RX Led
				
				buffer[1] = (PINB >> 7) == 1? 0x80 : 0x00;
				
				PORTD &= ~(1<<5); // Turn On TX Led by setting it low
				usb_rawhid_send(buffer, 50);
				PORTD |= (1<<5); // turn off TX Led by setting it high				
			} else if(cmd == CMD_SEND){		// SEND
				// Set PORTB
				PORTB =  (((buffer[1] & 0xFF) & 0x1F ) << 2 ); // 0x1F is 0001 1111
				buffer[0] = STATUS_OK;
				usb_rawhid_send(buffer, 50);
			} else {						// BAD Operation
				 buffer[0] = STATUS_ERR;
				 usb_rawhid_send(buffer, 50);				 
			} 
			PORTD |= (1<<4);
			
		}
		
	}
}