/*
*	SDHexSession.h, Copyright Jonathan Mackey 2020
*	Generates and interprets stream IO to/from the target.  The target is a
*	stream that consumes and responds to SDK500 instructions.
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
#ifndef SDHexSession_h
#define SDHexSession_h

#include <inttypes.h>
#include "IntelHexFile.h"
#include "AVRConfig.h"
#include "ContextualStream.h"
#ifndef __MACH__
#include "MSPeriod.h"
#include "USPeriod.h"
#else
#define Stream	ContextualStream
#endif

class AVRStreamISP;
class Stream;
class SDHexSession;
#define SUPPORT_REPLACEMENT_DATA	1

typedef  void (SDHexSession::*CmdHandler)(bool);

class SDHexSession : public IntelHexFile
{
public:
							SDHexSession(void);
	bool					begin(
								const char*				inPath,
								Stream*					inStream,
								AVRStreamISP*			inAVRStreamISP = nullptr,
								uint32_t				inTimestamp = 0);
	bool					Update(void);
	uint8_t					Error(void) const
								{return(mError);}
	uint8_t					Stage(void) const
								{return(mStage);}
	bool					InSession(void)
								{return(mStream != nullptr);}
	bool					Halt(void);
	uint32_t				HexByteCount(void) const
								{return(mConfig.byteCount);}
	uint32_t				BytesProcessed(void) const
								{return(mBytesProcessed);}
	uint8_t					PercentageProcessed(void) const	// 0 to 100
								{return(mPercentageProcessed);}
	enum EErrors
	{
		eNoErr,
		eTimeoutErr,
		eSyncErr,
		eUnknownErr,
		eLoadHexDataErr,
		eSignatureErr,
		eVerificationErr
	};
	enum EStage
	{
		eLoadingEEPROM = 'E',
		eLoadingFlash = 'F',
		eVerifyingEEPROM = 'e',
		eVerifyingFlash = 'f',
		eSessionCompleted = 'x'
	};
protected:
	Stream*			mStream;
	SAVRConfig		mConfig;
	ContextualStream mContextualStream;
	CmdHandler		mCmdHandler;
	CmdHandler		mOnSyncCmdHandler;	// Used by GetSync()
#ifndef __MACH__
	MSPeriod		mTimeout;
	USPeriod		mCmdDelay;
#endif
	uint16_t		mBytesPerPage;
	uint32_t		mCurrentPageAddress;
	uint32_t		mPageAddressMask;
	uint32_t		mBytesProcessed;
	uint16_t		mWordsPerPage;
	uint8_t			mPercentageProcessed;
	uint8_t			mCurrentAddressH;
	uint8_t			mDataIndex;
	uint8_t			mSyncRetries;
	uint8_t			mError;
	uint8_t			mStage;
	bool			mSerialISP;
#ifdef SUPPORT_REPLACEMENT_DATA
	uint16_t		mReplacementAddress;
	uint8_t			mReplacementDataIndex;
	uint8_t			mReplacementData[4];
	
	void					ReplaceData(void);
#endif
	bool					CanContinue(void) const
								{return(mStage != eSessionCompleted && !mError);}
	bool					WaitForAvailable(
								uint8_t					inBytesToWaitFor);
	void					WaitForAvailableForWrite(
								uint8_t					inBytesToWaitFor);
	bool					LoadNextDataRecord(void);
	bool					ResponseStatusOK(void);
	void					GetSync(
								bool					inIsResponse);
	void					SetDevice(
								bool					inIsResponse);
	void					SetDeviceExt(
								bool					inIsResponse);
	void					EnterProgramMode(
								bool					inIsResponse);
	void					LeaveProgramMode(
								bool					inIsResponse);
	void					ReadSignature(
								bool					inIsResponse);
	void					ChipErase(
								bool					inIsResponse);
	void					LoadAddress(
								bool					inIsResponse);
	void					LoadExtAddress(
								bool					inIsResponse);
	void					ProcessPage(
								bool					inIsResponse);
	bool					LoadPageFromSD(
								uint32_t				inWordAddress,
								uint32_t				inPageAddress,
								uint32_t				inNextPageAddress,
								Stream*					inStream);
	void					SetupUniversal(
								uint8_t					inByte1,
								uint8_t					inByte2,
								uint8_t					inByte3,
								uint8_t					inByte4);


};

#endif // SDHexSession_h
