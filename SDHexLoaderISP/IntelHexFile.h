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
*	IntelHexFile.h
*	Copyright (c) 2020 Jonathan Mackey
*
*	Interprets an IntelHex file per line.
*
*/

#ifndef IntelHexFile_h
#define IntelHexFile_h

#include <inttypes.h>
#ifdef __MACH__
#include <stdio.h>
#define SdFile	FILE
#else
#include "SdFat.h"
#endif

class IntelHexFile
{
public:
							IntelHexFile(void);
	bool					begin(
								const char*				inPath);
	void					end(void);	// Close Hex File
	bool					NextRecord(void);
	uint8_t					RecordType(void) const
								{return(mRecordType);}
	uint16_t				Address(void) const
								{return(mAddress);}
	uint8_t					AddressH(void) const
								{return(mAddressH);}
	uint32_t				Address32(void) const
								{return(((uint32_t)mAddressH << 16) | mAddress);}
	const uint8_t*			Data(void) const
								{return(mData);}
	uint8_t					ByteCount(void) const
								{return(mByteCount);}
	uint32_t				EstimateLength(void);
	bool					Rewind(void);
	enum ERecordType
	{
		eDataRecord,
		eEndOfFileRecord,
		eExtendedSegmentAddress,
		/*eStartSegmentAddress,
		eExtendedLinearAddress,
		eStartLinearAddress,*/
		eInvalidRecordType
	};

protected:
#ifndef __MACH__
	SdFile		mSdFile;
#endif
	SdFile*		mFile;
	bool		mEndOfFile;	// Set when the end of file record is read.
	uint8_t		mByteCount;
	uint8_t		mRecordType;
	uint8_t		mData[16];
	uint8_t		mAddressH; // Represents bits 19:16 of the final address.
	uint16_t	mAddress;

	uint8_t					NextChar(void);
};

#endif /* IntelHexFile_h */
