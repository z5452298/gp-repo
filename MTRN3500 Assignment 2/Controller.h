#pragma once
#include "controllerInterface.h"
#include "UGVModule.h"
//#using <System.dll>

ref class Controller : public UGVModule
{
public:
	Controller(SM_ThreadManagement^ SM_TM_, SM_VehicleControl^ SM_VC);
	Controller(SM_ThreadManagement^ SM_TM_, SM_VehicleControl^ SM_VC, DWORD controllerId, int inputMode);


	error_state processSharedMemory() override;
	bool getShutdownFlag() override;
	void threadFunction() override;

	error_state processHeartbeats();
	void shutdownModules();
	~Controller() {};
private:
	ControllerInterface* controller;
	SM_ThreadManagement^ SM_TM_;
	SM_VehicleControl^ SM_VC_;
	Stopwatch^ Watch;
};

