#include "userInterface.h"
#include "xmlInterface.h"
#include <sstream> 
#include <string>

////////////////////////////////////////////////////////////////////////////////////////////////////////
// try to open file in read mode a number of tries. 
// once it pass in opening file during the tries, it stops and return true
// if after n tries and file fails to open still, return false
////////////////////////////////////////////////////////////////////////////////////////////////////////
bool isFileExist(const char* pFileNamePath, int nAttempt, int nDelaySecond)
{
	FILE* fp = 0;
	while(nAttempt)
	{
		fp = fopen(pFileNamePath, "r");	
		if (fp){ fclose(fp); return true; }
		else{ sleep(nDelaySecond); nAttempt--; }
	}
	return false;
}
 

struct CMutexLock
{
	CMutexLock(pthread_mutex_t & mutex) : m_mutex(mutex)
	{
	    pthread_mutex_lock(&m_mutex);
	}
	~CMutexLock()
	{
	    pthread_mutex_unlock(&m_mutex);
	}
	pthread_mutex_t & m_mutex;
};

CuserEvxaInterface::CuserEvxaInterface(void) 
{ 
    	commonInit(); 
}


CuserEvxaInterface::CuserEvxaInterface(int argc, char *argv[], char *envp[]) : m_args(argc, argv) 
{ 
	commonInit(); 
}

CuserEvxaInterface::CuserEvxaInterface(const std::string& testerName, bool connect) 
{ 
	commonInit();
	m_args.testerName(testerName);
	m_args.testHead(1);
	if (connect) connectToTester();
}

CuserEvxaInterface::CuserEvxaInterface(const std::string& testerName, FA_ULONG headNumber, bool connect) 
{
	commonInit();
	m_args.testerName(testerName);
	m_args.testHead(headNumber);
	if (connect) connectToTester();
}

CuserEvxaInterface::~CuserEvxaInterface() 
{
	shutdownTester();
}

evxaTester::CevxaTester* CuserEvxaInterface::GetTesterClass() 
{  
    	return m_tester; 
}

bool CuserEvxaInterface::connectToTester(const std::string& testerName, int headNumber) 
{
	m_args.testerName(testerName);
	m_args.testHead(headNumber);
	return connectToTester();
}

bool CuserEvxaInterface::connectToTester() 
{
	shutdownTester(); // Be on the safe side.
	if (!args().haveTesterName()) return false;
	bool ok = (NULL != (m_tester = new evxaTester::CevxaTester(args().testerName(), args().testHead())));
	if (ok) ok = m_tester->onChange(evxaTester::ConChangeAPI::getPtr());
	if (ok) ok = m_tester->connect();
	return ok;
}

bool CuserEvxaInterface::debug(void) 
{ 
	return args().debug(); 
}

bool CuserEvxaInterface::debug(bool b) 
{
    	return args().debug(b);
}

bool CuserEvxaInterface::activeTester(void) 
{
    	if (NULL != m_tester) return m_tester->haveTestHeadConnection();
    	return false;
}

void CuserEvxaInterface::setTesterRestart(bool val)
{
    	m_testerRestart = val;
}

bool CuserEvxaInterface::isTesterRestart()
{
    	return m_testerRestart;
}

bool CuserEvxaInterface::registerCommandNotification(const int arr_size, const EVX_NOTIFY_COMMANDS evx_cmd[]) 
{
    	bool result = false;
    	if (m_tester) result = m_tester->registerCommandNotification(arr_size, evx_cmd);
    	return result;
}

const char* CuserEvxaInterface::getRecipeParseStatusName(EVX_RECIPE_PARSE_STATUS state)	
{
    if (m_tester) return m_tester->getRecipeParseStatusName(state);
    else return "";
}

void CuserEvxaInterface::shutdownTester(void) 
{
	// In case we're waiting for a thread to complete.
	if (m_currentState != MAX_EVX_STATE) 
	{
		m_goAway = true;
		pthread_cond_signal(&m_wakeUp);
	}

	if (NULL != m_tester) 
	{
		delete m_tester;
		m_tester = NULL;
	}
}

bool CuserEvxaInterface::getEnvVar(const std::string& token, std::string& value) 
{
	bool foundEnv = false;
#if (defined(WIN32) || defined(_WINDOWS)) // Might move to header define
	TCHAR *ev;
	TCHAR smallBuf[8];
	DWORD dwLen = GetEnvironmentVariable(token.c_str(), smallBuf, 0);

	// We have an environment variable, so get it
	if (dwLen > 0) 
	{ 
		if ((ev = new TCHAR [dwLen+2]) != NULL) 
		{
			dwLen = GetEnvironmentVariable(token.c_str(), ev, (dwLen+2));
			if (dwLen > 0) 
			{
				value = ev;
				foundEnv = true;
			}
			delete [] ev;
		}
	}
#else
	char *ev = getenv(token.c_str());
	if (ev) 
	{
		value = (const char *) ev;
		foundEnv = true;
	}
#endif
	return foundEnv;
}


bool CuserEvxaInterface::getTesterDirectories(std::string& mainTesterDir, std::string& pwrupDir, std::string& logDir) 
{
	mainTesterDir = "";
	pwrupDir = "";
	logDir = "";

	std::string l_base("/opt/ltx/testers/");
	l_base += args().testerName();
	if (0 != access(l_base.c_str(), F_OK)) 
	{
		if (getEnvVar("HOME", l_base)) 
		{ 
			l_base += std::string("/synchro_sim/") + args().testerName();
		}
	}

	pwrupDir = l_base + std::string("/pwrup");
	if (0 == access(pwrupDir.c_str(), F_OK)) 
	{
		mainTesterDir = l_base;
		logDir = l_base + std::string("/log");
	}
	else pwrupDir = "";

	return (pwrupDir.length() > 0);
}
    

void CuserEvxaInterface::commonInit() 
{
	m_Debug.enable = args().debug();
	m_Debug << "[DEBUG] Running in DEBUG mode..." << CUtil::CLog::endl;
	m_Log.enable = true;
	m_bDisableRobot = false;
        m_tester = NULL;
	m_testerRestart = false;
	m_goAway = false;
	m_taskComplete = false;
	m_currentState = MAX_EVX_STATE;
	m_currentMinorState = MAX_EVX_PROGRAM_STATE;
	m_currentProgramStateArray.clear();
	resetRecipeVars();
}

void CuserEvxaInterface::resetRecipeVars()
{
	m_recipeParse = false;
	m_recipeParseResult = true; // set to false to report PARSE_FAIL.
	m_skipProgramParam = false;
	m_statusResultsHaveBeenSent = false;
	m_recipeParseStatus = EVX_RECIPE_PARSE_BEGIN;
	clearAllParams();
}

ProgramControl *CuserEvxaInterface::PgmCtrl()
{	
	if (NULL != m_tester) return m_tester->progCtrl();
	return NULL;
}

void CuserEvxaInterface::dlogChange(const EVX_DLOG_STATE state){ if (debug()) std::cout << "CuserEvxaInterface::dlogChange" << std::endl; }
void CuserEvxaInterface::expressionChange(const char *expr_obj_name){ if (debug()) std::cout << "CuserEvxaInterface::expressionChange" << std::endl; }
void CuserEvxaInterface::expressionChange(const char *expr_obj_name, const char *expr_name, const char *expr_value, int site){ if (debug()) std::cout << "CuserEvxaInterface::expressionChange" << std::endl; }
void CuserEvxaInterface::objectChange(const XClientMessageEvent xmsg){ if (debug()) std::cout << "CuserEvxaInterface::objectChange" << std::endl; }

/* ------------------------------------------------------------------------------------------------------
evxa event handler for program change state
------------------------------------------------------------------------------------------------------ */
void CuserEvxaInterface::programChange(const EVX_PROGRAM_STATE state, const char *text_msg)
{
	m_Debug << "CuserEvxaInterface::programChange (state = " << state << ", Message = \"" << text_msg << "\")" << CUtil::CLog::endl;
	if (!PgmCtrl()) 
	{
		m_Debug << "No ProgramControl at CuserEvxaInterface::programChange." << CUtil::CLog::endl;
		return;
    	}
	switch(state)
	{
		case EVX_PROGRAM_LOADING:
			m_Debug << "programChange: EVX_PROGRAM_LOADING" << CUtil::CLog::endl;
			break;
		case EVX_PROGRAM_LOAD_FAILED:
			m_Debug << "programChange: EVX_PROGRAM_LOAD_FAILED" << CUtil::CLog::endl;
			SendMessageToHost(false, "005", "load failed");
		    	m_recipeParseResult = false;
			break;
		case EVX_PROGRAM_LOADED:
		    	m_Debug << "programChange: EVX_PROGRAM_LOADED" << CUtil::CLog::endl;
			SendMessageToHost(true, "003", "loaded");
			updateSTDFAfterProgLoad();
			break;
		case EVX_PROGRAM_START: 
			m_Debug << "programChange: EVX_PROGRAM_START" << CUtil::CLog::endl;
			break;
		case EVX_PROGRAM_RUNNING:
			m_Debug << "programChange: EVX_PROGRAM_RUNNING" << CUtil::CLog::endl;
			break;
		case EVX_PROGRAM_UNLOADING:
			m_Debug << "programChange: EVX_PROGRAM_UNLOADING" << CUtil::CLog::endl;
			break;
		case EVX_PROGRAM_UNLOADED: 
			SendMessageToHost(true, "004", "unloaded");
			m_Debug << "programChange: EVX_PROGRAM_UNLOADED" << CUtil::CLog::endl;
			// If the program was unloaded and not by xtrf then unblock the robots.
			if (m_recipeParse == false) 
			{
			    m_Debug << "programChange: EVX_PROGRAM_UNLOADED UnblockRobot" << CUtil::CLog::endl;
			    EVXAStatus status = PgmCtrl()->UnblockRobot();
			    if (status != EVXA::OK) m_Debug << "Error PROGRAM_UNLAODED UnblockRobot(): status: " << status << " " << PgmCtrl()->getStatusBuffer() << CUtil::CLog::endl;
			}
			break;
		case EVX_PROGRAM_ABORT_LOAD:
			m_Debug << "programChange: EVX_PROGRAM_ABORT_LOAD" << CUtil::CLog::endl;
			SendMessageToHost(true, "006", "load abort");
		    	m_recipeParseStatus = EVX_RECIPE_PARSE_ABORT;
		    	m_recipeParseResult = false;
			break;
		case EVX_PROGRAM_READY:
			m_Debug << "programChange: EVX_PROGRAM_READY" << CUtil::CLog::endl;
			break;
		default:
			//m_Debug << "programChange: Not Handled" << state << CUtil::CLog::endl;
			if(PgmCtrl()->getStatus() !=  EVXA::OK) m_Log << "ERROR OCCURED!!!!!!!!!!!!!!!!!!!!!!!!!" << CUtil::CLog::endl;
		break;
	}

	// tell the recipe thread we got the notific/ation.
	sendNotificationComplete(EVX_PROGRAM_CHANGE, state);
}



void CuserEvxaInterface::programRunDone(const int array_size, int site[], int serial[], int swbin[], int hwbin[], int pass[], LWORD dsp_status)
{
	m_Debug << "CuserEvxaInterface::programRunDone" << CUtil::CLog::endl;

	if (m_bDisableRobot)
	{
		PgmCtrl()->BlockRobot();
		m_bDisableRobot = false;
	}	
}

void CuserEvxaInterface::restartTester()
{
    m_Debug << "CuserEvxaInterface::restartTester" << CUtil::CLog::endl;
    m_testerRestart = true;
}

void CuserEvxaInterface::modVarChange(const int id, char *value){ m_Debug << "CuserEvxaInterface::modVarChange" << CUtil::CLog::endl; }
void CuserEvxaInterface::evtcDisconnected(){ m_Debug << "CuserEvxaInterface::evtcDisconnected" << CUtil::CLog::endl; }
void CuserEvxaInterface::evtcConnected(){ m_Debug << "CuserEvxaInterface::evtcConnected" << CUtil::CLog::endl; }
void CuserEvxaInterface::streamChange(){ m_Debug << "CuserEvxaInterface::streamChange" << CUtil::CLog::endl; }
void CuserEvxaInterface::tcBooting(){ m_Debug << "CuserEvxaInterface::tcBooting" << CUtil::CLog::endl; }
void CuserEvxaInterface::testerReady(){ m_Debug << "CuserEvxaInterface::testerReady" << CUtil::CLog::endl; }
void CuserEvxaInterface::gemRunning(){ m_Debug << "CuserEvxaInterface::gemRunning" << CUtil::CLog::endl; }
void CuserEvxaInterface::EvxioMessage(int responseNeeded, int responseAquired, char *evxio_msg){ m_Debug << "[DEBUG] CuserEvxaInterface::EvxioMessage" << CUtil::CLog::endl; }

void CuserEvxaInterface::alarmChange(const EVX_ALARM_STATE alarm_state, const ALARM_TYPE alarm_type, const FA_LONG time_occurred, const char *description)
{
	m_Debug << "CuserEvxaInterface::alarmChange: "<< description << CUtil::CLog::endl;
	m_Debug << "alarm_state: " << alarm_state << ", alarm_type: " << alarm_type << CUtil::CLog::endl;
}

/* ------------------------------------------------------------------------------------------------------
handles event when unison auto testing is paused/resumed
------------------------------------------------------------------------------------------------------ */
void CuserEvxaInterface::testerStateChange(const EVX_TESTER_STATE tester_state)
{
	m_Debug << "CuserEvxaInterface::testerStateChange" << CUtil::CLog::endl;

	if (!PgmCtrl()) 
	{
		m_Debug << "No ProgramControl at CuserEvxaInterface::testerStateChange." << CUtil::CLog::endl;
		return;
    	}
	EVXAStatus status = EVXA::OK;
	switch (tester_state) 
	{
		case EVX_TESTER_PAUSED:
			m_bDisableRobot = true;
			m_Log << "TESTER PAUSED when program is running. robot will stop after EOT." << CUtil::CLog::endl; 
			if (!PgmCtrl()->isProgramRunning())
			{ 
				m_Log << "TESTER PAUSED when program is not running. robot is stopped instantly." << CUtil::CLog::endl;
				status = PgmCtrl()->BlockRobot(); 
				m_bDisableRobot = false; 
			}
			break;
		case EVX_TESTER_RESUMED:
			m_bDisableRobot = false;
			status = PgmCtrl()->UnblockRobot();
			m_Log << "TESTER RESUMED. robot is activated instantly." << CUtil::CLog::endl;
			break;
		default: break;
	}
	if (status != EVXA::OK) m_Log << "Error testerStateChange: status: " << status << " " << PgmCtrl()->getStatusBuffer() << CUtil::CLog::endl;
}

void CuserEvxaInterface::waferChange(const EVX_WAFER_STATE wafer_state, const char *wafer_id)
{
   	m_Debug << "[DEBUG] CuserEvxaInterface::waferChange" << CUtil::CLog::endl;
  	sendNotificationComplete(EVX_WAFER_CHANGE, wafer_state);	    
}

void CuserEvxaInterface::lotChange(const EVX_LOT_STATE lot_state, const char *lot_id)
{
	m_Debug << "[DEBUG] CuserEvxaInterface::lotChange ( state = " << lot_state << ", Lot ID = \"" << lot_id << "\")" << CUtil::CLog::endl;
	sendNotificationComplete(EVX_LOT_CHANGE, lot_state);
}

void CuserEvxaInterface::RecipeDecodeAvailable(const char *recipe_text, bool &result)
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::RecipeDecodeAvailable()" << CUtil::CLog::endl;
	resetRecipeVars();
	
	//Check the recipe text for the xml header.
	result = false;
	if (recipe_text != NULL) 
	{
		if (strncasecmp("<?xml", recipe_text, strlen("<?xml")) == 0) result = true;
	}
}

/* ------------------------------------------------------------------------------------------------------
this event is fired up when a recipe is received by unison via GEM
------------------------------------------------------------------------------------------------------ */
void CuserEvxaInterface::RecipeDecode(const char *recipe_text)
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::RecipeDecode()" << CUtil::CLog::endl;

    	// reset recipe variables before starting parsing.
    	resetRecipeVars();
 
    	// parse the xml file.  parseXML opens up a file.
    	bool result = parseXML(recipe_text); 

    	// execute load based on reload strategy.
    	if (result) result = updateProgramLoad();

    	// If the program load failed then set the result to fail.
    	if (m_recipeParseResult == false) result = false;

    	// After program load, update parameters.
    	if ((result == true) && (m_skipProgramParam == false)) result = updateTestProgramData();
    
	// this line originally here but somehow calling this gives a false trigger to gem host that end-lot occurred.
	// not sure why, probably need to investigate this but low priority for now.
   	//sendRecipeResultStatus(result);  // parsing failed so just send the result back to cgem.
 }

/* ------------------------------------------------------------------------------------------------------
send S10F1 to host
------------------------------------------------------------------------------------------------------ */
bool CuserEvxaInterface::SendMessageToHost( bool bEvent, const std::string& id, const std::string& msg )
{
	if ( !m_args.s10f1() && m_ConfigArgs.S10F1.compare("true") ) return true;

	std::stringstream ss;
	ss << "EQ_RH_" << (bEvent? "EV":"ER") << "ID<" << id << ">: TP <" << PgmCtrl()->getProgramName() << "> " << msg;
	return PgmCtrl()? (PgmCtrl()->gemSendMsgToHost(ss.str()) == EVX_GEM_GOOD? true : false) : false;
}

/* ------------------------------------------------------------------------------------------------------
This function parses the xml string and finds the program to load. It stores the program and stdf 
information for use at a later time. recipe_text is the xml data.
------------------------------------------------------------------------------------------------------ */
bool CuserEvxaInterface::parseXML(const char*recipe_text)
{
  	m_Debug << "[DEBUG] Executing CuserEvxaInterface::parseXML()" << CUtil::CLog::endl;
	if (!PgmCtrl()) 
	{
		m_Log << "No ProgramControl at CuserEvxaInterface::parseXML." << CUtil::CLog::endl;
		return false;
    	}

	bool result = true;
	if (!recipe_text)
	{
		m_Log << "ERROR: recipe_text is NULL!!" << CUtil::CLog::endl;
		SendMessageToHost(false, "007", "recipe empty"); 
		return false;
	}

	if (!strlen(recipe_text))
	{
		m_Log << "ERROR: recipe_text length is 0!!" << CUtil::CLog::endl;
		SendMessageToHost(false, "007", "recipe empty"); 
		return false;
	}

	m_recipeParse = true;
	PgmCtrl()->clearStatus();
	PgmCtrl()->notifyRecipeStatus(EVX_RECIPE_PARSE_BEGIN);

	// clear all parameters that will come from xml
	result = clearAllParams();

	// dump the recipe content into a temp file. the XML parse library wants a text file instead of a string so...
	std::string xmlFileName("/tmp/loadxml.xml");
	FILE *fptr = fopen(xmlFileName.c_str(), "w");
	if (fptr) 
	{
		fprintf(fptr, "%s", recipe_text);
		fclose(fptr);
	}
  
	// now parse the file.
	XML_Node *rootNode = new XML_Node (xmlFileName.c_str());
	if (rootNode) 
	{
		std::string ptag = rootNode->fetchTag();
		m_Debug << "[DEBUG] rootNode tag: " << ptag << CUtil::CLog::endl;

		// Add else-if statements for other rootNodes that need parsing then add a function to parse that rootNode.
		if (ptag.compare("testerRecipe") == 0) { result = parseTesterRecipe(rootNode); }
		else if (ptag.compare("STDFandContextUpdate") == 0){ result = parseSTDFandContextUpdate(rootNode); }
		else 
		{
		    	m_Log << "ERROR: unknown root tag '" << ptag << "' found in XTRF. not parsing." << CUtil::CLog::endl;
			SendMessageToHost(false, "008", "XTRF unknown tag");
		    	result = false;
		}
	}

	// delete the file
	unlink(xmlFileName.c_str());

	// delete XML_Node
	if (rootNode) 
	{
		delete rootNode;
		rootNode = NULL;
	}
	return result;
}

/* ------------------------------------------------------------------------------------------------------
if root tag <STDFandContextUpdate> is found, parse its contents here
------------------------------------------------------------------------------------------------------ */
bool CuserEvxaInterface::parseSTDFandContextUpdate(XML_Node *parseSTDFUpdate)
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::parseSTDFandContextUpdate()" << CUtil::CLog::endl;
	bool result = true;
   
	m_Debug << "[DEBUG] <STDFandContextUpdate> has " << parseSTDFUpdate->numChildren() << " child tags." << CUtil::CLog::endl;

	// read all the child tags from this root tag and process only the ones we want 
	for( int ii=0; ii < parseSTDFUpdate->numChildren(); ii++ ) 
	{
		XML_Node *childNode = parseSTDFUpdate->fetchChild(ii);
		if (childNode) 
		{
			std::string ptag = childNode->fetchTag();
			m_Debug << "[DEBUG] 	Child Tag: " << ptag << CUtil::CLog::endl;

			// process <STDF> child tag
			if(ptag.compare("STDF") == 0) 
			{
				result = parseSTDF(childNode);
				if (!result) break; 
			}
			// we just ignore unknown child tags
			else 
			{
				m_Debug << "[DEBUG] 	unknown child tag " << ptag << " found." << CUtil::CLog::endl;
				SendMessageToHost(false, "008", "XTRF unknown tag");
			}
		}
		// we ignore empty child tags
		else m_Debug << "[DEBUG]	empty child tag found. " << CUtil::CLog::endl;
	}
	return result;
}

/* ------------------------------------------------------------------------------------------------------
if root tag <testerRecipe> is found, parse its contents here
------------------------------------------------------------------------------------------------------ */
bool CuserEvxaInterface::parseTesterRecipe(XML_Node *testerRecipe)
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::parseTesterRecipe()" << CUtil::CLog::endl;
    	bool result = true;

	// read all the child tags from this root tag and process only the ones we want 
	m_Debug << "[DEBUG] <testerRecipe> has " << testerRecipe->numChildren() << " child tags." << CUtil::CLog::endl;
	for( int ii = 0; ii < testerRecipe->numChildren(); ii++ ) 
	{
		XML_Node *childNode = testerRecipe->fetchChild(ii);
		if (childNode) 
		{
			std::string ptag = childNode->fetchTag();
			m_Debug << "[DEBUG] 	Child Tag: " << ptag << CUtil::CLog::endl;

			// process <testPrgmIdentifier> child tag
			if (ptag.compare("testPrgmIdentifier") == 0) 
			{
				result = parseTestPrgmIdentifier(childNode);
				if (!result) break; 
			}
			// process <STDF> child tag
			else if(ptag.compare("STDF") == 0) 
			{
				result = parseSTDF(childNode);
				if (!result) break; 
			}
			// we just ignore unknown child tags
			else  
			{
				m_Debug << "[DEBUG] 	unknown child tag " << ptag << " found." << CUtil::CLog::endl;
				SendMessageToHost(false, "008", "XTRF unknown tag");
			}
		}
		// we ignore empty child tags
		else m_Debug << "[DEBUG]	empty child tag found. " << CUtil::CLog::endl;
	}
	return result;
}

/* ------------------------------------------------------------------------------------------------------
if root tag <testPrgmIdentifier> is found, parse its contents here
------------------------------------------------------------------------------------------------------ */
bool CuserEvxaInterface::parseTestPrgmIdentifier(XML_Node *testPrgmIdentifier)
{
 	m_Debug << "[DEBUG] Executing CuserEvxaInterface::parseTestPrgmIdentifier()" << CUtil::CLog::endl;
    	bool result = true;
    
	m_Debug << "[DEBUG] <testPrgmIdentifier> has " << testPrgmIdentifier->numChildren() << " child tags." << CUtil::CLog::endl;
	for (int ii = 0; ii < testPrgmIdentifier->numChildren(); ii++) 
	{
		XML_Node *childNode = testPrgmIdentifier->fetchChild(ii);
		if (childNode) 
		{
			std::string ptag = childNode->fetchTag();
			m_Debug << "[DEBUG] 	Child Tag: " << ptag << CUtil::CLog::endl;

			// process <testPrgm> child tag	
	    		if (ptag.compare("testPrgm") == 0) 
			{
				result = parseTestPrgm(childNode);
				if (!result) break; 
	    		}
	    		else 
			{
				m_Debug << "[DEBUG] 	unknown child tag " << ptag << " found." << CUtil::CLog::endl;
				SendMessageToHost(false, "008", "XTRF unknown tag");
			}
		}
		else m_Debug << "[DEBUG]	empty child tag found. " << CUtil::CLog::endl;
     	}    
    	return result;
}

/* ------------------------------------------------------------------------------------------------------
if child tag <testPrgm> is found, parse its contents here.
------------------------------------------------------------------------------------------------------ */
bool CuserEvxaInterface::parseTestPrgm(XML_Node *testPrgm)
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::parseTestPrgm()" << CUtil::CLog::endl;
    	bool result = true;

	m_Debug << "[DEBUG] <testPrgmIdentifier> has " << testPrgm->numChildren() << " child tags." << CUtil::CLog::endl;
    	for (int ii = 0; ii < testPrgm->numChildren(); ii++) 
	{
		XML_Node *childNode = testPrgm->fetchChild(ii);
		if (childNode) 
		{
	    		std::string ptag = childNode->fetchTag();
			m_Debug << "[DEBUG] 	Child Tag: " << ptag << CUtil::CLog::endl;

	    		// <testPrgmCopierLoaderScript>
	    		if (ptag.compare("testPrgmCopierLoaderScript") == 0) 
			{
				result = parseTestPrgmCopierLoaderScript(childNode);
				if (!result) break; 
	    		}
			// <testPrgmLoader>, this may contain the test program path+name
	    		else if (ptag.compare("testPrgmLoader") == 0) 
			{
				result = parseTestPrgmLoader(childNode);
				if (!result) break;
			}
	    		else 
			{
				m_Debug << "[DEBUG] 	unknown child tag " << ptag << " found." << CUtil::CLog::endl;
				SendMessageToHost(false, "008", "XTRF unknown tag");		
			}
		}
		else m_Debug << "[DEBUG]	empty child tag found. " << CUtil::CLog::endl;
     	}
      	return result;
}

/* ------------------------------------------------------------------------------------------------------
parse <testPrgmLoader> from XTRF
testPrgmURI attribute 
- 	holds the folder/program name and may be used to parse mir.spec_nam
  	and mir.spec_rev if necessary
reloadStrategy, downloadStrategy, and backToIdleStrategy attributes
------------------------------------------------------------------------------------------------------ */
bool CuserEvxaInterface::parseTestPrgmLoader(XML_Node *testPrgmLoader)
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::parseTestPrgmLoader()" << CUtil::CLog::endl;

	if (!PgmCtrl()) 
	{
		m_Log << "No ProgramControl at CuserEvxaInterface::parseTestPrgmLoader." << CUtil::CLog::endl;
		return false;
    	}

    	bool result = true;
    	m_Debug << "[DEBUG] <" << testPrgmLoader->fetchTag() << "> Found with " << testPrgmLoader->numAttr() << " attributes, " <<  testPrgmLoader->numVals() << " values." << CUtil::CLog::endl;

	// loop through <testPrgmloader> attributes and find program path/name, loading strategy
    	for (int ii=0; ii < testPrgmLoader->numAttr(); ii++) 
	{
		m_Debug << "[DEBUG] <" << testPrgmLoader->fetchTag() << ">> Attr " << testPrgmLoader->fetchAttr(ii) << ": " << testPrgmLoader->fetchVal(ii) << CUtil::CLog::endl;
		if (testPrgmLoader->fetchAttr(ii).compare("reloadStrategy") == 0){ m_TPArgs.ReloadStrategy = testPrgmLoader->fetchVal(ii); continue; }
		if (testPrgmLoader->fetchAttr(ii).compare("downloadStrategy") == 0){ m_TPArgs.DownloadStrategy = testPrgmLoader->fetchVal(ii); continue; }
		if (testPrgmLoader->fetchAttr(ii).compare("back2IdleStrategy") == 0){ m_TPArgs.BackToIdleStrategy = testPrgmLoader->fetchVal(ii); continue; }

		// testPrgmURI is expected to contain "<progfolder>/<programname.una>" and is stored in m_TPArgs.TPName
		// <progfolder> is stored in m_TPArgs.TPPath. it will be referenced in download strategy later		
		if (testPrgmLoader->fetchAttr(ii).compare("testPrgmURI") == 0) 
		{
			m_TPArgs.TPName = testPrgmLoader->fetchVal(ii);
			unsigned found = m_TPArgs.TPName.find_first_of("/");			
			m_TPArgs.TPPath = (m_TPArgs.TPName.find_first_of("/") != std::string::npos)? m_TPArgs.TPName.substr(0, found) : "";
			continue;		
		}

		// if you reach this point, then it's an unknown attribute. we're not going to stop the application ut we'll let host know
		m_Debug << "[DEBUG] 	unknown attribute found in <" << testPrgmLoader->fetchTag() << ">: " << testPrgmLoader->fetchAttr(ii) << CUtil::CLog::endl;
		SendMessageToHost(false, "009", "XTRF unknown attribute");		
    	}

    	return result;
}


/* ------------------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------------------ */
bool CuserEvxaInterface::parseTestPrgmCopierLoaderScript(XML_Node *testPrgmCopierLoaderScript)
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::parseTestPrgmCopierLoaderScript()" << CUtil::CLog::endl;

	if (!PgmCtrl()) 
	{
		m_Log << "No ProgramControl at CuserEvxaInterface::parseTestPrgmCopierLoaderScript." << CUtil::CLog::endl;
		return false;
    	}

     	bool result = true;
    	m_Debug << "[DEBUG] <" << testPrgmCopierLoaderScript->fetchTag() << "> Found with " << testPrgmCopierLoaderScript->numAttr() << " attributes, ";
	m_Debug <<  testPrgmCopierLoaderScript->numVals() << " values, " <<  testPrgmCopierLoaderScript->numChildren() << " children." << CUtil::CLog::endl;

	// read the attributes of <testPrgmCopierLoaderScript>
    	for (int ii = 0; ii < testPrgmCopierLoaderScript->numAttr(); ii++) 
	{
		m_Debug << "[DEBUG] <" << testPrgmCopierLoaderScript->fetchTag() << ">> Attr " << testPrgmCopierLoaderScript->fetchAttr(ii);
		m_Debug << ": " << testPrgmCopierLoaderScript->fetchVal(ii) << CUtil::CLog::endl;

		if (testPrgmCopierLoaderScript->fetchAttr(ii).compare("reloadStrategy") == 0){ m_TPArgs.ReloadStrategy = testPrgmCopierLoaderScript->fetchVal(ii); continue; }
		if (testPrgmCopierLoaderScript->fetchAttr(ii).compare("downloadStrategy") == 0){ m_TPArgs.DownloadStrategy = testPrgmCopierLoaderScript->fetchVal(ii); continue; }

		// if you reach this point, then it's an unknown attribute. we're not going to stop the application ut we'll let host know
		m_Debug << "[DEBUG] 	unknown attribute found in <" << testPrgmCopierLoaderScript->fetchTag() << ">: " << testPrgmCopierLoaderScript->fetchAttr(ii) << CUtil::CLog::endl;
		SendMessageToHost(false, "009", "XTRF unknown attribute");		
    	}

	// read the child tags of <testPrgmCopierLoaderScript>
	m_Debug << "[DEBUG] <" << testPrgmCopierLoaderScript->fetchTag() << "> has " << testPrgmCopierLoaderScript->numChildren() << " child tags." << CUtil::CLog::endl;
	for (int ii = 0; ii< testPrgmCopierLoaderScript->numChildren(); ii++) 
	{
		XML_Node *argumentParameter = testPrgmCopierLoaderScript->fetchChild(ii);
		if (argumentParameter)
		{
			m_Debug << "[DEBUG]	" << argumentParameter->fetchTag() << ": ";
			m_Debug << argumentParameter->numAttr() << " attributes, ";
			m_Debug << argumentParameter->numVals() << " values, ";
			m_Debug << argumentParameter->numChildren() << " children, ";
			m_Debug << argumentParameter->fetchAttr(0) << " ";
			m_Debug << argumentParameter->fetchVal(0) << " ";
			m_Debug << argumentParameter->fetchText() << CUtil::CLog::endl;

	    		std::string temp = argumentParameter->fetchVal(0);
	    		std::string result = argumentParameter->fetchText();
		    	if (temp.compare("TEST_PROGRAM_NAME") == 0)
	    		{ 
				m_TPArgs.TPName = result; 

				// check if program name as extension
				unsigned found = m_TPArgs.TPName.find_last_of(".");  

				// check if program name extension = .eva
				std::string szExt = m_TPArgs.TPName.substr(found+1);
	     			
				if ((szExt.compare("una") == 0) || (szExt.compare("UNA") == 0)){}
				else{ m_TPArgs.TPName += ".una"; }
	    
				m_Log << "TEST_PROGRAM_NAME: " << m_TPArgs.TPName << CUtil::CLog::endl; 
			}// allan added

			if (temp.compare("TEST_PROGRAM_PATH") == 0){ m_TPArgs.TPPath = result; m_Log << "TEST_PROGRAM_PATH: " << m_TPArgs.TPPath << CUtil::CLog::endl; }// allan added
			if (temp.compare("TEST_PROGRAM_FILE") == 0){ m_TPArgs.TPFile = result; m_Log << "TEST_PROGRAM_FILE: " << m_TPArgs.TPFile << CUtil::CLog::endl; }// allan added
			else if (temp.compare("RcpFileSupport") == 0) 	m_TPArgs.RcpFileSupport = result;
			else if (temp.compare("Flow") == 0) 		m_TPArgs.Flow = result;
			else if (temp.compare("Salestype") == 0) 	m_TPArgs.Salestype = result;
			else if (temp.compare("Temperature") == 0) 	m_TPArgs.Temperature =  result;
			else if (temp.compare("Product") == 0) 		m_TPArgs.Product = result;
			else if (temp.compare("Parallelism") == 0) 	m_TPArgs.Parallelism = result;
			else if (temp.compare("endLot") == 0) 		m_TPArgs.EndLotEnable = result;
			else if (temp.compare("endWafer") == 0) 	m_TPArgs.EndWaferEnable = result;
			else if (temp.compare("startLot") == 0) 	m_TPArgs.StartLotEnable = result;
			else if (temp.compare("startWafer") == 0) 	m_TPArgs.StartWaferEnable = result;
		}
	}
	return result;
}

/* ------------------------------------------------------------------------------------------------------
parse the <stdf> tag
------------------------------------------------------------------------------------------------------ */
bool CuserEvxaInterface::parseSTDF(XML_Node *stdf)
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::parseSTDF()" << CUtil::CLog::endl;

	if (!PgmCtrl()) 
	{
		m_Log << "No ProgramControl at CuserEvxaInterface::parseSTDF." << CUtil::CLog::endl;
		return false;
    	}

	// read the child tags of <stdf>
    	bool result = true;
	m_Debug << "[DEBUG] <" << stdf->fetchTag() << "> has " << stdf->numChildren() << " child tags." << CUtil::CLog::endl;
    	for (int ii = 0; ii < stdf->numChildren(); ii++) 
	{
		XML_Node *childNode = stdf->fetchChild(ii);
		if (childNode) 
		{
			std::string ptag = childNode->fetchTag();
			m_Debug << "[DEBUG]	child tag: " << ptag << CUtil::CLog::endl;

		    	// if found a record, let's parse it
		    	if (ptag.compare("STDFrecord") == 0) 
			{
				if (!parseSTDFRecord(childNode)) break;
		    	}
		    	else 
			{
				m_Debug << "[DEBUG] 	unknown child tag " << ptag << " found." << CUtil::CLog::endl;
				SendMessageToHost(false, "008", "XTRF unknown tag");	
		   	}	      
		}
		else m_Debug << "[DEBUG]	empty child tag found. " << CUtil::CLog::endl;
     	}

    	return result;
}


/*---------------------------------------------------------------------------------
parse <STDFRecord>
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::parseSTDFRecord(XML_Node *STDFRecord)
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::parseSTDFRecord()" << CUtil::CLog::endl;

   	bool result = true;

	// let's look for "recordName" as attribute. if unknown attribute is found, we ignore it but tell host about it
    	std::string rname("");
    	for (int ii = 0; ii < STDFRecord->numAttr(); ii++) 
	{
		if (STDFRecord->fetchAttr(ii).compare("recordName") == 0) rname = STDFRecord->fetchVal(ii); 
		else 
		{
			m_Debug << "[DEBUG] 	unknown attribute found in <" << STDFRecord->fetchTag() << ">: " << STDFRecord->fetchAttr(ii) << CUtil::CLog::endl;
			SendMessageToHost(false, "009", "XTRF unknown attribute");		
		}    	
	}
    	m_Debug << "Record Name: " << rname << CUtil::CLog::endl;

    	if (rname.compare("MIR") == 0){ result = parseMIR(STDFRecord); }
    	else if (rname.compare("SDR") == 0){ result = parseSDR(STDFRecord); }
    	else if (rname.compare("GDR") == 0){ result = parseGDR(STDFRecord); }
    	else 
	{ 	
		m_Log << "[ERROR] parseSTDFRecord unknown recordName: " << rname << CUtil::CLog::endl;
		SendMessageToHost(false, "010", "XTRF unknown attribute value");		
	}

    	return result;
}

/*---------------------------------------------------------------------------------
any STDF field to be set by evxa (e.g. XTRF not available, info from tester)
is done here. this is called after program is loaded
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::updateSTDFAfterProgLoad()
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::updateSTDFAfterProgLoad()" << CUtil::CLog::endl;

	if (!PgmCtrl()) 
	{
		m_Log << "No ProgramControl at CuserEvxaInterface::updateSTDFAfterProgLoad." << CUtil::CLog::endl;
		return false;
    	}

	// make sure to change exec_typ to "Unison". note that we arent writing exec_typ here yet. we are just updating its variable
	std::string SystemName = PgmCtrl()->getLotInformation(EVX_LotSystemName);
	if (SystemName.empty() || SystemName.compare("enVision") == 0) m_MIRArgs.ExecTyp = "Unison";

	// we send SDR.HAND_TYP to FAmodule so it can send it to STDF during onlotstart(). this ensures Unison doesn't overwrite it
	PgmCtrl()->faprocSet("Current Equipment: HAND_TYP", m_SDRArgs.HandTyp.value);
	PgmCtrl()->faprocSet("Current Equipment: HAND_TYP_REQ", m_SDRArgs.HandTyp.required);
	PgmCtrl()->faprocSet("Current Equipment: HAND_TYP_OVR", m_SDRArgs.HandTyp.override);
	std::string hand_typ;
	PgmCtrl()->faprocGet("Current Equipment: HAND_TYP", hand_typ);
	m_Debug << "[DEBUG] HAND_TYP sent to faproc: " << hand_typ << CUtil::CLog::endl;	

	// send GUI_NAM and GUI_REV to FAmodule
	PgmCtrl()->faprocSet("Current Equipment: GUI_NAM_VAL", m_GDR.gui_nam.value);
	PgmCtrl()->faprocSet("Current Equipment: GUI_NAM_VAL_REQ", m_GDR.gui_nam.required);
	PgmCtrl()->faprocSet("Current Equipment: GUI_NAM_VAL_OVR", m_GDR.gui_nam.override);
	m_Debug << "[DEBUG] GUI_NAM_VAL: " << m_GDR.gui_nam.value << " set in FAmodule." << CUtil::CLog::endl;

	PgmCtrl()->faprocSet("Current Equipment: GUI_REV_VAL", m_GDR.gui_rev.value);
	PgmCtrl()->faprocSet("Current Equipment: GUI_REV_VAL_REQ", m_GDR.gui_rev.required);
	PgmCtrl()->faprocSet("Current Equipment: GUI_REV_VAL_OVR", m_GDR.gui_rev.override);
	m_Debug << "[DEBUG] GUI_REV_VAL: " << m_GDR.gui_rev.value << " set in FAmodule." << CUtil::CLog::endl;

	// send GDR.AUTO_NAM to FAmodule
	PgmCtrl()->faprocSet("Current Equipment: AUTO_NAM_VAL", m_GDR.auto_nam.value);
	PgmCtrl()->faprocSet("Current Equipment: AUTO_NAM_VAL_REQ", m_GDR.auto_nam.required);
	PgmCtrl()->faprocSet("Current Equipment: AUTO_NAM_VAL_OVR", m_GDR.auto_nam.override);
	m_Debug << "[DEBUG] AUTO_NAM_VAL: " << m_GDR.auto_nam.value << " set in FAmodule." << CUtil::CLog::endl;
	
	// send GDR.AUTO_REV to FAmodule
	PgmCtrl()->faprocSet("Current Equipment: AUTO_VER_VAL", m_GDR.auto_ver.value);
	PgmCtrl()->faprocSet("Current Equipment: AUTO_VER_VAL_REQ", m_GDR.auto_ver.required);
	PgmCtrl()->faprocSet("Current Equipment: AUTO_VER_VAL_OVR", m_GDR.auto_ver.override);
	m_Debug << "[DEBUG] AUTO_VER_VAL: " << m_GDR.auto_ver.value << " set in FAmodule." << CUtil::CLog::endl;

	// send GDR.TRF-XTRF value to FAmodule
	// make the value always taken from lotid_flowid, unless TRF_XTRF field in XTRF is "strict" 
	if (m_GDR.trf_xtrf.value.empty())
	{
		m_Debug << "[DEBUG] Didn't find TRF-XTRF from XTRF. setting it instead to " << m_MIRArgs.LotId.value << "_" << m_MIRArgs.FlowId.value << " (lotid_flowid)" << CUtil::CLog::endl;		
		std::stringstream ssTrfXtrf; 
		ssTrfXtrf << m_MIRArgs.LotId.value << "_" << m_MIRArgs.FlowId.value;
		m_GDR.trf_xtrf.value = ssTrfXtrf.str();
	}
	PgmCtrl()->faprocSet("Current Equipment: TRF-XTRF_VAL", m_GDR.trf_xtrf.value);
	m_Debug << "[DEBUG] TRF-XTRF_VAL: " << m_GDR.trf_xtrf.value << " set in FAmodule." << CUtil::CLog::endl;
	
	// send GDR.AUTOMATION.SG_STATUS to FAmodule
	EVX_GemControlState gemCtrlState = PgmCtrl()->gemGetControlState();
	if (gemCtrlState == EVX_controlDisabled)	{ m_GDR.sg_status.value = "DISABLED"; 		m_Debug << "[DEBUG] GemCtrlState: DISABLED" << CUtil::CLog::endl; }
	if (gemCtrlState == EVX_equipOffline)		{ m_GDR.sg_status.value = "OFFLINE"; 		m_Debug << "[DEBUG] GemCtrlState: OFFLINE" << CUtil::CLog::endl; }
	if (gemCtrlState == EVX_attemptOnline)		{ m_GDR.sg_status.value = "ATTEMPONLINE"; 	m_Debug << "[DEBUG] GemCtrlState: ATTEMPONLINE" << CUtil::CLog::endl; }
	if (gemCtrlState == EVX_onlineLocal)		{ m_GDR.sg_status.value = "LOCAL"; 		m_Debug << "[DEBUG] GemCtrlState: LOCAL" << CUtil::CLog::endl; }
	if (gemCtrlState == EVX_onlineRemote)		{ m_GDR.sg_status.value = "REMOTE"; 		m_Debug << "[DEBUG] GemCtrlState: REMOTE" << CUtil::CLog::endl; }
	if (gemCtrlState == EVX_controlNoConnect)	{ m_GDR.sg_status.value = "NOCONNECT"; 		m_Debug << "[DEBUG] GemCtrlState: NOCONNECT" << CUtil::CLog::endl; }
	PgmCtrl()->faprocSet("Current Equipment: SG_STATUS_VAL", m_GDR.sg_status.value);
	m_Debug << "[DEBUG] SG_STATUS_VAL: " << m_GDR.sg_status.value << " set in FAmodule." << CUtil::CLog::endl;

	// hard code CGEM information
	m_GDR.sg_nam.value = "CGEM";
	PgmCtrl()->faprocSet("Current Equipment: SG_NAM_VAL", m_GDR.sg_nam.value);
	m_Debug << "[DEBUG] SG_NAM_VAL: " << m_GDR.sg_nam.value << " set in FAmodule." << CUtil::CLog::endl;
	m_GDR.sg_rev.value = "1.0";
	PgmCtrl()->faprocSet("Current Equipment: SG_REV_VAL", m_GDR.sg_rev.value);
	m_Debug << "[DEBUG] SG_REV_VAL: " << m_GDR.sg_rev.value << " set in FAmodule." << CUtil::CLog::endl;


	// hard code API_NAM and API_REV. sabine suggest to leave this empty 
	PgmCtrl()->faprocSet("Current Equipment: API_NAM_VAL", m_GDR.api_nam.value);
	PgmCtrl()->faprocSet("Current Equipment: API_NAM_VAL_REQ", m_GDR.api_nam.required);
	PgmCtrl()->faprocSet("Current Equipment: API_NAM_VAL_OVR", m_GDR.api_nam.override);
	m_Debug << "[DEBUG] API_NAM_VAL: " << m_GDR.api_nam.value << " set in FAmodule." << CUtil::CLog::endl;

	PgmCtrl()->faprocSet("Current Equipment: API_REV_VAL", m_GDR.api_rev.value);
	PgmCtrl()->faprocSet("Current Equipment: API_REV_VAL_REQ", m_GDR.api_rev.required);
	PgmCtrl()->faprocSet("Current Equipment: API_REV_VAL_OVR", m_GDR.api_rev.override);
	m_Debug << "[DEBUG] API_REV_VAL: " << m_GDR.api_rev.value << " set in FAmodule." << CUtil::CLog::endl;

	// set DRV_NAM and DRV_REV.
	PgmCtrl()->faprocSet("Current Equipment: DRV_NAM_VAL", m_GDR.drv_nam.value);
	PgmCtrl()->faprocSet("Current Equipment: DRV_NAM_VAL_REQ", m_GDR.drv_nam.required);
	PgmCtrl()->faprocSet("Current Equipment: DRV_NAM_VAL_OVR", m_GDR.drv_nam.override);
	m_Debug << "[DEBUG] DRV_NAM_VAL: " << m_GDR.drv_nam.value << " set in FAmodule." << CUtil::CLog::endl;

	PgmCtrl()->faprocSet("Current Equipment: DRV_REV_VAL", m_GDR.drv_rev.value);
	PgmCtrl()->faprocSet("Current Equipment: DRV_REV_VAL_REQ", m_GDR.drv_rev.required);
	PgmCtrl()->faprocSet("Current Equipment: DRV_REV_VAL_OVR", m_GDR.drv_rev.override);
	m_Debug << "[DEBUG] DRV_REV_VAL: " << m_GDR.drv_rev.value << " set in FAmodule." << CUtil::CLog::endl;

	// send STDF_FRM to FAmodule
	PgmCtrl()->faprocSet("Current Equipment: STDF_FRM_VAL", m_GDR.stdf_frm.value);
	PgmCtrl()->faprocSet("Current Equipment: STDF_FRM_VAL_REQ", m_GDR.stdf_frm.required);
	PgmCtrl()->faprocSet("Current Equipment: STDF_FRM_VAL_OVR", m_GDR.stdf_frm.override);
	m_Debug << "[DEBUG] STDF_FRM_VAL: " << m_GDR.stdf_frm.value << " set in FAmodule." << CUtil::CLog::endl;

	// assuming SERL_NUM is to be set as hostname, we do it here. note that this is temporary as we don't know yet what to put here
	if (m_MIRArgs.SerlNum.empty())
	{
		m_MIRArgs.SerlNum = PgmCtrl()->getLotInformation(EVX_LotTcName);
		PgmCtrl()->setLotInformation(EVX_LotTesterSerNum, m_MIRArgs.SerlNum.c_str());
		m_Debug << "MIR.SerlNum is set to " << PgmCtrl()->getLotInformation(EVX_LotTesterSerNum) << " (copied from hostname) because it is empty." << CUtil::CLog::endl;
	}

	// now let's send custom GDR's to CURI
	PgmCtrl()->faprocSet("Current Equipment: GDR_CUSTOM_CNT", m_GDR.customs.size());
	for (unsigned int i = 0; i < m_GDR.customs.size(); i++)
	{
		// send size of each custom GDR sets
		std::stringstream ss;
		ss << "Current Equipment: GDR_CUSTOM" << i << "_CNT";
		PgmCtrl()->faprocSet(ss.str(), m_GDR.customs[i].fields.size());

		for (unsigned int j = 0; j < m_GDR.customs[i].fields.size(); j++)
		{
			std::stringstream ss;
			ss << "Current Equipment: GDR_CUSTOM" << i << "_VAL" << j;
			PgmCtrl()->faprocSet(ss.str(), m_GDR.customs[i].fields[j].value);
		}
		
	}

	// send sublotid from XTRF to FAmodule, if any
	PgmCtrl()->faprocSet("Current Equipment: SUBLOTID_VAL", m_MIRArgs.SblotId.value);
	PgmCtrl()->faprocSet("Current Equipment: SUBLOTID_REQ", m_MIRArgs.SblotId.required);
	PgmCtrl()->faprocSet("Current Equipment: SUBLOTID_OVR", m_MIRArgs.SblotId.override);
	m_Debug << "[DEBUG] SUBLOTID_VAL: " << m_MIRArgs.SblotId.value << " set in FAmodule." << CUtil::CLog::endl;

	return true;
}

/*---------------------------------------------------------------------------------
using wsConfig command, try to get unison version
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::getUnisonVersion()
{
	// call command and store output to temp file
	system("wsConfig -d -v LTXC |grep -i core > /tmp/unison.ver" );

	// read temp file and dump to string variable
	std::ifstream f;
	f.open("/tmp/unison.ver");
	std::stringstream buf;
	buf << f.rdbuf();	
	f.close();
	std::string s(buf.str());
	
	// check if this line contain unison version
	std::string::size_type c = s.find("LTXC:unison");

	// if we found it
	if (c != std::string::npos)
	{
		// set l to now contain the unison version		
		std::string unisonVer = s.substr(s.find("LTXC:") + 5);		

		// if there's EOL on l, remove it
		//l.erase(std::remove(l.begin(), l.end(), '\n'), l.end());

		for (unsigned int i = 0; i < unisonVer.size(); i++)
		{
			if (unisonVer[i] == '\n') unisonVer[i] = '\0';
			if (unisonVer[i] == '\0') unisonVer[i] = '\0';
		}

		std::cout << "UNISON VERSION: " << unisonVer << "-- " << std::endl;

		PgmCtrl()->faprocSet("Current Equipment: UNISON_VERSION_WSCONFIG", unisonVer);
		std::string out;
		PgmCtrl()->faprocGet("Current Equipment: UNISON_VERSION_WSCONFIG", out);
		if (debug()) std::cout << "[DEBUG] UNISON VERSION (wsConfig): '" << out << "' set in FAmodule." << std::endl;
	}

	return true;
/*
	

	std::string unisonVer = "Unison_Version_Null";
	while (s.size())
	{
		if (s.find('u') == std::string::npos) std::cout << "found a EOF" << std::endl;
		else std::cout << "found EOL: '" << s.find('u') << "'" << std::endl;

		// get the line
		std::string l = s.substr(0, s.find('\n'));
		//std::string l = s.substr(0, std::string::npos);

		std::cout << l;

		// remove this line from the full string
		s = s.substr(l.size());

		// check if this line contain unison version
		std::string::size_type c = l.find("LTXC:unison-U1709-core");
		// if we found it
		if (c != std::string::npos)
		{
			// set l to now contain the unison version		
			unisonVer = l.substr(s.find("LTXC:"));		

			// if there's EOL on l, remove it
			//l.erase(std::remove(l.begin(), l.end(), '\n'), l.end());

			std::cout << "UNISON VERSION: " << l << std::endl;
			break;
		}

	}
*/

	return true;
}

/*---------------------------------------------------------------------------------
parse GDR
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::parseGDR(XML_Node *GDRRecord)
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::parseGDR()" << CUtil::CLog::endl;

	if (!PgmCtrl()) 
	{
		m_Log << "No ProgramControl at CuserEvxaInterface::parseGDR." << CUtil::CLog::endl;
		return false;
    	}

     	bool result = true;

	// get custom name of this GDR
    	std::string customName("");
    	for (int ii = 0; ii < GDRRecord->numAttr(); ii++)
	{ 
		GDRRecord->fetchAttr(ii).compare("customName") == 0? customName = GDRRecord->fetchVal(ii) : customName = ""; 
	}

	// start reading fields from XTRF 
    	XML_Node *STDFfields = GDRRecord->fetchChild("STDFfields");
	
	// loop through each <STDFfield> found in <STDFfields> of <STDFrecord recordName="GDR">
     	for(int jj = 0; jj < STDFfields->numChildren(); jj++) 
	{
		XML_Node *STDFfield = STDFfields->fetchChild(jj);
		if (STDFfield)  
		{
			// get the value that this field contains
			std::string result = STDFfield->fetchText();

			std::string comment, fieldname, required, override;
			// loop through attributes of each <STDFfield> and get their expected values
			for (int ii=0; ii<STDFfield->numAttr(); ii++)
			{
				if (STDFfield->fetchAttr(ii).compare("comment") == 0){ comment = STDFfield->fetchVal(ii); }
				if (STDFfield->fetchAttr(ii).compare("fieldName") == 0){ fieldname = STDFfield->fetchVal(ii); }
				if (STDFfield->fetchAttr(ii).compare("required") == 0){ required = STDFfield->fetchVal(ii); }
				if (STDFfield->fetchAttr(ii).compare("override") == 0){ override = STDFfield->fetchVal(ii); }
			}

			// we expect only GEN_DATA fields so...
			if (fieldname.compare("GEN_DATA") == 0)
			{
				if ( (customName.compare("MIR_ADD") == 0) || (customName.compare("AUTOMATION") == 0) || (customName.compare("REF_DIE") == 0) )
				{
					if (comment.compare("GUI_NAM_VAL") == 0){ m_GDR.gui_nam.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] GUI_NAM " << STDFfield->fetchText() << CUtil::CLog::endl; }
					if (comment.compare("GUI_REV_VAL") == 0){ m_GDR.gui_rev.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] GUI_REV " << STDFfield->fetchText() << CUtil::CLog::endl; }
					if (comment.compare("TRF-XTRF_VAL") == 0){ m_GDR.trf_xtrf.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] TRF_XTRF " << STDFfield->fetchText() << CUtil::CLog::endl; }
					if (comment.compare("AUTO_NAM_VAL") == 0){ m_GDR.auto_nam.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] AUTO_NAM " << STDFfield->fetchText() << CUtil::CLog::endl; }
					if (comment.compare("AUTO_VER_VAL") == 0){ m_GDR.auto_ver.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] AUTO_VER " << STDFfield->fetchText() << CUtil::CLog::endl; }
					if (comment.compare("STDF_FRM_VAL") == 0){ m_GDR.stdf_frm.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] STD_FRM " << STDFfield->fetchText() << CUtil::CLog::endl; }
					if (comment.compare("API_NAM_VAL") == 0){ m_GDR.api_nam.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] API_NAM " << STDFfield->fetchText() << CUtil::CLog::endl; }
					if (comment.compare("API_REV_VAL") == 0){ m_GDR.api_rev.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] API_REV " << STDFfield->fetchText() << CUtil::CLog::endl; }
					if (comment.compare("DRV_NAM_VAL") == 0){ m_GDR.drv_nam.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] DRV_NAM " << STDFfield->fetchText() << CUtil::CLog::endl; }
					if (comment.compare("DRV_REV_VAL") == 0){ m_GDR.drv_rev.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] DRV_REV " << STDFfield->fetchText() << CUtil::CLog::endl; }
				}
				// let's handle GEN_DATA belonging to custom GDR's. note that GDR's that aren't AUTOMATION, MIR_ADD, or REF_DIE are custom GDR's
				else
				{
					m_GDR.addCustom(customName, STDFfield->fetchText(), required, override);
					m_Debug << "[DEBUG] CUSTOM_GDR " << customName << ":" << STDFfield->fetchText() << CUtil::CLog::endl;				
				}
			}	
		}	
	}

	return result;
}

/*---------------------------------------------------------------------------------
parse MIR
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::parseMIR(XML_Node *MIRRecord)
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::parseMIR()" << CUtil::CLog::endl;
    	bool result = true;

	// start reading fields from XTRF 
     	XML_Node *STDFfields = MIRRecord->fetchChild("STDFfields");

 	// loop through each <STDFfield> found in <STDFfields> of <STDFrecord recordName="MIR">
     	for(int jj=0; jj < STDFfields->numChildren(); jj++) 
	{
		XML_Node *STDFfield = STDFfields->fetchChild(jj);
		if (STDFfield) 
		{
			// get the value that this field contains
			std::string result = STDFfield->fetchText();

			std::string comment, fieldname, required, override;
			// loop through attributes of each <STDFfield> and get their expected values
			for (int ii=0; ii<STDFfield->numAttr(); ii++)
			{
				if (STDFfield->fetchAttr(ii).compare("comment") == 0){ comment = STDFfield->fetchVal(ii); }
				if (STDFfield->fetchAttr(ii).compare("fieldName") == 0){ fieldname = STDFfield->fetchVal(ii); }
				if (STDFfield->fetchAttr(ii).compare("required") == 0){ required = STDFfield->fetchVal(ii); }
				if (STDFfield->fetchAttr(ii).compare("override") == 0){ override = STDFfield->fetchVal(ii); }
			}
			
	    		if (fieldname.compare("LOT_ID") == 0){ m_MIRArgs.LotId.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] LOT_ID: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("CMOD_COD") == 0){ m_MIRArgs.CmodCod.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] CMOD_COD: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("FLOW_ID") == 0){ m_MIRArgs.FlowId.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] FLOW_ID: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("DSGN_REV") == 0){ m_MIRArgs.DsgnRev.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] DSGN_REV: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("DATE_COD") == 0){ m_MIRArgs.DateCod.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] DATE_COD: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("OPER_FRQ") == 0){ m_MIRArgs.OperFrq.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] OPER_FRQ: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("OPER_NAM") == 0){ m_MIRArgs.OperNam.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] OPER_NAM: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("NODE_NAM") == 0){ m_MIRArgs.NodeNam.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] NODE_NAM: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	   		else if (fieldname.compare("PART_TYP") == 0){ m_MIRArgs.PartTyp.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] PART_TYP: " << STDFfield->fetchText() << CUtil::CLog::endl; }
			else if (fieldname.compare("ENG_ID") == 0){ m_MIRArgs.EngId.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] ENG_ID: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("TST_TEMP") == 0){ m_MIRArgs.TestTmp.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] TST_TEMP: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("FACIL_ID") == 0){ m_MIRArgs.FacilId.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] FACIL_ID: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("FLOOR_ID") == 0){ m_MIRArgs.FloorId.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] FLOOR_ID: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("STAT_NUM") == 0){ m_MIRArgs.StatNum.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] STAT_NUM: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("PROC_ID") == 0){ m_MIRArgs.ProcId.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] PROC_ID: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("MODE_COD") == 0){ m_MIRArgs.ModCod.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] MODE_COD: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("FAMLY_ID") == 0){ m_MIRArgs.FamilyId.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] FAMLY_ID: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	   	 	else if (fieldname.compare("PKG_TYP") == 0){ m_MIRArgs.PkgTyp.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] PKG_TYP: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("SBLOT_ID") == 0){ m_MIRArgs.SblotId.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] SBLOT_ID: " << STDFfield->fetchText() << CUtil::CLog::endl; }
			else if (fieldname.compare("JOB_NAM") == 0){ m_MIRArgs.JobNam.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] JOB_NAM: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("SETUP_ID") == 0){ m_MIRArgs.SetupId.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] SETUP_ID: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("JOB_REV") == 0){ m_MIRArgs.JobRev.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] JOB_REV: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("AUX_FILE") == 0){ m_MIRArgs.AuxFile.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] AUX_FILE: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("RTST_COD") == 0){ m_MIRArgs.RtstCod.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] RTST_COD: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("TEST_COD") == 0){ m_MIRArgs.TestCod.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] TEST_COD: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("USER_TXT") == 0){ m_MIRArgs.UserText.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] USER_TXT: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("ROM_COD") == 0){ m_MIRArgs.RomCod.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] ROM_COD: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("SERL_NUM") == 0){ m_MIRArgs.SerlNum.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] SERL_NUM: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("SPEC_NAM") == 0){ m_MIRArgs.SpecNam.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] SPEC_NAM: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("SPEC_VER") == 0){ m_MIRArgs.SpecVer.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] SPEC_VER: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("TSTR_TYP") == 0){ m_MIRArgs.TstrTyp.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] TSTR_TYP: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("SUPR_NAM") == 0){ m_MIRArgs.SuprNam.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] SUPR_NAM: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else if (fieldname.compare("PROT_COD") == 0){ m_MIRArgs.ProtCod.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] PROT_COD: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else 
			{
				m_Log << "[ERROR] parseMIR unknown field: " << fieldname << CUtil::CLog::endl;
				SendMessageToHost(false, "009", "XTRF unknown attribute");		
			}
		}
    	}			    
    	return result;
}

/*---------------------------------------------------------------------------------
parse SDR
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::parseSDR(XML_Node *SDRRecord)
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::parseSDR()" << CUtil::CLog::endl;
    	bool result = true;

	// start reading fields from XTRF 
     	XML_Node *STDFfields = SDRRecord->fetchChild("STDFfields");

 	// loop through each <STDFfield> found in <STDFfields> of <STDFrecord recordName="SDR">
     	for(int jj=0; jj < STDFfields->numChildren(); jj++) 
	{
		XML_Node *STDFfield = STDFfields->fetchChild(jj);
		if (STDFfield) 
		{
			// get the value that this field contains
			std::string result = STDFfield->fetchText();

			std::string comment, fieldname, required, override;
			// loop through attributes of each <STDFfield> and get their expected values
			for (int ii=0; ii<STDFfield->numAttr(); ii++)
			{
				if (STDFfield->fetchAttr(ii).compare("comment") == 0){ comment = STDFfield->fetchVal(ii); }
				if (STDFfield->fetchAttr(ii).compare("fieldName") == 0){ fieldname = STDFfield->fetchVal(ii); }
				if (STDFfield->fetchAttr(ii).compare("required") == 0){ required = STDFfield->fetchVal(ii); }
				if (STDFfield->fetchAttr(ii).compare("override") == 0){ override = STDFfield->fetchVal(ii); }
			}

			if (fieldname.compare("HAND_TYP") == 0){ m_SDRArgs.HandTyp.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] HAND_TYP: " << STDFfield->fetchText() << CUtil::CLog::endl; }
			else if (fieldname.compare("CARD_ID") == 0){ m_SDRArgs.CardId.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] CARD_ID: " << STDFfield->fetchText() << CUtil::CLog::endl; }
			else if (fieldname.compare("LOAD_ID") == 0){ m_SDRArgs.LoadId.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] LOAD_ID: " << STDFfield->fetchText() << CUtil::CLog::endl; }
			else if (fieldname.compare("HAND_ID") == 0){ m_SDRArgs.PHId.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] HAND_ID: " << STDFfield->fetchText() << CUtil::CLog::endl; }	
			else if (fieldname.compare("DIB_TYP") == 0){ m_SDRArgs.DibTyp.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] DIB_TYP: " << STDFfield->fetchText() << CUtil::CLog::endl; }
			else if (fieldname.compare("CABL_ID") == 0){ m_SDRArgs.CableId.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] CABL_ID: " << STDFfield->fetchText() << CUtil::CLog::endl; }
			else if (fieldname.compare("CONT_TYP") == 0){ m_SDRArgs.ContTyp.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] CONT_TYP: " << STDFfield->fetchText() << CUtil::CLog::endl; }
			else if (fieldname.compare("LOAD_TYP") == 0){ m_SDRArgs.LoadTyp.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] LOAD_TYP: " << STDFfield->fetchText() << CUtil::CLog::endl; }
			else if (fieldname.compare("CONT_ID") == 0){ m_SDRArgs.ContId.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] CONT_ID: " << STDFfield->fetchText() << CUtil::CLog::endl; }
			else if (fieldname.compare("LASR_TYP") == 0){ m_SDRArgs.LaserTyp.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] LASR_TYP: " << STDFfield->fetchText() << CUtil::CLog::endl; }
			else if (fieldname.compare("LASR_ID") == 0){ m_SDRArgs.LaserId.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] LASR_ID: " << STDFfield->fetchText() << CUtil::CLog::endl; }
			else if (fieldname.compare("EXTR_TYP") == 0){ m_SDRArgs.ExtrTyp.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] EXTR_TYP: " << STDFfield->fetchText() << CUtil::CLog::endl; }
			else if (fieldname.compare("EXTR_ID") == 0){ m_SDRArgs.ExtrId.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] EXTR_ID: " << STDFfield->fetchText() << CUtil::CLog::endl; }
			else if (fieldname.compare("DIB_ID") == 0){ m_SDRArgs.DibId.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] DIB_ID: " << STDFfield->fetchText() << CUtil::CLog::endl; }
			else if (fieldname.compare("CARD_TYP") == 0){ m_SDRArgs.CardTyp.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] CARD_TYP: " << STDFfield->fetchText() << CUtil::CLog::endl; }
			else if (fieldname.compare("CABL_TYP") == 0){ m_SDRArgs.CableTyp.set( STDFfield->fetchText(), required, override); m_Debug << "[DEBUG] CABL_TYP: " << STDFfield->fetchText() << CUtil::CLog::endl; }
	    		else 
			{
				m_Log << "[ERROR] parseSDR unknown field: " << fieldname << CUtil::CLog::endl;
				SendMessageToHost(false, "009", "XTRF unknown attribute");		
			}
		}
    	}			    
    	return result;
}

/*---------------------------------------------------------------------------------
if field value exists, required = strict, and override is blank, 
we force this field into STDF
if field value exists, required = strict, and override is "onEmpty", 
we only set this field into STDF if current value in STDF is empty
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::setLotInformation(const EVX_LOTINFO_TYPE type, param& field, const char* label)
{
	if (!PgmCtrl()) 
	{
		m_Log << "No ProgramControl at CuserEvxaInterface::setLotInformation." << CUtil::CLog::endl;
		return false;
    	}
	EVXAStatus status = EVXA::OK;

   	if (!field.empty()) 
	{
		//if (!field.required.compare("strict"))
		{
			// if override != onEmpty, let's force the value from XTRF to be set here
			if (field.override.compare("onEmpty")) { status = PgmCtrl()->setLotInformation(type, field.c_str()); }

			// if override == onEmpty, we only put XTRF value if it's empty now
			else
			{ 
				std::string str = PgmCtrl()->getLotInformation(type); 
				m_Debug << "onEmpty " << label << ": " << PgmCtrl()->getLotInformation(type) << CUtil::CLog::endl;
				if(str.empty()) status = PgmCtrl()->setLotInformation(type, field.c_str()); 
			} 
		}
		if (status != EVXA::OK)
		{
			m_Log << "[ERROR] sending " << label << " to Unison." << CUtil::CLog::endl;
			return false;
		}
		m_Log << "Query " << label << ": " << PgmCtrl()->getLotInformation(type) << CUtil::CLog::endl;
   	}
	// if field is empty from XTRF, let's leave this field with whatever its current value is
	else
	{ 
		m_Log << "Field is empty, will not set " << label << " to Unison." << CUtil::CLog::endl; 
	}
	return true;
}

/*---------------------------------------------------------------------------------
send MIR fields to STDF
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::sendMIRParams()
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::sendMIRParams()" << CUtil::CLog::endl;

	if (!PgmCtrl()) 
	{
		m_Log << "No ProgramControl at CuserEvxaInterface::sendMIRParams." << CUtil::CLog::endl;
		return false;
    	}

	// handle special scenario here where by default we take specnam/var from TestPrgmURI. but later on we update it with value and settings from XTRF
	// note that testPrgmURI is expected to contain "<progfolder>/<programname.una>" and <progfolder> is stored in m_TPArgs.TPPath
	if (!m_TPArgs.TPPath.empty())
	{
		// TPPath might have subfolders. we're only interested in main folder, so we get rid of subs
		std::string toSpec( m_TPArgs.TPPath );
		std::size_t p = toSpec.find_first_of("/");
		if (p != std::string::npos) toSpec = toSpec.substr(0, p);

		p = toSpec.find_last_of("_");
		if (p != std::string::npos)
		{
			// by default, we set specnam/ver from testPrgmURI values directly to Lot Information
			// if specnam/ver is set by XTRF, let parseMIR() and sendMIRParams() overwrite this if required
			PgmCtrl()->setLotInformation(EVX_LotTestSpecName, m_TPArgs.TPPath.substr(0, p).c_str());
			PgmCtrl()->setLotInformation(EVX_LotTestSpecRev, m_TPArgs.TPPath.substr(p + 1, std::string::npos).c_str());

			m_Debug << "MIR.SPEC_NAM from testPrgmURI (default): " << PgmCtrl()->getLotInformation(EVX_LotTestSpecName) << CUtil::CLog::endl;
			m_Debug << "MIR.SPEC_VER from testPrgmURI (default): " << PgmCtrl()->getLotInformation(EVX_LotTestSpecRev) << CUtil::CLog::endl;
		}
	}

	// now let's send all XTRF values to Lot Information
 
	if ( !setLotInformation(EVX_LotLotID, 		m_MIRArgs.LotId, 	"MIR.LotLotID") ) return false;
	if ( !setLotInformation(EVX_LotCommandMode, 	m_MIRArgs.CmodCod, 	"MIR.LotCommandMode") ) return false;
	if ( !setLotInformation(EVX_LotActiveFlowName, 	m_MIRArgs.FlowId, 	"MIR.LotActiveFlowName") ) return false;
	if ( !setLotInformation(EVX_LotDesignRev, 	m_MIRArgs.DsgnRev, 	"MIR.LotDesignRev") ) return false;
	if ( !setLotInformation(EVX_LotDateCode, 	m_MIRArgs.DateCod, 	"MIR.LotDateCode") ) return false;
	if ( !setLotInformation(EVX_LotOperFreq, 	m_MIRArgs.OperFrq, 	"MIR.LotOperFreq") ) return false;
	if ( !setLotInformation(EVX_LotOperator, 	m_MIRArgs.OperNam, 	"MIR.LotOperator") ) return false;
	if ( !setLotInformation(EVX_LotTcName, 		m_MIRArgs.NodeNam, 	"MIR.LotTcName") ) return false;
	if ( !setLotInformation(EVX_LotDevice, 		m_MIRArgs.PartTyp, 	"MIR.LotDevice") ) return false;
	if ( !setLotInformation(EVX_LotEngrLotId, 	m_MIRArgs.EngId, 	"MIR.LotEngrLotId") ) return false;
	if ( !setLotInformation(EVX_LotTestTemp, 	m_MIRArgs.TestTmp, 	"MIR.LotTestTemp") ) return false;
	if ( !setLotInformation(EVX_LotTestFacility, 	m_MIRArgs.FacilId, 	"MIR.LotTestFacility") ) return false;
	if ( !setLotInformation(EVX_LotTestFloor, 	m_MIRArgs.FloorId, 	"MIR.LotTestFloor") ) return false;
	if ( !setLotInformation(EVX_LotHead, 		m_MIRArgs.StatNum, 	"MIR.LotHead") ) return false;
	if ( !setLotInformation(EVX_LotFabricationID, 	m_MIRArgs.ProcId, 	"MIR.LotFabricationID") ) return false;
	if ( !setLotInformation(EVX_LotTestMode, 	m_MIRArgs.ModCod, 	"MIR.LotTestMode") ) return false;
	if ( !setLotInformation(EVX_LotProductID, 	m_MIRArgs.FamilyId, 	"MIR.LotProductID") ) return false;
	if ( !setLotInformation(EVX_LotPackage, 	m_MIRArgs.PkgTyp, 	"MIR.LotPackage") ) return false;
	if ( !setLotInformation(EVX_LotSublotID, 	m_MIRArgs.SblotId, 	"MIR.LotSublotID") ) return false;
	if ( !setLotInformation(EVX_LotTestSetup, 	m_MIRArgs.SetupId, 	"MIR.LotTestSetup") ) return false;
	if ( !setLotInformation(EVX_LotFileNameRev, 	m_MIRArgs.JobRev, 	"MIR.LotFileNameRev") ) return false;
	if ( !setLotInformation(EVX_LotAuxDataFile, 	m_MIRArgs.AuxFile, 	"MIR.LotAuxDataFile") ) return false;
	if ( !setLotInformation(EVX_LotTestPhase, 	m_MIRArgs.TestCod, 	"MIR.LotTestPhase") ) return false;
	if ( !setLotInformation(EVX_LotUserText, 	m_MIRArgs.UserText, 	"MIR.LotUserText") ) return false;
	if ( !setLotInformation(EVX_LotRomCode, 	m_MIRArgs.RomCod, 	"MIR.LotRomCode") ) return false;
	if ( !setLotInformation(EVX_LotTesterSerNum, 	m_MIRArgs.SerlNum, 	"MIR.LotTesterSerNum") ) return false;
	if ( !setLotInformation(EVX_LotTesterType, 	m_MIRArgs.TstrTyp, 	"MIR.LotTesterType") ) return false;
	if ( !setLotInformation(EVX_LotSupervisor, 	m_MIRArgs.SuprNam, 	"MIR.LotSupervisor") ) return false;
	if ( !setLotInformation(EVX_LotSystemName, 	m_MIRArgs.ExecTyp, 	"MIR.LotSystemName") ) return false;
	if ( !setLotInformation(EVX_LotTargetName, 	m_MIRArgs.ExecVer, 	"MIR.LotTargetName") ) return false;
	if ( !setLotInformation(EVX_LotTestSpecName, 	m_MIRArgs.SpecNam, 	"MIR.LotTestSpecName") ) return false;
	if ( !setLotInformation(EVX_LotTestSpecRev, 	m_MIRArgs.SpecVer, 	"MIR.LotTestSpecRev") ) return false;
	if ( !setLotInformation(EVX_LotProtectionCode, 	m_MIRArgs.ProtCod, 	"MIR.LotProtectionCode") ) return false;	  
    	return true;
}

/*---------------------------------------------------------------------------------
send MIR fields to STDF
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::sendSDRParams()
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::sendSDRParams()" << CUtil::CLog::endl;	
 
	if ( !setLotInformation(EVX_LotHandlerType, 	m_SDRArgs.HandTyp, 	"SDR.LotHandlerType") ) return false;
	if ( !setLotInformation(EVX_LotCardId, 		m_SDRArgs.CardId, 	"SDR.LotCardId") ) return false;
	if ( !setLotInformation(EVX_LotLoadBrdId, 	m_SDRArgs.LoadId, 	"SDR.LotLoadBrdId") ) return false;
	if ( !setLotInformation(EVX_LotProberHandlerID, m_SDRArgs.PHId, 	"SDR.LotProberHandlerID") ) return false;
	if ( !setLotInformation(EVX_LotDIBType, 	m_SDRArgs.DibTyp, 	"SDR.LotDIBType") ) return false;
	if ( !setLotInformation(EVX_LotIfCableId, 	m_SDRArgs.CableId, 	"SDR.LotIfCableId") ) return false;
	if ( !setLotInformation(EVX_LotContactorType, 	m_SDRArgs.ContTyp, 	"SDR.LotContactorType") ) return false;
	if ( !setLotInformation(EVX_LotLoadBrdType, 	m_SDRArgs.LoadTyp, 	"SDR.LotLoadBrdType") ) return false;
	if ( !setLotInformation(EVX_LotContactorId, 	m_SDRArgs.ContId, 	"SDR.LotContactorId") ) return false;
	if ( !setLotInformation(EVX_LotLaserType, 	m_SDRArgs.LaserTyp, 	"SDR.LotLaserType") ) return false;
	if ( !setLotInformation(EVX_LotLaserId, 	m_SDRArgs.LaserId, 	"SDR.LotLaserId") ) return false;
	if ( !setLotInformation(EVX_LotExtEquipType, 		m_SDRArgs.ExtrTyp, 	"SDR.LotExtEquipType") ) return false;
	if ( !setLotInformation(EVX_LotExtEquipId, 		m_SDRArgs.ExtrId, 	"SDR.LotExtEquipId") ) return false;
	if ( !setLotInformation(EVX_LotActiveLoadBrdName, 	m_SDRArgs.DibId, 	"SDR.LotActiveLoadBrdName") ) return false;
	if ( !setLotInformation(EVX_LotCardType, 		m_SDRArgs.CardTyp, 	"SDR.LotCardType") ) return false;
	if ( !setLotInformation(EVX_LotIfCableType, 		m_SDRArgs.CableTyp, 	"SDR.LotIfCableType") ) return false;
    	return true;
}

/*---------------------------------------------------------------------------------

---------------------------------------------------------------------------------*/
void CuserEvxaInterface::sendRecipeResultStatus(bool result)
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::sendRecipeResultStatus()" << CUtil::CLog::endl;

	if (!PgmCtrl()) 
	{
		m_Log << "No ProgramControl at CuserEvxaInterface::sendRecipeResultStatus." << CUtil::CLog::endl;
		return;
    	}

     	if (m_recipeParse && !m_statusResultsHaveBeenSent ) 
	{
		m_recipeParse = false;

		PgmCtrl()->clearStatus();
		EVXAStatus status = EVXA::OK;

		// result = false;
		// If m_recipeParseStatus was set during parsing then use it 
		// because it was set earlier in the process.
		// Otherwise look at the result flag.
		if (m_recipeParseStatus != EVX_RECIPE_PARSE_BEGIN) 
		{
			m_Debug << "sendRecipeResultStatus  " << getRecipeParseStatusName(m_recipeParseStatus) << CUtil::CLog::endl;
		    	status = PgmCtrl()->notifyRecipeStatus(m_recipeParseStatus);
		}	
		else if (true == result) 
		{
		    	m_Debug << "sendRecipeResultStatus EVX_RECIPE_PARSE_COMPLETE" << CUtil::CLog::endl;
		    	status = PgmCtrl()->notifyRecipeStatus(EVX_RECIPE_PARSE_COMPLETE);
		}
		else 
		{
		    	m_Debug <<  "sendRecipeResulttatus EVX_RECIPE_PARSE_FAIL" << CUtil::CLog::endl;
		    	status = PgmCtrl()->notifyRecipeStatus(EVX_RECIPE_PARSE_FAIL);
		}

		if (status != EVXA::OK) 
		{
		    m_Debug <<  "Error notifyRecipeStatus: status: " << status << " " << PgmCtrl()->getStatusBuffer() << CUtil::CLog::endl;
		}
    	}

    	// Mark that results have been sent so we don't send them again until the next recipe parse.
    	m_statusResultsHaveBeenSent = true;	
}

//--------------------------------------------------------------------
bool CuserEvxaInterface::clearAllParams()
{
	m_TPArgs.clearParams();
	m_MIRArgs.clear();
	m_SDRArgs.clear();
	m_GDR.clear();
	return true;
}

//--------------------------------------------------------------------
/* This function takes the SDR parameters and sends them to 
   The Test Program.
*/

//--------------------------------------------------------------------
/* This function takes the TP parameters and sends them to 
   The Test Program.
*/

/*---------------------------------------------------------------------------------

---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::sendTPParams()
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::sendTPParams()" << CUtil::CLog::endl;

	if (!PgmCtrl()) 
	{
		m_Log << "No ProgramControl at CuserEvxaInterface::sendTPParams." << CUtil::CLog::endl;
		return false;
    	}

	EVXAStatus status = EVXA::OK;
	bool result = true;
	// Set the Active Flow to the Flow parameter.
	if (!m_TPArgs.Flow.empty()) 
	{
		if (status == EVXA::OK) status = PgmCtrl()->setActiveObject(EVX_ActiveFlow, m_TPArgs.Flow.c_str());
		if (status != EVXA::OK) 
		{
			m_Log << "[Error] sendTPParams set Flow: " << PgmCtrl()->getStatusBuffer() << CUtil::CLog::endl;
			result = false;
		}
	}
	// Set the Lot ID to the LotId paramter.
	if (!m_TPArgs.LotId.empty()) 
	{
		if (status == EVXA::OK) status = PgmCtrl()->setLotInformation(EVX_LotLotID, m_TPArgs.LotId.c_str());
		if (status != EVXA::OK) 
		{
			m_Log << "[Error] sendTPParams set LotID: " << PgmCtrl()->getStatusBuffer() << CUtil::CLog::endl;
			result = false;
		}
	}
	    
    	return result;
}

/*---------------------------------------------------------------------------------
event handler for start-of-lot
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::sendStartOfLot()
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::sendStartOfLot()" << CUtil::CLog::endl;
	if (!PgmCtrl()) 
	{
		m_Log << "No ProgramControl at CuserEvxaInterface::sendStartOfLot." << CUtil::CLog::endl;
		return false;
    	}

    	bool result = true;
    	if ((m_TPArgs.StartLotEnable.compare("true") == 0) || (m_TPArgs.StartLotEnable.compare("TRUE") == 0)) 
	{
		setupWaitForNotification(EVX_LOT_CHANGE, EVX_LOT_START);	    
	    	EVXAStatus status = PgmCtrl()->setStartOfLot();
	    	if (status == EVXA::OK) 
		{
			waitForNotification();
	    	}
	    	else 	
		{
			m_Log << "[Error] sendStartOfLot setStartOfLot failed: " << PgmCtrl()->getStatusBuffer() << CUtil::CLog::endl;
			result = false;
		}
    	}
      	return result;
}

/*---------------------------------------------------------------------------------

---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::sendEndOfLot()
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::sendEndOfLot()" << CUtil::CLog::endl;
	if (!PgmCtrl()) 
	{
		m_Log << "No ProgramControl at CuserEvxaInterface::sendEndOfLot." << CUtil::CLog::endl;
		return false;
    	}

    	bool result = true;
    	if ((m_TPArgs.EndLotEnable.compare("true") == 0) || (m_TPArgs.EndLotEnable.compare("TRUE") == 0)) 
	{
		setupWaitForNotification(EVX_LOT_CHANGE, EVX_LOT_END);	    
	    	EVXAStatus status = PgmCtrl()->setEndOfLot(true);  // true is to send final summary.
	    	if (status == EVXA::OK) 
		{
			waitForNotification();
	    	}
	    	else 
		{
			m_Log <<  "[Error] sendEndOfLot setEndOfLot failed: " << PgmCtrl()->getStatusBuffer() << CUtil::CLog::endl;
			result = false;
	    	}
	    	if (status == EVXA::OK) status = PgmCtrl()->setLotInformation(EVX_LotNextSerial, "1"); // reset the serial number to 1.
	    	if (status != EVXA::OK) 
		{
			m_Log << "Error sendEndOfLot setLotINformation failed: " << PgmCtrl()->getStatusBuffer() << CUtil::CLog::endl;
	    	}
    	}
    
    	return result;
}

/*---------------------------------------------------------------------------------

---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::sendStartOfWafer()
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::sendStartOfWafer()" << CUtil::CLog::endl;
	if (!PgmCtrl()) 
	{
		m_Log << "No ProgramControl at CuserEvxaInterface::sendStartOfWafer." << CUtil::CLog::endl;
		return false;
    	}

   	 bool result = true;
    	if ((m_TPArgs.StartWaferEnable.compare("true") == 0) || (m_TPArgs.StartWaferEnable.compare("TRUE") == 0)) 
	{
		setupWaitForNotification(EVX_WAFER_CHANGE, EVX_WAFER_START);	    
		EVXAStatus status = PgmCtrl()->setStartOfWafer();  
		if (status == EVXA::OK) 
		{
	    		waitForNotification();
		}
		else 
		{
	    		m_Log << "Error sendStartOfWafer setStartOfWafer failed: " << PgmCtrl()->getStatusBuffer() << CUtil::CLog::endl;
	    		result = false;
		}
    	}
    	return result;
}

/*---------------------------------------------------------------------------------

---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::sendEndOfWafer()
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::sendEndOfWafer()" << CUtil::CLog::endl;
	if (!PgmCtrl()) 
	{
		m_Log << "No ProgramControl at CuserEvxaInterface::sendEndOfWafer." << CUtil::CLog::endl;
		return false;
    	}

    	bool result = true;
    	if ((m_TPArgs.EndWaferEnable.compare("true") == 0) || (m_TPArgs.EndWaferEnable.compare("TRUE") == 0)) 
	{
		setupWaitForNotification(EVX_WAFER_CHANGE, EVX_WAFER_END);	    
		EVXAStatus status = PgmCtrl()->setEndOfWafer();  
		if (status == EVXA::OK) 
		{
	 		waitForNotification();
		}
		else 
		{
		    	m_Log << "[Error] sendEndOfWafer setEndOfWafer failed: " << PgmCtrl()->getStatusBuffer() <<CUtil::CLog::endl;
		    	result = false;
		}
	}
	return result;
}

/*---------------------------------------------------------------------------------

---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::updateProgramLoad()
{
   	m_Debug << "[DEBUG] Executing CuserEvxaInterface::updateProgramLoad() m_recipeParse: " << m_recipeParse << CUtil::CLog::endl;
	bool result = true; // set to false when something bad occurs.

    	// If there was a recipe to parse then do the work.
    	if (m_recipeParse == false)
	{
		m_Debug << "[DEBUG] No recipe to load." << CUtil::CLog::endl;
		return result;
	}

    	// check the download strategy and download the program accordingly
    	// check the reload strategy and load accordingly
    	if (m_TPArgs.TPName.empty() == false) 
	{
		m_Debug << "[DEBUG] Test Program to be loaded: " << m_TPArgs.TPName.c_str() << CUtil::CLog::endl;
		if (result == true) result = executeRecipeReload();
    	}
	else{ m_Log << "No test program name to load." << CUtil::CLog::endl;}

    	return result;
}

/*---------------------------------------------------------------------------------
This function updates the test program data.
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::updateTestProgramData()
{
	// If ABORT_LOAD occurred during this function then m_recipeParseResult would be false
	// and don't continue with the rest of the function.
	bool result = true;
	/*
	if (m_recipeParseResult == false) result = false;
	if (result == true) result = sendEndOfWafer();

	if (m_recipeParseResult == false) result = false;
	if (result == true) result = sendEndOfLot();

	if (m_recipeParseResult == false) result = false;
	if (result == true) result = sendStartOfLot();

	if (m_recipeParseResult == false) result = false;
	if (result == true) result = sendStartOfWafer();
	*/

	if (m_recipeParseResult == false) result = false;
	if (result == true) result = sendTPParams();

	if (m_recipeParseResult == false) result = false;
	if (result == true) result = sendMIRParams();

	if (m_recipeParseResult == false) result = false;
	if (result == true) result = sendSDRParams();

	if (m_recipeParseResult == false) result = false;
	return result;
}

//--------------------------------------------------------------------
/* This function checks the ReloadStrategy 
// If the requested program is different than what is currently loaded
// then load the requested program.
// Except Force is always load requested program.
// The DownloadStrategy is used within the specific load functions.
// Choices for DownloadStrategy are Force, Attempt, Never, Indifferent.
// Here's the truth table for downloading and loading the program:
DownloadStrategy |	 ReloadStrategy	     Expected behavior
-------------------------------------------------------------------------------------------------
Force		 |	 Dont care (any)     If requested TP is available remotely
                 |                              download & reload
		 |                           else 
	         |                              error
-----------------|--------------------------------------------------------------------------------
                 |       Force	             If requested TP is available remotely
		 |                              download & reload.
		 |                           else 
		 |                              if requested TP is available locally
		 |                                 reload
		 |                              else
		 |                                 error
Attempt	         |---------------------------------------------------------------------------------
	         |       Attempt	     If requested TP is available remotely
		 |                              download & reload.
		 |                           else 
		 |                              if requested TP is available locally
		 |                                 reload
		 |                              else
		 |                                  If requested TP is currently loaded
		 |                                     use currently loaded TP.
		 |                                  else
		 |                                     error
                 |---------------------------------------------------------------------------------
	         |       Never	             Same as ReloadStrategy = Attempt
                 |---------------------------------------------------------------------------------
	         |       Indifferent	     Same as ReloadStrategy = Attempt
-----------------|--------------------------------------------------------------------------------
	         |       Force	             If requested TP is available locally
		 |                              reload
		 |                           else 
		 |                              if requested TP is available remotely
	         |                                 download & reload
		 |                              else
		 |                                 error
Never            |--------------------------------------------------------------------------------
                 |       Attempt	     If requested TP is available locally
		 |                              reload
		 |                           else 
		 |                              if requested TP is available remotely
		 |                                 download & reload
		 |                              else
		 |                                 If requested TP is currently loaded
		 |                                    use currently loaded TP.
		 |                                 else
		 |                                    error
                 |--------------------------------------------------------------------------------
	         |       Never	             If requested TP is currently loaded
		 |                              use currently loaded TP.
		 |                           else 
		 |                              if requested TP is available locally
		 |                                 reload
		 |                              else
	         |                                 If requested TP is available remotely
		 |                                    download & reload
		 |                                 else
	         |                                    error
                 |--------------------------------------------------------------------------------
		 |       Indifferent	     Same as ReloadStrategy = Attempt
-----------------|--------------------------------------------------------------------------------
	         |       Force	             If requested TP is available remotely
		 |                              download & reload.
		 |                           else 
		 |                              if requested TP is available locally
		 |                                 reload
		 |                              else
		 |                                 error
                 |--------------------------------------------------------------------------------
Indifferent	 |       Attempt	     If requested TP is available remotely
		 |                              download & reload.
		 |                           else 
		 |                              if requested TP is available locally
		 |                                 reload
		 |                                  If requested TP is currently loaded
	         |                                    use currently loaded TP
		 |                                  else
		 |                                    error
                 |--------------------------------------------------------------------------------
	         |       Never	             Same as ReloadStrategy = Attempt
                 |--------------------------------------------------------------------------------
	         |       Indifferent	     Same as ReloadStrategy = Attempt
-----------------|-------------------------------------------------------------------------------- 
*/ 
bool CuserEvxaInterface::executeRecipeReload()
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::executeRecipeReload()" << CUtil::CLog::endl;
	if (!PgmCtrl()) 
	{
		m_Log << "No ProgramControl at CuserEvxaInterface::executeRecipeReload." << CUtil::CLog::endl;
		return false;
    	}

 	bool result = true;
    	
	// check if program is already loaded in unison
	bool pgmLoaded = PgmCtrl()->isProgramLoaded();    	
	if (pgmLoaded == true) 
	{
		std::string temp = PgmCtrl()->getProgramPath();
		m_Log << "[OK] A test program is already loaded with full path name: " << temp << CUtil::CLog::endl;
		m_TPArgs.CurrentProgName = temp;
		//unsigned found = temp.find_last_of("/");
		//if (found != std::string::npos) m_TPArgs.CurrentProgName = temp.substr(found+1);
		//else m_TPArgs.CurrentProgName = temp;
		//fprintf(stdout, "Test Program already loaded with Name: %s\n", m_TPArgs.CurrentProgName.c_str());
    	}

	std::string dload(m_TPArgs.DownloadStrategy);
	std::string rload(m_TPArgs.ReloadStrategy);
	m_Debug << "[DEBUG] download strategy: " << dload.c_str() << CUtil::CLog::endl;
	m_Debug << "[DEBUG] reload strategy: " << rload.c_str() << CUtil::CLog::endl;

	// download = force, reload = doesn't matter
	// look for TP in remote folder and download it, move to program folder, and unpack
	// unload any program loaded in tester and load this new one
	if((dload.compare("Force") == 0) || (dload.compare("force") == 0)){ result = forceDownloadAndLoad(); }

	// download = never
	else if ((dload.compare("Never") == 0) || (dload.compare("never") == 0)) 
	{
		// reload = force
		// if TP available in local folder, reload it. 
		// else, if TP available remotely, download it. unpack. unload current program if any. load new one
		if ((rload.compare("Force") == 0) || (rload.compare("force") == 0)) result = neverDownloadForceLoad();

		// reload = never
		else if ((rload.compare("Never") == 0) || (rload.compare("never") == 0)) result = neverDownloadNeverLoad();

		// reload = attempt		
		else  result = neverDownloadAttemptLoad();
	}
	// indifferent, attempt download are the same
	else 
	{  
		if ((rload.compare("Force") == 0) || (rload.compare("force") == 0)) result = attemptDownloadForceLoad();
		// attempt, indifferent, never reload are the same.
		else result = attemptDownloadAttemptLoad();
	}    
    	return result;    
}

/*---------------------------------------------------------------------------------
handle back to idle strategy
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::handleBackToIdleStrategy()
{
	// if backtoidle = retain
	// keep program loaded and TP files test folder untouched

	// backtoidle = unloadAndRetain
	// unload TP but keep TP files in and test folder untouched
	if((m_TPArgs.BackToIdleStrategy.compare("unloadAndRetain") == 0) || (m_TPArgs.BackToIdleStrategy.compare("UnloadAndRetain") == 0))
	{ 
		if (!unloadProgram(m_TPArgs.FullTPName, false)) return false;
		else return true;		
	}

	// backtoidle = unloadAndDelete
	// unload TP and delete TP files in test folder
	if((m_TPArgs.BackToIdleStrategy.compare("unloadAndDelete") == 0) || (m_TPArgs.BackToIdleStrategy.compare("UnloadAndDelete") == 0))
	{ 
		if (!unloadProgram(m_TPArgs.FullTPName, false)) return false;
				
		// SAFELY delete TP files in test folder. make sure this string is not empty before proceeding
		if (!m_ConfigArgs.ProgLocation.empty()) 
		{
		    std::stringstream rmCmd;
		    rmCmd << "/bin/rm -rf " << m_ConfigArgs.ProgLocation << "/*";
		    m_Debug << "clean localdir cmd: " << rmCmd.str().c_str() << CUtil::CLog::endl;
		    system(rmCmd.str().c_str());
		    m_Debug << "clean localdir done." << CUtil::CLog::endl;
		}
	}

	// backtoidle = unload
	// same as unloadAndRetain
	if((m_TPArgs.BackToIdleStrategy.compare("unload") == 0) || (m_TPArgs.BackToIdleStrategy.compare("Unload") == 0))
	{ 
		if (! unloadProgram(m_TPArgs.FullTPName, false)) return false;
		else return true;		
	}

	// backtoidle = indifferent
	// same behavior as unload
	if((m_TPArgs.BackToIdleStrategy.compare("indifferent") == 0) || (m_TPArgs.BackToIdleStrategy.compare("Indifferent") == 0))
	{ 
		if (! unloadProgram(m_TPArgs.FullTPName, false)) return false;
		else return true;		
	}

	return true;
//sendRecipeResultStatus(result);  // parsing failed so just send the result back to cgem.
}


/*---------------------------------------------------------------------------------
load program
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::loadProgram(const std::string &szProgFullPath)
{
 	m_Debug << "[DEBUG] Executing CuserEvxaInterface::loadProgram()" << CUtil::CLog::endl;
	if (!PgmCtrl()){ m_Log << "No ProgramControl at CuserEvxaInterface::loadProgram." << CUtil::CLog::endl; return false; }

	EVXAStatus status = EVXA::OK;
 
	// load our program from XTRF
	m_Debug << "[DEBUG] loading test program " <<  szProgFullPath << "..." << CUtil::CLog::endl;
	EVX_PROGRAM_STATE states[] = { EVX_PROGRAM_LOADED, EVX_PROGRAM_LOAD_FAILED, EVX_PROGRAM_UNLOADED, MAX_EVX_PROGRAM_STATE };
	setupProgramNotification(states);
	status = PgmCtrl()->load(szProgFullPath.c_str(), EVXA::NO_WAIT, EVXA::DISPLAY);

	if (EVXA::OK == status)
	{ 
		waitForNotification(); 
		m_Log << "[OK] test program " <<  szProgFullPath << " successfully loaded." << CUtil::CLog::endl;
	}
	else 
	{
	    m_Log << "[ERROR] Something went wrong in loading test program " << szProgFullPath.c_str() << ": " << PgmCtrl()->getStatusBuffer() << CUtil::CLog::endl;
	    return false;
	}
		
	return true;
}

/*---------------------------------------------------------------------------------
unload program
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::unloadProgram(const std::string &szProgFullPath, bool notify)
{
 	m_Debug << "[DEBUG] Executing CuserEvxaInterface::unloadProgram()" << CUtil::CLog::endl;

	EVXAStatus status = EVXA::OK;
     	if (!PgmCtrl()){ m_Debug << "[ERROR] Failed to access ProgramControl object." << CUtil::CLog::endl; return false; }
	
	// If there's a program loaded then unload it.
	if (!szProgFullPath.empty()) 
	{
		m_Debug << "[DEBUG] unloading test program " <<  szProgFullPath << "..." << CUtil::CLog::endl;
	    	EVX_PROGRAM_STATE states[] = { EVX_PROGRAM_UNLOADED, MAX_EVX_PROGRAM_STATE };
	    	if (notify) setupProgramNotification(states);
	    	status = PgmCtrl()->unload(EVXA::NO_WAIT);  
		m_Debug << "[DEBUG] Done unloading test program " <<  szProgFullPath << "..." << CUtil::CLog::endl;		
	    	if (EVXA::OK != status) 
		{
	    		m_Log << "[ERROR] Something went wrong in unloading test program: " << PgmCtrl()->getStatusBuffer() << CUtil::CLog::endl;
			return false;
	    	}
		m_Debug << "[DEBUG] Done unloading test program " <<  szProgFullPath << "..." << CUtil::CLog::endl;		
	    	if (notify) waitForNotification();
		m_Debug << "[DEBUG] Done unloading test program " <<  szProgFullPath << "..." << CUtil::CLog::endl;		
	    	if (!m_recipeParseResult){return m_recipeParseResult;}
		m_Debug << "[DEBUG] Done unloading test program " <<  szProgFullPath << "..." << CUtil::CLog::endl;		
	}	
	return true;
}

/*-----------------------------------------------------------------------------------------
check if file exist
-----------------------------------------------------------------------------------------*/
bool CuserEvxaInterface::isFileExist(const std::string& szFile)
{
	// check if the program is available remotely
	m_Debug << "[DEBUG] Checking if " << szFile << " exists..." << CUtil::CLog::endl;
     	if (access(szFile.c_str(), F_OK) > -1) { m_Debug << "[DEBUG] " << szFile << " exist." << CUtil::CLog::endl; return true; }
   	else { m_Log << "[WARNING] Cannot access " << szFile << ". It does not exist." << CUtil::CLog::endl; return false; }
} 

/*-----------------------------------------------------------------------------------------
check if new program we are trying to load is already loaded
-----------------------------------------------------------------------------------------*/
bool CuserEvxaInterface::isProgramToLoadAlreadyLoaded()
{
	// get the supposed fullpathname of TP we want to load
    	std::stringstream fullProgName;
	fullProgName << m_ConfigArgs.ProgLocation << "/" << m_TPArgs.TPName;
			
	// check if current program loaded (if any) is same as the program we are trying to load
	if (fullProgName.str().compare(m_TPArgs.CurrentProgName) == 0)
	{
		m_TPArgs.FullTPName = fullProgName.str();
		m_Debug << "[DEBUG] Test program <" << fullProgName.str() << "> is already loaded." << CUtil::CLog::endl;
		return true;
	}
	else
	{
		m_Log << "[WARNING] Test program to load is <" << fullProgName.str() << ">. But current Program loaded is <" << m_TPArgs.CurrentProgName << ">" << CUtil::CLog::endl;
		return false;
	}
}

/*---------------------------------------------------------------------------------
copy file from to
from - 	full path and filename with extension of file to be copied
to - 	folder path to paste the file
---------------------------------------------------------------------------------*/
void CuserEvxaInterface::copyFile(const std::string& from, const std::string& to)
{
	std::stringstream cpCmd; 
	cpCmd << "/bin/cp -rf " << from << " " << to << "/.";
	m_Debug << "[DEBUG] copying " <<  from << " to " << to << "..." << CUtil::CLog::endl;
	system(cpCmd.str().c_str());
	m_Debug << "[DEBUG] Done copying. " << CUtil::CLog::endl;	
}

/*---------------------------------------------------------------------------------
download program
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::downloadProgramFromServerToLocal()
{
  	m_Debug << "[DEBUG] Executing CuserEvxaInterface::downloadProgramFromServerToLocal()" << CUtil::CLog::endl;
 
	//get the full path-filename-extension of the TP file from remote folder
   	std::stringstream fullRemoteProgPath;
    	if (!m_TPArgs.TPPath.empty())
	{ 
		fullRemoteProgPath << m_ConfigArgs.RemoteLocation << "/" << m_TPArgs.TPPath << "." << m_ConfigArgs.PackageType; 
		m_Log << "[OK] Downloading " << fullRemoteProgPath.str() << "..." << CUtil::CLog::endl;
	}
	else { m_Log << "[ERROR] test program file name to download is invalid." << CUtil::CLog::endl; return false; }
 
	// check if TP file exist in remote folder. return false if not
	if (!isFileExist(fullRemoteProgPath.str())) return false;

	// copy TP file from remote folder to local folder. no checking
	copyFile(fullRemoteProgPath.str(), m_ConfigArgs.LocalLocation );

	// unpack TP file from local folder to test folder
	if (!unpackProgramFromLocalToTest()) return false;

    	return true;
}

/*---------------------------------------------------------------------------------
unpack TP file from local folder to test folder
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::unpackProgramFromLocalToTest()
{
  	m_Debug << "[DEBUG] Executing CuserEvxaInterface::unpackProgramFromLocalToTest()" << CUtil::CLog::endl;

	// get full path-filename-extension of TP file to local folder
	std::stringstream fullLocalProgPath;
	fullLocalProgPath << m_ConfigArgs.LocalLocation << "/" << m_TPArgs.TPPath << "." << m_ConfigArgs.PackageType; 

	// check if TP file exist in local folder
	if (!isFileExist(fullLocalProgPath.str())) return false;

	// chmod the TP file on local folder so we can unpack it
	std::stringstream chmodCmd;
	chmodCmd << "/bin/chmod +x " << fullLocalProgPath.str();
	m_Debug << "[DEBUG] chmod cmd: " << chmodCmd.str().c_str() << CUtil::CLog::endl;
	system(chmodCmd.str().c_str());
	m_Debug << "[DEBUG] chmod done." << CUtil::CLog::endl;
 
	// unpack to TP file from local folder into program folder
	std::stringstream tarCmd;
	tarCmd << "/bin/tar -C " << m_ConfigArgs.ProgLocation << " -xvf " << fullLocalProgPath.str();
	m_Debug << "[DEBUG] unpacking with cmd: " << tarCmd.str() << CUtil::CLog::endl;
	system(tarCmd.str().c_str());
	m_Debug << "[DEBUG] unpackdone. " << CUtil::CLog::endl;
 
	// check if TP program exists in program folder
    	std::stringstream fullProgName;
	fullProgName << m_ConfigArgs.ProgLocation << "/" << m_TPArgs.TPName;
	if (!isFileExist(fullProgName.str())) return false;

	// finally let's store the test program name to load
	m_TPArgs.FullTPName = fullProgName.str();
	return true;
}

/*---------------------------------------------------------------------------------
download strategy: force
reload strategy: don't care
- download program from server. error if failed
- unload existing program. error if failed
- load downloaded program. error if failed.
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::forceDownloadAndLoad()
{
 	m_Debug << "[DEBUG] Executing CuserEvxaInterface::forceDownloadAndLoad()" << CUtil::CLog::endl;
  
    	// check if program is available remotely
    	if (downloadProgramFromServerToLocal())	
	{
		// If there's a program loaded then unload it.
		if (!unloadProgram(m_TPArgs.CurrentProgName)) return false;

		// load our program from XTRF
		if (!loadProgram(m_TPArgs.FullTPName)) return false;
	}	
    	else 
	{ 
		m_Log <<  "[ERROR] Failed to force download program." << CUtil::CLog::endl;
		return false;
    	}    
	return true;
}

/*---------------------------------------------------------------------------------
download strategy: never
reload strategy: force
- if TP file exists in local folder, unpack to test folder
- if not, try to download from remote folder and unpack. error if failed.
- unload existing program. error if failed
- load downloaded program. error if failed.
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::neverDownloadForceLoad()
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::neverDownloadForceLoad()" << CUtil::CLog::endl;
 
	// try unpacking TP file from local folder to test folder
	if (!unpackProgramFromLocalToTest())
	{
		// if failed, try downloading TP file from remote folder to local folder
		// this also unpack TP file from local folder to test folder
		if (!downloadProgramFromServerToLocal())	
		{
			m_Log << "[ERROR] Failed to download program." << CUtil::CLog::endl;
			return false;	
		}
	}
	// If there's a program loaded then unload it.
	if (! unloadProgram(m_TPArgs.CurrentProgName)) return false;

	// load our program from XTRF
	if (!loadProgram(m_TPArgs.FullTPName)) return false;

	return true;
}
/*---------------------------------------------------------------------------------
download strategy: never
reload strategy: attempt
- if TP file exists in local folder, unpack to test folder
	- if not, try to download from remote folder and unpack. 
		- if failed to download from remote folder, use current loaded TP
			- if current TP != new TP, or current TP = null, error.
- unload existing program. error if failed
- load downloaded program. error if failed.
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::neverDownloadAttemptLoad()
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::neverDownloadForceLoad()" << CUtil::CLog::endl;
 
	// try unpacking TP file from local folder to test folder
	if (!unpackProgramFromLocalToTest())
	{
		// if failed, try downloading TP file from remote folder to local folder
		// this also unpack TP file from local folder to test folder
		if (!downloadProgramFromServerToLocal())	
		{
			m_Log << "[ERROR] Failed to download program." << CUtil::CLog::endl;

			if (isProgramToLoadAlreadyLoaded()) return true;
			else return false;
		}
	}
	// If there's a program loaded then unload it.
	if (! unloadProgram(m_TPArgs.CurrentProgName)) return false;
	// load our program from XTRF
	if (!loadProgram(m_TPArgs.FullTPName)) return false;
	return true;
}

/*---------------------------------------------------------------------------------
download strategy: never
reload strategy: never
- if current TP = new TP. ok, done
- if not, try unpacking TP from local folder
	- if failed, try downloading new TP from remote server. error if fail
- if unpacking TP from local folder to test folder succeeds	
	- unload existing program. error if failed
	- load downloaded program. error if failed.
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::neverDownloadNeverLoad()
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::neverDownloadNeverLoad()" << CUtil::CLog::endl;
 
	// check if test program we are trying to load is already loaded
	if (isProgramToLoadAlreadyLoaded()) return true;
	else 
	{
		// if not, try to reload from local folder
		if (!unpackProgramFromLocalToTest())
		{
			// if program does not exist in local folder, try downloading it
			if (!downloadProgramFromServerToLocal()) return false;
		}
	}
	// if we reached this point, TP file is successfully unpacked to test folder

	// If there's a program loaded then unload it.
	if (! unloadProgram(m_TPArgs.CurrentProgName)) return false;
	// load our program from XTRF
	if (!loadProgram(m_TPArgs.FullTPName)) return false;
	return true;
}

/*---------------------------------------------------------------------------------
download strategy: attempt
reload strategy: force
- try to download TP file from remote folder
	- if failed, try unpacking TP file from local folder
		- error if failed
- unload existing program. error if failed
- load downloaded program. error if failed.
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::attemptDownloadForceLoad()
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::attemptDownloadForceLoad()" << CUtil::CLog::endl;

	// try downloading program from remote folder
	if (!downloadProgramFromServerToLocal())
	{
		// if failed to download from remote folder, try unpacking the one from local folder
		if (!unpackProgramFromLocalToTest()) return false;
	}
	// If there's a program loaded then unload it.
	if (!unloadProgram(m_TPArgs.CurrentProgName)) return false;
	// load our program from XTRF
	if (!loadProgram(m_TPArgs.FullTPName)) return false;
	return true;
}

/*---------------------------------------------------------------------------------
download strategy: attempt
reload strategy: force
- try to download TP file from remote folder
	- if failed, try unpacking TP file from local folder
		- check if TP already loaded before. error if not
- unload existing program. error if failed
- load downloaded program. error if failed.
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::attemptDownloadAttemptLoad()
{
	m_Debug << "[DEBUG] Executing CuserEvxaInterface::attemptDownloadAttemptLoad()" << CUtil::CLog::endl;

	// try downloading program from remote folder
	if (!downloadProgramFromServerToLocal())
	{
		// if failed to download from remote folder, try unpacking the one from local folder
		if (!unpackProgramFromLocalToTest()) 
		{
			// if failed to unpack from local folder, check if new TP is already loaded (current TP)
			if (isProgramToLoadAlreadyLoaded()) return true;
			else return false;
		}
	}
	// If there's a program loaded then unload it.
	if (!unloadProgram(m_TPArgs.CurrentProgName)) return false;
	// load our program from XTRF
	if (!loadProgram(m_TPArgs.FullTPName)) return false;
	return true;
}

//--------------------------------------------------------------------
/* setupWaitForNotification will setup a condition variable
and wait for the StateNotification to occur before continuing.
This is so we can serialize events so the RecipeDecodeResult will
be the last thing sent to cgem.
*/ 
void CuserEvxaInterface::setupWaitForNotification(FA_ULONG wait_state, FA_ULONG wait_minor_state)
{
	// initialize the condition we are waiting for
	m_currentState = wait_state;
	m_currentMinorState = wait_minor_state;
	m_taskComplete = false;
	m_goAway = false;
	pthread_cond_init(&m_wakeUp, 0);
	pthread_mutex_init(&m_condMutex, 0);
}

/* setupProgramNotification will setup a condition variable
and wait for the StateNotification to occur before continuing.
This version will take an arry of EVX_PROGRAM_STATE so we can 
wait for 1 of several different states.
The array must end with MAX_EVX_PROGRAM_STATE
*/ 
void CuserEvxaInterface::setupProgramNotification(EVX_PROGRAM_STATE *wait_program_states)
{
	if (wait_program_states == NULL) return;

	m_currentProgramStateArray.clear();
	for (int ii = 0; (wait_program_states[ii] != MAX_EVX_PROGRAM_STATE); ii++) 
	{
		m_currentProgramStateArray.push_back(wait_program_states[ii]);
	}
	m_currentState = EVX_PROGRAM_CHANGE;
	m_taskComplete = false;
	m_goAway = false;
	pthread_cond_init(&m_wakeUp, 0);
	pthread_mutex_init(&m_condMutex, 0);
}

//--------------------------------------------------------------------
/* waitForNotification will wait for teh condition variable to be set 
from the StateNotification.
*/ 
void CuserEvxaInterface::waitForNotification()
{
	// lock the block and wait for the signal from the other thread.
	CMutexLock mtx(m_condMutex);

	// We need a while loop just in case the wait has a spurrious
	// return (which may occur according to the manual).
	while (m_taskComplete == false) 
	{
		// Are we done yet?
		if (m_goAway)
		return;  // get out of here is shutting down.

		// Wait for something to happen.
		pthread_cond_wait(&m_wakeUp, &m_condMutex);
	}
}

//--------------------------------------------------------------------
/* sendWaitForNotification will set the condtion variable to contiue 
if the given wait_state and wait_program_wait state match
what was given from setup.
*/ 
void CuserEvxaInterface::sendNotificationComplete(FA_ULONG wait_state, FA_ULONG wait_minor_state)
{
    if ((MAX_EVX_STATE == m_currentState) && (MAX_EVX_PROGRAM_STATE == m_currentMinorState))
	return;

    int arrsize = m_currentProgramStateArray.size();
    bool foundNotification = false;
    if ((EVX_PROGRAM_CHANGE == wait_state) && (arrsize > 0)) {
	for (int ii=0; ii<arrsize; ii++) {
	    if (m_currentProgramStateArray[ii] == wait_minor_state) {
		foundNotification = true;
		break;
	    }
	}
    }
    else if ((wait_state == m_currentState) && (wait_minor_state == m_currentMinorState))
    {
	foundNotification = true;
    }

    if (foundNotification == true)
    {
	// fprintf(stdout, "Handle sendNotificationComplete: current %ld %ld %d passed_in: %ld %ld\n", 
	// m_currentState, m_currentMinorState, arrsize, wait_state, wait_minor_state);
	// lock the block and wait to reach pthread_cond_wait from main thread.
	CMutexLock mtx(m_condMutex);

	// reset currentState since we are handling it.
	m_currentState = MAX_EVX_STATE;
	m_currentMinorState = MAX_EVX_PROGRAM_STATE;
	m_currentProgramStateArray.clear();
	m_taskComplete = true;
	pthread_cond_signal(&m_wakeUp);
    }
}


/*---------------------------------------------------------------------------------
PARSE THE RECIPEHANDLER_CONFIG.XML FILE
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::parseRecipeHandlerConfigurationFile(const std::string& recipeFilePath)
{
   	m_Debug << "[DEBUG] Executing CuserEvxaInterface::parseRecipeHandlerConfigurationFile() with " << recipeFilePath << " as parameter" << CUtil::CLog::endl;
    	bool result = true;

    	// clear all parameters that will come from recipehandler_config.xml
    	result = m_ConfigArgs.clearConfigParams();

    	// Now parse the file.  
    	XML_Node *rootNode = new XML_Node (recipeFilePath.c_str());
    	if (rootNode) 
	{
		std::string ptag = rootNode->fetchTag();
		m_Debug << "[DEBUG] 	Root Tag: " << ptag << CUtil::CLog::endl;

		// process <RECIPEHANDLER_CONF> tag
		if (ptag.compare("RECIPEHANDLER_CONF") == 0)
		{ 
			result = parseRecipeHandlerConfiguration(rootNode);
		}
		else
		{
			m_Debug << "[DEBUG] 	unknown root tag " << ptag << " found." << CUtil::CLog::endl;
			SendMessageToHost(false, "011", "Config unknown tag");
		}
    	}

    	// delete XML_Node
    	if (rootNode) 
	{
		delete rootNode;
		rootNode = NULL;
    	}
    	return result;
}

/*---------------------------------------------------------------------------------
parse the child tags of <RECIPEHANDLER_CONF> tag
must contain <CurrentSiteConfiguration> which specifies the
active <SiteConfiguration>, so we find that first.
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::parseRecipeHandlerConfiguration(XML_Node *recipeConfig)
{
   	m_Debug << "[DEBUG] Executing CuserEvxaInterface::parseRecipeHandlerConfiguration()" << CUtil::CLog::endl;
	bool result = true;
    
	m_Debug << "[DEBUG] <" << recipeConfig->fetchTag() << "> has " << recipeConfig->numChildren() << " child tags." << CUtil::CLog::endl;

	// find the active config and process that
	std::string configName("");
	for(int ii = 0; ii < recipeConfig->numChildren(); ii++) 
	{
		XML_Node *childNode = recipeConfig->fetchChild(ii);
		if (childNode) 
		{
			std::string ptag = childNode->fetchTag();
			m_Debug << "[DEBUG] 	Child Tag: " << ptag << CUtil::CLog::endl;

			// <CurrentSiteConfiguration> specifies that active configuration 
			if (ptag.compare("CurrentSiteConfiguration") == 0) 
			{
				for (int ii = 0; ii< childNode->numAttr(); ii++) 
				{
					m_Debug << "[DEBUG] CurrentSiteConfiguration Attr " << childNode->fetchAttr(ii) << " : " << childNode->fetchVal(ii) << CUtil::CLog::endl; 
					if (childNode->fetchAttr(ii).compare("ConfigurationName") == 0) 
					{
						configName = childNode->fetchVal(ii);
						m_ConfigArgs.ConfigurationName = configName;
						m_Log << "Active <ConfigurationName> is " << m_ConfigArgs.ConfigurationName << CUtil::CLog::endl;
					}
				}
			}
			// or is this one of the <SiteConfiguration> in the list?
			else if (ptag.compare("SiteConfiguration") == 0) 
			{
				for (int ii = 0; ii < childNode->numAttr(); ii++) 
				{
					m_Debug << "[DEBUG] Configuration Attr " << childNode->fetchAttr(ii) << " : " << childNode->fetchVal(ii) << CUtil::CLog::endl;
					if (childNode->fetchAttr(ii).compare("ConfigurationName") == 0) 
					{
						if(childNode->fetchVal(ii).compare(configName) == 0) 
						{
							parseSiteConfiguration(childNode);
							break; // break from for loop
						}
					}
				}	    
			}
			else 
			{
				m_Debug << "[DEBUG] 	unknown child tag " << ptag << " found." << CUtil::CLog::endl;
				SendMessageToHost(false, "011", "Config unknown tag");
			}
		}
		else m_Debug << "[DEBUG]	empty child tag found. " << CUtil::CLog::endl;
	}
	return result;
}

/*---------------------------------------------------------------------------------
parse the child tags of <SiteConfiguration> tag
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::parseSiteConfiguration(XML_Node *siteConfig)
{
   	m_Debug << "[DEBUG] Executing CuserEvxaInterface::parseSiteConfiguration()" << CUtil::CLog::endl;
    	bool result = true;
    
 	m_Debug << "[DEBUG] <" << siteConfig->fetchTag() << "> has " << siteConfig->numChildren() << " child tags." << CUtil::CLog::endl;

    	for(int ii = 0; ii < siteConfig->numChildren(); ii++) 
	{
		XML_Node *childNode = siteConfig->fetchChild(ii);
		if (childNode) 
		{
	    		std::string ptag = childNode->fetchTag();
			m_Debug << "[DEBUG] 	Child Tag: " << ptag << CUtil::CLog::endl;

	    		if (ptag.compare("argParameter") == 0) 
			{
				for (int ii = 0; ii < childNode->numAttr(); ii++) 
				{
		    			m_Debug << "argParameter Attr " << childNode->fetchAttr(ii) << " : " << childNode->fetchVal(ii) << CUtil::CLog::endl;
		    
					if (childNode->fetchAttr(ii).compare("RemoteLocation") == 0){ m_ConfigArgs.RemoteLocation = childNode->fetchVal(ii); }
				    	else if (childNode->fetchAttr(ii).compare("LocalLocation") == 0){ m_ConfigArgs.LocalLocation = childNode->fetchVal(ii); }
		    			else if (childNode->fetchAttr(ii).compare("ProgLocation") == 0){ m_ConfigArgs.ProgLocation = childNode->fetchVal(ii); }
		    			else if (childNode->fetchAttr(ii).compare("PackageType") == 0){ m_ConfigArgs.PackageType = childNode->fetchVal(ii); }
		    			else if (childNode->fetchAttr(ii).compare("S10F1") == 0){ m_ConfigArgs.S10F1 = childNode->fetchVal(ii); }
		    			else m_Log << "[ERROR] Unknown SiteConfig Parameter: " << childNode->fetchAttr(ii) << " : " << childNode->fetchVal(ii) <<CUtil::CLog::endl;
				}
	    		}
	    		else 
			{
				m_Debug << "[DEBUG] 	unknown child tag " << ptag << " found." << CUtil::CLog::endl;
				SendMessageToHost(false, "011", "Config unknown tag");
			}
		}
		else m_Debug << "[DEBUG]	empty child tag found. " << CUtil::CLog::endl;
    	}
    	return result;
}

// print the results of parsing the configuration file.
bool CuserEvxaInterface::printConfigParams()
{
    bool result = m_ConfigArgs.printConfigParams();
    return result;
}
    
// check that all params of the configuration file have been parsed.
bool CuserEvxaInterface::checkConfigParams()
{
    bool result = m_ConfigArgs.checkConfigParams();
    return result;
}

bool CuserEvxaInterface::checkProgLocation()
{
    bool result = m_ConfigArgs.checkProgLocation();
    return result;
}
