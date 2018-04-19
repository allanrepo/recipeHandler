#include "userInterface.h"
#include "xmlInterface.h"
#include <sstream> 
#include <string>

bool g_bDisableRobot = false;


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

CuserEvxaInterface::CuserEvxaInterface(const std::string& testerName, unsigned long headNumber, bool connect) 
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

bool CuserEvxaInterface::connectToTester(void) 
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
    if (m_tester)
    	result = m_tester->registerCommandNotification(arr_size, evx_cmd);
    return result;
}

const char* CuserEvxaInterface::getRecipeParseStatusName(EVX_RECIPE_PARSE_STATUS state)	
{
    if (m_tester)
	return m_tester->getRecipeParseStatusName(state);
    else
	return "";
}

void CuserEvxaInterface::shutdownTester(void) 
{
    // In case we're waiting for a thread to complete.
    if (m_currentState != MAX_EVX_STATE) {
	m_goAway = true;
	pthread_cond_signal(&m_wakeUp);
    }

    if (NULL != m_tester) {
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

    if (dwLen > 0) { // We have an environment variable, so get it
	if ((ev = new TCHAR [dwLen+2]) != NULL) {
	    dwLen = GetEnvironmentVariable(token.c_str(), ev, (dwLen+2));
	    if (dwLen > 0) {
		value = ev;
		foundEnv = true;
	    }
	    delete [] ev;
	}
    }
#else
    char *ev = getenv(token.c_str());
    if (ev) {
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
    if (0 != access(l_base.c_str(), F_OK)) {
	if (getEnvVar("HOME", l_base)) { 
	    l_base += std::string("/synchro_sim/") + args().testerName();
	}
    }

    pwrupDir = l_base + std::string("/pwrup");
    if (0 == access(pwrupDir.c_str(), F_OK)) {
	mainTesterDir = l_base;
	logDir = l_base + std::string("/log");
    }
    else pwrupDir = "";

    return (pwrupDir.length() > 0);
}
    

void CuserEvxaInterface::commonInit() 
{
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

}

ProgramControl *CuserEvxaInterface::PgmCtrl()
{	
    if (NULL != m_tester) return m_tester->progCtrl();
    return NULL;
}

void CuserEvxaInterface::dlogChange(const EVX_DLOG_STATE state)
{
   if (debug()) std::cout << "CuserEvxaInterface::dlogChange" << std::endl;
}

void CuserEvxaInterface::expressionChange(const char *expr_obj_name)
{
    if (debug()) std::cout << "CuserEvxaInterface::expressionChange" << std::endl;
}

void CuserEvxaInterface::expressionChange(const char *expr_obj_name, const char *expr_name,
                              const char *expr_value, int site)
{
    if (debug()) std::cout << "CuserEvxaInterface::expressionChange" << std::endl;
}

void CuserEvxaInterface::objectChange(const XClientMessageEvent xmsg)
{
    if (debug()) std::cout << "CuserEvxaInterface::objectChange" << std::endl;
}

// temporarily comment out logs to quiet it down. return them later
void CuserEvxaInterface::programChange(const EVX_PROGRAM_STATE state, const char *text_msg)
{
    //allan// if (debug()) std::cout << "CuserEvxaInterface::programChange (state = " << state << ", Message = \"" << text_msg << "\")" << std::endl;

    ProgramControl *pgm = PgmCtrl();

    switch(state)
    {
    case EVX_PROGRAM_LOADING:
	if (debug()) std::cout << "programChange: EVX_PROGRAM_LOADING" << std::endl;
	break;
    case EVX_PROGRAM_LOAD_FAILED:
	{
	    // if load failed then set the result to false.
	    m_recipeParseResult = false;
	}
	break;
	case EVX_PROGRAM_LOADED:
	{
	    	if (debug()) std::cout << "programChange: EVX_PROGRAM_LOADED" << std::endl;
		updateSTDFAfterProgLoad();
	}
	break;
    case EVX_PROGRAM_START:
	if (debug()) std::cout << "programChange: EVX_PROGRAM_START" << std::endl;
	break;
    case EVX_PROGRAM_RUNNING:
	if (debug()) std::cout << "programChange: EVX_PROGRAM_RUNNING" << std::endl;
	break;
    case EVX_PROGRAM_UNLOADING:
	if (debug()) std::cout << "programChange: EVX_PROGRAM_UNLOADING" << std::endl;
	break;
    case EVX_PROGRAM_UNLOADED:
	if (debug()) std::cout << "programChange: EVX_PROGRAM_UNLOADED" << std::endl;
	// If the program was unloaded and not by xtrf then unblock the robots.
	if (m_recipeParse == false) {
	    if (debug()) std::cout << "programChange: EVX_PROGRAM_UNLOADED UnblockRobot" << std::endl;
	    EVXAStatus status = pgm->UnblockRobot();
	    if (status != EVXA::OK)
		fprintf(stdout, "Error PROGRAM_UNLAODED UnblockRobot(): status:%d %s\n", status, pgm->getStatusBuffer());

	}
	break;
    case EVX_PROGRAM_ABORT_LOAD:
	{
	    m_recipeParseStatus = EVX_RECIPE_PARSE_ABORT;
	    m_recipeParseResult = false;

	}
	break;
    case EVX_PROGRAM_READY:
	if (debug()) std::cout << "programChange: EVX_PROGRAM_READY" << std::endl;
	break;
    default:
	// allan //if (debug()) std::cout << "programChange: Not Handled" << state << std::endl;
	break;
    }

    // tell the recipe thread we got the notification.
    sendNotificationComplete(EVX_PROGRAM_CHANGE, state);
}

void CuserEvxaInterface::modVarChange(const int id, char *value)
{
    if (debug()) std::cout << "CuserEvxaInterface::modVarChange" << std::endl;
}


void CuserEvxaInterface::programRunDone(const int array_size,
                            int site[],
                            int serial[],
                            int swbin[],
                            int hwbin[],
                            int pass[],
                            LWORD dsp_status
                            )
{
    if (debug()) std::cout << "CuserEvxaInterface::programRunDone" << std::endl;


	if (g_bDisableRobot)
	{
    		// wait
		//sleep(1000);

		// stop robot
		PgmCtrl()->BlockRobot();

		g_bDisableRobot = false;
	}	
}


void CuserEvxaInterface::restartTester(void)
{
    if (debug()) std::cout << "CuserEvxaInterface::restartTester" << std::endl;
    m_testerRestart = true;
}

void CuserEvxaInterface::evtcDisconnected(void)
{
    if (debug()) std::cout << "CuserEvxaInterface::evtcDisconnected" << std::endl;
}

void CuserEvxaInterface::evtcConnected(void)
{
    if (debug()) std::cout << "CuserEvxaInterface::evtcConnected" << std::endl;
}

void CuserEvxaInterface::streamChange(void)
{
    if (debug()) std::cout << "CuserEvxaInterface::streamChange" << std::endl;

}

void CuserEvxaInterface::tcBooting(void)
{
    if (debug()) std::cout << "CuserEvxaInterface::tcBooting" << std::endl;
}

void CuserEvxaInterface::testerReady(void)
{
    if (debug()) std::cout << "CuserEvxaInterface::testerReady" << std::endl;
}

void CuserEvxaInterface::gemRunning(void)
{
    if (debug()) std::cout << "CuserEvxaInterface::gemRunning" << std::endl;
}

void CuserEvxaInterface::alarmChange(const EVX_ALARM_STATE alarm_state, const ALARM_TYPE alarm_type,
                         const long time_occurred, const char *description)
{
    if (debug()) std::cout << "CuserEvxaInterface::alarmChange" << std::endl;
}

void CuserEvxaInterface::testerStateChange(const EVX_TESTER_STATE tester_state)
{
    if (debug()) std::cout << "CuserEvxaInterface::testerStateChange" << std::endl;
    ProgramControl *pgm = PgmCtrl();
    if (NULL == pgm) {
	fprintf(stdout, "Error testerStateChange: no ProgramControl\n");
	return;
    }
    EVXAStatus status = EVXA::OK;
    switch (tester_state) {
    case EVX_TESTER_PAUSED:
	g_bDisableRobot = true;
	fprintf(stdout, "TESTER PAUSED when program is running. robot will stop after EOT.\n");
	if (!pgm->isProgramRunning())
	{ 
		fprintf(stdout, "TESTER PAUSED when program is not running. robot is stopped instantly.\n");
		status = pgm->BlockRobot(); 
		g_bDisableRobot = false; 
	}
	break;
    case EVX_TESTER_RESUMED:
	g_bDisableRobot = false;
	status = pgm->UnblockRobot();
	fprintf(stdout, "TESTER RESUMED. robot is activated instantly.\n");
	break;
    default:
	break;
    }
    if (status != EVXA::OK)
	fprintf(stdout, "Error testerStateChange: status:%d %s\n", status, pgm->getStatusBuffer());
}

void CuserEvxaInterface::waferChange(const EVX_WAFER_STATE wafer_state, const char *wafer_id)
{
    if (debug()) std::cout << "CuserEvxaInterface::waferChange" << std::endl;
 
   // tell the recipe thread we got the notification.
    sendNotificationComplete(EVX_WAFER_CHANGE, wafer_state);	    
}

void CuserEvxaInterface::lotChange(const EVX_LOT_STATE lot_state, const char *lot_id)
{
    if (debug()) std::cout << "CuserEvxaInterface::lotChange ( state = " << lot_state
              << ", Lot ID = \"" << lot_id << "\")" << std::endl;


    // tell the recipe thread we got the notification.
    sendNotificationComplete(EVX_LOT_CHANGE, lot_state);

	//handleBackToIdleStrategy();
	    

}

void CuserEvxaInterface::EvxioMessage(int responseNeeded, int responseAquired, char *evxio_msg)
{
    if (debug()) std::cout << "CuserEvxaInterface::EvxioMessage" << std::endl;
}

void CuserEvxaInterface::RecipeDecodeAvailable(const char *recipe_text, bool &result)
{
  	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::RecipeDecodeAvailable()" << std::endl;

     resetRecipeVars();



    //Check the recipe text for the xml header.
    result = false;
    if (recipe_text != NULL) {
	if (strncasecmp("<?xml", recipe_text, strlen("<?xml")) == 0) {
	    result = true;
	}
    }
}

void CuserEvxaInterface::RecipeDecode(const char *recipe_text)
{
	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::RecipeDecode()" << std::endl;
	bool result = true;

    	// reset recipe variables before starting parsing.
    	resetRecipeVars();
 
    	// parse the xml file.  parseXML opens up a file.
    	if (recipe_text != NULL) { result = parseXML(recipe_text); }
    	else{ std::cout << "[ERROR] RecipeDecode recipe_text is NULL" <<  std::endl; result = false; }

    	// execute load based on reload strategy.
    	if (result == true) result = updateProgramLoad();

    	// If the program load failed then set the result to fail.
    	if (m_recipeParseResult == false) result = false;

    	// After program load, update parameters.
    	if ((result == true) && (m_skipProgramParam == false)) result = updateTestProgramData();
    
	// this line originally here but somehow calling this gives a false trigger to gem host that end-lot occurred.
	// not sure why, probably need to investigate this but low priority for now.
   	sendRecipeResultStatus(result);  // parsing failed so just send the result back to cgem.
 }



// USER SPECIFIC CODE FOR XML DATA
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------
/* This function parses the xml string and finds the program to load.
 It stores the program and stdf information for use at a later time.
recipe_text is the xml data.
*/
bool CuserEvxaInterface::parseXML(const char*recipe_text)
{
  	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::parseXML()" << std::endl;

    bool result = true;

    m_recipeParse = true;
    PgmCtrl()->clearStatus();
    PgmCtrl()->notifyRecipeStatus(EVX_RECIPE_PARSE_BEGIN);

    // clear all parameters that will come from xml
    result = clearAllParams();

    // create a file
    std::string xmlFileName("/tmp/loadxml.xml");
    FILE *fptr = fopen(xmlFileName.c_str(), "w");
    if (fptr != NULL) {
	fprintf(fptr, "%s", recipe_text);
	fclose(fptr);
    }
  
    // Now parse the file.
    XML_Node *rootNode = new XML_Node (xmlFileName.c_str());
    if (rootNode) {
	std::string ptag = rootNode->fetchTag();
	if (debug()) fprintf(stdout, "rootNode tag: %s\n", ptag.c_str());

	// Add else-if statements for other rootNodes that need parsing
	// Then add a function to parse that rootNode.
	if (ptag.compare("testerRecipe") == 0) {
	    result = parseTesterRecipe(rootNode);
	}
	else if (ptag.compare("STDFandContextUpdate") == 0) {
	    result = parseSTDFandContextUpdate(rootNode);
	}
	else {
	    fprintf(stdout, "rootNode unkown: %s, Not Parsed\n", ptag.c_str());
	    result = false;
	}
    }

    // delete the file
    unlink(xmlFileName.c_str());

    // delete XML_Node
    if (rootNode) {
	delete rootNode;
	rootNode = NULL;
    }
    return result;
}

bool CuserEvxaInterface::parseSTDFandContextUpdate(XML_Node *parseSTDFUpdate)
{
  	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::parseSTDFandContextUpdate()" << std::endl;
    bool result = true;
   
    int numChildren = parseSTDFUpdate->numChildren();
    if (debug()) fprintf(stdout, "parseSTDFUpdate numChildren: %d\n", numChildren);

    // Need to know what Recipe Children you want to parse.ls -
    // Check for the ones we care about.  
    for(int ii=0; ii<numChildren; ii++) {
	XML_Node *childNode = parseSTDFUpdate->fetchChild(ii);
	if (childNode) {
	    std::string ptag = childNode->fetchTag();
	    if (debug()) fprintf(stdout, "parseSTDFUpdate childNode tag: %s\n", ptag.c_str());

	    // Add else-if statements for other children that need parsing
	    // Then add a function to parse that child.
	    if(ptag.compare("STDF") == 0) {
		result = parseSTDF(childNode);
		if (result == false)  // if parsing did not succeed then exit out of for loop.
		    break; 
	    }
	    else {
		fprintf(stdout, "parseSTDFandContextUpdate unkown child: %s, Not Parsed\n", ptag.c_str());
	    }
	}
	else
	    fprintf(stdout, "parseSTDFandContextUpdate: childNode is NULL\n");
    }
    return result;
}

bool CuserEvxaInterface::parseTesterRecipe(XML_Node *testerRecipe)
{
 	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::parseTesterRecipe()" << std::endl;
    bool result = true;
    
    int numChildren = testerRecipe->numChildren();
    if (debug()) fprintf(stdout, "testerRecipe numChildren: %d\n", numChildren);

    // Need to know what Recipe Children you want to parse.
    // Check for the ones we care about.  
    for(int ii=0; ii<numChildren; ii++) {
	XML_Node *childNode = testerRecipe->fetchChild(ii);
	if (childNode) {
	    std::string ptag = childNode->fetchTag();
	    if (debug()) fprintf(stdout, "testerRecipe childNode tag: %s\n", ptag.c_str());

	    // Add else-if statements for other children that need parsing
	    // Then add a function to parse that child.
	    if (ptag.compare("testPrgmIdentifier") == 0) {
		result = parseTestPrgmIdentifier(childNode);
		if (result == false)  // if parsing did not succeed then exit out of for loop.
		    break; 
	    }
	    else if(ptag.compare("STDF") == 0) {
		result = parseSTDF(childNode);
		if (result == false)  // if parsing did not succeed then exit out of for loop.
		    break; 
	    }
	    else {
		fprintf(stdout, "testerRecipe unkown child: %s, Not Parsed\n", ptag.c_str());
	    }
	}
	else
	    fprintf(stdout, "testerRecipe: childNode is NULL\n");
    }
    return result;
}

bool CuserEvxaInterface::parseTestPrgmIdentifier(XML_Node *testPrgmIdentifier)
{
 	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::parseTestPrgmIdentifier()" << std::endl;
    bool result = true;
    
    int numChildren = testPrgmIdentifier->numChildren();
    for (int ii=0; ii<numChildren; ii++) {
	XML_Node *childNode = testPrgmIdentifier->fetchChild(ii);
	if (childNode) {
	    std::string ptag = childNode->fetchTag();
	    if (debug()) fprintf(stdout, "testPrgmIdentifier childNode tag: %s\n", ptag.c_str());

	    // Add else-if statements for other children that need parsing
	    // Then add a function to parse that child.
	    if (ptag.compare("testPrgm") == 0) {
		result = parseTestPrgm(childNode);
		if (result == false)  // if parsing did not succeed then exit out of for loop.
		    break; 
	    }
	    else {
		fprintf(stdout, "testPrgmIdentifier unknown child: %s, Not Parsed\n", ptag.c_str());
	    }
	}
	else
	    fprintf(stdout, "testPrgmIdentifier: childNode is NULL\n");
     }
    
    return result;
}

bool CuserEvxaInterface::parseTestPrgm(XML_Node *testPrgm)
{
	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::parseTestPrgm()" << std::endl;
    bool result = true;

    int numChildren = testPrgm->numChildren();
    for (int ii=0; ii<numChildren; ii++) {
	XML_Node *childNode = testPrgm->fetchChild(ii);
	if (childNode) {
	    std::string ptag = childNode->fetchTag();
	    if (debug()) fprintf(stdout, "testPrgm childNode tag: %s\n", ptag.c_str());

	    // Add else-if statements for other children that need parsing
	    // Then add a function to parse that child.
	    if (ptag.compare("testPrgmCopierLoaderScript") == 0) {
		result = parseTestPrgmCopierLoaderScript(childNode);
		if (result == false)  // if parsing did not succeed then exit out of for loop.
		    break; 
	    }
	    else if (ptag.compare("testPrgmLoader") == 0) {
		result = parseTestPrgmLoader(childNode);
		if (result == false)  // if parsing did not succeed then exit out of for loop.
		    break; 
	    }
	    else {
		fprintf(stdout, "testPrgm unknown child: %s, Not Parsed\n", ptag.c_str());
	    }
	}
	else
	    fprintf(stdout, "testPrgmIdentifier: childNode is NULL\n");
     }
    
    return result;

}

/*---------------------------------------------------------------------------------
any STDF field to be set by evxa (e.g. XTRF not available, info from tester)
is done here. this is called after program is loaded
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::updateSTDFAfterProgLoad()
{
	if (!PgmCtrl()){ std::cout << "[ERROR] CuserEvxaInterface::updateSTDFAfterProgLoad(): Cannot access ProgramControl object." << std::endl; return false; }

	// make sure to change exec_typ to "Unison"
	std::string SystemName = PgmCtrl()->getLotInformation(EVX_LotSystemName);
	if (SystemName.empty() || SystemName.compare("enVision") == 0) m_MIRArgs.ExecTyp = "Unison";

	// we send SDR.HAND_TYP to famodule so it can send it to STDF during onlotstart(). this ensures Unison doesn't overwrite it
	PgmCtrl()->faprocSet("Current Equipment: HAND_TYP", m_SDRArgs.HandTyp);
	std::string hand_typ;
	PgmCtrl()->faprocGet("Current Equipment: HAND_TYP", hand_typ);
	if (debug()) std::cout << "[DEBUG] HAND_TYP sent to faproc: " << hand_typ << std::endl;	

	return true;
}

/*---------------------------------------------------------------------------------
parse <testPrgmLoader> from XTRF
testPrgmURI attribute 
- holds the folder/program name and will be used to parse mir.spec_nam
  and mir.spec_rev
reloadStrategy, downloadStrategy, and backToIdleStrategy attributes		
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::parseTestPrgmLoader(XML_Node *testPrgmLoader)
{
	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::parseTestPrgmLoader()" << std::endl;
    	bool result = true;
    	if (debug()){fprintf(stdout, "[DEBUG] <testPrgmLoader> Found %d attributes with %d values in %s tag\n", testPrgmLoader->numAttr(), testPrgmLoader->numVals(), testPrgmLoader->fetchTag().c_str());}

    	for (int ii=0; ii<testPrgmLoader->numAttr(); ii++) 
	{
		if (debug()){ fprintf(stdout, "[DEBUG] <testPrgmLoader> Attr %s: %s\n", testPrgmLoader->fetchAttr(ii).c_str(), testPrgmLoader->fetchVal(ii).c_str()); }	
		if (testPrgmLoader->fetchAttr(ii).compare("reloadStrategy") == 0) m_TPArgs.ReloadStrategy = testPrgmLoader->fetchVal(ii);
		if (testPrgmLoader->fetchAttr(ii).compare("downloadStrategy") == 0) m_TPArgs.DownloadStrategy = testPrgmLoader->fetchVal(ii);
		if (testPrgmLoader->fetchAttr(ii).compare("backToIdleStrategy") == 0) m_TPArgs.BackToIdleStrategy = testPrgmLoader->fetchVal(ii);  

		/*
		testPrgmURI is expected to contain "<progfolder>/<programname.una>" and is stored in m_TPArgs.TPName
		<progfolder> is stored in m_TPArgs.TPPath. it will be referenced in download strategy later
		*/		
		if (testPrgmLoader->fetchAttr(ii).compare("testPrgmURI") == 0) 
		{
			m_TPArgs.TPName = testPrgmLoader->fetchVal(ii);
			unsigned found = m_TPArgs.TPName.find_first_of("/");
			if (found != std::string::npos){ m_TPArgs.TPPath = m_TPArgs.TPName.substr(0, found); }
			else { m_TPArgs.TPPath = ""; }		

			// try to get mir.spec_nam and mir.spec_ver from TPPath
			if (!m_TPArgs.TPPath.empty())
			{
				// TPPath might have subfolders. we're only interested in main folder, so we get rid of subs
				std::string toSpec( m_TPArgs.TPPath );
				std::size_t p = toSpec.find_first_of("/");
				if (p != std::string::npos) toSpec = toSpec.substr(0, p);

				p = toSpec.find_last_of("_");
				if (p != std::string::npos)
				{
					m_TPArgs.SpecNam = m_TPArgs.TPPath.substr(0, p);
					m_TPArgs.SpecVer = m_TPArgs.TPPath.substr(p + 1, std::string::npos);
					if (debug()) std::cout << "MIR.SPEC_NAM from testPrgmURI: " << m_TPArgs.SpecNam << std::endl;
					if (debug()) std::cout << "MIR.SPEC_VER from testPrgmURI: " << m_TPArgs.SpecVer << std::endl;
				}
			}				
		}
    	}

    	return result;
}

bool CuserEvxaInterface::_parseTestPrgmLoader(XML_Node *testPrgmLoader)
{
	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::_parseTestPrgmLoader()" << std::endl;
    	bool result = true;

    	if (debug()) 
	{
		fprintf(stdout, "%s: %d %d %d \n", 
				testPrgmLoader->fetchTag().c_str(),
	    			testPrgmLoader->numAttr(),
	    			testPrgmLoader->numVals(),
	    			testPrgmLoader->numChildren());
    	}

    	for (int ii=0; ii<testPrgmLoader->numAttr(); ii++) 
	{
		if (debug()){ fprintf(stdout, "testPrgmLoader Attr %s: %s\n", testPrgmLoader->fetchAttr(ii).c_str(), testPrgmLoader->fetchVal(ii).c_str()); }
		if (testPrgmLoader->fetchAttr(ii).compare("testPrgmURI") == 0) m_TPArgs.TPName = testPrgmLoader->fetchVal(ii);
		if (testPrgmLoader->fetchAttr(ii).compare("reloadStrategy") == 0) m_TPArgs.ReloadStrategy = testPrgmLoader->fetchVal(ii);
		if (testPrgmLoader->fetchAttr(ii).compare("downStrategy") == 0) m_TPArgs.DownloadStrategy = testPrgmLoader->fetchVal(ii);
		if (testPrgmLoader->fetchAttr(ii).compare("backToIdleStrategy") == 0) m_TPArgs.BackToIdleStrategy = testPrgmLoader->fetchVal(ii);
    	}

    	int numChildren = testPrgmLoader->numChildren();
    	if (debug()) fprintf(stdout, "testPrgmLoader numChildren = %d\n", numChildren);
    	
	for (int ii=0; ii<numChildren; ii++) 
	{
		XML_Node *argumentParameter = testPrgmLoader->fetchChild(ii);
		if (argumentParameter)
		{
 	    		if (debug()) 
			{
				fprintf(stdout, "%s: %d %d %d %s %s %s\n", 
		    			argumentParameter->fetchTag().c_str(),
		    			argumentParameter->numAttr(),
		    			argumentParameter->numVals(),
		    			argumentParameter->numChildren(),
		    			argumentParameter->fetchAttr(0).c_str(),
		    			argumentParameter->fetchVal(0).c_str(),
		   	 		argumentParameter->fetchText().c_str());
	    		}

	    		std::string temp = argumentParameter->fetchVal(0);
	    		std::string result = argumentParameter->fetchText();
	    
			if (temp.compare("PRIS_TESTPROG_NAME") == 0) m_TPArgs.TPName = result;
	    		else if (temp.compare("CAM_PRODUCT") == 0) m_TPArgs.Product = result;
	    		else if (temp.compare("PRIS_DIE_CODE") == 0) m_TPArgs.DieCode = result;
	    		else if (temp.compare("PRIS_PARALLEL_TEST") == 0) m_TPArgs.Parallelism = result;
	    		else if (temp.compare("CAM_FLOW") == 0) m_TPArgs.Flow = result;
	    		else if (temp.compare("CMOD_COD") == 0) m_TPArgs.CmodCode = result;
	    		else if (temp.compare("LOT_ID") == 0) m_TPArgs.LotId = result;
	    		else if (temp.compare("FLOW_ID") == 0) m_TPArgs.FlowId = result;
	    		else if (temp.compare("PRIS_GROSS_DIE") == 0) m_TPArgs.GrossDie = result;
	    		else if (temp.compare("Active_Controller") == 0) m_TPArgs.ActiveController = result;
	    		else if (temp.compare("endLot") == 0) m_TPArgs.EndLotEnable = result;
	    		else if (temp.compare("endWafer") == 0) m_TPArgs.EndWaferEnable = result;
	    		else if (temp.compare("startLot") == 0) m_TPArgs.StartLotEnable = result;
	    		else if (temp.compare("startWafer") == 0) m_TPArgs.StartWaferEnable = result;
		}
   	 }

    	return result;
}

bool CuserEvxaInterface::parseTestPrgmCopierLoaderScript(XML_Node *testPrgmCopierLoaderScript)
{
	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::parseTestPrgmCopierLoaderScript()" << std::endl;
     bool result = true;

    if (debug()) {
/*
	fprintf(stdout, "%s: %d %d %d %s %s\n", testPrgmCopierLoaderScript->fetchTag().c_str(),
	    testPrgmCopierLoaderScript->numAttr(),
	    testPrgmCopierLoaderScript->numVals(),
	    testPrgmCopierLoaderScript->numChildren(),
	    testPrgmCopierLoaderScript->fetchAttr(0).c_str(),
	    testPrgmCopierLoaderScript->fetchVal(0).c_str());
*/
	fprintf(stdout, ">>>>%s: %d %d %d \n", testPrgmCopierLoaderScript->fetchTag().c_str(),
	    testPrgmCopierLoaderScript->numAttr(),
	    testPrgmCopierLoaderScript->numVals(),
	    testPrgmCopierLoaderScript->numChildren()
	);
    }
    for (int ii=0; ii<testPrgmCopierLoaderScript->numAttr(); ii++) {
	if (debug()) {
	    fprintf(stdout, "testPrgmCopierLoaderScript Attr %s: %s\n", 
		    testPrgmCopierLoaderScript->fetchAttr(ii).c_str(), testPrgmCopierLoaderScript->fetchVal(ii).c_str());
	}
	if (testPrgmCopierLoaderScript->fetchAttr(ii).compare("reloadStrategy") == 0)
	    m_TPArgs.ReloadStrategy = testPrgmCopierLoaderScript->fetchVal(ii);
	if (testPrgmCopierLoaderScript->fetchAttr(ii).compare("downloadStrategy") == 0)
	    m_TPArgs.DownloadStrategy = testPrgmCopierLoaderScript->fetchVal(ii);
    }
    int numChildren = testPrgmCopierLoaderScript->numChildren();
    if (debug()) fprintf(stdout, "testPrgmCopierLoaderScript numChildren = %d\n", numChildren);
    for (int ii=0; ii<numChildren; ii++) {
	XML_Node *argumentParameter = testPrgmCopierLoaderScript->fetchChild(ii);
	if (argumentParameter)
	{
 	    if (debug()) {
		fprintf(stdout, "%s: %d %d %d %s %s %s\n", 
		    argumentParameter->fetchTag().c_str(),
		    argumentParameter->numAttr(),
		    argumentParameter->numVals(),
		    argumentParameter->numChildren(),
		    argumentParameter->fetchAttr(0).c_str(),
		    argumentParameter->fetchVal(0).c_str(),
		    argumentParameter->fetchText().c_str());
	    }

	    std::string temp = argumentParameter->fetchVal(0);
	    std::string result = argumentParameter->fetchText();
//	    if (temp.compare("TEST_PROGRAM") == 0)  // allan remove this as we are now looking for other fields to determin test program specifics
	    if (temp.compare("TEST_PROGRAM_NAME") == 0)
	    	{ 
			m_TPArgs.TPName = result; 

			// check if program name as extension
			unsigned found = m_TPArgs.TPName.find_last_of(".");  

			// check if program name extension = .eva
			std::string szExt = m_TPArgs.TPName.substr(found+1);
     			
			if ((szExt.compare("una") == 0) || (szExt.compare("UNA") == 0)){}
			else{ m_TPArgs.TPName += ".una"; }
	    
			fprintf(stdout, "TEST_PROGRAM_NAME: %s\n", m_TPArgs.TPName.c_str()); 
		}// allan added

	    if (temp.compare("TEST_PROGRAM_PATH") == 0){ m_TPArgs.TPPath = result; fprintf(stdout, "TEST_PROGRAM_PATH: %s\n", m_TPArgs.TPPath.c_str()); }// allan added
	    if (temp.compare("TEST_PROGRAM_FILE") == 0){ m_TPArgs.TPFile = result; fprintf(stdout, "TEST_PROGRAM_FILE: %s\n", m_TPArgs.TPFile.c_str()); }// allan added
	    else if (temp.compare("RcpFileSupport") == 0)
		m_TPArgs.RcpFileSupport = result;
	    else if (temp.compare("Flow") == 0)
		m_TPArgs.Flow = result;
	    else if (temp.compare("Salestype") == 0)
		m_TPArgs.Salestype = result;
	    else if (temp.compare("Temperature") == 0)
		m_TPArgs.Temperature =  result;
	    else if (temp.compare("Product") == 0)
		m_TPArgs.Product = result;
	    else if (temp.compare("Parallelism") == 0)
		m_TPArgs.Parallelism = result;
	    else if (temp.compare("endLot") == 0)
		m_TPArgs.EndLotEnable = result;
	    else if (temp.compare("endWafer") == 0)
		m_TPArgs.EndWaferEnable = result;
	    else if (temp.compare("startLot") == 0)
		m_TPArgs.StartLotEnable = result;
	    else if (temp.compare("startWafer") == 0)
		m_TPArgs.StartWaferEnable = result;
	}
    }
    return result;
}

bool CuserEvxaInterface::parseSTDF(XML_Node *stdf)
{
	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::parseSTDF()" << std::endl;
    bool result = true;

    int numChildren = stdf->numChildren();
    for (int ii=0; ii<numChildren; ii++) {
	XML_Node *childNode = stdf->fetchChild(ii);
	if (childNode) {
	    std::string ptag = childNode->fetchTag();
	    if (debug()) fprintf(stdout, "stdf childNode tag: %s\n", ptag.c_str());

	    // Add else-if statements for other children that need parsing
	    // Then add a function to parse that child.
	    if (ptag.compare("STDFrecord") == 0) {
		result = parseSTDFRecord(childNode);
		if (result == false)  // if parsing did not succeed then exit out of for loop.
		    break; 
	    }
	    else {
		fprintf(stdout, "stdf unkown child: %s, Not Parsed\n", ptag.c_str());
	    }	      
	}
	else
	    fprintf(stdout, "stdf: childNode is NULL\n");
     }

    return result;
}

bool CuserEvxaInterface::parseSTDFRecord(XML_Node *STDFRecord)
{
	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::parseSTDFRecord()" << std::endl;
   bool result = true;

    std::string rname("");
    for (int ii=0; ii<STDFRecord->numAttr(); ii++) {
	if (STDFRecord->fetchAttr(ii).compare("recordName") == 0) {
	    rname = STDFRecord->fetchVal(ii);
	}
    }
    if (debug()) fprintf(stdout,"RecordName: %s\n", rname.c_str());
    if (rname.compare("MIR") == 0) {
	result = parseMIR(STDFRecord);
    }
    else if (rname.compare("SDR") == 0) {
	result = parseSDR(STDFRecord);
    }
    else if (rname.compare("GDR") == 0) {
	result = parseGDR(STDFRecord);
    }
    else {
	fprintf(stdout, "[ERROR] parseSTDFRecord unknown recordName: %s\n", rname.c_str());
    }

    return result;
}

bool CuserEvxaInterface::parseMIR(XML_Node *MIRRecord)
{
	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::parseMIR()" << std::endl;
    bool result = true;

    XML_Node *STDFfields = MIRRecord->fetchChild("STDFfields");
    int nfields = STDFfields->numChildren();
    for(int jj=0; jj<nfields; jj++) {
	XML_Node *STDFfield = STDFfields->fetchChild(jj);
	if (STDFfield) {
	    std::string temp = STDFfield->fetchVal(0);
	    std::string result = STDFfield->fetchText();
	    if (debug()) fprintf(stdout, "parseMIR: %s %s\n", temp.c_str(), result.c_str());
				
	    if (temp.compare("LOT_ID") == 0)
		m_MIRArgs.LotId = result;
	    else if (temp.compare("CMOD_COD") == 0)
		m_MIRArgs.CmodCod = result;
	    else if (temp.compare("FLOW_ID") == 0)
		m_MIRArgs.FlowId = result;
	    else if (temp.compare("DSGN_REV") == 0)
		m_MIRArgs.DsgnRev = result;
	    else if (temp.compare("DATE_COD") == 0)
		m_MIRArgs.DateCod = result;
	    else if (temp.compare("OPER_FRQ") == 0)
		m_MIRArgs.OperFrq = result;
	    else if (temp.compare("OPER_NAM") == 0)
		m_MIRArgs.OperNam = result;
	    else if (temp.compare("NODE_NAM") == 0)
		m_MIRArgs.NodeNam = result;
	    else if (temp.compare("PART_TYP") == 0)
		m_MIRArgs.PartTyp = result;
	    else if (temp.compare("ENG_ID") == 0)
		m_MIRArgs.EngId = result;
	    else if (temp.compare("TST_TEMP") == 0)
		m_MIRArgs.TestTmp = result;
	    else if (temp.compare("FACIL_ID") == 0)
		m_MIRArgs.FacilId = result;
	    else if (temp.compare("FLOOR_ID") == 0)
		m_MIRArgs.FloorId = result;
	    else if (temp.compare("STAT_NUM") == 0)
		m_MIRArgs.StatNum = result;
	    else if (temp.compare("PROC_ID") == 0)
		m_MIRArgs.ProcId = result;
	    else if (temp.compare("MODE_COD") == 0)
		m_MIRArgs.ModCod = result;
	    else if (temp.compare("FAMLY_ID") == 0)
		m_MIRArgs.FamilyId = result;
	    else if (temp.compare("PKG_TYP") == 0)
		m_MIRArgs.PkgTyp = result;
	    else if (temp.compare("SBLOT_ID") == 0)
		m_MIRArgs.SblotId = result;

	    else if (temp.compare("JOB_NAM") == 0)
		m_MIRArgs.JobNam = result;
	    else if (temp.compare("SETUP_ID") == 0)
		m_MIRArgs.SetupId = result;
	    else if (temp.compare("JOB_REV") == 0)
		m_MIRArgs.JobRev = result;
	    else if (temp.compare("AUX_FILE") == 0)
		m_MIRArgs.AuxFile = result;
	    else if (temp.compare("RTST_COD") == 0)
		m_MIRArgs.RtstCod = result;
	    else if (temp.compare("TEST_COD") == 0)
		m_MIRArgs.TestCod = result;
	    else if (temp.compare("USER_TXT") == 0)
		m_MIRArgs.UserText = result;
	    else if (temp.compare("ROM_COD") == 0)
		m_MIRArgs.RomCod = result;
	    else if (temp.compare("SERL_NUM") == 0)
		m_MIRArgs.SerlNum = result;
	    else if (temp.compare("SPEC_NAM") == 0)
		m_MIRArgs.SpecNam = result;
	    else if (temp.compare("TSTR_TYP") == 0)
		m_MIRArgs.TstrTyp = result;
	    else if (temp.compare("SUPR_NAM") == 0)
		m_MIRArgs.SuprNam = result;
	    else if (temp.compare("SPEC_VER") == 0)
		m_MIRArgs.SpecVer = result;
	    else if (temp.compare("PROT_COD") == 0)
		m_MIRArgs.ProtCod = result;

	    else
		fprintf(stdout, "parseMIR unkown field: %s\n", temp.c_str());
	}
    }			    
    return result;
}

bool CuserEvxaInterface::parseSDR(XML_Node *SDRRecord)
{
	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::parseSDR()" << std::endl;
     bool result = true;

    XML_Node *STDFfields = SDRRecord->fetchChild("STDFfields");
    int nfields = STDFfields->numChildren();
    for(int jj=0; jj<nfields; jj++) {
	XML_Node *STDFfield = STDFfields->fetchChild(jj);
	if (STDFfield) {
	    std::string temp = STDFfield->fetchVal(0);
	    std::string result = STDFfield->fetchText();
	    if (debug()) fprintf(stdout, "parseSDR: %s %s\n", temp.c_str(), result.c_str());
				
	    if (temp.compare("HAND_TYP") == 0)
		m_SDRArgs.HandTyp = result;
	    else if (temp.compare("CARD_ID") == 0)
		m_SDRArgs.CardId = result;
	    else if (temp.compare("LOAD_ID") == 0)
		m_SDRArgs.LoadId = result;
	    else if (temp.compare("HAND_ID") == 0)
		m_SDRArgs.PHId = result;	

	    else if (temp.compare("DIB_TYP") == 0)
		m_SDRArgs.DibTyp = result;
	    else if (temp.compare("CABL_ID") == 0)
		m_SDRArgs.CableId = result;
	    else if (temp.compare("CONT_TYP") == 0)
		m_SDRArgs.ContTyp = result;
	    else if (temp.compare("LOAD_TYP") == 0)
		m_SDRArgs.LoadTyp = result;
	    else if (temp.compare("CONT_ID") == 0)
		m_SDRArgs.ContId = result;
	    else if (temp.compare("LASR_TYP") == 0)
		m_SDRArgs.LaserTyp = result;
	    else if (temp.compare("LASR_ID") == 0)
		m_SDRArgs.LaserId = result;
	    else if (temp.compare("EXTR_TYP") == 0)
		m_SDRArgs.ExtrTyp = result;
	    else if (temp.compare("EXTR_ID") == 0)
		m_SDRArgs.ExtrId = result;
	    else if (temp.compare("DIB_ID") == 0)
		m_SDRArgs.DibId = result;
	    else if (temp.compare("CARD_TYP") == 0)
		m_SDRArgs.CardTyp = result;
	    else if (temp.compare("CABL_TYP") == 0)
		m_SDRArgs.CableTyp = result;
	    else
		fprintf(stdout, "parseSDR unkown field: %s\n", temp.c_str());
	}
    }
    return result;
}

/*---------------------------------------------------------------------------------
parse GDR
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::parseGDR(XML_Node *GDRRecord)
{
	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::parseGDR()" << std::endl;
     	bool result = true;

	// create xtrf xml object
	tinyxtrf::Xtrf* xtrf(tinyxtrf::Xtrf::instance());
	xtrf->clear();
	tinyxtrf::GdrRecord gdrs;

	// start reading fields from XTRF 
    	XML_Node *STDFfields = GDRRecord->fetchChild("STDFfields");
    	int nfields = STDFfields->numChildren();
    	for(int jj=0; jj<nfields; jj++) 
	{
		XML_Node *STDFfield = STDFfields->fetchChild(jj);
		if (STDFfield) 
		{
	    		std::string temp = STDFfield->fetchVal(0);
	    		std::string result = STDFfield->fetchText();
	    		if (debug()) fprintf(stdout, "[DEBUG] GDR found: %s %s\n", temp.c_str(), result.c_str());				
	    		if (temp.compare("GEN_DATA") == 0){ gdrs.push_back(tinyxtrf::GdrField( temp.c_str(), "C*n", result.c_str())); }	
	    		else fprintf(stdout, "[ERROR] parseGDR unknown field: %s\n", temp.c_str());
		}
    	}
	/*
	// add hard coded GDR field
	gdrs.push_back(tinyxtrf::GdrField("GEN_DATA", "C*n", "STDF_FRM"));
	gdrs.push_back(tinyxtrf::GdrField("GEN_DATA", "C*n", "REV_I"));

	std::string driverId, driverRev;
	PgmCtrl()->faprocGet("Current Equipment:QUERY_CHUCK_TEMP", driverId);
	PgmCtrl()->faprocGet("Driver Revision", driverRev);
	std::cout << "Driver ID: " << driverId << ", Driver Rev: " << driverRev << std::endl;	
*/
	gdrs.insert(gdrs.begin(), 1, tinyxtrf::GdrField("FIELD_CNT", "U*2", num2stdstring(gdrs.size())));
	xtrf->addGdr(gdrs);
	xtrf->dumpGdrs("/tmp/gdr_auto.xtrf");

    	return result;
}

//--------------------------------------------------------------------
void CuserEvxaInterface::sendRecipeResultStatus(bool result)
{
	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::sendRecipeResultStatus()" << std::endl;
     if ((m_recipeParse == true) && (m_statusResultsHaveBeenSent == false)) {
	m_recipeParse = false;
	ProgramControl *pgm = PgmCtrl();
	if (NULL == pgm) {
	    fprintf(stdout, "Error sendRecipeResultStatus: no ProgramControl\n");
	    return;
	}
	pgm->clearStatus();
	EVXAStatus status = EVXA::OK;
	// result = false;
	// If m_recipeParseStatus was set during parsing then use it 
	// because it was set earlier in the process.
	// Otherwise look at the result flag.
	if (m_recipeParseStatus != EVX_RECIPE_PARSE_BEGIN) {
	    if (debug()) fprintf(stdout, "sendRecipeResultStatus %s\n", getRecipeParseStatusName(m_recipeParseStatus));
	    status = pgm->notifyRecipeStatus(m_recipeParseStatus);
	}	
	else if (true == result) {
	    if (debug()) fprintf(stdout, "sendRecipeResultStatus EVX_RECIPE_PARSE_COMPLETE\n");
	    status = pgm->notifyRecipeStatus(EVX_RECIPE_PARSE_COMPLETE);
	}
	else {
	    if (debug()) fprintf(stdout, "sendRecipeResultStatus EVX_RECIPE_PARSE_FAIL\n");
	    status = pgm->notifyRecipeStatus(EVX_RECIPE_PARSE_FAIL);
	}

	if (status != EVXA::OK) {
	    fprintf(stdout, "Error notifyRecipeStatus: status:%d %s\n", status, pgm->getStatusBuffer());
	}
    }

    // Mark that results have been sent so we don't send them again until the next recipe parse.
    m_statusResultsHaveBeenSent = true;	
}

//--------------------------------------------------------------------
bool CuserEvxaInterface::clearAllParams()
{
    m_TPArgs.clearParams();
    m_MIRArgs.clearParams();
    m_SDRArgs.clearParams();
    return true;
}

//--------------------------------------------------------------------
/* This function takes the MIR parameters and sends them to 
   The Test Program.
*/
bool CuserEvxaInterface::sendMIRParams()
{
	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::sendMIRParams()" << std::endl;
 
    ProgramControl *pgm = PgmCtrl();
    if (NULL == pgm) {
	fprintf(stdout, "Error sendMIRParams: no ProgramControl\n");
	return false;
    }
    EVXAStatus status = EVXA::OK;
    bool result = true;
    if (!m_MIRArgs.LotId.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotLotID, m_MIRArgs.LotId.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "Error sendMIRParams EVX_LotLotID: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
	fprintf(stdout, "MIR.LotLotID: %s\n", pgm->getLotInformation(EVX_LotLotID));
    }
    if (!m_MIRArgs.CmodCod.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotCommandMode, m_MIRArgs.CmodCod.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "Error sendMIRParams EVX_LotCommandMode: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
    }		
    if (!m_MIRArgs.FlowId.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotActiveFlowName, m_MIRArgs.FlowId.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "Error sendMIRParams EVX_LotActiveFlowName: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
    }
    if (!m_MIRArgs.DsgnRev.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotDesignRev, m_MIRArgs.DsgnRev.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "Error sendMIRParams EVX_LotDesignRev: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
    }
    if (!m_MIRArgs.DateCod.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotDateCode, m_MIRArgs.DateCod.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "Error sendMIRParams EVX_LotDateCode: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
    }
    if (!m_MIRArgs.OperFrq.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotOperFreq, m_MIRArgs.OperFrq.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "Error sendMIRParams EVX_LotOperFreq: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
    }
    if (!m_MIRArgs.OperNam.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotOperator, m_MIRArgs.OperNam.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "Error sendMIRParams EVX_LotOperator: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
    }
    if (!m_MIRArgs.NodeNam.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotTcName, m_MIRArgs.NodeNam.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "Error sendMIRParams EVX_LotTcName: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
    }
    if (!m_MIRArgs.PartTyp.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotDevice, m_MIRArgs.PartTyp.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "Error sendMIRParams EVX_LotDevice: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
    }
    if(!m_MIRArgs.EngId.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotEngrLotId, m_MIRArgs.EngId.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "Error sendMIRParams EVX_LotEngrLotId: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
    }
    if (!m_MIRArgs.TestTmp.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotTestTemp, m_MIRArgs.TestTmp.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "Error sendMIRParams EVX_LotTestTemp: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
    }
    if (!m_MIRArgs.FacilId.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotTestFacility, m_MIRArgs.FacilId.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "Error sendMIRParams EVX_LotTestFacility: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	} 
    }
    if (!m_MIRArgs.FloorId.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotTestFloor, m_MIRArgs.FloorId.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "Error sendMIRParams EVX_LotTestFloor: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
    }
    if (!m_MIRArgs.StatNum.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotHead, m_MIRArgs.StatNum.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "Error sendMIRParams EVX_LotHead: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
    }
    if (!m_MIRArgs.ProcId.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotFabricationID, m_MIRArgs.ProcId.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "Error sendMIRParams EVX_LotFabricationID: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
                        fprintf(stdout, "MIR.ProcId: %s\n", pgm->getLotInformation(EVX_LotFabricationID));
    }
    if (!m_MIRArgs.ModCod.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotTestMode, m_MIRArgs.ModCod.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "Error sendMIRParams EVX_LotTestMode: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
    }
    if (!m_MIRArgs.FamilyId.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotProductID, m_MIRArgs.FamilyId.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "Error sendMIRParams EVX_LotProductID: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
    }
    if (!m_MIRArgs.PkgTyp.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotPackage, m_MIRArgs.PkgTyp.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "Error sendMIRParams EVX_LotPackage: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
    }
    if (!m_MIRArgs.SblotId.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotSublotID, m_MIRArgs.SblotId.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "Error sendMIRParams EVX_LotSublotID: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
    }

    if (!m_MIRArgs.SetupId.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotTestSetup, m_MIRArgs.SetupId.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "[ERROR] sendMIRParams EVX_LotTestSetup: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
    }

    if (!m_MIRArgs.JobRev.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotFileNameRev, m_MIRArgs.JobRev.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "[ERROR] sendMIRParams EVX_LotFileNameRev: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
    }

	// set MIR.EXEC_TYP
	if (!m_MIRArgs.ExecTyp.empty()) 
	{
		if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotSystemName, m_MIRArgs.ExecTyp.c_str());
		if (status != EVXA::OK) 
		{
			fprintf(stdout, "[ERROR] sendMIRParams EVX_LotSystemName: status:%d %s\n", status, pgm->getStatusBuffer());
			result = false;
		}
		fprintf(stdout, "MIR.ExecTyp: %s\n", pgm->getLotInformation(EVX_LotSystemName));
	}
 
	// set MIR.EXEC_VER
	if (!m_MIRArgs.ExecVer.empty()) 
	{
		if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotTargetName, m_MIRArgs.ExecVer.c_str());
		if (status != EVXA::OK) 
		{
			fprintf(stdout, "[ERROR] sendMIRParams EVX_LotTargetName: status:%d %s\n", status, pgm->getStatusBuffer());
			result = false;
		}
		fprintf(stdout, "MIR.ExecVer: %s\n", pgm->getLotInformation(EVX_LotTargetName));
	}
       
    if (!m_MIRArgs.AuxFile.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotAuxDataFile, m_MIRArgs.AuxFile.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "[ERROR] sendMIRParams EVX_LotAuxDataFile: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
    }

    if (!m_MIRArgs.TestCod.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotTestPhase, m_MIRArgs.TestCod.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "[ERROR] sendMIRParams EVX_LotTestPhase: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
    }

    if (!m_MIRArgs.UserText.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotUserText, m_MIRArgs.UserText.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "[ERROR] sendMIRParams EVX_LotUserText: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
    }

    if (!m_MIRArgs.RomCod.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotRomCode, m_MIRArgs.RomCod.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "[ERROR] sendMIRParams EVX_LotRomCode: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
    }

    if (!m_MIRArgs.SerlNum.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotTesterSerNum, m_MIRArgs.SerlNum.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "[ERROR] sendMIRParams EVX_LotTesterSerNum: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
                      fprintf(stdout, "MIR.SerlNum: %s\n", pgm->getLotInformation(EVX_LotTesterSerNum));
    }


    if (!m_MIRArgs.TstrTyp.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotTesterType, m_MIRArgs.TstrTyp.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "[ERROR] sendMIRParams EVX_LotTesterType: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
                      fprintf(stdout, "MIR.TstrTyp: %s\n", pgm->getLotInformation(EVX_LotTesterType));
    }

    if (!m_MIRArgs.SuprNam.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotSupervisor, m_MIRArgs.SuprNam.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "[ERROR] sendMIRParams EVX_LotSupervisor: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
    }

	// set MIR.SPEC_NAM
	if (!m_TPArgs.SpecNam.empty())
	{
		if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotTestSpecName, m_TPArgs.SpecNam.c_str());
		if (status != EVXA::OK) 
		{
			fprintf(stdout, "[ERROR] sendMIRParams EVX_LotTestSpecName: status:%d %s\n", status, pgm->getStatusBuffer());
			result = false;
		}
		fprintf(stdout, "MIR.SpecNam(From Load Recipe): %s\n", pgm->getLotInformation(EVX_LotTestSpecName));		
	}
	else if (!m_MIRArgs.SpecNam.empty()) 
	{
		if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotTestSpecName, m_MIRArgs.SpecNam.c_str());
		if (status != EVXA::OK) 
		{
			fprintf(stdout, "[ERROR] sendMIRParams EVX_LotTestSpecName: status:%d %s\n", status, pgm->getStatusBuffer());
			result = false;
		}
		fprintf(stdout, "MIR.SpecNam: %s\n", pgm->getLotInformation(EVX_LotTestSpecName));
	}

	// set MIR.SPEC_VER
	if (!m_TPArgs.SpecVer.empty())
	{
		if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotTestSpecRev, m_TPArgs.SpecVer.c_str());
		if (status != EVXA::OK) 
		{
			fprintf(stdout, "[ERROR] sendMIRParams EVX_LotTestSpecRev: status:%d %s\n", status, pgm->getStatusBuffer());
			result = false;
		}
		fprintf(stdout, "MIR.SpecVer(From Load Recipe): %s\n", pgm->getLotInformation(EVX_LotTestSpecRev));
	}
	else if (!m_MIRArgs.SpecVer.empty()) 
	{
		if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotTestSpecRev, m_MIRArgs.SpecVer.c_str());
		if (status != EVXA::OK) 
		{
			fprintf(stdout, "[ERROR] sendMIRParams EVX_LotTestSpecRev: status:%d %s\n", status, pgm->getStatusBuffer());
			result = false;
		}
		fprintf(stdout, "MIR.SpecVer: %s\n", pgm->getLotInformation(EVX_LotTestSpecRev));
	}


    if (!m_MIRArgs.ProtCod.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotProtectionCode, m_MIRArgs.ProtCod.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "[ERROR] sendMIRParams EVX_LotProtectionCode: status:%d %s\n", status, pgm->getStatusBuffer());
	    result = false;
	}
        fprintf(stdout, "MIR.ProtCod: %s\n", pgm->getLotInformation(EVX_LotProtectionCode));
    }
    
    return true;
}

//--------------------------------------------------------------------
/* This function takes the SDR parameters and sends them to 
   The Test Program.
*/
bool CuserEvxaInterface::sendSDRParams()
{
	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::sendSDRParams()" << std::endl;
  
    ProgramControl *pgm = PgmCtrl();
    if (NULL == pgm) {
	fprintf(stdout, "Error sendSDRParams: no ProgramControl\n");
	return false;
    }

    EVXAStatus status = EVXA::OK;
    bool result = true;
    if (!m_SDRArgs.HandTyp.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotHandlerType, m_SDRArgs.HandTyp.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "Error sendSDRParams set handler: %s\n", pgm->getStatusBuffer());
	    result = false;
	}
        fprintf(stdout, "SDR.HandTyp: %s\n", pgm->getLotInformation(EVX_LotHandlerType));
    }


    if (!m_SDRArgs.CardId.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotCardId, m_SDRArgs.CardId.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "Error sendSDRParams set card ID: %s\n", pgm->getStatusBuffer());
	    result = false;
	}
    }
    if (!m_SDRArgs.LoadId.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotLoadBrdId, m_SDRArgs.LoadId.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "Error sendSDRParams set Loadboard ID: %s\n", pgm->getStatusBuffer());
	    result = false;
	}
    }
    if (!m_SDRArgs.PHId.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotProberHandlerID, m_SDRArgs.PHId.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "Error sendSDRParams set Prober Handler ID: %s\n", pgm->getStatusBuffer());
	    result = false;
	}
    }

    if (!m_SDRArgs.DibTyp.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotDIBType, m_SDRArgs.DibTyp.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "[ERROR] sendSDRParams set LotDIBType: %s\n", pgm->getStatusBuffer());
	    result = false;
	}
    }

    if (!m_SDRArgs.CableId.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotIfCableId, m_SDRArgs.CableId.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "[ERROR] sendSDRParams set LotIfCableId: %s\n", pgm->getStatusBuffer());
	    result = false;
	}
    }

    if (!m_SDRArgs.ContTyp.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotContactorType, m_SDRArgs.ContTyp.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "[ERROR] sendSDRParams set LotContactorType: %s\n", pgm->getStatusBuffer());
	    result = false;
	}
    }

    if (!m_SDRArgs.LoadTyp.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotLoadBrdType, m_SDRArgs.LoadTyp.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "[ERROR] sendSDRParams set LotLoadBrdType: %s\n", pgm->getStatusBuffer());
	    result = false;
	}
    }

    if (!m_SDRArgs.ContId.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotContactorId, m_SDRArgs.ContId.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "[ERROR] sendSDRParams set LotContactorId: %s\n", pgm->getStatusBuffer());
	    result = false;
	}
    }

    if (!m_SDRArgs.LaserTyp.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotLaserType, m_SDRArgs.LaserTyp.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "[ERROR] sendSDRParams set LotLaserType: %s\n", pgm->getStatusBuffer());
	    result = false;
	}
    }

    if (!m_SDRArgs.LaserId.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotLaserId, m_SDRArgs.LaserId.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "[ERROR] sendSDRParams set LotLaserId: %s\n", pgm->getStatusBuffer());
	    result = false;
	}
    }
    if (!m_SDRArgs.ExtrTyp.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotExtEquipType, m_SDRArgs.ExtrTyp.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "[ERROR] sendSDRParams set LotExtEquipType: %s\n", pgm->getStatusBuffer());
	    result = false;
	}
    }
    if (!m_SDRArgs.ExtrId.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotExtEquipId, m_SDRArgs.ExtrId.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "[ERROR] sendSDRParams set LotExtEquipId: %s\n", pgm->getStatusBuffer());
	    result = false;
	}
    }
    if (!m_SDRArgs.DibId.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotActiveLoadBrdName, m_SDRArgs.DibId.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "[ERROR] sendSDRParams set LotActiveLoadBrdName: %s\n", pgm->getStatusBuffer());
	    result = false;
	}
    }
    if (!m_SDRArgs.CardTyp.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotCardType, m_SDRArgs.CardTyp.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "[ERROR] sendSDRParams set LotCardType: %s\n", pgm->getStatusBuffer());
	    result = false;
	}
    }
    if (!m_SDRArgs.CableTyp.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotIfCableType, m_SDRArgs.CableTyp.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "[ERROR] sendSDRParams set LotIfCableType: %s\n", pgm->getStatusBuffer());
	    result = false;
	}
        fprintf(stdout, "SDR.CableTyp: %s\n", pgm->getLotInformation(EVX_LotIfCableType));
    }



    return result;
}

//--------------------------------------------------------------------
/* This function takes the TP parameters and sends them to 
   The Test Program.
*/
bool CuserEvxaInterface::sendTPParams()
{
	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::sendTPParams()" << std::endl;
  
    ProgramControl *pgm = PgmCtrl();
    if (NULL == pgm) {
	fprintf(stdout, "Error sendTPParams: no ProgramControl\n");
	return false;
    }

    // Not sure what these parameters are used for.  
    // Check with ST>

    // ReloadStrategy;
    // DownloadStrategy
    // TPName;
    // RcpFileSupport;
    // Flow;
    // Salestype;
    // Temperature;
    // Product;
    // Parallelism;


    // Here's what I think should happen with some of them.

    EVXAStatus status = EVXA::OK;
    bool result = true;
    // Set the Active Flow to the Flow parameter.
    if (!m_TPArgs.Flow.empty()) {
	if (status == EVXA::OK) status = pgm->setActiveObject(EVX_ActiveFlow, m_TPArgs.Flow.c_str());
	if (status != EVXA::OK) {
	    fprintf(stdout, "Error sendTPParams set Flow: %s\n", pgm->getStatusBuffer());
	    result = false;
	}
   }
    // Set the Lot ID to the LotId paramter.
    if (!m_TPArgs.LotId.empty()) {
	if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotLotID, m_TPArgs.LotId.c_str());
 	if (status != EVXA::OK) {
	    fprintf(stdout, "Error sendTPParams set LotID: %s\n", pgm->getStatusBuffer());
	    result = false;
	}
    }
    
    return result;
}

//--------------------------------------------------------------------
/* This function will send end of lot to the tester if enabled 
*/
bool CuserEvxaInterface::sendStartOfLot()
{
    if (debug()) fprintf(stdout, "sendStartOfLot\n");

    ProgramControl *pgm = PgmCtrl();
    if (NULL == pgm) {
	fprintf(stdout, "Error sendStartOfLot: no ProgramControl\n");
	return false;
    }

    bool result = true;
    if ((m_TPArgs.StartLotEnable.compare("true") == 0) ||
	(m_TPArgs.StartLotEnable.compare("TRUE") == 0)) {
	setupWaitForNotification(EVX_LOT_CHANGE, EVX_LOT_START);	    
	    EVXAStatus status = pgm->setStartOfLot();
	    if (status == EVXA::OK) {
		waitForNotification();
	    }
	    else {
		fprintf(stdout, "Error sendStartOfLot setStartOfLot failed: %s\n", pgm->getStatusBuffer());
		result = false;

	    }
    }
    
    return result;
}

//--------------------------------------------------------------------
/* This function will send end of lot to the tester if enabled 
*/
bool CuserEvxaInterface::sendEndOfLot()
{
    if (debug()) fprintf(stdout, "sendEndOfLot\n");

    ProgramControl *pgm = PgmCtrl();
    if (NULL == pgm) {
	fprintf(stdout, "Error sendEndOfLot: no ProgramControl\n");
	return false;
    }

    bool result = true;
    if ((m_TPArgs.EndLotEnable.compare("true") == 0) ||
	(m_TPArgs.EndLotEnable.compare("TRUE") == 0)) {
	setupWaitForNotification(EVX_LOT_CHANGE, EVX_LOT_END);	    
	    EVXAStatus status = pgm->setEndOfLot(true);  // true is to send final summary.
	    if (status == EVXA::OK) {
		waitForNotification();
	    }
	    else {
		fprintf(stdout, "Error sendEndOfLot setEndOfLot failed: %s\n", pgm->getStatusBuffer());
		result = false;
	    }
	    if (status == EVXA::OK) status = pgm->setLotInformation(EVX_LotNextSerial, "1"); // reset the serial number to 1.
	    if (status != EVXA::OK) {
		fprintf(stdout, "Error sendEndOfLot setLotINformation failed: %s\n", pgm->getStatusBuffer());
	    }
    }
    
    return result;
}

//--------------------------------------------------------------------
/* This function will send end of wafer to the tester if enabled 
*/
bool CuserEvxaInterface::sendStartOfWafer()
{
    if (debug()) fprintf(stdout, "sendStartOfWafer\n");

    ProgramControl *pgm = PgmCtrl();
    if (NULL == pgm) {
	fprintf(stdout, "Error sendStartOfWafer: no ProgramControl\n");
	return false;
    }

    bool result = true;
    if ((m_TPArgs.StartWaferEnable.compare("true") == 0) ||
	(m_TPArgs.StartWaferEnable.compare("TRUE") == 0)) {
	setupWaitForNotification(EVX_WAFER_CHANGE, EVX_WAFER_START);	    
	EVXAStatus status = pgm->setStartOfWafer();  
	if (status == EVXA::OK) {
	    waitForNotification();
	}
	else {
	    fprintf(stdout, "Error sendStartOfWafer setStartOfWafer failed: %s\n", pgm->getStatusBuffer());
	    result = false;
	}
    }
    

    return result;
}

//--------------------------------------------------------------------
/* This function will send end of wafer to the tester if enabled 
*/
bool CuserEvxaInterface::sendEndOfWafer()
{
    if (debug()) fprintf(stdout, "sendEndOfWafer\n");

    ProgramControl *pgm = PgmCtrl();
    if (NULL == pgm) {
	fprintf(stdout, "Error sendEndOfWafer: no ProgramControl\n");
	return false;
    }

    bool result = true;
    if ((m_TPArgs.EndWaferEnable.compare("true") == 0) ||
	(m_TPArgs.EndWaferEnable.compare("TRUE") == 0)) {
	setupWaitForNotification(EVX_WAFER_CHANGE, EVX_WAFER_END);	    
	EVXAStatus status = pgm->setEndOfWafer();  
	if (status == EVXA::OK) {
	    waitForNotification();
	}
	else {
	    fprintf(stdout, "Error sendEndOfWafer setEndOfWafer failed: %s\n", pgm->getStatusBuffer());
	    result = false;
	}
    }
    

    return result;
}

//--------------------------------------------------------------------
/* This function updates the tester with what came from the recipe. 
This is called from recipeDecode after parsing.
And if a program is loadced this is called from programChange.
*/ 
bool CuserEvxaInterface::updateProgramLoad()
{
   	if (debug()) fprintf(stdout, "[DEBUG] Executing CuserEvxaInterface::updateProgramLoad() m_recipeParse:%d\n", m_recipeParse);
	bool result = true; // set to false when something bad occurs.

    	// If there was a recipe to parse then do the work.
    	if (m_recipeParse == false)
	{
		if (debug()) std::cout << "No recipe to load." << std::endl;
		return result;
	}

    	// check the download strategy and download the program accordingly
    	// check the reload strategy and load accordingly
    	if (m_TPArgs.TPName.empty() == false) 
	{
		if (debug()) fprintf(stdout, "[DEBUG] Test Program to be loaded: %s\n",  m_TPArgs.TPName.c_str());
		if (result == true) result = executeRecipeReload();
    	}
	else{std::cout << "No test program name to load." << std::endl;}

    	return result;
}

//--------------------------------------------------------------------
/* This function updates the test program data.
*/ 
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
  	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::executeRecipeReload()" << std::endl;
	bool result = true;
   	EVXAStatus status = EVXA::OK;

    	ProgramControl *pgm = PgmCtrl();
    	if (NULL == pgm) 
	{
		fprintf(stdout, "[ERROR] executeRecipeReload: no ProgramControl\n");
		return false;
    	}
    	
	bool pgmLoaded = pgm->isProgramLoaded();
    	if (EVXA::OK != status) 
	{
		fprintf(stdout, "[ERROR] error on call to isProgramLoaded(): %s\n", pgm->getStatusBuffer());
		return false;
    	}
    	
	if (pgmLoaded == true) 
	{
		std::string temp = pgm->getProgramPath();
		std::cout << "[OK] Test program already loaded with full path name: " << temp << std::endl;
		m_TPArgs.CurrentProgName = temp;
		//unsigned found = temp.find_last_of("/");
		//if (found != std::string::npos) m_TPArgs.CurrentProgName = temp.substr(found+1);
		//else m_TPArgs.CurrentProgName = temp;
		//fprintf(stdout, "Test Program already loaded with Name: %s\n", m_TPArgs.CurrentProgName.c_str());
    	}

	std::string dload(m_TPArgs.DownloadStrategy);
	std::string rload(m_TPArgs.ReloadStrategy);
	if(debug()) std::cout << "[DEBUG] download strategy: " << dload.c_str() << std::endl;
	if(debug()) std::cout << "[DEBUG] reload strategy: " << rload.c_str() << std::endl;

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
		if (! unloadProgram(m_TPArgs.FullTPName, false)) return false;
		else return true;		
	}

	// backtoidle = unloadAndDelete
	// unload TP and delete TP files in test folder
	if((m_TPArgs.BackToIdleStrategy.compare("unloadAndDelete") == 0) || (m_TPArgs.BackToIdleStrategy.compare("UnloadAndDelete") == 0))
	{ 
		if (! unloadProgram(m_TPArgs.FullTPName, false)) return false;
				
		// SAFELY delete TP files in test folder. make sure this string is not empty before proceeding
		if (!m_ConfigArgs.ProgLocation.empty()) 
		{
		    std::stringstream rmCmd;
		    rmCmd << "/bin/rm -rf " << m_ConfigArgs.ProgLocation << "/*";
		    if(debug()) fprintf(stdout, "clean localdir cmd:%s\n", rmCmd.str().c_str());
		    system(rmCmd.str().c_str());
		    if(debug()) fprintf(stdout, "clean localdir done\n");
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
 	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::loadProgram()" << std::endl;

	EVXAStatus status = EVXA::OK;
     	if (!PgmCtrl()){ std::cout << "[ERROR] Failed to access ProgramControl object." << std::endl; return false;}

	// load our program from XTRF
	if(debug()) std::cout << "[DEBUG] loading test program " <<  szProgFullPath << "..." << std::endl;
	EVX_PROGRAM_STATE states[] = { EVX_PROGRAM_LOADED, EVX_PROGRAM_LOAD_FAILED, EVX_PROGRAM_UNLOADED, MAX_EVX_PROGRAM_STATE };
	setupProgramNotification(states);
	status = PgmCtrl()->load(szProgFullPath.c_str(), EVXA::NO_WAIT, EVXA::NO_DISPLAY);

	if (EVXA::OK == status)
	{ 
		waitForNotification(); 
		std::cout << "[OK] test program " <<  szProgFullPath << " successfully loaded." << std::endl;
	}
	else 
	{
	    std::cout << "[ERROR] Something went wrong in loading test program " << szProgFullPath.c_str() << ": " << PgmCtrl()->getStatusBuffer() << std::endl;
	    return false;
	}
		
	return true;
}

/*---------------------------------------------------------------------------------
unload program
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::unloadProgram(const std::string &szProgFullPath, bool notify)
{
 	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::unloadProgram()" << std::endl;

	EVXAStatus status = EVXA::OK;
     	if (!PgmCtrl()){ std::cout << "[ERROR] Failed to access ProgramControl object." << std::endl; return false;}
	
	// If there's a program loaded then unload it.
	if (!szProgFullPath.empty()) 
	{
		if (debug()) std::cout << "[DEBUG] unloading test program " <<  szProgFullPath << "..." << std::endl;
	    	EVX_PROGRAM_STATE states[] = { EVX_PROGRAM_UNLOADED, MAX_EVX_PROGRAM_STATE };
	    	if (notify) setupProgramNotification(states);
	    	status = PgmCtrl()->unload(EVXA::NO_WAIT);  
		if (debug()) std::cout << "[DEBUG] Done unloading test program " <<  szProgFullPath << "..." << std::endl;		
	    	if (EVXA::OK != status) 
		{
	    		std::cout << "[ERROR] Something went wrong in unloading test program: " << PgmCtrl()->getStatusBuffer() << std::endl;
			return false;
	    	}
		if (debug()) std::cout << "[DEBUG] Done unloading test program " <<  szProgFullPath << "..." << std::endl;		
	    	if (notify) waitForNotification();
		if (debug()) std::cout << "[DEBUG] Done unloading test program " <<  szProgFullPath << "..." << std::endl;		
	    	if (!m_recipeParseResult){return m_recipeParseResult;}
		if (debug()) std::cout << "[DEBUG] Done unloading test program " <<  szProgFullPath << "..." << std::endl;		
	}	
	return true;
}

/*-----------------------------------------------------------------------------------------
check if file exist
-----------------------------------------------------------------------------------------*/
bool CuserEvxaInterface::isFileExist(const std::string& szFile)
{
	// check if the program is available remotely
	if (debug()) std::cout << "[DEBUG] Checking if " << szFile << " exists..." << std::endl;
     	if (access(szFile.c_str(), F_OK) > -1) { if (debug()) std::cout << "[OK] " << szFile << " exist." << std::endl; return true; }
   	else { std::cout << "[ERROR] Cannot access " << szFile << ". It does not exist." << std::endl; return false; }
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
		if (debug()) std::cout << "[OK] Test program <" << fullProgName.str() << "> is already loaded." << std::endl;
		return true;
	}
	else
	{
		std::cout << "[ERROR] Test program to load is <" << fullProgName.str() << ">. But current Program loaded is <" << m_TPArgs.CurrentProgName << ">" << std::endl;
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
	if(debug()) std::cout << "[DEBUG] copying " <<  from << " to " << to << "..." << std::endl;
	system(cpCmd.str().c_str());
	if(debug()) std::cout << "[DEBUG] Done copying. " << std::endl;	
}

/*---------------------------------------------------------------------------------
download program
---------------------------------------------------------------------------------*/
bool CuserEvxaInterface::downloadProgramFromServerToLocal()
{
  	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::downloadProgramFromServerToLocal()" << std::endl;
 
	//get the full path-filename-extension of the TP file from remote folder
   	std::stringstream fullRemoteProgPath;
    	if (!m_TPArgs.TPPath.empty())
	{ 
		fullRemoteProgPath << m_ConfigArgs.RemoteLocation << "/" << m_TPArgs.TPPath << "." << m_ConfigArgs.PackageType; 
		std::cout << "[OK] Downloading " << fullRemoteProgPath.str() << "..." << std::endl;
	}
	else { std::cout << "[ERROR] test program file name to download is invalid." << std::endl; return false; }
 
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
  	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::unpackProgramFromLocalToTest()" << std::endl;

	// get full path-filename-extension of TP file to local folder
	std::stringstream fullLocalProgPath;
	fullLocalProgPath << m_ConfigArgs.LocalLocation << "/" << m_TPArgs.TPPath << "." << m_ConfigArgs.PackageType; 

	// check if TP file exist in local folder
	if (!isFileExist(fullLocalProgPath.str())) return false;

	// chmod the TP file on local folder so we can unpack it
	std::stringstream chmodCmd;
	chmodCmd << "/bin/chmod +x " << fullLocalProgPath.str();
	if(debug()) std::cout << "chmod cmd: " << chmodCmd.str().c_str() << std::endl;
	system(chmodCmd.str().c_str());
	if(debug()) std::cout << "chmod done." << std::endl;
 
	// unpack to TP file from local folder into program folder
	std::stringstream tarCmd;
	tarCmd << "/bin/tar -C " << m_ConfigArgs.ProgLocation << " -xvf " << fullLocalProgPath.str();
	if(debug()) std::cout << "[DEBUG] unpacking with cmd: " << tarCmd.str() << std::endl;
	system(tarCmd.str().c_str());
	if(debug()) std::cout << "[DEBUG] unpackdone. " << std::endl;
 
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
 	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::forceDownloadAndLoad()" << std::endl;
  
    	// check if program is available remotely
    	if (downloadProgramFromServerToLocal())	
	{
		// If there's a program loaded then unload it.
		if (! unloadProgram(m_TPArgs.CurrentProgName)) return false;

		// load our program from XTRF
		if (!loadProgram(m_TPArgs.FullTPName)) return false;
	}	
    	else 
	{ 
		fprintf(stdout, "[ERROR] Failed to force download program\n");
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
	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::neverDownloadForceLoad()" << std::endl;
 
	// try unpacking TP file from local folder to test folder
	if (!unpackProgramFromLocalToTest())
	{
		// if failed, try downloading TP file from remote folder to local folder
		// this also unpack TP file from local folder to test folder
		if (!downloadProgramFromServerToLocal())	
		{
			std::cout << "[ERROR] Failed to download program." << std::endl;
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
	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::neverDownloadForceLoad()" << std::endl;
 
	// try unpacking TP file from local folder to test folder
	if (!unpackProgramFromLocalToTest())
	{
		// if failed, try downloading TP file from remote folder to local folder
		// this also unpack TP file from local folder to test folder
		if (!downloadProgramFromServerToLocal())	
		{
			std::cout << "[ERROR] Failed to download program." << std::endl;

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
	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::neverDownloadNeverLoad()" << std::endl;
 
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
	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::attemptDownloadForceLoad()" << std::endl;

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
	if (debug()) std::cout << "[DEBUG] Executing CuserEvxaInterface::attemptDownloadAttemptLoad()" << std::endl;

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
void CuserEvxaInterface::setupWaitForNotification(unsigned long wait_state, unsigned long wait_minor_state)
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
    for (int ii = 0; (wait_program_states[ii] != MAX_EVX_PROGRAM_STATE); ii++) {
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
    while (m_taskComplete == false) {
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
void CuserEvxaInterface::sendNotificationComplete(unsigned long wait_state, unsigned long wait_minor_state)
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


// PARSE THE RECIPEHANDLER_CONFIG.XML FILE
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------
/* This function parses the recipeHandler_cnfig.xml file
and stores the contents in variables.
*/
bool CuserEvxaInterface::parseRecipeHandlerConfigurationFile(const std::string& recipeFilePath)
{
   	if (debug()) fprintf(stdout, "[DEBUG] Executing CuserEvxaInterface::parseRecipeHandlerConfigurationFile() with %s as parameter\n", recipeFilePath.c_str());
    	bool result = true;

    	// clear all parameters that will come from recipehandler_config.xml
    	result = m_ConfigArgs.clearConfigParams();

    	// Now parse the file.  
    	XML_Node *rootNode = new XML_Node (recipeFilePath.c_str());
    	if (rootNode) 
	{
		std::string ptag = rootNode->fetchTag();
		if (debug()) fprintf(stdout, "rootNode tag: %s\n", ptag.c_str());

		// Add else-if statements for other rootNodes that need parsing
		// Then add a function to parse that rootNode.
		if (ptag.compare("RECIPEHANDLER_CONF") == 0){ result = parseRecipeHandlerConfiguration(rootNode);}
		else{fprintf(stdout, "rootNode unkown: %s, Not Parsed\n", ptag.c_str());}
    	}

    	// delete XML_Node
    	if (rootNode) 
	{
		delete rootNode;
		rootNode = NULL;
    	}
    	return result;
}

bool CuserEvxaInterface::parseRecipeHandlerConfiguration(XML_Node *recipeConfig)
{
   	if (debug()) fprintf(stdout, "[DEBUG] Executing CuserEvxaInterface::parseRecipeHandlerConfiguration()\n");
	bool result = true;
    
    	int numChildren = recipeConfig->numChildren();
    	if (debug()) fprintf(stdout, "recipeConfig numChildren: %d\n", numChildren);

    // Need to know what Recipe Config Children you want to parse.
    // Check for the ones we care about. 
    std::string configName("");
    for(int ii=0; ii<numChildren; ii++) {
	XML_Node *childNode = recipeConfig->fetchChild(ii);
	if (childNode) {
	    std::string ptag = childNode->fetchTag();
	    if (debug()) fprintf(stdout, "recipeConfig childNode tag: %s\n", ptag.c_str());

	    // Add else-if statements for other children that need parsing
	    // Then add a function to parse that child.
	    if (ptag.compare("CurrentSiteConfiguration") == 0) 
	    {
		 
		for (int ii=0; ii<childNode->numAttr(); ii++) {
		    if (debug()) {
			fprintf(stdout, "CurrentSiteConfiguration Attr %s: %s\n", 
				childNode->fetchAttr(ii).c_str(), childNode->fetchVal(ii).c_str());
		    }
		    if (childNode->fetchAttr(ii).compare("ConfigurationName") == 0) {
			configName = childNode->fetchVal(ii);

			m_ConfigArgs.ConfigurationName = configName;
			fprintf(stdout, "ConfigurationName: %s\n", m_ConfigArgs.ConfigurationName.c_str());
		    }
		}
	    }
	    else if(ptag.compare("SiteConfiguration") == 0) {
		for (int ii=0; ii<childNode->numAttr(); ii++) {
		    if (debug()) {
			fprintf(stdout, "Configuration Attr %s: %s\n", 
				childNode->fetchAttr(ii).c_str(), childNode->fetchVal(ii).c_str());
		    }
		    if (childNode->fetchAttr(ii).compare("ConfigurationName") == 0) {
		        if(childNode->fetchVal(ii).compare(configName) == 0) {
			    parseSiteConfiguration(childNode);
			    break; // break from for loop
			}
		    }
		}	    
	    }
	    else {
		fprintf(stdout, "recipeConfig unkown child: %s, Not Parsed\n", ptag.c_str());
	    }
	}
	else
	    fprintf(stdout, "recipeConfig: childNode is NULL\n");
    }
    return result;
}

bool CuserEvxaInterface::parseSiteConfiguration(XML_Node *siteConfig)
{
    if (debug()) fprintf(stdout, "parseSiteConfiguration\n");
    bool result = true;
    
    int numChildren = siteConfig->numChildren();
    if (debug()) fprintf(stdout, "siteConfig numChildren: %d\n", numChildren);

    // Need to know what Recipe Children you want to parse.
    // Check for the ones we care about.  
    for(int ii=0; ii<numChildren; ii++) {
	XML_Node *childNode = siteConfig->fetchChild(ii);
	if (childNode) {
	    std::string ptag = childNode->fetchTag();
	    if (debug()) fprintf(stdout, "siteConfig childNode tag: %s\n", ptag.c_str());

	    // Add else-if statements for other children that need parsing
	    // Then add a function to parse that child.
	    if (ptag.compare("argParameter") == 0) {
		for (int ii=0; ii<childNode->numAttr(); ii++) {
		    if (debug()) {
			fprintf(stdout, "argParameter Attr %s: %s\n", 
				childNode->fetchAttr(ii).c_str(), childNode->fetchVal(ii).c_str());
		    }
		    if (childNode->fetchAttr(ii).compare("RemoteLocation") == 0) {
			m_ConfigArgs.RemoteLocation = childNode->fetchVal(ii);
		    }
		    else if (childNode->fetchAttr(ii).compare("LocalLocation") == 0) {
			m_ConfigArgs.LocalLocation = childNode->fetchVal(ii);
		    }
		    else if (childNode->fetchAttr(ii).compare("ProgLocation") == 0) {
			m_ConfigArgs.ProgLocation = childNode->fetchVal(ii);
		    }
		    else if (childNode->fetchAttr(ii).compare("PackageType") == 0) {
			m_ConfigArgs.PackageType = childNode->fetchVal(ii);
		    }
		    else
			fprintf(stdout, "Unknown SiteConfig Parameter: %s: %s\n", childNode->fetchAttr(ii).c_str(), childNode->fetchVal(ii).c_str());
		}
	    }
	    else {
		fprintf(stdout, "siteConfig unkown child: %s, Not Parsed\n", ptag.c_str());
	    }
	}
	else
	    fprintf(stdout, "siteConfig: childNode is NULL\n");
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
