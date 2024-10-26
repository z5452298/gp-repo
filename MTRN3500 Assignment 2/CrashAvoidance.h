#pragma once
//#using <System.dll>
#include "NetworkedModule.h"
#include "SMObjects.h"
#include <cmath>

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

//Not sure how crash avoid should be written need to give more time for this 
//just a basic understnading although thread function not needed ig since this is not a thread so

ref class CrashAvoidance : public NetworkedModule
{
public:
	CrashAvoidance(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_VehicleControl^ SM_VC);
	error_state processSharedMemory() override;
	bool getShutdownFlag() override;
	void threadFunction() override;


	error_state processHeartbeats();
	void shutdownModules();

private:
	bool detectObstacleAhead(double safetyDistance);
	void inhibitMotion();

	SM_ThreadManagement^ SM_TM_;
	SM_Laser^ SM_Laser_;
	SM_VehicleControl^ SM_VC_;
	Stopwatch^ Watch;
	array<unsigned char>^ ReadData;
};
