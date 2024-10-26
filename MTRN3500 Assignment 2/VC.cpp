#using <System.dll>
#include "VC.h"
#include "NetworkedModule.h"

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

VehicleControl::VehicleControl(SM_ThreadManagement^ SM_TM, SM_VehicleControl^ SM_VC)
{
	SM_VC_ = SM_VC;
	SM_TM_ = SM_TM;
	Watch = gcnew Stopwatch;
	flag = false;
}

void VehicleControl::threadFunction()
{
	Console::WriteLine("VehicleControl thread is starting");
	//setup the stopwatch
	Watch = gcnew Stopwatch;

	// wait at the barrier
	SM_TM_->ThreadBarrier->SignalAndWait();

	// Establish connection at the specified address and port
	connect(WEEDER_ADDRESS, 25000);

	// start the stopwatch
	Watch->Start();

	// start thread loop
	while (!getShutdownFlag())
	{
		Console::WriteLine("VehicleControl thread is running");
		processHeartbeats();
		if (communicate() == SUCCESS)
		{
			processSharedMemory();
		}
		Thread::Sleep(20);
	}
	Console::WriteLine("VehicleControl thread is terminating");
}

void VehicleControl::shutdownModules() {
	SM_TM_->shutdown = 0xFF;
}

bool VehicleControl::getShutdownFlag()
{
	return SM_TM_->shutdown & bit_VC;
}

error_state VehicleControl::processSharedMemory()
{
	return SUCCESS;
}

error_state VehicleControl::processHeartbeats()
{
	// is the VC bit in the heartbeat byte down?
	if ((SM_TM_->heartbeat & bit_VC) == 0)
	{
		// put the vc bit up
		SM_TM_->heartbeat |= bit_VC;

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

/////////////////////////////////////////////////////////////////////////////////////////////////
// Establish TCP connection
error_state VehicleControl::connect(String^ hostName, int portNumber)
{

	Client = gcnew TcpClient(hostName, portNumber);
	Stream = Client->GetStream();
	Client->NoDelay = true;
	Client->ReceiveTimeout = 25000;
	Client->SendTimeout = 25000;
	Client->ReceiveBufferSize = 1024;
	Client->SendBufferSize = 1024;
	SendData = gcnew array<unsigned char>(64);
	ReadData = gcnew array<unsigned char>(2048);


	// Define an authentication command as a string.
	String^ authCommand = "5452298\n";

	// Convert the authentication command to a byte array using ASCII encoding.
	array<unsigned char>^ byteArray = Encoding::ASCII->GetBytes(authCommand);

	// Send the byte array over the network stream.
	Stream->Write(byteArray, 0, byteArray->Length);

	// Pause for a short period to allow the command to be processed.
	System::Threading::Thread::Sleep(50);

	// Read the incoming response data from the stream into the ReadData buffer.
	Stream->Read(ReadData, 0, ReadData->Length);


	return SUCCESS;
	
}

error_state VehicleControl::communicate()
{
	// Formulate the command string with current steering, speed, and flag status.
	String^ commandString = "# " + SM_VC_->Steering + " " + SM_VC_->Speed + " " + (flag ? "1" : "0") + " #";

	// Convert the constructed command string into a byte array for transmission.
	array<unsigned char>^ byteData = Encoding::ASCII->GetBytes(commandString);
	Stream->Write(byteData, 0, byteData->Length);

	System::Threading::Thread::Sleep(10);

	// Switch the flag state.
	flag = !flag;

	return SUCCESS;
}
