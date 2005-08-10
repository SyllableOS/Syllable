#include "execute.h" //<util/execute.h>

class Execute::Private
{
public:
    Private()
    {
        m_cCommand = "";
        m_cArguments = "";
        m_nType = EXECUTE_EXECVP;
    }
    
    String m_cCommand;
    String m_cArguments;
    uint32 m_nType;
};

/*************************************************************
* Description: The execute class, executes a command based on the parameters
*			   passed to it.
* Author: Rick Caudill 
* Date: Sun Oct 24 2004 03:46:04 
*************************************************************/
Execute::Execute(const String& cCommand, const String& cArguments, uint32 nType)
{
    m = new Private;
    m->m_cCommand = cCommand;
    m->m_cArguments = cArguments;
    m->m_nType = nType;
}

Execute::~Execute()
{
	delete m;
}

/*************************************************************
* Description: Sets the command
* Author: Rick Caudill 
* Date: Sun Oct 24 2004 03:47:37 
*************************************************************/
void Execute::SetCommand(const String& cCommand)
{
    m->m_cCommand = cCommand;
}

/*************************************************************
* Description: Sets the arguments to launch with the command
* Author: Rick Caudill 
* Date: Sun Oct 24 2004 03:48:36 
*************************************************************/
void Execute::SetArguments(const String& cArguments)
{
    m->m_cArguments = cArguments;
}

/*************************************************************
* Description: Sets the type, look at header file
* Author: Rick Caudill 
* Date: Sun Oct 24 2004 03:49:25 
*************************************************************/
void Execute::SetType(uint32 nType)
{
    m->m_nType = nType;
}

/*************************************************************
* Description: Gets the command name
* Author: Rick Caudill 
* Date: Sun Oct 24 2004 03:52:37 
*************************************************************/
String Execute::GetCommand()
{
    return m->m_cCommand;
}

/*************************************************************
* Description: Gets the arguments
* Author: Rick Caudill 
* Date: Sun Oct 24 2004 03:53:00 
*************************************************************/
String Execute::GetArguments()
{
    return m->m_cArguments;
}

/*************************************************************
* Description: Gets the type, look at the header file
* Author: Rick Caudill 
* Date: Sun Oct 24 2004 03:53:15 
*************************************************************/
uint32 Execute::GetType()
{
    return m->m_nType;
}

/*************************************************************
* Description: Determines if the command is a valid command
* Author: Rick Caudill 
* Date: Sun Oct 24 2004 03:53:59 
*************************************************************/
bool Execute::IsValid()
{
    bool bReturn;
    File* pcFile = new File(m->m_cCommand);
    bReturn = pcFile->IsValid();
    delete pcFile;
    return bReturn;
}

/*************************************************************
* Description: Runs the command
* Author: Rick Caudill 
* Date: Sun Oct 24 2004 03:54:29 
*************************************************************/
void Execute::Run()
{
    switch (m->m_nType)
    {
        case EXECUTE_EXECVP:
        {
                if (IsValid())
                {                
                     _RunExecvp();
                }
                break;
        }
        
        case EXECUTE_SYSTEM:
        {
                break;
        }

        case EXECUTE_POPEN:
        {
                break;
        }
    }
}

/*************************************************************
* Description: Internal
* Author: Rick Caudill 
* Date: Sun Oct 24 2004 03:54:46 
*************************************************************/
void Execute::_RunExecvp()
{
	char* ppzParameters[1024]={NULL,};
	_GetParameters(ppzParameters);
	
    if (fork() == 0)
    {
    	set_thread_priority(-1, 0);
        execvp(m->m_cCommand.c_str(),ppzParameters);
	}
}

/*this function was a pain in the arse :) */
void Execute::_GetParameters(char** ppzReturn)
{
	char* pzTemp=0;
	char* pzPointer=0;
	int i=1;
	ppzReturn[0] = (char*)m->m_cCommand.c_str();
	pzTemp = strtok_r((char*)m->m_cArguments.c_str(), " ",&pzPointer);
	while (pzTemp != NULL)
	{
		ppzReturn[i++] = pzTemp;
		pzTemp = strtok_r(NULL, " ",&pzPointer);
	}
	ppzReturn[i] = 0;
}
