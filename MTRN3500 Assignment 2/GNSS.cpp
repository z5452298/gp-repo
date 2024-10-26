#using <System.dll>
#include "GNSS.h"
#include "NetworkedModule.h"
#include "SMObjects.h"

using namespace System;
using namespace System::Threading;
using namespace System::Diagnostics;

/////////////////////////////////////////////////////////////////////////////////////////////////
#define CRC32_POLYNOMIAL 0xEDB88320L
unsigned long CRC32Value(int i);
unsigned long CalculateBlockCRC32(
	unsigned long ulCount,
	unsigned char* ucBuffer);

#pragma pack(push, 4)
struct GNSS_struct
{
	unsigned int Header;
	unsigned char Discards1[40]; 
	double Northing; 
	double Easting; 
	double Height; 
	unsigned char Discards2[40];
	unsigned int CRC;
};
#pragma pack(pop, 4)
//refer back to lecture if any issue found here
////////////////////////////////////////////////////////////////////////
GNSS::GNSS(SM_ThreadManagement^ SM_TM, SM_GPS^ SM_GNSS)
{
	SM_GNSS_ = SM_GNSS;
	SM_TM_ = SM_TM;
	Watch = gcnew Stopwatch;
}

void GNSS::threadFunction()
{
	Console::WriteLine("GNSS thread is starting");
	//setup the stopwatch
	Watch = gcnew Stopwatch;

	// wait at the barrier
	SM_TM_->ThreadBarrier->SignalAndWait();

	// Establish connection at the specified address and port
	connect(WEEDER_ADDRESS, 24000);

	// start the stopwatch
	Watch->Start();

	// start thread loop
	while (!getShutdownFlag())
	{
		Console::WriteLine("GNSS thread is running");
		processHeartbeats();
		if (communicate() == SUCCESS)
		{
			processSharedMemory();
		}
		Thread::Sleep(20);
	}
	Console::WriteLine("GNSS thread is terminating");
}

error_state GNSS::processHeartbeats()
{
	// is the GNSS bit in the heartbeat byte down?
	if ((SM_TM_->heartbeat & bit_GPS) == 0)
	{
		// put the GNSS bit up
		SM_TM_->heartbeat |= bit_GPS;

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

void GNSS::shutdownModules()
{
	SM_TM_->shutdown = 0xFF;
}

bool GNSS::getShutdownFlag()
{
	return SM_TM_->shutdown & bit_GPS;
}

unsigned long CRC32Value(int i)
{
	int j;
	unsigned long ulCRC;
	ulCRC = i;
	for (j = 8; j > 0; j--) {
		if (ulCRC & 1)
			ulCRC = (ulCRC >> 1) ^ CRC32_POLYNOMIAL;
		else
			ulCRC >>= 1;
	}
	return ulCRC;
}

unsigned long CalculateBlockCRC32(
	unsigned long ulCount,
	unsigned char* ucBuffer)
{
	unsigned long ulTemp1;
	unsigned long ulTemp2;
	unsigned long ulCRC = 0;
	while (ulCount-- != 0)
	{
		ulTemp1 = (ulCRC >> 8) & 0x00FFFFFFL;
		ulTemp2 = CRC32Value(((int)ulCRC ^ *ucBuffer++) & 0xff);
		ulCRC = ulTemp1 ^ ulTemp2;
	}
	return(ulCRC);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Establish TCP connection
error_state GNSS::connect(String^ hostName, int portNumber)
{
	
	Client = gcnew TcpClient(hostName, portNumber);
	Stream = Client->GetStream();
	Client->NoDelay = true;
	Client->ReceiveTimeout = 500;
	Client->SendTimeout = 500;
	Client->ReceiveBufferSize = 1024;
	Client->SendBufferSize = 1024;
	SendData = gcnew array<unsigned char>(64);
	ReadData = gcnew array<unsigned char>(1024);
	System::Threading::Thread::Sleep(20);
	return SUCCESS;

}

error_state GNSS::communicate()
{
	Stream->Read(ReadData, 0, ReadData->Length);
	System::Threading::Thread::Sleep(10);
	return SUCCESS;
}

error_state GNSS::processSharedMemory()
{
	GNSS_struct myGNSS;
	unsigned char* BytePtr = (unsigned char*)&myGNSS;
	for (int i = 0; i < sizeof(GNSS_struct); i++) {
		*(BytePtr + i) = ReadData[i];
	}

	Monitor::Enter(SM_GNSS_->lockObject);
	
	SM_GNSS_->Northing = myGNSS.Northing;
	SM_GNSS_->Easting = myGNSS.Easting;
	SM_GNSS_->Height = myGNSS.Height;
	
	unsigned int CRCRef = CalculateBlockCRC32(108, BytePtr);

	Monitor::Exit(SM_GNSS_->lockObject);

	Console::WriteLine("Northing:{0:F4} Easting:{1:F4} Height:{2:F4} CRC:{3:X}", SM_GNSS_->Northing, SM_GNSS_->Easting, SM_GNSS_->Height, myGNSS.CRC);
	

	return SUCCESS;
}

