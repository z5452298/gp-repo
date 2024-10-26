#pragma once
#include "Controller.h"
#include <UGVModule.h>
#include "SMObjects.h"

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;


ref struct ThreadProperties {

    ThreadStart^ ThreadStart_;
    bool Critical;
    String^ ThreadName; 
    uint8_t BitID;
    ThreadProperties(ThreadStart^ start, bool crit, uint8_t bit_id, String^ threadName)
	{
		ThreadStart_ = start;
		Critical = crit;        
		ThreadName = threadName;
        BitID = bit_id;	
	}


};

ref class ThreadManagement : public UGVModule {
public:
    // Create shared memory objects
    error_state setupSharedMemory();

    // Send/Recieve data from shared memory structures
    error_state processSharedMemory() override;

    // Shutdown all modules in the software
    void shutdownModules();

    // Get Shutdown signal for module, from Thread Management SM
    bool getShutdownFlag() override;

    // Thread function for TMM
    void threadFunction() override;

    // Heartbeates 
    error_state processHeartbeates();

private:
    // Add any additional data members or helper functions here    
    SM_ThreadManagement^ SM_TM_;
	SM_Laser^ SM_Laser_;
	SM_GPS^ SM_GNSS_;
	SM_VehicleControl^ SM_VC_;
	array<Stopwatch^>^ StopwatchList;
	array<Thread^>^ ThreadList;
	array<ThreadProperties^>^ ThreadPropertiesList;

};
