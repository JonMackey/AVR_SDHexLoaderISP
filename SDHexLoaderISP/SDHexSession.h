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
								bool					inSetFusesAndBootloader = false,
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
		// All errors must be reflected in SDHexLoader.cpp & .h
		eNoErr,
		eTimeoutErr,
		eSyncErr,
		eUnknownErr,
		eLoadHexDataErr,
		eSignatureErr,
		eVerificationErr,
		eUnlockErr,
		eLockErr,
		eFuseErr,
		eEFuseErr = eFuseErr,
		eHFuseErr,
		eLFuseErr
	};
	enum EStage
	{
		eSessionCompleted,		// 0
		eChipErase,
		eVerifySignature,
		eVerifyUnlocked,
		eVerifyFuse,
		eVerifyExtendedFuse		= eVerifyFuse,
		eVerifyHighFuse,
		eVerifyLowFuse,
		eVerifyLockBits,		// 7
		// Bit 0 set is flash, cleared is EEPROM
		eIsFlash				= 1,	// For both eLoadingXX and eVerifyingXX
		eLoadingMemory			= 0x08,
		eLoadingEEPROM			= eLoadingMemory,
		eLoadingFlash,
		eVerifyingMemory		= 0x10,
		eVerifyingEEPROM		= eVerifyingMemory,
		eVerifyingFlash,

		eFuseWritten			= 0x20,	// Stage modifier
		eFuseWriteResponse		= 0x40,	// Stage modifier
		eFuseVerified			= 0x80	// Stage modifier
	};
	enum EOperation
	{
		eIsProgramming			= 1,
		eProgramFlash			= eIsProgramming,
		eProgramEEPROM			= 3,
		eSetFuses				= 0x04,
		eSetFusesAndBootloader	= 0x08
	};
protected:
	struct SFuseInst
	{
		uint8_t	readInstByte1;
		uint8_t	readInstByte2;
		uint8_t	writeInstByte2;
	};
	SFuseInst		mFuseInst;
	Stream*			mStream;
	AVRStreamISP*	mAVRStreamISP;
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
	uint8_t			mStageModifier;
	uint8_t			mOperation;
	bool			mSerialISP;
#ifdef SUPPORT_REPLACEMENT_DATA
	uint16_t		mReplacementAddress;
	uint8_t			mReplacementDataIndex;
	uint8_t			mReplacementData[4];
	static const SFuseInst	kFuseInst[];

	
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
	void					VerifyUnlocked(
								bool					inIsResponse);
	void					VerifyFuse(
								bool					inIsResponse);
	void					VerifyLockBits(
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
