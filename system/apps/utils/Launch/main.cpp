#include "application.h"

using namespace os;

int main(int argc, char** argv)
{
	RunApplication* pcApp = new RunApplication();
	pcApp->Run();
	return 0;
}	
