#pragma once

//#using <System.dll>
#include "NetworkedModule.h"

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

ref class GNSS : public NetworkedModule
{
public:
	GNSS(SM_ThreadManagement^ SM_TM_, SM_GPS^ SM_GNSS);
	error_state processSharedMemory() override;
	bool getShutdownFlag() override;
	void threadFunction() override;

	error_state connect(String^ hostName, int portNumber) override;
	error_state communicate() override;

	error_state processHeartbeats();
	void shutdownModules();
	~GNSS() {};

private:
	SM_ThreadManagement^ SM_TM_;
	SM_GPS^ SM_GNSS_;
	array<unsigned char>^ SendData;
	Stopwatch^ Watch;

};