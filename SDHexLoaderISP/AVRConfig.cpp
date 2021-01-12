/*
*	AVRConfig.cpp, Copyright Jonathan Mackey 2020
*
*	This is a minimal parser for key=value config files of the same form as
*	boards.txt and platform.txt accept this class interprets the values as
*	either strings or numbers with support for notted values and hex values.
*	Only the select set of keys defined below are read.  Any other keys are
*	skipped.
*
*	To keep the code size small, very little error checking performed.
*	It's assumed that the config file was created using the HexLoaderUtility
*	application with Unix (LF) line endings.
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
#include "AVRConfig.h"
#ifndef __MACH__
#include <Arduino.h>
#include "SdFat.h"
#include "sdios.h"
#else
#include <string>
#define PROGMEM
#define pgm_read_ptr_near
#define strcmp_P strcmp
#endif

/*
*	The list of desired keys
*/
const char kByteCountKeyStr[] PROGMEM = "byte_count";
const char kChipEraseDelayKeyStr[] PROGMEM = "chip_erase_delay";
const char kDescKeyStr[] PROGMEM = "desc";
const char kEepromMinWriteDelayKeyStr[] PROGMEM = "eeprom.min_write_delay";
const char kEepromPageSizeKeyStr[] PROGMEM = "eeprom.page_size";
const char kEepromSizeKeyStr[] PROGMEM = "eeprom.size";
const char kFCPUKeyStr[] PROGMEM = "f_cpu";
const char kFlashMinWriteDelayKeyStr[] PROGMEM = "flash.min_write_delay";
const char kFlashPageSizeKeyStr[] PROGMEM = "flash.page_size";
const char kFlashReadSizeKeyStr[] PROGMEM = "flash.readsize";
const char kSignatureKeyStr[] PROGMEM = "signature";
const char kSTK500DevCodeKeyStr[] PROGMEM = "stk500_devcode";
const char kTimestampKeyStr[] PROGMEM = "timestamp";
const char kUploadMaximumSizeKeyStr[] PROGMEM = "upload.maximum_size";
const char kUploadSpeedKeyStr[] PROGMEM = "upload.speed";

const char* const kDesiredConfigKeys[] PROGMEM =
{	// Sorted alphabetically
	kByteCountKeyStr,
	kChipEraseDelayKeyStr,
	kDescKeyStr,
	kEepromMinWriteDelayKeyStr,
	kEepromPageSizeKeyStr,
	kEepromSizeKeyStr,
	kFCPUKeyStr,
	kFlashMinWriteDelayKeyStr,
	kFlashPageSizeKeyStr,
	kFlashReadSizeKeyStr,
	kSignatureKeyStr,
	kSTK500DevCodeKeyStr,
	kTimestampKeyStr,
	kUploadMaximumSizeKeyStr,
	kUploadSpeedKeyStr
};

enum EDesiredKeyIndexes
{
	eInvalidKeyIndex,
	// Must align with kDesiredConfigKeys
	eByteCount,
	eChipEraseDelay,
	eDesc,
	eEepromMinWriteDelay,
	eEepromPageSize,
	eEepromSize,
	eFCPU,
	eFlashMinWriteDelay,
	eFlashPageSize,
	eFlashReadSize,
	eSignature,
	eSTK500DevCode,
	eTimestamp,
	eUploadMaximumSize,
	eUploadSpeed
};

/********************************* AVRConfig **********************************/
AVRConfig::AVRConfig(void)
: mFile(nullptr)
{
}

/********************************** ReadFile **********************************/
/*
*	It's assumed SdFat.begin was successfully called prior to calling this
*	routine.
*	This routine returns true if the file contains all of the following
*	key values: desc, signature, upload.speed, f_cpu, flash.page_size
*/
bool AVRConfig::ReadFile(
	const char*	inPath)
{
	mConfig = {0};
	uint8_t	requiredKeyValues = 0;
#ifndef __MACH__
	SdFile file;
	bool	fileOpened = file.open(inPath, O_RDONLY);
	mFile = &file;
#else
	mFile = fopen(inPath, "r+");
	bool	fileOpened = mFile != nullptr;
#endif
	if (fileOpened)
	{
		char	thisChar = NextChar();
		while (thisChar)
		{
			thisChar = SkipWhitespaceAndHashComments(thisChar);
			if (thisChar)
			{
				char keyStr[32];
				keyStr[0] = thisChar;
				thisChar = ReadStr('=', 31, &keyStr[1]);
				while (thisChar)
				{
					uint8_t keyIndex = FindKeyIndex(keyStr);
					if (keyIndex)
					{
						// There's currently only one key value of type
						// string.  All the rest of the desired config
						// values are numbers.
						if (keyIndex != eDesc)
						{
							uint32_t	value;
							thisChar = ReadUInt32Number(value);
							switch (keyIndex)
							{
							
								case eByteCount:
									mConfig.byteCount = value;
									break;
								case eChipEraseDelay:
									mConfig.chipEraseDelay = value;
									break;
								case eEepromMinWriteDelay:
									mConfig.eepromMinWriteDelay = value;
									break;
								case eEepromPageSize:
									mConfig.eepromPageSize = value;
									break;
								case eEepromSize:
									mConfig.eepromSize = value;
									break;
								case eFCPU:
									mConfig.fCPU = value;
									requiredKeyValues++;
									break;
								case eFlashMinWriteDelay:
									mConfig.flashMinWriteDelay = value;
									break;
								case eFlashPageSize:
									mConfig.flashPageSize = value;
									requiredKeyValues++;
									break;
								case eFlashReadSize:
									mConfig.flashReadSize = value;
									break;
								case eSignature:
									mConfig.signature[0] = value >> 16;
									mConfig.signature[1] = value >> 8;
									mConfig.signature[2] = value;
									requiredKeyValues++;
									break;
								case eSTK500DevCode:
									mConfig.devcode = value;
									break;
								case eTimestamp:
									mConfig.timestamp = value;
									break;
								case eUploadMaximumSize:
									mConfig.uploadMaximumSize = value;
									break;
								case eUploadSpeed:
									mConfig.uploadSpeed = value;
									requiredKeyValues++;
									break;
							}
						} else
						{
							thisChar = ReadStr('\n', sizeof(mConfig.desc), mConfig.desc);
							requiredKeyValues++;
						}
					} else
					{
						thisChar = SkipToNextLine();
					}
					break;
				}
			}
		}
	#ifndef __MACH__
		mFile->close();
	#else
		fclose(mFile);
	#endif
		mFile = nullptr;
	}
	return(requiredKeyValues == 5);
}

/********************************** NextChar **********************************/
char AVRConfig::NextChar(void)
{
	char	thisChar;
#ifdef __MACH__
	thisChar = getc(mFile);
	if (thisChar == -1)
	{
		thisChar = 0;
	}
#else
	if (mFile->read(&thisChar,1) != 1)
	{
		thisChar = 0;
	}
#endif
	return(thisChar);
}

/******************************** FindKeyIndex ********************************/
/*
*	Returns the index of inKey within the array kDesiredConfigKeys + 1.
*	Returns 0 if inKey is not found.
*/
uint8_t AVRConfig::FindKeyIndex(
	const char*	inKey)
{
	uint8_t leftIndex = 0;
	uint8_t rightIndex = (sizeof(kDesiredConfigKeys)/sizeof(char*)) -1;
	while (leftIndex <= rightIndex)
	{
		uint8_t current = (leftIndex + rightIndex) / 2;
	#ifndef __MACH__
		const char* currentPtr = (const char*)pgm_read_ptr_near(&kDesiredConfigKeys[current]);
	#else
		const char* currentPtr = kDesiredConfigKeys[current];
	#endif
		int cmpResult = strcmp_P(inKey, currentPtr);
		if (cmpResult == 0)
		{
			return(current+1);	// Add 1, 0 is reserved for "not found"
		} else if (cmpResult < 0)
		{
			rightIndex = current - 1;
		} else
		{
			leftIndex = current + 1;
		}
	}
	return(0);
}

/******************************* SkipWhitespace *******************************/
char AVRConfig::SkipWhitespace(
	char	inCurrChar)
{
	char	thisChar = inCurrChar;
	for (; thisChar != 0; thisChar = NextChar())
	{
		if (isspace(thisChar))
		{
			continue;
		}
		break;
	}
	return(thisChar);
}

/*********************** SkipWhitespaceAndHashComments ************************/
char AVRConfig::SkipWhitespaceAndHashComments(
	char	inCurrChar)
{
	char	thisChar = inCurrChar;
	while (thisChar)
	{
		if (isspace(thisChar))
		{
			thisChar = NextChar();
			continue;
		} else if (thisChar == '#')
		{
			thisChar = SkipToNextLine();
			continue;
		}
		break;
	}
	return(thisChar);
}

/******************************* SkipToNextLine *******************************/
/*
*	Returns the character following the newline (if any).
*	Does not support Windows CRLF line endings.
*/
char AVRConfig::SkipToNextLine(void)
{
	char thisChar = NextChar();
	for (; thisChar; thisChar = NextChar())
	{
		if (thisChar != '\n')
		{
			continue;
		}
		thisChar = NextChar();
		break;
	}
	return(thisChar);
}

/****************************** ReadUInt32Number ******************************/
char AVRConfig::ReadUInt32Number(
	uint32_t&	outValue)
{
	bool		bitwiseNot = false;
	bool 		isHex = false;
	uint32_t	value = 0;
	char		thisChar = SkipWhitespaceAndHashComments(NextChar());
	if (thisChar)
	{
		bitwiseNot = thisChar == '~';
		/*
		*	If notted THEN
		*	get the next char after the not.
		*/
		if (bitwiseNot)
		{
			thisChar = SkipWhitespaceAndHashComments(NextChar());
		}
		if (thisChar == '0')
		{
			thisChar = NextChar();	// Get the character following the leading zero.
			isHex = thisChar == 'x';
			/*
			*	If this is a hex prefix THEN
			*	get the character following it.
			*/
			if (isHex)
			{
				thisChar = NextChar();
			}
		}
		// Now just consume characters till a non valid numeric char is hit.
		if (isHex)
		{
			for (; isxdigit(thisChar); thisChar = NextChar())
			{
				thisChar -= '0';
				if (thisChar > 9)
				{
					thisChar -= 7;
					if (thisChar > 15)
					{
						thisChar -= 32;
					}
				}
				value = (value << 4) + thisChar;
			}
		} else
		{
			for (; isdigit(thisChar); thisChar = NextChar())
			{
				value = (value * 10) + (thisChar - '0');
			}
		}
		thisChar = SkipWhitespaceAndHashComments(thisChar);
	}
	if (bitwiseNot)
	{
		value = ~value;
	}
	outValue = value;
	return(thisChar);
}

/********************************** ReadStr ***********************************/
/*
*	Reads the string till the delimiter is hit.
*	Once the string is full (inMaxStrLen), characters are ignored till the
*	delimiter is hit.
*	Hash (#) comments are not allowed because the strings aren't quoted.
*/
char AVRConfig::ReadStr(
	char	inDelimiter,
	uint8_t	inMaxStrLen,
	char*	outStr)
{
	char		thisChar;
	char*		endOfStrPtr = &outStr[inMaxStrLen -1];

	while((thisChar = NextChar()) != 0)
	{
		if (thisChar != inDelimiter)
		{
			/*
			*	If outStr isn't full THEN
			*	append thisChar to it.
			*/
			if (outStr < endOfStrPtr)
			{
				*(outStr++) = thisChar;
			} // else discard the character, the outStr is full
			continue;
		}
		break;
	}
	*outStr = 0;	// Terminate the string
	return(thisChar);
}

