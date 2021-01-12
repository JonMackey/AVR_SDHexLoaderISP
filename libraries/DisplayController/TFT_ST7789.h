/*
*	TFT_ST7789.h, Copyright Jonathan Mackey 2019
*	Class to control an SPI TFT ST7789 display controller.
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
#ifndef TFT_ST7789_h
#define TFT_ST7789_h

#include "TFT_ST77XX.h"

class TFT_ST7789 : public TFT_ST77XX
{
public:
							TFT_ST7789(
								uint8_t					inDCPin,
								int8_t					inResetPin = -1,	
								int8_t					inCSPin = -1,
								int8_t					inBacklightPin = -1,
								uint16_t				inHeight = 240,
								uint16_t				inWidth = 240,
								bool					inCentered = false,
								bool					inIsBGR = false);
protected:
	virtual void			Init(void);
	virtual uint16_t		VerticalRes(void) const
								{return(320);}
	virtual uint16_t		HorizontalRes(void) const
								{return(240);}
};

#endif // TFT_ST7789_h
