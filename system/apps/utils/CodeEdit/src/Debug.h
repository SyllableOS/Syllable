#ifndef DEBUG_H
#define DEBUG_H

#include <iostream>
#include <fstream>
using namespace std;

#define DEBUG_ENABLED 1
template <class T>
class Debug
{
public:
    Debug(T vList)
    {
        std:: cout << vList << std::endl;
    }
    Debug(const char* path, T vList)
    {
        ofstream file(path, ios::app);
        if (path)
            file << vList << endl;
    }
private:
};
#endif





