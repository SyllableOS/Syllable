#ifndef _TIME_NODE_H
#define _TIME_NODE_H

#include <iostream>
#include <list>
#include <util/string.h>
#include <util/datetime.h>

using namespace std;
using namespace os;

class TimeNode
{
public:
	TimeNode();
	
	void AddNode(const String&);
	bool FindNode(const DateTime&);
	
private:
};

#endif				
