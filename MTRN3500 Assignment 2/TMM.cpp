#using <System.dll>

#include "TMM.h"
#include "Laser.h"
#include "GNSS.h"
#include "VC.h"
#include "Display.h"
#include "UGVModule.h"


using namespace System;
using namespace System::Diagnostics;
using namespace System::Threading;

error_state ThreadManagement::setupSharedMemory() {
	SM_TM_ = gcnew SM_ThreadManagement;
	SM_Laser_ = gcnew SM_Laser;
	SM_GNSS_ = gcnew SM_GPS;
	SM_VC_ = gcnew SM_VehicleControl;

	return error_state::SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
error_state ThreadManagement::processSharedMemory() {
	return error_state::SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void ThreadManagement::shutdownModules() {
	SM_TM_->shutdown = 0xFF;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
bool ThreadManagement::getShutdownFlag() {
	return (SM_TM_ -> shutdown & bit_TM);	
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void ThreadManagement::threadFunction() {
	int i = 0;
	Console::WriteLine("TMM Thread is starting.");

	//// make a list of thread properties
	ThreadPropertiesList = gcnew array<ThreadProperties^> {
		gcnew ThreadProperties(gcnew ThreadStart(gcnew Laser(SM_TM_, SM_Laser_), &Laser::threadFunction), true, bit_LASER, "Laser thread"),
			gcnew ThreadProperties(gcnew ThreadStart(gcnew GNSS(SM_TM_, SM_GNSS_), &GNSS::threadFunction), false, bit_GPS, "GNSS thread"),
			gcnew ThreadProperties(gcnew ThreadStart(gcnew VehicleControl(SM_TM_, SM_VC_), &VehicleControl::threadFunction), true, bit_VC, "VehicleControl thread"),
			gcnew ThreadProperties(gcnew ThreadStart(gcnew Controller(SM_TM_, SM_VC_), &Controller::threadFunction), true, bit_CONTROLLER, "Controller thread"),
			gcnew ThreadProperties(gcnew ThreadStart(gcnew Display(SM_TM_, SM_Laser_, SM_GNSS_), &Display::threadFunction), true, bit_DISPLAY, "Display thread")
	};

	// make a list of threads
	ThreadList = gcnew array<Thread^>(ThreadPropertiesList->Length);

	// make the stopwatch list
	StopwatchList = gcnew array<Stopwatch^>(ThreadPropertiesList->Length);

	// make the thread barrier
	SM_TM_->ThreadBarrier = gcnew Barrier(ThreadPropertiesList->Length + 1);	
	// start all the threads
	for (int i = 0; i < ThreadPropertiesList->Length; i++)
	{
		StopwatchList[i] = gcnew Stopwatch;
		ThreadList[i] = gcnew Thread(ThreadPropertiesList[i]->ThreadStart_);
		ThreadList[i]->Start();
	}

	// wait at the TMT thread barrier
	SM_TM_->ThreadBarrier->SignalAndWait();
	
	// start all the stop watches
	for (int i = 0; i < ThreadList->Length; i++)
	{
		StopwatchList[i]->Start();
	}

	//start the thread loop
	char key = '\0'; // Initialize key to a null character or some value that doesn't equal 'q'

	// Start the thread loop using a while loop
	while (key != 'q' && !getShutdownFlag())
	{
		if (Console::KeyAvailable)
		{
			key = static_cast<char>(Console::ReadKey(true).KeyChar); 
		}

		// Other logic remains the same
		processHeartbeates();
		Thread::Sleep(50);
	}

	// end of thread loop
	// shutdown threads
	shutdownModules();

	// join all threads
	for (int i = 0; i < ThreadPropertiesList->Length; i++) {
		ThreadList[i]->Join();
	}

	Console::WriteLine("TMM Thread terminating...");
}

/////////////////////////////////////////////////////////////////////////////////////////////////
error_state ThreadManagement::processHeartbeates() {
	for (int i = 0; i < ThreadList->Length; i++)
	{
		// check the heartbeat flag of ith thread (is it high?)
		if (SM_TM_->heartbeat & ThreadPropertiesList[i]->BitID)
		{
			// if high put ith bit (flag) down
			SM_TM_->heartbeat ^= ThreadPropertiesList[i]->BitID;
			// reset the stopwatch 
			StopwatchList[i]->Restart();
		}
		else
		{
			// check the stopwatch. Is time exceeded crash time limit?
			if (StopwatchList[i]->ElapsedMilliseconds > CRASH_LIMIT)
			{

				// is ith process a critical process?
				if (ThreadPropertiesList[i]->Critical)
				{
					// shutdown all
					Console::WriteLine(ThreadList[i]->Name + " failure. Shutting down all threads because critical thread failed.");
					shutdownModules();
					return ERR_SM;
				}
				else
				{
					// try to restart
					Console::WriteLine(ThreadList[i]->Name + " failed. Attempting to restart the failed thread.");
					ThreadList[i]->Abort();					
					ThreadList[i] = gcnew Thread(ThreadPropertiesList[i]->ThreadStart_);
					SM_TM_->ThreadBarrier = gcnew Barrier(1);
					ThreadList[i]->Start();
				}
			}
		}
	}
	return SUCCESS;	
}