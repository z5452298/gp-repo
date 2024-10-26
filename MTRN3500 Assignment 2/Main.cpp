#include "TMM.h"
#include "Display.h"
#include "Laser.h"
#include "GNSS.h"
#include "Controller.h"
#include "VC.h"
#include "NetworkedModule.h"
#include<iostream>

int main(void) {
	ThreadManagement^ myTMM = gcnew ThreadManagement();
	myTMM->setupSharedMemory();
	myTMM->threadFunction();

	Console::ReadKey();
	return 0;
}