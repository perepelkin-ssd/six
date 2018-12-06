#pragma once

// MUST start with 0 and end with pseudo-entry _STAG_END (this is for
// simplicity of verification, that all constructors are assigned).
enum STAGS {
	STAG_Locator_CyclicLocator,
	STAG_Value_IntValue,
	STAG_MyMain1,
	STAG_My2A,
	STAG_My2B,
	STAG_DelDf,
	STAG_Delivery,
	STAG_StoreDf,
	STAG_SubmitDfToCf,
	STAG_MonitorSignal,
	_STAG_END
};
