#ifndef __F_UTIL__EXECUTE_H__
#define __F_UTIL__EXECUTE_H__

#include <util/string.h>
#include <unistd.h>
#include <stdio.h>
#include <storage/file.h>
#include <string.h>

using namespace os;

class Execute
{
public:
    
    enum
    {
        EXECUTE_EXECVP=0x002,
        EXECUTE_SYSTEM,
        EXECUTE_POPEN
    };
    
    Execute(const String& cExecute="", const String& cParam="", uint32 nType=EXECUTE_EXECVP);
    ~Execute();
    
    void Run();
    
    String GetCommand();
    String GetArguments();
    uint32 GetType();
    
    void SetCommand(const String&);
    void SetArguments(const String&);
    void SetType(uint32);
    
    bool IsValid();
private:
    class Private;
    Private* m;
    
    void _RunExecvp();
    void _RunSystem();
    void _RunPopen();  
    void _GetParameters(char**);  
};
    
#endif    //__F_UTIL__EXECUTE_H__
