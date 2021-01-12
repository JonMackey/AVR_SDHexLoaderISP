/*
*	SerialUtils.h, Copyright Jonathan Mackey 2019
*	Handles serial commands.
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
#ifndef SerialUtils_h
#define SerialUtils_h

#include <time.h>
#ifdef __MACH__
#include <inttypes.h>
#endif

class SerialUtils
{
public:
	static uint32_t			GetUInt32FromSerial(void);
	static uint8_t			GetChar(void);
	static bool				LoadLine(
								uint8_t					inMaxLen,
								char* 					outLine,
								bool					inAddCRLF = false);
	static inline uint8_t	HexAsciiToBin(
								uint8_t					inByte)
								{return (inByte <= '9' ?
											(inByte - '0') :
											(inByte - ('A' - 10)));}

};

#endif // SerialUtils_h
