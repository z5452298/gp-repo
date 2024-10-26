#using <System.dll>
#include "Display.h"
#include "NetworkedModule.h"

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

Display::Display() {}

Display::Display(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser, SM_GPS^ SM_GNSS)
{
	SM_TM_ = SM_TM;
	SM_Laser_ = SM_Laser;
	SM_GNSS_ = SM_GNSS;
}

void Display::shutdownModules()
{
	SM_TM_->shutdown = 0xFF;
}
bool Display::getShutdownFlag()
{
	return SM_TM_->shutdown & bit_DISPLAY;
}

error_state Display::processSharedMemory()
{
	return SUCCESS;
}

void Display::threadFunction()
{
	Console::WriteLine("Display thread is starting");
	//setup the stopwatch
	Watch = gcnew Stopwatch;

	// wait at the barrier
	SM_TM_->ThreadBarrier->SignalAndWait();

	// Establish connection at the specified address and port
	connect(DISPLAY_ADDRESS, 28000);

	// start the stopwatch
	Watch->Start();

	// start thread loop
	while (!getShutdownFlag())
	{
		Console::WriteLine("Display thread is running");
		processHeartbeats();
		if (communicate() == SUCCESS)
		{
			processSharedMemory();
		}
		Thread::Sleep(20);
	}
	Console::WriteLine("Display thread is terminating");
}

error_state Display::processHeartbeats()
{
	// is the Display bit in the heartbeat byte down?
	if ((SM_TM_->heartbeat & bit_DISPLAY) == 0)
	{
		// put the Display bit up
		SM_TM_->heartbeat |= bit_DISPLAY;

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

// Establish TCP connection
error_state Display::connect(String^ hostName, int portNumber)
{

	Client = gcnew TcpClient(hostName, portNumber);
	Stream = Client->GetStream();
	Client->NoDelay = true;
	Client->ReceiveTimeout = 500;
	Client->SendTimeout = 500;
	Client->ReceiveBufferSize = 1024;
	Client->SendBufferSize = 1024;

	SendData = gcnew array<unsigned char>(64);
	ReadData = gcnew array<unsigned char>(64);

	return SUCCESS;
	
}

void Display::sendDisplayData()
{
	Monitor::Enter(SM_Laser_->lockObject);
	
	// Serialize the data arrays to a byte array
	//(format required for sending)
	array<Byte>^ dataX = gcnew array<Byte>(SM_Laser_->x-> Length * sizeof(double));
	Buffer::BlockCopy(SM_Laser_->x, 0, dataX, 0, dataX->Length);
	array<Byte>^ dataY = gcnew array<Byte>(SM_Laser_->y->Length * sizeof(double));
	Buffer::BlockCopy(SM_Laser_->y, 0, dataY, 0, dataY->Length);
	// Send byte data over connection
	Stream->Write(dataX, 0, dataX->Length);
	Thread::Sleep(10);
	Stream->Write(dataY, 0, dataY->Length);
	
	Monitor::Exit(SM_Laser_->lockObject);
	
}

error_state Display::communicate()
{
	sendDisplayData();
	return SUCCESS;

}
