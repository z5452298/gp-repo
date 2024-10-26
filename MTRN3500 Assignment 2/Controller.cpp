#using <System.dll>

#include "Controller.h"
#include <UGVModule.h>

#define STEER_SCALER -40.0
#define FORWARD 1
#define REVERSE -1

// Default constructor that initializes the Controller class.
// Sets the default controller ID to 1 and input type to Xbox.
Controller::Controller(SM_ThreadManagement^ SM_TM, SM_VehicleControl^ SM_VC) :
    Controller(SM_TM, SM_VC, 1, 1) {}

//inputType 0 = > xbox, 1 = > keyboard
Controller::Controller(SM_ThreadManagement^ SM_TM, SM_VehicleControl^ SM_VC, DWORD controllerId, int inputMode)
{
    SM_VC_ = SM_VC;
    SM_TM_ = SM_TM;
    Watch = gcnew Stopwatch;
    controller = new ControllerInterface(controllerId, inputMode);
}

void Controller::threadFunction()
{
    Console::WriteLine("Controller thread is starting");
    //setup the stopwatch
    Watch = gcnew Stopwatch;

    // wait at the barrier
    SM_TM_->ThreadBarrier->SignalAndWait();

    // start the stopwatch
    Watch->Start();

    // start thread loop
    while (!getShutdownFlag())
    {
        //Console::WriteLine("Controller thread is running");
        processHeartbeats();
        processSharedMemory();
        Thread::Sleep(20);
    }
    Console::WriteLine("Controller thread is terminating");
}

error_state Controller::processHeartbeats()
{
    // is the GNSS bit in the heartbeat byte down?
    if ((SM_TM_->heartbeat & bit_CONTROLLER) == 0)
    {
        // put the GNSS bit up
        SM_TM_->heartbeat |= bit_CONTROLLER;

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

void Controller::shutdownModules()
{
    SM_TM_->shutdown = 0xFF;
}

bool Controller::getShutdownFlag()
{
    return SM_TM_->shutdown & bit_CONTROLLER;
}

error_state Controller::processSharedMemory()
{
    // Retrieve the controller state
    controllerState ctrlState = controller->GetState();
    double adjustedSteering = ctrlState.rightThumbX * STEER_SCALER;
    int speedValue = 0;

    // Determine the speed and shutdown status based on controller state
    if (ctrlState.isConnected)
    {
        speedValue = (ctrlState.rightTrigger) ? FORWARD : (ctrlState.leftTrigger) ? REVERSE : 0;

        // Check if shutdown button is pressed
        if (ctrlState.buttonX)
        {
            shutdownModules();
        }
    }
    else
    {
        adjustedSteering = 0.0;
        speedValue = 0;
        Console::WriteLine("Unable to establish connection with controller");
        return ERR_CONNECTION;
    }

    // Update shared memory with lock protection
    Monitor::Enter(SM_VC_->lockObject);
    SM_VC_->Steering = adjustedSteering;
    SM_VC_->Speed = speedValue;
    Monitor::Exit(SM_VC_->lockObject);

    return SUCCESS;
}

