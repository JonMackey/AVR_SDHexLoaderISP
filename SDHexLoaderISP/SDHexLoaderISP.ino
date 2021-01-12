/*
*	SDHexLoaderISP.ino, Copyright Jonathan Mackey 2020
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
*/
//#define SERIAL_RX_BUFFER_SIZE	256
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>

#include "SDHexLoaderConfig.h"
#include "TFT_ST7789.h"
#include "SDHexLoader.h"
#include "ATmega644RTC.h"
#include "DS3231SN.h"

TFT_ST7789	display(Config::kDispDCPin, Config::kDispResetPin,
						Config::kDispCSPin, Config::kBacklightPin, 240, 240);
SDHexLoader	hexLoader;
// Define "xFont" to satisfy the auto-generated code within the font files.
// This implementation uses 'hexLoader' as a subclass of XFont
#define xFont hexLoader
#include "MyriadPro-Regular_36_1b.h"
#include "MyriadPro-Regular_18.h"

DS3231SN	externalRTC;

/*********************************** setup ************************************/
void setup(void)
{
	Serial.begin(BAUD_RATE);

	Wire.begin();
	SPI.begin();

	// Pull-up all unused pins
#if (HEX_LOADER_VER >= 12)
	pinMode(Config::kUnusedPinC4, INPUT_PULLUP);
#else
	pinMode(Config::kUnusedPinB1, INPUT_PULLUP);
#endif
#if (HEX_LOADER_VER != 12)
	pinMode(Config::kUnusedPinA0, INPUT_PULLUP);
#endif
#if (HEX_LOADER_VER != 13)
	pinMode(Config::kUnusedPinD7, INPUT_PULLUP);
#endif
	pinMode(Config::kUnusedPinB0, INPUT_PULLUP);
#if (HEX_LOADER_VER < 14)
	pinMode(Config::kUnusedPinD4, INPUT_PULLUP);
#endif
	pinMode(Config::kUnusedPinD6, INPUT_PULLUP);
	
	externalRTC.begin();
	ATmega644RTC::RTCInit(0, &externalRTC);
	UnixTime::SetTimeFromExternalRTC();
	{
		uint8_t	flags;
		EEPROM.get(Config::kFlagsAddr, flags);
		UnixTime::SetFormat24Hour((flags & 1) == 0);	// Default is 12 hour.
	}
	UnixTime::ResetSleepTime();	// Display sleep time

	display.begin(Config::kDisplayRotation);	// Init TFT
	display.Fill();		// Clear the display

	hexLoader.begin(&display, &MyriadPro_Regular_36_1b::font,
								&MyriadPro_Regular_18::font);
}

/************************************ loop ************************************/
void loop(void)
{
	hexLoader.Update();	
}
