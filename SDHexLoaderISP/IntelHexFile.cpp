/*******************************************************************************
	License
	****************************************************************************
	This program is free software; you can redistribute it
	and/or modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation; either version 3 of the License, or
	(at your option) any later version.
 
	This program is distributed in the hope that it will
	be useful, but WITHOUT ANY WARRANTY; without even the
	implied warranty of MERCHANTABILITY or FITNESS FOR A
	PARTICULAR PURPOSE. See the GNU General Public
	License for more details.
 
	Licence can be viewed at
	http://www.gnu.org/licenses/gpl-3.0.txt
//
	Please maintain this license information along with authorship
	and copyright notices in any redistribution of this code
*******************************************************************************/
/*
*	IntelHexFile.cpp
*	Copyright (c) 2020 Jonathan Mackey
*/
#include "IntelHexFile.h"
#ifndef __MACH__
#include <Arduino.h>
#include "sdios.h"
#endif

/******************************** IntelHexFile ********************************/
IntelHexFile::IntelHexFile(void)
	: mFile(nullptr)
{
}

/*********************************** begin ************************************/
bool IntelHexFile::begin(
	const char*	inPath)
{
#ifdef __MACH__
	mFile = fopen(inPath, "r+");
	bool success = mFile != nullptr;
#else
	bool success = mSdFile.open(inPath, O_RDONLY);
	mFile = success ? &mSdFile : nullptr;
#endif
	Rewind();
	return(success);
}

/************************************ end *************************************/
void IntelHexFile::end(void)
{
	if (mFile)
	{
	#ifndef __MACH__
		mFile->close();
	#else
		fclose(mFile);
	#endif
		mFile = nullptr;
	}
}

/*********************************** Rewind ***********************************/
bool IntelHexFile::Rewind(void)
{
	mRecordType = eInvalidRecordType;
	mByteCount = 0;
	mAddressH = 0;
	mEndOfFile = false;
	bool success = false;
	if (mFile)
	{
	#ifdef __MACH__
		success = fseek(mFile, 0, SEEK_SET) == 0;
	#else
		success = mFile->seekSet(0);
	#endif
	}
	return(success);
}

/********************************** NextChar **********************************/
uint8_t IntelHexFile::NextChar(void)
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

/********************************* NextRecord *********************************/
bool IntelHexFile::NextRecord(void)
{
	uint8_t	checksum = 1;	// Error if NextChar doesn't return a startcode (:)
	mRecordType = eInvalidRecordType;
	/*
	*	If startcode...
	*/
	if (NextChar() == ':')
	{
		bool	highNibble = true;
		uint8_t	field = 0;
		uint8_t	nibble;
		uint8_t	thisByte = 0;
		uint8_t	dataIndex = 0;
		checksum = 0;
		while((nibble = NextChar()))
		{
			nibble -= '0';
			if (nibble >= ('A'-'0')) // Only support uppercase hex ascii
			{
				nibble -= 7;
			}
			if (highNibble)
			{
				highNibble = false;
				thisByte = nibble << 4;
				continue;
			} else
			{
				highNibble = true;
				thisByte += nibble;
				//fprintf(stderr, "%02hhX", thisByte);
				checksum += thisByte;
				field++;
				switch (field)
				{
					case 1:	// Byte count
						mByteCount = thisByte;
						if (mByteCount <= 16)
						{
							continue;
						}
						checksum = 1;	// Flag as error and exit
						break;
					case 2:	// Address high
						mAddress = (uint16_t)thisByte << 8;
						continue;
					case 3: // Address low
						mAddress |= thisByte;
						continue;
					case 4:	// Record type
						mRecordType = thisByte;
						switch (thisByte)
						{
							case 0:	// Data
								dataIndex = 0;
								field = 9;	// field+1 = 10 -> 26 is data/default
								continue;
							case 1:	// End Of File
								mEndOfFile = true;
								field = 6;	// Checksum (field+1 = 7)
								continue;
							case 2: // Extended Segment Address
								// Extended Segment Address High Byte (field+1 = 5)
								mByteCount = 0;
								continue;
							default: // The other record types can be treated as errors.
								break;
						}
						checksum = 1;	// Flag as error and exit
						break;
					case 5:	// Extended Segment Address High Byte
						// Extended segment addresses are in the range 0x1000 to
						// 0xF000.  To get the final address you multiply this
						// value by 16 and add mAddress.  For our purposes we
						// only need to know the high 4 bits of the 20 bit address.
						// Ex: 0x10 becomes 0x01 representing address bits 19:16.
						// This allows addressing up to 1MB of address space.
						mAddressH = thisByte >> 4;
						continue;
					case 6:	// Extended Segment Address Low Byte
							// Ignore, the low byte should always be zero.
						continue;
					case 7:	// Checksum
						// Skip the line ending (CRLF or LF)
						if (NextChar() == '\r')
						{
							NextChar();
						}
						break;	// Exit. checksum should be zero at this point.
					default:	// Data
						mData[dataIndex++] = thisByte;
						if (dataIndex < mByteCount)
						{
							continue;
						}
						field = 6;	// Checksum (field+1 = 7)
						continue;
				}
			}
			break;
		}
	}
	//fprintf(stderr, "\n");
	return(checksum == 0);
}

/******************************* EstimateLength *******************************/
/*
*	Because for large files it takes a while to read the entire file using
*	SdFat, this code attempts to estimate the data length by reading the first
*	data record and the last data record.  Then, based on the expected number of
*	bytes in each row an estimate of the number of 64K segments is added to the
*	total.
*
*	The assumption is that the data is contiguous, therefore the value returned
*	is an estimate.  The estimated length is used for the progress indicator.
*
*	This code will return the wrong estimate when just under the edge case of
*	mutiples of 64KB.  In most cases the space occupied by the bootloader keeps
*	this from happening.  I suppose you could check the last address to catch
*	this edge case.
*/
uint32_t IntelHexFile::EstimateLength(void)
{
	uint32_t	estimatedLength = 0;
	if (mFile)
	{
		size_t	fileSize;
	#ifdef __MACH__
		fseek(mFile, 0, SEEK_END);
		fileSize = ftell(mFile);
		fseek(mFile, 0, SEEK_SET);
	#else
		fileSize = mFile->fileSize();
	#endif
		if (fileSize > 256)
		{
			while (NextRecord() && RecordType() != eDataRecord){}
			uint32_t	startingAddress = Address32();
			if (
		#ifdef __MACH__
				fseek(mFile, fileSize-256, SEEK_SET) == 0
		#else
				mFile->seekSet(fileSize - 256)
		#endif
			)
			{
				// Skip to the start of the next line.
				uint8_t thisChar = NextChar();
				for (; thisChar; thisChar = NextChar())
				{
					if (thisChar != '\r')
					{
						if (thisChar != '\n')
						{
							continue;
						}
					} else
					{
						// Skip the expected (NL) that follows a (CR)
						NextChar();
					}
					break;
				}
				uint16_t	lastAddress = 0;
				// Get the address of the last data byte
				while (NextRecord() && RecordType() == eDataRecord)
				{
					lastAddress = Address() + ByteCount();
				}
				/*
				*	Hex lines supported by this class have a maximum of 16 data
				*	bytes.  This means that the line, including the line ending,
				*	is 44 or 45 characters depending on whether it's Windows or
				*	Unix.
				*
				*	appLineCount is an estimated line count.  Extended segment
				*	address records and the misalignment that occurs when
				*	transitioning from the code (aka text) to the data segment
				*	may effect the line count estimate.
				*/
				uint32_t	appLineCount = (uint32_t)fileSize/(thisChar == '\r' ? 45 : 44);
				estimatedLength = ((appLineCount << 4) & 0xF0000) + lastAddress - startingAddress;
			}
			Rewind();
		}
	}
	return(estimatedLength);
}
