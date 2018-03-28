#include "userInterface.h"

// The following includes are for signal handling.
#include <wait.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

int main (int argc, char *argv[], char *envp[]);
int mainEvxaUserLoop(CuserEvxaInterface& evxaTester);

/***********************************************************************
childSigCatcher is the function that gets called for SIGCHLD signal
handling.  The function gets the signal, sets a flag and returns to
main program.
Used to cleanup pset defunct child process.
***********************************************************************/

void childSigCatcher(int the_sig)
{
    int status, childId;
    while ( (childId = waitpid (-1, &status, WNOHANG )) > 0 ) {
	//      fprintf(stderr, "childId: %d\n", childId);
    }	
}

//////////////////////////////////////////////////////////////////////////////////////

int main (int argc, char *argv[], char *envp[])
{
 	// setup signal to capture children proceeses terminating.
	signal(SIGCHLD, childSigCatcher);
	signal(SIGPIPE, SIG_IGN);

	// create our main object
	CuserEvxaInterface l_evxaDemo(argc, argv, envp);

	// if we don't have tester name, let's make up one
	if (!l_evxaDemo.args().haveTesterName()) 
	{
		std::string str("");
		if (!l_evxaDemo.getEnvVar("LTX_TESTER", str)) 
		{
			str = "sim";
			uid_t uid = getuid();
			struct passwd *passwd_info = getpwuid(uid);
			if (NULL != passwd_info) 
			{
				str = passwd_info->pw_name;
				str += "_sim";
			}
		}
		l_evxaDemo.args().testerName(str);
	}

	// if it's call for help, print the help options, or show version if that's what it's asking for
	if (l_evxaDemo.args().isSet("help")){l_evxaDemo.args().displayHelp(std::cout);return 0;} 
	if (l_evxaDemo.args().isSet("version")) {std::cout << EVXA_DEMO_VERSION << std::endl;return 0;} 

	// if we're in debug mode...
	if (!l_evxaDemo.args().debug()){std::string str("");if (l_evxaDemo.getEnvVar("LTX_EVXA_DEMO_DEBUG", str)) l_evxaDemo.args().debug(true);}


	// make sure there's a config file, if not, let's GTFO
	if (l_evxaDemo.args().haveConfig())
	{
		if (!l_evxaDemo.parseRecipeHandlerConfigurationFile(l_evxaDemo.args().config()))
		{
			fprintf(stdout, "[ERROR] Something went wrong in parsing recipe handler configuration file.\n");	
		}		
	}
	l_evxaDemo.printConfigParams();
	if (!l_evxaDemo.checkProgLocation())
	{
		fprintf(stdout, "[ERROR] Program location does not exist. exiting...\n");
 		return 0;
	}

	// let the fun begin
	return mainEvxaUserLoop(l_evxaDemo);        

}

//////////////////////////////////////////////////////////////////////////////////////

bool setupRegisterCommands(CuserEvxaInterface& evxaTester)
{
    int arr_size = 2;
    EVX_NOTIFY_COMMANDS cmds[2];
    cmds[0] = EVX_CMD_RECIPE_DECODE_AVAILABLE;
    cmds[1] = EVX_CMD_RECIPE_DECODE;
    evxaTester.registerCommandNotification(arr_size, cmds);
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////

int mainEvxaUserLoop(CuserEvxaInterface& evxaTester)
{
    int retVal = 0;

    try
    {
	// Just stay in here forever  Need to ctl-c to exit.
	while (1) {
	    fprintf(stderr, "Waiting for tester to startup\n");
	    // Wait for the tester to startup.
	    while (!evxaTester.connectToTester()) {
		_SLEEP_MSEC_(1000);
	    }

	    // Wait for restart_tester to finish.
	    while (!evxaTester.activeTester()) 
		_SLEEP_MSEC_(200); // get past restart tester

	    // Setup Command Notifications
	    setupRegisterCommands(evxaTester);


	    // DEBUG XML ONLY.
	    // Comment out when using with HOST.
	   // evxaTester.RecipeDecode(xml_trial); // allan commented for debugging purpose

	    // Stay in this loop while the tester is running.
	    fprintf(stderr, "Tester is running\n");
	    evxaTester.setTesterRestart(false); // explicitly set this to false
	    while (!evxaTester.isTesterRestart())
	    {
		_SLEEP_MSEC_(500); // never ending loop just to watch messages come through the interface.
	    }
	    fprintf(stderr, "Tester is done running\n");
	    evxaTester.shutdownTester();
	}
    }
    catch (...)
    {
        retVal = 1;
    }

    return retVal;
}

