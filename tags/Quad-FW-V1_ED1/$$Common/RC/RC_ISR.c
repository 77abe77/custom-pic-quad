#include "RC\RCLocal.h"

//************************************************************
// RC-related Interrupt Routines
//************************************************************

//*************************************************************
void __attribute__((__interrupt__, __no_auto_psv__)) _T3Interrupt(void)
	{
	// Happens every 25 millisecond if not reset in _CNInterrupt
	// routine below...
	//-----------------------------------------------------
	T3CONbits.TON 	= 0; 	// Stop Timer3 
	_T3IF 	= 0; 			// Clear Timer3 interrupt flag
	//---------------------------------------------------------
	CNEN1	= 0;	// Disable all individual Change
	CNEN2	= 0;	// Notification Interrupts
	//-------------
	_CNIF 	= 0; 	// Reset CN interrupt request (if any...)
	//=========================================================

	//-----------------------------------------------
	_RCSampleReady = 1;		// Set READY flag
	_RCSampleTimed = 1;		// Indicate that sample 
							// generated by Timer
	//-----------------------------------------------
	// Set Connection Status flag
	//-----------------------------------------------
	_RCConnActive 	= 0;	// Connection to Receiver
							// is lost...
	//=========================================================
	// Reset control automaton
	//-----------------------------------------------------
	_RCState		= 0;	
	_CN17IE			= 1;	// Enable CN interrupt on Ch1 (CN17/RC7)
							// Hopefully CN interrupt will happen
							// before the Timer3 interrupt :)	
	//-----------------------------------------------------
	TMR3			= 0;	// Re-set Timer3 counter
	T3CONbits.TON 	= 1; 	// Start Timer3 - counting 25 msec safety margin;
	//=========================================================
	return;
	}
//*************************************************************


 
void __attribute__((interrupt, no_auto_psv)) _CNInterrupt(void)
	{
	//================================================================
	// First and foremost, let's reset interrupt request(s) :)
	//----------------------------------------------------------------
	CNEN1	= 0;	// Disable all individual Change
	CNEN2	= 0;	// Notification Interrupts
	//-------------
	_CNIF 	= 0; 	// Reset CN interrupt request (if any...)

	//================================================================
	// Second, we need to capture the timestamp of the interrupt...
	//----------------------------------------------------------------
	ulong	TS = TMRGetTS();

	//================================================================
	// Third, we need to identify whether this is the change notifica-
	// tion that we are expecting at this stage.
	//----------------------------------------------------------------
	switch (_RCState)
		{
		case	0:	// We are expecting the rising edge on Ch1
			if (1 == _RC7)
				{
				// Yest, this is the rising edge on Ch1
				_RCTimes.RCTimingArray[_RCState] 	= TS;	// Capture time
				_RCState++;									// Advance state
				//----------------------
				_CN20IE	= 1;		// Prepare for Ch2 (CN20/RC8)
				}
			else
				// Unexpected pin configuration!
				goto ResetAutomaton;
			break;

		case	1:	// We are expecting the rising edge on Ch2
			if (1 == _RC8)
				{
				// Yest, this is the rising edge on Ch2
				_RCTimes.RCTimingArray[_RCState] 	= TS;	// Capture time
				_RCState++;									// Advance state
				//----------------------
				_CN19IE	= 1;	// Prepare for Ch3 (CN19/RC9)
				}
			else
				// Unexpected pin configuration!
				goto ResetAutomaton;
			break;

		case	2:	// We are expecting the rising edge on Ch3
			if (1 == _RC9)
				{
				// Yest, this is the rising edge on Ch3
				_RCTimes.RCTimingArray[_RCState] 	= TS;	// Capture time
				_RCState++;									// Advance state
				//----------------------
				_CN16IE	= 1;	// Prepare for Ch4 (CN16/RB10)
				}
			else
				// Unexpected pin configuration!
				goto ResetAutomaton;
			break;

		case	3:	// We are expecting the rising edge on Ch4
			if (1 == _RB10)
				{
				// Yes, this is the rising edge on Ch4
				_RCTimes.RCTimingArray[_RCState] 	= TS;	// Capture time
				_RCState++;									// Advance state
				//----------------------
				_CN15IE	= 1;	// Prepare for Ch5 (CN15/RB11)
				}
			else
				// Unexpected pin configuration!
				goto ResetAutomaton;
			break;

		case	4:	// We are expecting the rising edge on Ch5
			if (1 == _RB11)
				{
				// Yes, this is the rising edge on Ch5
				_RCTimes.RCTimingArray[_RCState] 	= TS;	// Capture time
				_RCState++;									// Advance state
				//----------------------
				// Now we will be waiting for the falling edge on the
				// same channel Ch5
				//----------------------
				_CN15IE	= 1;	// Prepare for Ch5 (CN15/RB11)
				}
			else
				// Unexpected pin configuration!
				goto ResetAutomaton;
			break;

		case	5:	// We are expecting the falling edge on Ch5
			if (0 == _RB11)
				{
				// Yes, this is the falling edge on Ch5
				_RCTimes.RCTimingArray[_RCState] 	= TS;	// Capture time
				_RCState++;									// Advance state
							// NOTE: This will advance state beyond the max
							// 		 expected state of "5" triggering reset
							//		 of Control Automaton through "Default"
							//		 if we receive and interrupt, which
							//		 WE SHOULD NOT!!!
				//----------------------
				goto VerifyTiming;
				}
			else
				// Unexpected pin configuration!
				goto ResetAutomaton;
			break;

		default:	// Should never get here!!!
			goto ResetAutomaton;
			break;
		}


	//================================================================
	// Normal interrupt processing completed - return from the 
	// interrupt routine.
	//----------------------------------------------------------------
	return;

	RCRawData	Temp;

VerifyTiming:
	//-----------------------------------------------
	// Calculate pulse width on each of the channels
	//-----------------------------------------------
	{
	Temp.Ch1 = 	_RCTimes.Ch2Start - _RCTimes.Ch1Start; 
	Temp.Ch2 = 	_RCTimes.Ch3Start - _RCTimes.Ch2Start; 
	Temp.Ch3 = 	_RCTimes.Ch4Start - _RCTimes.Ch3Start; 
	Temp.Ch4 = 	_RCTimes.Ch5Start - _RCTimes.Ch4Start; 
	Temp.Ch5 = 	_RCTimes.Ch5End   - _RCTimes.Ch5Start; 
	//-----------------------------------------------
	// Verify pulse width on each of the channels
	//-----------------------------------------------
	if (Temp.Ch1 < _RCTimeCountMinMin || Temp.Ch1 > _RCTimeCountMaxMax)
		goto ResetAutomaton;	// Ignore this reading...
	if (Temp.Ch2 < _RCTimeCountMinMin || Temp.Ch2 > _RCTimeCountMaxMax)
		goto ResetAutomaton;	// Ignore this reading...
	if (Temp.Ch3 < _RCTimeCountMinMin || Temp.Ch3 > _RCTimeCountMaxMax)
		goto ResetAutomaton;	// Ignore this reading...
	if (Temp.Ch4 < _RCTimeCountMinMin || Temp.Ch4 > _RCTimeCountMaxMax)
		goto ResetAutomaton;	// Ignore this reading...
	if (Temp.Ch5 < _RCTimeCountMinMin || Temp.Ch5 > _RCTimeCountMaxMax)
		goto ResetAutomaton;	// Ignore this reading...
	//-----------------------------------------------
	// Normalize pulse width on each of the channels
	//-----------------------------------------------
	if (Temp.Ch1 < _RCTimeCountMin)
		Temp.Ch1 = _RCTimeCountMin;
	else	if (Temp.Ch1 > _RCTimeCountMax)
				Temp.Ch1 = _RCTimeCountMax;
	//-----------------------------
	if (Temp.Ch2 < _RCTimeCountMin)
		Temp.Ch2 = _RCTimeCountMin;
	else	if (Temp.Ch2 > _RCTimeCountMax)
				Temp.Ch2 = _RCTimeCountMax;
	//-----------------------------
	if (Temp.Ch3 < _RCTimeCountMin)
		Temp.Ch3 = _RCTimeCountMin;
	else	if (Temp.Ch3 > _RCTimeCountMax)
				Temp.Ch3 = _RCTimeCountMax;
	//-----------------------------
	if (Temp.Ch4 < _RCTimeCountMin)
		Temp.Ch4 = _RCTimeCountMin;
	else	if (Temp.Ch4 > _RCTimeCountMax)
				Temp.Ch4 = _RCTimeCountMax;
	//-----------------------------
	if (Temp.Ch5 < _RCTimeCountMin)
		Temp.Ch5 = _RCTimeCountMin;
	else	if (Temp.Ch5 > _RCTimeCountMax)
				Temp.Ch5 = _RCTimeCountMax;
	//-----------------------------
	}


	//-----------------------------------------------
	// Expose RAW pulse width on each of the channels
	//-----------------------------------------------
	_RCRawSample.Ch1	= Temp.Ch1 - _RCTimeCountMidPoint;	// Roll
	_RCRawSample.Ch2	= Temp.Ch2 - _RCTimeCountMidPoint;	// Pitch
	_RCRawSample.Ch4	= Temp.Ch4 - _RCTimeCountMidPoint;	// Yaw
	//-----------------------------------------------
	_RCRawSample.Ch3	= Temp.Ch3 - _RCTimeCountMin;		// Throttle
	//-----------------------------------------------
	if (Temp.Ch5 < _RCTimeCountMidPoint)					// Control
		_RCRawSample.Ch5	= 0;
	else
		_RCRawSample.Ch5	= 1;
	//-----------------------------------------------
	// Set flags
	//-----------------------------------------------
	_RCSampleReady = 1;		// Set READY flag
	_RCSampleTimed = 0;		// Indicate that sample 
							// generated by Receiver
	//-----------------------------------------------
	// Set Connection Status flag
	//-----------------------------------------------
	_RCConnActive 	= 1;	// Connection to Receiver
							// is active...
	//-----------------------------------------------
	// Re-Start safety Margin counter
	//-----------------------------------------------
	T3CONbits.TON 	= 0; 	// Stop Timer3;
	_T3IF 			= 0 ; 	// Clear TMR3 interrupt flag
	TMR3			= 0;	// Re-set Timer3 counter
	T3CONbits.TON 	= 1; 	// Start Timer3 - counting 25 msec safety margin;
	//---------------------------						


	//-----------------------------------------------
	// Get ready for next reading...
	//-----------------------------------------------
ResetAutomaton:
	//-----------------------------------------------------
	// Reset control automaton
	//-----------------------------------------------------
	_RCState	= 0;	
	_CN17IE		= 1;	// Enable CN interrupt on Ch1 (CN17/RC7)	
	//-----------------------------------------------------
	return;
	}
//************************************************************
