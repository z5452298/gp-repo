#using <System.dll>
#include "CrashAvoidance.h"
#include "NetworkedModule.h"
#include "SMObjects.h"

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

CrashAvoidance::CrashAvoidance(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_VehicleControl^ SM_VC)
{
	SM_TM_ = SM_TM;
	SM_Laser_ = SM_Laser;
	SM_VC_ = SM_VC;
	Watch = gcnew Stopwatch;
}

void CrashAvoidance::threadFunction()
{	
	// setup the stopwatch
	Watch = gcnew Stopwatch;

	// wait at the barrier
	SM_TM_->ThreadBarrier->SignalAndWait();

	// start the stopwatch
	Watch->Start();

	// start thread loop
	while (!getShutdownFlag())
	{
		processHeartbeats();
		if (processSharedMemory() == SUCCESS)
		{
			if (detectObstacleAhead(1.0))
			{
				inhibitMotion();
			}
		}
		Thread::Sleep(20);
	}
	Console::WriteLine("Crash Avoidance thread is terminating");
}

error_state CrashAvoidance::processHeartbeats()
{
	// is the CrashAvoidance bit in the heartbeat byte down?
	if ((SM_TM_->heartbeat & bit_CRASHAVOIDANCE) == 0)
	{
		// put the CrashAvoidance bit up
		SM_TM_->heartbeat |= bit_CRASHAVOIDANCE;

		// reset stopwatch
		Watch->Restart();
	}
	else
	{
		// has the time elapsed exceeded the crash time limit?
		if (Watch->ElapsedMilliseconds > CRASH_LIMIT)
		{
			// shutdown all threads
			shutdownModules();
			return ERR_SM;
		}
	}
	return SUCCESS;
}

void CrashAvoidance::shutdownModules()
{
	SM_TM_->shutdown = 0xFF;
}

bool CrashAvoidance::getShutdownFlag()
{
	return SM_TM_->shutdown & bit_CRASHAVOIDANCE;
}

error_state CrashAvoidance::processSharedMemory()
{
	// For CrashAvoidance, we only need to read from Laser data and Vehicle Control data
	Monitor::Enter(SM_Laser_->lockObject);
	Monitor::Enter(SM_VC_->lockObject);

	// We could add any additional processing of shared memory here if needed

	Monitor::Exit(SM_VC_->lockObject);
	Monitor::Exit(SM_Laser_->lockObject);

	return SUCCESS;
}

bool CrashAvoidance::detectObstacleAhead(double safetyDistance)
{
	bool obstacleDetected = false;
	Monitor::Enter(SM_Laser_->lockObject);

	// Iterate through the laser data to determine if there are any obstacles within the safety distance
	for (int i = 0; i < SM_Laser_->x->Length; ++i)
	{
		double distance = Math::Sqrt(SM_Laser_->x[i] * SM_Laser_->x[i] + SM_Laser_->y[i] * SM_Laser_->y[i]);
		if (distance <= safetyDistance)
		{
			obstacleDetected = true;
			break;
		}
	}

	Monitor::Exit(SM_Laser_->lockObject);
	return obstacleDetected;
}

void CrashAvoidance::inhibitMotion()
{
	Monitor::Enter(SM_VC_->lockObject);
	// Set vehicle speed to zero to inhibit motion
	SM_VC_->Speed = 0;
	Monitor::Exit(SM_VC_->lockObject);
	Console::WriteLine("Crash Avoidance: Motion inhibited due to obstacle detected within 1m.");
}
