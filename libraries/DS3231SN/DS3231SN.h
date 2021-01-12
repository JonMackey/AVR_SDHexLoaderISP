/*
*	DS3231SN.h, Copyright Jonathan Mackey 2020
*	Minimal class to access/control the DS3231SN RTC Chip.
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

#ifndef DS3231SN_H
#define DS3231SN_H

union DSDateTime
{
	struct
	{
		uint8_t	second;	// 0
		uint8_t	minute;	// 1
		uint8_t	hour;	// 2
		uint8_t	day;	// 3
		uint8_t	date;	// 4
		uint8_t	month;	// 5
		uint8_t	year;	// 6
	} dt;
	uint8_t	da[7];
};

class DS3231SN
{
public:
							DS3231SN(
								uint8_t					inDeviceAddress = 0x68);
	void					begin(
								bool					inEnable32KHzOutput = true);
	void					SetTime(
								DSDateTime&				inDateAndTime) const;
	void					GetTime(
								DSDateTime&				inDateAndTime) const;
protected:
	uint8_t		mDeviceAddress;

enum ERegAddr
{
	eSecondsReg,
	eMinutesReg,
	eHoursReg,
	eDayReg,
	eDateReg,	// Day of month
	eMonthCenturyReg,
	eYearReg,
	eAlarm1SecondsReg,
	eAlarm1MinutesReg,
	eAlarm1HoursReg,
	eAlarm1DayDateReg,
	eAlarm2MinutesReg,
	eAlarm2HoursReg,
	eAlarm2DayDateReg,
	eControlReg,
	eControlStatusReg,
	eAgingOffsetReg,
	eTempMSBReg,
	eTempLSBReg
};

enum EHoursReg
{
	e12HourFormat	= 6
};

enum EControlReg
{
	eA1IE,	// Alarm 1 Interrupt Enable (A1IE), POR 0
	eA2IE,	// Alarm 2 Interrupt Enable (A2IE), POR 0
	eINTCN,	// Interrupt Control (INTCN), POR 1
	eRS1,	// Rate Select (RS2 and RS1), POR 1, 1
	eRS2,	// 
	eCONV,	// Convert Temperature (CONV), POR 0
	eBBSQW,	// Battery-Backed Square-Wave Enable (BBSQW), POR 0
	eEOSC	// Enable Oscillator (!EOSC), POR 0
};

enum EControlStatusReg
{
	eA1F,		// Alarm 1 Flag (A1F)
	eA2F,		// Alarm 2 Flag (A2F)
	eBSY,		// Busy (BSY)
	eEN32KHz,	// Enable 32kHz Output (EN32kHz), POR 1
	eOSF			= 7	// Oscillator Stop Flag (OSF), POR 1
};
};

#endif