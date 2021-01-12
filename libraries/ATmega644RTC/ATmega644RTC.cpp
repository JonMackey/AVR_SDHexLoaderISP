/*
*	ATmega644RTC.cpp, Copyright Jonathan Mackey 2020
*
*	Class to manage the date and time.  This does not handle leap seconds.
*
*	GNU license:
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*	Please maintain this license information along with authorship and copyright
*	notices in any redistribution of this code.
*
*/
#include "ATmega644RTC.h"
#ifndef __MACH__
#include "DS3231SN.h"
#include <Arduino.h>
#include <avr/sleep.h>
#endif


#ifndef __MACH__
/********************************** RTCInit ***********************************/
void ATmega644RTC::RTCInit(
	time32_t	inTime,
	DS3231SN*	inExternalRTC)
{
	sExternalRTC = inExternalRTC;
	cli();					// Disable interrupts
	TIMSK2 &= ~((1<<TOIE2)|(1<<OCIE2A)|(1<<OCIE2B));		// Make sure all TC2 interrupts are disabled
	
	/*
	*	Set Timer/counter0 to be asynchronous from the CPU clock
	*	with a second external clock (32,768kHz) driving it.
	*
	*	If using a single pin external 32KHz input source THEN
	*	set the EXCLK input bit before enabling asynchronous operation.
	*	Else, using a standard watch crystal as the 32KHz clock source.
	*/
	if (inExternalRTC)
	{
		// The doc states "Writing to EXCLK should be done before asynchronous operation is selected."
		ASSR = _BV(EXCLK);
		ASSR = (_BV(AS2)+_BV(EXCLK));
	} else
	{
		ASSR = _BV(AS2);
	}

	TCNT2 =0;												// Reset timer counter register

	/*
	*	Prescale to 128 (32768/128 = 256Hz with the 8 bit TCNT2 overflowing at 255 results in 1 tick/second)
	*/	
	TCCR2A = (0<<WGM20) | (0<<WGM21);		// Overflow		
	TCCR2B = ((1<<CS22)|(0<<CS21)|(1<<CS20)|(0<<WGM22));	// Prescale the timer to be clock source/128 to make it
															// exactly 1 second for every overflow to occur
	
	// Note that if there is no crystal OR no external clock source, this while() will hang														
	while (ASSR & ((1<<TCN2UB)|(1<<OCR2AUB)|(1<<OCR2BUB)|(1<<TCR2AUB)|(1<<TCR2BUB))){}	//Wait until TC2 is updated
	
	TIMSK2 |= (1<<TOIE2);									// Set 8-bit Timer/Counter0 Overflow Interrupt Enable
	if (!inExternalRTC)
	{
		sTime = inTime;
	}
	sei();													// Set the Global Interrupt Enable Bit
	
}

/********************************* RTCDisable *********************************/
void ATmega644RTC::RTCDisable(void)
{
	// Power-Down mode is currently used to put the MCU to sleep so just
	// disabling the interrupt should be enough.
	cli();						// Disable interrupts
	TIMSK2 &= ~(1<<TOIE2);		// Disable all TC2 interrupts are disabled
	sei();						// Set the Global Interrupt Enable Bit
}

/********************************* RTCEnable **********************************/
void ATmega644RTC::RTCEnable(void)
{
	cli();						// Disable interrupts
	TIMSK2 |= (1<<TOIE2);		// Set 8-bit Timer/Counter0 Overflow Interrupt Enable
	sei();						// Set the Global Interrupt Enable Bit
}

/************************** Timer/Counter2 Overflow ***************************/
ISR(TIMER2_OVF_vect)
{
	UnixTime::Tick();
}
#endif

