/*
*	ContextualStream.cpp, Copyright Jonathan Mackey 2020

	This is a stream subclass with 2 circular buffers.  The buffer associated
	with the input or output Stream functions is based on the context setting. 
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
#include "ContextualStream.h"

/********************************* ContextualStream **********************************/
ContextualStream::ContextualStream(void)
 : mBuffer1Head(0), mBuffer1Tail(0), mBuffer2Head(0), mBuffer2Tail(0),
 	mReadFrom1(false)
{
}

/*********************************** begin ************************************/
void ContextualStream::begin(void)
{
}

/*********************************** flush ************************************/
void ContextualStream::flush(void)
{
	mBuffer1Head = 0;
	mBuffer1Tail = 0;
	mBuffer2Head = 0;
	mBuffer2Tail = 0;
}

/******************************** FlushBuffer1 ********************************/
void ContextualStream::FlushBuffer1(void)
{
	mBuffer1Head = 0;
	mBuffer1Tail = 0;
}

/******************************** FlushBuffer2 ********************************/
void ContextualStream::FlushBuffer2(void)
{
	mBuffer2Head = 0;
	mBuffer2Tail = 0;
}

/********************************* ReadFrom1 **********************************/
void ContextualStream::ReadFrom1(
	bool	inReadFrom1)
{
	mReadFrom1 = inReadFrom1;
	if (inReadFrom1)
	{
		mBuffer1Head = 0;
		mBuffer2Tail = 0;
	} else
	{
		mBuffer2Head = 0;
		mBuffer1Tail = 0;
	}
}

/********************************* available **********************************/
int ContextualStream::available(void)
{
	return(mReadFrom1 ? mBuffer1Tail - mBuffer1Head :
						mBuffer2Tail - mBuffer2Head);
}

/************************************ read ************************************/
int ContextualStream::read(void)
{
	uint8_t	thisByte = -1;
	if (mReadFrom1)
	{
		if (mBuffer1Head != mBuffer1Tail)
		{
			thisByte = mBuffer1[mBuffer1Head++];
		}
	} else if (mBuffer2Head != mBuffer2Tail)
	{
		thisByte = mBuffer2[mBuffer2Head++];
	}
	return(thisByte);
}

/************************************ peek ************************************/
int ContextualStream::peek(void)
{
	uint8_t	thisByte = -1;
	if (mReadFrom1)
	{
		if (mBuffer1Head != mBuffer1Tail)
		{
			thisByte = mBuffer1[mBuffer1Head];
		}
	} else if (mBuffer2Head != mBuffer2Tail)
	{
		thisByte = mBuffer2[mBuffer2Head];
	}
	return(thisByte);
}

/*********************************** write ************************************/
size_t ContextualStream::write(
	uint8_t	inByte)
{
	// The buffers should never overrun
	/*
	*	If writing to buffer 2...
	*/
	if (mReadFrom1)
	{
		if (mBuffer2Tail < AVR_BUFFER_SIZE)
		{
			mBuffer2[mBuffer2Tail++] = inByte;
		}
	} else if (mBuffer1Tail < AVR_BUFFER_SIZE)
	{
		mBuffer1[mBuffer1Tail++] = inByte;
	}

	return(1);
}

/*********************************** write ************************************/
size_t ContextualStream::write(
	const uint8_t*	inBuffer,
	size_t			inLength)
{
	uint16_t	bytesWritten = 0;
	
	if (mReadFrom1)
	{
		if ((mBuffer2Tail + (uint16_t)inLength) <= AVR_BUFFER_SIZE)
		{
			memcpy(&mBuffer2[mBuffer2Tail], inBuffer, inLength);
			mBuffer2Tail+=(uint16_t)inLength;
			bytesWritten = inLength;
		}
	} else
	{
		if ((mBuffer1Tail + (uint16_t)inLength) <= AVR_BUFFER_SIZE)
		{
			memcpy(&mBuffer1[mBuffer1Tail], inBuffer, inLength);
			mBuffer1Tail+=(uint16_t)inLength;
			bytesWritten = inLength;
		}
	}
	return(bytesWritten);
}

/******************************** StReadFrom1 *********************************/
StReadFrom1::StReadFrom1(
	ContextualStream&	inContextualStream,
	bool				inReadFrom1)
: mSavedReadFrom1(inContextualStream.ReadingFrom1()),
	mContextualStream(inContextualStream)
{
	inContextualStream.ReadFrom1(inReadFrom1);
}

/******************************** ~StReadFrom1 ********************************/
StReadFrom1::~StReadFrom1(void)
{
	mContextualStream.ReadFrom1(mSavedReadFrom1);
}
