/*
*	DS3231SN.cpp, Copyright Jonathan Mackey 2020
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

#include "Arduino.h"
#include "DS3231SN.h"
#include <Wire.h>


/********************************** DS3231SN **********************************/
DS3231SN::DS3231SN(
	uint8_t	inDeviceAddress)
	: mDeviceAddress(inDeviceAddress)
{
}

/*********************************** begin ************************************/
void DS3231SN::begin(
	bool	inEnable32KHzOutput)
{
	Wire.begin();
	Wire.beginTransmission(mDeviceAddress);
	Wire.write(eControlStatusReg);
	Wire.write(inEnable32KHzOutput ? _BV(eEN32KHz) : 0);
	Wire.endTransmission(true);
}

uint8_t HexToBCD(
	uint8_t	inByte)
{
	return((inByte % 10) + ((inByte / 10) * 16));
}

uint8_t BCDToHex(
	uint8_t	inBCD)
{
	return((inBCD % 16) + ((inBCD / 16) * 10));
}

/********************************** SetTime ***********************************/
// inDateAndTime is an array of 7 unsigned bytes.
// The data order is the same as ERegAddr
void DS3231SN::SetTime(
	DSDateTime&	inDateAndTime) const
{
	Wire.beginTransmission(mDeviceAddress);
	Wire.write(eSecondsReg);
	for (uint8_t i = 0; i < 7; i++)
	{
		Wire.write(HexToBCD(inDateAndTime.da[i]));
	}
	Wire.endTransmission(true);
}

/********************************** GetTime ***********************************/
void DS3231SN::GetTime(
	DSDateTime&	inDateAndTime) const
{
	Wire.beginTransmission(mDeviceAddress);
	Wire.write(eSecondsReg);
	Wire.endTransmission(true);
	uint8_t	bytesRead = Wire.requestFrom(mDeviceAddress, (uint8_t)7, (uint8_t)true);
	if (bytesRead == 7)
	{
		for (uint8_t i = 0; i < 7; i++)
		{
			inDateAndTime.da[i] = BCDToHex(Wire.read());
		}
	}
}
