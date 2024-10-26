#pragma once
#include <NetworkedModule.h>

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

ref class Display : public NetworkedModule
{
public:
	// Default
	Display();

	// Constructor
	Display(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_GPS^ SM_GNSS);

	error_state processSharedMemory() override;
	bool getShutdownFlag() override;
	void threadFunction() override;

	virtual error_state connect(String^ hostName, int portNumber) override;
	virtual error_state communicate() override;
	error_state processHeartbeats();
	void shutdownModules();
	void sendDisplayData();

private:
	SM_ThreadManagement^ SM_TM_;
	SM_Laser^ SM_Laser_;
	SM_GPS^ SM_GNSS_;
	array<unsigned char>^ SendData;
	int tweak = 0;
	Stopwatch^ Watch;
};

