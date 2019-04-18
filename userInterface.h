#ifndef _CGEM_EVXA_HEADER_INCLUDED_

#include <cmos_curi_platform.h>
#include "evxaTester.h" // Set of helper classes that address tester side access/threading
#include "cmdLineSwitches.h"
#include "recipeSupport.h"
#include <unistd.h>
#include <pwd.h>
#include <iostream>
#include <fstream>
//#include <xtrf.h>
#include <sstream> 
#include <utility.h>

// S10F1 terminal message list
#define S10F1_Loaded "EQ_RH_EVID<003>: TP <TP Name> loaded"
#define S10F1_Unloaded "EQ_RH_EVID<004>: TP <TP Name> unloaded"


template < typename T >
std::string num2stdstring(T value) {
	return static_cast< std::ostringstream* >(&(std::ostringstream() << value))->str();
}

// This is to compile in both 32bit and 64bit
#if __x86_64__
	typedef int32_t FA_LONG;
	typedef uint32_t FA_ULONG;
	#define _FAPROC_LONG_FS_ d
	#define _FAPROC_ULONG_FS_ u
#else
	typedef long FA_LONG;
	typedef unsigned long FA_ULONG;
	#define _FAPROC_LONG_FS_ ld
	#define _FAPROC_ULONG_FS_ lu
#endif

#define EVXA_DEMO_VERSION "1.00"

////////////////////////////////////////

class XML_Node; // forward declaration.
class CuserEvxaInterface : public evxaTester::ConChangeAPI  {
private:

	// debug logger
	CUtil::CLog m_Debug; 
	CUtil::CLog m_Log; 

	bool m_bDisableRobot;

    CuserCmdSwitches m_args;
    evxaTester::CevxaTester *m_tester;

    // flage to determine when a restartTester has occurred.
    bool m_testerRestart;

    // flag to determine if we are parsing a recipe.
    // Set to true when we are parsing a recipe.
    // If we are not parsing a recipe then we don't want to send status 
    // after a program is loaded.
    // reset to false on initialization and at beginning of parsing.
    bool m_recipeParse;

    // flag to determine if the recipe parsing is successful or not.
    // reset to false on initialization and at beginning of parsing.
    bool m_recipeParseResult;
    
    // flag to determine if we should send test program parameter data.
    // If reload strategy is never and there isn't a program loaded
    // then don't send teh test program parameters.
    // reset to false on initialization and at beginning of parsing.
    bool m_skipProgramParam;

    // This flag is set true when the recipe results have been sent.
    // reset to false on initialization and at beginning of parsing.
    bool m_statusResultsHaveBeenSent;

    // This variable will hold the recipe parse status that is sent back to cgem.
    // The default will be EVX_RECIPE_PARSE_BEGIN.  Will be set to EVX_RECIPE_PARSE_ABORT
    // if the recipe handler detects an abort load occurred during program load.
    EVX_RECIPE_PARSE_STATUS m_recipeParseStatus;

	CTestProgArgs m_TPArgs;
	CMIRArgs m_MIRArgs;
	CSDRArgs m_SDRArgs;
	CRecipeConfig m_ConfigArgs; 
	CGDR m_GDR;

    // This section is for Thread synchronization 
    // It uses a condition variable to synchronize the recipeDecode thread
    // with the StateNotification thread.
    pthread_mutex_t m_condMutex;
    pthread_cond_t m_wakeUp;
    bool m_goAway;
    bool m_taskComplete;
    FA_ULONG m_currentState;
    FA_ULONG m_currentMinorState;
    std::vector<unsigned int> m_currentProgramStateArray;

    void setupWaitForNotification(FA_ULONG wait_state, FA_ULONG wait_minor_state);
    void setupProgramNotification(EVX_PROGRAM_STATE *wait_program_states);
    void waitForNotification();
    void sendNotificationComplete(FA_ULONG wait_state, FA_ULONG wait_minor_state);

    void commonInit();
    ProgramControl *PgmCtrl(); 

    // XML Parsing and Data Gathering
    bool parseXML(const char* recipe_text);
    bool parseTesterRecipe(XML_Node *testerRecipe);
    bool parseSTDFandContextUpdate(XML_Node *parseSTDFUpdate);
    bool parseTestPrgmIdentifier(XML_Node *testPrgmIdentifier);
    bool parseTestPrgm(XML_Node *testPrgm);
    bool parseTestPrgmLoader(XML_Node *testPrgmLoader);
    bool parseTestPrgmCopierLoaderScript(XML_Node *testPrgmCopierLoaderScript);
    bool parseSTDF(XML_Node *stdf);
    bool parseSTDFRecord(XML_Node *STDFRecord);
    bool parseMIR(XML_Node *MIRRecord);
    bool parseSDR(XML_Node *SDRRecord);

    // Config file parsing
    bool parseRecipeHandlerConfiguration(XML_Node *recipeConfig);
    bool parseSiteConfiguration(XML_Node *siteConfig);
    bool updateProgramLoad();
    bool updateTestProgramData();
    bool executeRecipeReload(); 
    bool forceDownloadAndLoad();
    bool loadProgramURI();
    bool neverDownloadForceLoad();
    bool neverDownloadAttemptLoad();
    bool neverDownloadNeverLoad();
    bool attemptDownloadForceLoad();
    bool attemptDownloadAttemptLoad();
    bool clearAllParams();
    bool sendMIRParams(); 
    bool sendSDRParams();
    bool sendTPParams();
    bool sendEndOfLot();
    bool sendEndOfWafer();
    bool sendStartOfLot();
    bool sendStartOfWafer();
    void sendRecipeResultStatus(bool result);
    void resetRecipeVars();

	bool loadProgram(const std::string &szProgFullPath);
	bool unloadProgram(const std::string &sszProgFullPath, bool notify = true);
	bool downloadProgramFromServerToLocal();
	void copyFile(const std::string& from, const std::string& to);
	bool isFileExist(const std::string& szFile);
	bool unpackProgramFromLocalToTest();
	bool isProgramToLoadAlreadyLoaded();
	bool handleBackToIdleStrategy();
   	bool parseGDR(XML_Node *GDRRecord);
	bool updateSTDFAfterProgLoad();
 	bool setLotInformation(const EVX_LOTINFO_TYPE type, param& field, const char* label);

	class customGDR 
	{
	public:
		std::string name;
		std::vector< std::string > fields;
	};

	std::vector< customGDR > m_customGDRs;

public:

    CuserEvxaInterface(void);
    CuserEvxaInterface(int argc, char *argv[], char *envp[]);
    CuserEvxaInterface(const std::string& testerName, bool connect);

    CuserEvxaInterface(const std::string& testerName, FA_ULONG headNumber, bool connect);
    virtual ~CuserEvxaInterface();
    evxaTester::CevxaTester* GetTesterClass();

    inline CuserCmdSwitches& args(void) { return m_args; }

    bool connectToTester(const std::string& testerName, int headNumber = 1);
    bool connectToTester(void);

    bool debug(void);
    bool debug(bool b);

    bool activeTester(void);
    void setTesterRestart(bool val);
    bool isTesterRestart();
    bool registerCommandNotification(const int arr_size, const EVX_NOTIFY_COMMANDS evx_cmd[]);
    const char* getRecipeParseStatusName(EVX_RECIPE_PARSE_STATUS state);

    void shutdownTester(void);


    bool getEnvVar(const std::string& token, std::string& value);


    bool getTesterDirectories(std::string& mainTesterDir, std::string& pwrupDir, std::string& logDir);
    
    ///////////////// Overloaded ConChangeAPI functions  //////////////
   ///
   ///

    void dlogChange(const EVX_DLOG_STATE state);
    void expressionChange(const char *expr_obj_name);
    void expressionChange(const char *expr_obj_name, const char *expr_name,
                                  const char *expr_value, int site);
    void objectChange(const XClientMessageEvent xmsg);
    void programChange(const EVX_PROGRAM_STATE state, const char *text_msg);
    void modVarChange(const int id, char *value);
    void programRunDone(const int array_size,
                            int site[],
                            int serial[],
                            int swbin[],
                            int hwbin[],
                            int pass[],
                            LWORD dsp_status
                            );
    void restartTester(void);
    void evtcDisconnected(void);
    void evtcConnected(void);
    void streamChange(void);
    void tcBooting(void);
    void testerReady(void);
    void gemRunning(void);
    void alarmChange(const EVX_ALARM_STATE alarm_state, const ALARM_TYPE alarm_type,
                             const FA_LONG time_occurred, const char *description);
    void testerStateChange(const EVX_TESTER_STATE tester_state);
    void waferChange(const EVX_WAFER_STATE wafer_state, const char *wafer_id);
    void lotChange(const EVX_LOT_STATE lot_state, const char *lot_id);
    void EvxioMessage(int responseNeeded, int responseAquired, char *evxio_msg);

    // Temporarilly return constant value untill find the way how to identify that
    bool IsRobotPresent() {  return false;  }

    // Recipe Handlers
    void RecipeDecodeAvailable(const char *recipe_text, bool &result);
    void RecipeDecode(const char *recipe_text);

    // configuration file parsing
    bool parseRecipeHandlerConfigurationFile(const std::string& recipeFilePath);
    
    // print the results of parsing the configuration file.
    bool printConfigParams();

    // check that all params of the configuration file have been parsed.
    bool checkConfigParams();
    bool checkProgLocation();

	bool getUnisonVersion();

	bool SendMessageToHost( bool bEvent, bool bTestProgram, const std::string& id, const std::string& msg, const std::string& val );

};


#define _CGEM_EVXA_HEADER_INCLUDED_
#endif // _CGEM_EVXA_HEADER_INCLUDED_
