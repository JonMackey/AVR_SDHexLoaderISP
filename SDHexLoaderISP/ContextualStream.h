/*
*	ContextualStream.h, Copyright Jonathan Mackey 2020

	This is a Stream subclass with two non-circular FIFO buffers.  The buffer
	associated with the input or output Stream functions is based on the the
	ReadFrom1 setting.  This allows the buffer direction to be switched
	depending on the context.

	When ReadFrom1 is set to true, available(), read(), and peek() get their
	data from buffer1.  write() puts data in buffer2.  When ReadFrom1 is set to
	false the buffers associated with available(), read(), peek(), and write()
	are swapped.

	Because it's not circular, all of the data in the associated read buffer
	must be consumed by the receiver before switching the context.
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
#ifndef ContextualStream_h
#define ContextualStream_h

#include <inttypes.h>
#ifdef __MACH__
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#else
#include <Stream.h>
#endif

/*
*	The longest AVR related block sent or received is 261 bytes.
*/
#define AVR_BUFFER_SIZE	262

#ifdef __MACH__
class ContextualStream
#else
class ContextualStream : public Stream
#endif
{
public:
							ContextualStream(void);
	void					begin(void);
	void					ReadFrom1(
								bool					inReadFrom1);
	virtual int				available(void);
	virtual int				read(void);
	virtual int				peek(void);

	virtual size_t			write(
								uint8_t					inByte);
	virtual size_t			write(
								const uint8_t*			inBuffer,
								size_t					inLength);
	virtual void			flush(void);

	// Low level access to buffers
	bool					ReadingFrom1(void) const
								{return(mReadFrom1);}
	uint8_t*				Buffer1(void)
								{return(mBuffer1);}
	void					FlushBuffer1(void);
	uint8_t*				Buffer2(void)
								{return(mBuffer2);}
	void					FlushBuffer2(void);
protected:
	uint16_t	mBuffer1Head;
	uint16_t	mBuffer1Tail;
	uint16_t	mBuffer2Head;
	uint16_t	mBuffer2Tail;
	uint8_t		mBuffer1[AVR_BUFFER_SIZE];
	uint8_t		mBuffer2[AVR_BUFFER_SIZE];
	bool		mReadFrom1;
};

/*
*	Stack class to set then restore the contextual stream's mReadFrom1 when the
*	class instance goes out of scope.
*/
class StReadFrom1
{
public:
							StReadFrom1(
								ContextualStream&		inContextualStream,
								bool					inReadFrom1);
							~StReadFrom1(void);
protected:
	bool				mSavedReadFrom1;
	ContextualStream&	mContextualStream;
};
#endif // ContextualStream_h
