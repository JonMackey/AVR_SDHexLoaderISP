/*
*	SDHexLoaderConfig.h, Copyright Jonathan Mackey 2020
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
#ifndef SDHexLoaderConfig_h
#define SDHexLoaderConfig_h

#include <inttypes.h>

#define BAUD_RATE	19200
#define HEX_LOADER_VER	13	// 1.3
#define ROTATE_DISPLAY	1	// Rotate display and buttons by 90 degrees

#ifdef __MACH__
#define DEBUG_AVR_STREAM	1
#else
/*
*	Defining DEBUG_AVR_STREAM will send a dump of the SDK500 commands/responses
*	to Serial1 for anything sent to AVRStreamISP.  When set, obviously the SD
*	source should not be used for anything that uses Serial1.
*/
//#define DEBUG_AVR_STREAM	1
#endif

namespace Config
{
	const uint8_t	kUnusedPinB0		= 0;	// PB0
#if (HEX_LOADER_VER >= 12)
	const uint8_t	kReset3v3OEPin		= 1;	// PB1
#else
	const uint8_t	kUnusedPinB1		= 1;	// PB1
#endif
	const uint8_t	kSDSelectPin		= 2;	// PB2
	const uint8_t	kISP_OE_Pin			= 3;	// PB3	t
	const uint8_t	kResetPin			= 4;	// PB4	t
	const uint8_t	kMOSI				= 5;	// PB5
	const uint8_t	kMISO				= 6;	// PB6
	const uint8_t	kSCK				= 7;	// PB7
	
	const uint8_t	kRxPin				= 8;	// PD0
	const uint8_t	kTxPin				= 9;	// PD1
	const uint8_t	kRx1Pin				= 10;	// PD2
	const uint8_t	kTx1Pin				= 11;	// PD3
#if (HEX_LOADER_VER >= 14)
	const uint8_t	kHeartbeatPin		= 12;	// PD4	t
#else
	const uint8_t	kUnusedPinD4		= 12;	// PD4	t
#endif
	const uint8_t	kSDDetectPin		= 13;	// PD5	PCINT29	t
	const int8_t	kUnusedPinD6		= 14;	// PD6	t
#if (HEX_LOADER_VER == 13)
	const uint8_t	kHeartbeatPin		= 15;	// PD7	t
#else
	const uint8_t	kUnusedPinD7		= 15;	// PD7	t
#endif

	const uint8_t	kSCL				= 16;	// PC0
	const uint8_t	kSDA				= 17;	// PC1
	const uint8_t	kErrorPin			= 18;	// PC2
	const uint8_t	kProgModePin		= 19;	// PC3
#if (HEX_LOADER_VER >= 12)
	const uint8_t	kUnusedPinC4		= 20;	// PC4
#else
	const uint8_t	kHeartbeatPin		= 20;	// PC4
#endif
	const uint8_t	kDispDCPin			= 21;	// PC5
	const uint8_t	k32KHzPin			= 22;	// PC6
	const uint8_t	kDispCSPin			= 23;	// PC7
	
#if (HEX_LOADER_VER == 12)
	const uint8_t	kHeartbeatPin		= 24;	// PA0
#else
	const int8_t	kUnusedPinA0		= 24;	// PA0
#endif
	const uint8_t	kDispResetPin		= 25;	// PA1
	const uint8_t	kBacklightPin		= 26;	// PA2
	const uint8_t	kUpBtnPin			= 27;	// PA3	PCINT3
	const uint8_t	kLeftBtnPin 		= 28;	// PA4	PCINT4
	const uint8_t	kEnterBtnPin		= 29;	// PA5	PCINT5
	const uint8_t	kRightBtnPin		= 30;	// PA6	PCINT6
	const uint8_t	kDownBtnPin			= 31;	// PA7	PCINT7

	const uint8_t	kEnterBtn			= _BV(PINA5);
#ifndef ROTATE_DISPLAY
	const uint8_t	kDisplayRotation	= 2;	// 180
	const uint8_t	kRightBtn			= _BV(PINA6);
	const uint8_t	kDownBtn			= _BV(PINA7);
	const uint8_t	kUpBtn				= _BV(PINA3);
	const uint8_t	kLeftBtn			= _BV(PINA4);
#else
	const uint8_t	kDisplayRotation	= 3;	// 270
	const uint8_t	kUpBtn 				= _BV(PINA6);
	const uint8_t	kRightBtn			= _BV(PINA7);
	const uint8_t	kLeftBtn			= _BV(PINA3);
	const uint8_t	kDownBtn			= _BV(PINA4);
#endif
	const uint8_t	kPINABtnMask = (_BV(PINA3) | _BV(PINA4) | _BV(PINA5) | _BV(PINA6) | _BV(PINA7));

	const uint8_t	kSDDetect			= _BV(PIND5);

	/*
	*	EEPROM usage, 2K bytes
	*
	*	[0]		uint8_t		flags, bit 0 is for 24 hour clock format on all boards.
	*						0 = 24 hour
	*						bit 1 is enable sleep, 1 = enable (default)
	*						bit 2 is force ISP, 0 = force, 1 = auto (default)
	*	[1]		uint8_t		ISP SPI clock speed.  Clock = value * 4 MHz, where
	*						0 is 1 MHz.
	*/
	const uint16_t	kFlagsAddr			= 0;
	const uint8_t	k12HourClockBit		= 0;
	const uint8_t	kEnableSleepBit		= 1;
	const uint8_t	kOnlyUseISPBit		= 2;
	const uint16_t	kISP_SPIClockAddr	= 1;


	const uint8_t	kTextInset			= 3; // Makes room for drawing the selection frame
	const uint8_t	kTextVOffset		= 6; // Makes room for drawing the selection frame
	// To make room for the selection frame the actual font height in the font
	// file is reduced.  The actual height is kFontHeight.
	const uint8_t	kFontHeight			= 43;
	const uint8_t	kDisplayWidth		= 240;
}

#endif // SDHexLoaderConfig_h

