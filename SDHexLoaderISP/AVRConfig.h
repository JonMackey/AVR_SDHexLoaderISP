/*
*	AVRConfig.h, Copyright Jonathan Mackey 2020
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


#ifndef AVRConfig_h
#define AVRConfig_h

#include <inttypes.h>

#ifdef __MACH__
#include <stdio.h>
#define SdFile	FILE
#else
class SdFile;
#endif

struct SAVRConfig
{
	char		desc[20];
	uint8_t		devcode;
	uint8_t		signature[3];
	uint16_t	chipEraseDelay;
	uint16_t	eepromMinWriteDelay;
	uint16_t	eepromPageSize;
	uint16_t	eepromSize;
	uint32_t	fCPU;
	uint16_t	flashMinWriteDelay;
	uint16_t	flashPageSize;
	uint16_t	flashReadSize;
	uint16_t	timestamp;
	uint32_t	uploadMaximumSize;
	uint32_t	uploadSpeed;
	uint32_t	byteCount;	// Of related hex file.
};

class AVRConfig
{
public:
							AVRConfig(void);
	bool					ReadFile(
								const char*				inPath);
	const SAVRConfig&		Config(void) const
								{return(mConfig);}
protected:
	SdFile*		mFile;
	SAVRConfig	mConfig;
	
	char					NextChar(void);
	uint8_t					FindKeyIndex(
								const char*				inKey);
	char					SkipWhitespace(
								char					inCurrChar);
	char					SkipWhitespaceAndHashComments(
								char					inCurrChar);
	char					SkipToNextLine(void);
	char					ReadUInt32Number(
								uint32_t&				outValue);
	char					ReadStr(
								char					inDelimiter,
								uint8_t					inMaxStrLen,
								char*					outStr);
};

#endif /* AVRConfig_h */
