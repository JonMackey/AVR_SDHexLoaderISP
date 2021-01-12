/*
*	TFT_ST77XX.h, Copyright Jonathan Mackey 2019
*	Base class of the SPI TFT ST77XX display controller family.
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
#ifndef TFT_ST77XX_h
#define TFT_ST77XX_h
#include <SPI.h>
#include "DisplayController.h"

class DataStream;

class TFT_ST77XX : public DisplayController
{
public:
							TFT_ST77XX(
								uint8_t					inDCPin,
								int8_t					inResetPin,	
								int8_t					inCSPin,
								int8_t					inBacklightPin,
								uint16_t				inHeight,
								uint16_t				inWidth,
								bool					inCentered,
								bool					inIsBGR);

							// Rotation, one of 0, 1, 2, or 3.
							// Corresponds to MADCTL in doc
							//			MY	MX	MV	
							// 0	0	0	0	0	
							// 1	90	0	1	1	
							// 2	180	1	1	0	
							// 3	270	1	0	1	
	void					begin(
								uint8_t					inRotation = 0,
								bool					inResetLevel = LOW);
	virtual void			MoveTo(
								uint16_t				inRow,
								uint16_t				inColumn);
	virtual void			MoveToRow(
								uint16_t				inRow);
	virtual void			MoveToColumn(
								uint16_t				inColumn);
	/*
	*	Turns the display off.
	*/
	virtual void			Sleep(void);
	/*
	*	Turns the display on.
	*/
	virtual void			WakeUp(void);

	/*
	*	FillPixels: Sets a run of inPixelsToFill to inFillColor from the
	*	current position and column clipping.
	*/
	virtual void			FillPixels(
								uint16_t				inPixelsToFill,
								uint16_t				inFillColor);
								
	/*
	/*
	*	SetColumnRange: Sets a the absolute column clipping to inStartColumn to
	*	inEndColumn.
	*/
	virtual void			SetColumnRange(
								uint16_t				inStartColumn,
								uint16_t				inEndColumn);
								
	/*
	*	SetRowRange: Sets a the absolute row range clipping to
	*	inStartRow to inEndRow.
	*/
	virtual void			SetRowRange(
								uint16_t				inStartRow,
								uint16_t				inEndRow);

	/*
	*	StreamCopy: Blindly copies inPixelsToCopy 16 bit pixels from inDataStream
	*	starting at the current row and column.  No checking to see if the data
	*	will fit on the display without clipping, skewing or wrapping.  The
	*	current row and column will be undefined (not updated within the class.)
	*/
	virtual void			StreamCopy(
								DataStream*				inDataStream,
								uint16_t				inPixelsToCopy);
								
	virtual void			CopyPixels(
								const void*				inPixels,
								uint16_t				inPixelsToCopy);

	virtual void			SetAddressingMode(
								EAddressingMode			inAddressingMode){}
protected:
	enum ECmds
	{
		// Read commands are not included because the MISO pin isn't generally available.
		eSWRESETCmd			= 0x01,	// Software Reset
		eSLPINCmd			= 0x10,	// Sleep in
		eSLPOUTCmd			= 0x11,	// Sleep Out.  This command turns off sleep mode.
		ePTLONCmd			= 0x12,	// Partial Display Mode On
		eNORONCmd			= 0x13,	// Normal Display Mode On
		eINVOFFCmd			= 0x20,	// Display Inversion Off
		eINVONCmd			= 0x21,	// Display Inversion On
		eGAMSETCmd			= 0x26,	// Gama Set
		eDISPONCmd			= 0x29,	// Display on
		eCASETCmd			= 0x2A,	// Column Address Set (column range)
		eRASETCmd			= 0x2B,	// Row Address Set (row range)
		eRAMWRCmd			= 0x2C,	// Memory Write. Row/col resets to range origin
		ePTLARCmd			= 0x30,	// Partial Area
		eVSCRDEFCmd			= 0x33,	// Vertical Scrolling Definition
		eTEOFFCmd			= 0x34,	// Tearing Effect Line off
		eTEONCmd			= 0x35,	// Tearing Effect Line on
		eMADCTLCmd			= 0x36,	// Memory Data Access Control (aka MADCTR)
		eVSCSADCmd			= 0x37,	// Vertical Scroll Start address of RAM
		eIDMOFFCmd			= 0x38,	// Idle Mode Off
		eIDMONCmd			= 0x39,	// Idle Mode On
		eCOLMODCmd			= 0x3A,	// Interface Pixel Format
		eWRMEMCCmd			= 0x3C,	// Write Memory Continue
		//eSTECmd			= 0x44, // Set Tear Scanline
		//eWRDISBVCmd		= 0x51,	// Write Display Brightness
		//eWRCTRLDCmd		= 0x53,	// Write CTRL Display
		eFRMCTR1Cmd			= 0xB1,	// Frame Rate Control (In normal mode/ Full colors)
		eFRMCTR2Cmd			= 0xB2,	// Frame Rate Control (In Idle mode/ 8-colors)
		eFRMCTR3Cmd			= 0xB3,	// Frame Rate Control (In Partial mode/ full colors)
		eINVCTRCmd			= 0xB4, // Display Inversion Control
		eDISSET5Cmd			= 0xB6,	// Display Function set 5
		ePWCTR1Cmd			= 0xC0, // Power Control 1
		ePWCTR2Cmd			= 0xC1,	// Power Control 2
		ePWCTR3Cmd			= 0xC2,	// Power Control 3 (in Normal mode/ Full colors)
		ePWCTR4Cmd			= 0xC3,	// Power Control 4 (in Idle mode/ 8-colors)
		ePWCTR5Cmd			= 0xC4,	// Power Control 5 (in Partial mode/ full-colors)
		eVMCTR1Cmd			= 0xC5, // VCOM Control 1
		eGMCTRP1Cmd			= 0xE0,	// Gamma (‘+’polarity) Correction Characteristics Setting
		eGMCTRN1CMD			= 0xE1	// Gamma (‘-’polarity) Correction Characteristics Setting
	};
	int8_t		mCSPin;
	uint8_t		mDCPin;
	uint8_t		mResetPin;
	int8_t		mBacklightPin;
	uint8_t		mChipSelBitMask;
	uint8_t		mDCBitMask;
	uint8_t		mRowOffset;
	uint8_t		mColOffset;
	bool		mIsBGR;		// Set when display pixel RGB order is opposite of the controller docs (bug fix)
	bool		mCentered;	// Display pixels are physically centered within the controllers memory space.
							// When false the the display pixel origin is 0,0 at 0 degree rotation
	bool		mResetLevel;// Allows the reset pin value to be inverted when run through an inverting level shifter.
	volatile uint8_t*	mChipSelPortReg;
	volatile uint8_t*	mDCPortReg;
	SPISettings	mSPISettings;


	virtual void			Init(void);
	void					WriteSleepCmds(void);
	void					WriteWakeUpCmds(void);
	void					SetRotation(
								uint8_t					inRotation);
	inline void				BeginTransaction(void)
							{
								SPI.beginTransaction(mSPISettings);
								if (mCSPin >= 0)
								{
									*mChipSelPortReg &= ~mChipSelBitMask;
								}
							}

	inline void				EndTransaction(void)
							{
								if (mCSPin >= 0)
								{
									*mChipSelPortReg |= mChipSelBitMask;
								}
								SPI.endTransaction();
							}

	inline void				WriteCmd(
								uint8_t					inCmd) const;
	void					WriteCmd(
								uint8_t					inCmd,
								const uint8_t*			inCmdData,
								uint8_t					inCmdDataLen,
								bool					inPGM = false) const;
	void					WriteCmds(
								const uint8_t*			inCmds) const;
	void					WriteData(
								const uint8_t*			inData,
								uint16_t				inDataLen,
								bool					inPGM) const;
	void					WriteData16(
								const uint16_t*			inData,
								uint16_t				inDataLen) const;
							// The native controller display resolution.
	virtual uint16_t		VerticalRes(void) const = 0;
	virtual uint16_t		HorizontalRes(void) const = 0;
};
#endif // TFT_ST77XX_h
