#using <System.dll>
#include "Laser.h"
#include "NetworkedModule.h"
#include "SMObjects.h"

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

Laser::Laser(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser) {
	SM_Laser_ = SM_Laser;
	SM_TM_ = SM_TM;
	Watch = gcnew Stopwatch;
}

void Laser::threadFunction()
{
	Console::WriteLine("Laser thread is starting");
	//setup the stopwatch
	Watch = gcnew Stopwatch;

	// wait at the barrier
	SM_TM_->ThreadBarrier->SignalAndWait();

	// Establish connection at the specified address and port
	connect(WEEDER_ADDRESS, 23000);

	// start the stopwatch
	Watch->Start();

	// start thread loop
	while (!getShutdownFlag())
	{
		Console::WriteLine("Laser thread is running");
		processHeartbeats();
		if (communicate() == SUCCESS)
		{
			processSharedMemory();
		}
		Thread::Sleep(20);
	}
	Console::WriteLine("Laser thread is terminating");
}

void Laser::shutdownModules()
{
	SM_TM_->shutdown = 0xFF;
}

bool Laser::getShutdownFlag()
{
	return SM_TM_->shutdown & bit_LASER;
}

error_state Laser::processHeartbeats()
{
	// is the laser bit in the heartbeat byte down?
	if ((SM_TM_->heartbeat & bit_LASER) == 0)
	{
		// put the laser bit up
		SM_TM_->heartbeat |= bit_LASER;

		// reset stopwatch
		Watch->Restart();
	}
	else {

		if (Watch->ElapsedMilliseconds > CRASH_LIMIT) {
			shutdownModules();
			return ERR_SM;
		}

	}
	return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Establish TCP connection
error_state Laser::connect(String^ hostName, int portNumber)
{
	Client = gcnew TcpClient(hostName, portNumber);
	Stream = Client->GetStream();
	Client->NoDelay = true;
	Client->ReceiveTimeout = 500;
	Client->SendTimeout = 500;
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


error_state Laser::communicate()
{
	//not sure of this need more tesing with laser
	String^ Command = "sRN LMDscandata";
	SendData = Encoding::ASCII->GetBytes(Command);
	Stream->WriteByte(0x02); 
	Stream->Write(SendData, 0, SendData->Length);
	Stream->WriteByte(0x03); 
	System::Threading::Thread::Sleep(50);
	Stream->Read(ReadData, 0, ReadData->Length);

	return SUCCESS;
}

error_state Laser::processSharedMemory()
{	
	String^ laserData = Encoding::ASCII->GetString(ReadData);

	// Check if the total number of fields have been received 
	array<String^>^ StringArray = laserData->Split(' ');
	int NumPoints = System::Convert::ToInt32(StringArray[25], 16);

	Monitor::Enter(SM_Laser_->lockObject);

	// Write the extracted data to shared memory 
	for (int pointIdx = 0; pointIdx < NumPoints; ++pointIdx)
	{
		// Convert the hex value from the string array to an integer
		int convertedValue = System::Convert::ToInt32(StringArray[26 + pointIdx], 16);

		// Calculate the angle for the current point formula taken feom lecture
		double currentAngle = pointIdx * 0.5 * Math::PI / 180.0;

		// Calculate and assign the x and y coordinates
		SM_Laser_->x[pointIdx] = convertedValue * Math::Cos(currentAngle);
		SM_Laser_->y[pointIdx] = convertedValue * Math::Sin(currentAngle);
	}

	Monitor::Exit(SM_Laser_->lockObject);

	// Return success or invalid data error based on the length of the string array 
	if (StringArray->Length >= 393) {
		return SUCCESS;
	}
	else {
		return ERR_INVALID_DATA;
	}
}
