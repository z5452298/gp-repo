#pragma once

//#using <System.dll>
#include "NetworkedModule.h"

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

ref class VehicleControl : public NetworkedModule
{
public:
	VehicleControl(SM_ThreadManagement^ SM_TM_, SM_VehicleControl^ SM_VC_);
	error_state processSharedMemory() override;
	bool getShutdownFlag() override;
	void threadFunction() override;

	error_state connect(String^ hostName, int portNumber) override;
	error_state communicate() override;

	error_state processHeartbeats();
	void shutdownModules();
	~VehicleControl() {};

private:
	SM_ThreadManagement^ SM_TM_;
	SM_VehicleControl^ SM_VC_;
	array<unsigned char>^ SendData;
	bool flag;
	Stopwatch^ Watch;
};

