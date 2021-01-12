/*
*	MSPeriod.h, Copyright Jonathan Mackey 2019
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
#ifndef MSPeriod_h
#define MSPeriod_h

#include <Arduino.h>

class MSPeriod
{
public:
							// Setting the period to zero disables Passed().
							MSPeriod(
								uint32_t				inPeriod = 0)
								: mPeriod(inPeriod){}
	inline void				Set(
								uint32_t				inPeriod)
								{mPeriod = inPeriod;}
	inline uint32_t			Get(void) const
								{return(mPeriod);}
	inline void				SetElapsed(void)
								{mPeriod = ElapsedTime();}
	inline uint32_t			ElapsedTime(void) const
								{return(millis() - mStart);}
	inline bool				Passed(void) const
								{return(mPeriod && ElapsedTime() >= mPeriod);}
	inline void				Start(
								uint32_t				inDelta = 0)
								{mStart = millis() + inDelta;}
protected:
	uint32_t	mStart;
	uint32_t	mPeriod;
};

#endif // MSPeriod_h
