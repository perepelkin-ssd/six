#pragma once

// MUST start with 0 and end with pseudo-entry _STAG_END (this is for
// simplicity of verification, that all constructors are assigned).
enum STAGS {
	STAG_DelDf,
	STAG_Delivery,
	STAG_ExecJsonFp,
	STAG_Locator_CyclicLocator,
	STAG_MonitorSignal,
	STAG_StoreDf,
	STAG_SubmitDfToCf,
	STAG_Value_IntValue,
	_STAG_END
};
