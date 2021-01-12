/*
*	DataStream.cpp, Copyright Jonathan Mackey 2019
*	Base class for a generic data stream.
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
#include "DataStream.h"
#ifdef __MACH__
#include "pgmspace_stub.h"
#else
#include <avr/pgmspace.h>
#include <EEPROM.h>
#endif
#include <string.h>

/******************************* DataStreamImpl *******************************/
DataStreamImpl::DataStreamImpl(
	const void*	inStartAddress,
	uint32_t	inLength)
	: mStartAddr((const uint8_t*)inStartAddress), mCurrent((const uint8_t*)inStartAddress),
	  mEndAddr((const uint8_t*)inStartAddress + inLength)
{
}

/************************************ Seek ************************************/
bool DataStreamImpl::Seek(
	int32_t		inOffset,
	EOrigin		inOrigin)
{
	const uint8_t*	newCurrent;
	switch (inOrigin)
	{
		case eSeekSet:
			newCurrent = mStartAddr + inOffset;
			break;
		case eSeekCur:
			newCurrent = mCurrent + inOffset;
			break;
		case eSeekEnd:
			newCurrent = mEndAddr + inOffset;
			break;
	}
	bool success = newCurrent >= mStartAddr && newCurrent <= mEndAddr;
	if (success)
	{
		mCurrent = newCurrent;
	}
	return(success);
}

/*********************************** AtEOF ************************************/
bool DataStreamImpl::AtEOF(void) const
{
	return(mCurrent >= mEndAddr);
}

/*********************************** GetPos ***********************************/
uint32_t DataStreamImpl::GetPos(void) const
{
	return((uint32_t)(mCurrent-mStartAddr));
}

/************************************ Clip ************************************/
/*
*	Used to preflight/clip the requested length so that it doesn't overrun mEndAddr.
*/
uint32_t DataStreamImpl::Clip(
	uint32_t	inLength) const
{
	return((mCurrent + inLength) <= mEndAddr ? inLength : (uint32_t)(mEndAddr - mCurrent));
}


/****************************** DataStream_S *******************************/
DataStream_S::DataStream_S(
	const void*	inStartAddress,
	uint32_t	inLength)
	: DataStreamImpl(inStartAddress, inLength)
{
}

/************************************ Read ************************************/
uint32_t DataStream_S::Read(
	uint32_t	inLength,
	void*		outBuffer)
{
	uint32_t	bytesRead = Clip(inLength);
	memcpy(outBuffer, mCurrent, bytesRead);
	mCurrent += bytesRead;
	return(bytesRead);
}

/*********************************** Write ************************************/
uint32_t DataStream_S::Write(
	uint32_t	inLength,
	const void*	inBuffer)
{
	uint32_t	bytesWritten = Clip(inLength);
	memcpy((void*)mCurrent, inBuffer, bytesWritten);
	mCurrent += bytesWritten;
	return(bytesWritten);
}

/******************************** DataStream_P ********************************/
DataStream_P::DataStream_P(
	const void*	inStartAddress,
	uint32_t	inLength)
	: DataStreamImpl(inStartAddress, inLength)
{
}

/************************************ Read ************************************/
uint32_t DataStream_P::Read(
	uint32_t	inLength,
	void*		outBuffer)
{
	uint32_t	bytesRead = Clip(inLength);
	memcpy_P(outBuffer, mCurrent, bytesRead);
	mCurrent += bytesRead;
	return(bytesRead);
}

/*********************************** Write ************************************/
uint32_t DataStream_P::Write(
	uint32_t	inLength,
	const void*	inBuffer)
{
	// Does nothing
	return(Clip(inLength));
}

/****************************** DataStream_E *******************************/
DataStream_E::DataStream_E(
	const void*	inStartAddress,
	uint32_t	inLength)
	: DataStreamImpl(inStartAddress, inLength)
{
}

/************************************ Read ************************************/
uint32_t DataStream_E::Read(
	uint32_t	inLength,
	void*		outBuffer)
{
	uint32_t	bytesRead = Clip(inLength);
#ifndef EEPROM_h
	memcpy(outBuffer, mCurrent, bytesRead);
#else
	EEPtr e = (int)mCurrent;
	uint8_t *ptr = (uint8_t*)outBuffer;
	for( int count = bytesRead ; count ; --count, ++e )  *ptr++ = *e;
#endif
	mCurrent += bytesRead;
	return(bytesRead);
}

/*********************************** Write ************************************/
uint32_t DataStream_E::Write(
	uint32_t	inLength,
	const void*	inBuffer)
{
	uint32_t	bytesWritten = Clip(inLength);
#ifndef EEPROM_h
	memcpy((void*)mCurrent, inBuffer, bytesWritten);
#else
	EEPtr e = (int)mCurrent;
	const uint8_t *ptr = (const uint8_t*)inBuffer;
	for( int count = bytesWritten ; count ; --count, ++e )  (*e).update( *ptr++ );
#endif
	mCurrent += bytesWritten;
	return(bytesWritten);
}

