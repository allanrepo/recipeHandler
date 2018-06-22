#include "evxaTester.h"


using namespace evxaTester;


///////////// CMonitor ////////////////////

Cmonitor::Cmonitor() 
{
    m_timeout = 5000; // Defaulting to 5 seconds.
}

Cmonitor::~Cmonitor() 
{
    m_thread.stop();
}

bool Cmonitor::startListener(curi::utilities::CThreadCallback *cb) 
{
    bool ok = m_thread.setCallback(cb);

    if (!ok) { // Thread may be running
	m_thread.stop();
	ok = m_thread.setCallback(cb);
    }
    if (ok) ok = m_thread.start();
    return ok;
}

long Cmonitor::timeout(long t) 
{ 
    m_timeout = t; 
    return m_timeout; 
}

///////////// CStateNotify ////////////////////

CevxaTester::CStateNotify::CStateNotify(const TestheadConnection &thc, ConChangeAPI& notify) : StateNotification(thc), m_notify(notify) 
{
}

CevxaTester::CStateNotify::~CStateNotify() 
{
}

bool CevxaTester::CStateNotify::startListener(void) 
{ 	
    return Cmonitor::startListener(this); 
}

bool CevxaTester::CStateNotify::run(signed int userID, void *pUserData) 
{
    struct timespec tv;
    fd_set l_watchFd;

    while (!stopRequested()) {
	tv.tv_sec = 0;
	tv.tv_nsec = 40000000; // 40ms

	FD_ZERO(&l_watchFd);
	FD_SET( StateNotification::getSocketId(), &l_watchFd);

	if (1 == pselect((1+StateNotification::getSocketId()), &l_watchFd, NULL, NULL, &tv, NULL)) {
	    if (EVXA::OK != StateNotification::respond(StateNotification::getSocketId())) {
		fprintf(stdout, "Error StateNotification::respond: %s\n", StateNotification::getStatusBuffer());		
		return false; // Terminates thread.
	    }
	}
    }
    return true;
}

void CevxaTester::CStateNotify::dlogChange(const EVX_DLOG_STATE state) 
{
    CSemLock l(sem(), timeout());
    if (!l.locked()) return;

    m_notify.dlogChange(state);
}

void CevxaTester::CStateNotify::expressionChange(const char *expr_obj_name) 
{
    CSemLock l(sem(), timeout());
    if (!l.locked()) return;

    m_notify.expressionChange(expr_obj_name);
}

void CevxaTester::CStateNotify::expressionChange(const char *expr_obj_name, const char *expr_name,
                              const char *expr_value, int site) 
{
    CSemLock l(sem(), timeout());
    if (!l.locked()) return;

    m_notify.expressionChange(expr_obj_name, expr_name, expr_value, site);
}

void CevxaTester::CStateNotify::objectChange(const XClientMessageEvent xmsg) 
{
    CSemLock l(sem(), timeout());
    if (!l.locked()) return;

    m_notify.objectChange(xmsg);
}

void CevxaTester::CStateNotify::programChange(const EVX_PROGRAM_STATE state, const char *text_msg) 
 {
    CSemLock l(sem(), timeout());
    if (!l.locked()) return;

    m_notify.programChange(state, text_msg);
}

void CevxaTester::CStateNotify::modVarChange(const int id, char *value) 
{
    CSemLock l(sem(), timeout());
    if (!l.locked()) return;

    m_notify.modVarChange(id, value);
}

void CevxaTester::CStateNotify::programRunDone(const int array_size, int site[], int serial[], int swbin[],
				  int hwbin[], int pass[], LWORD dsp_status) 
{            
    CSemLock l(sem(), timeout());

    // We still need to delete
    if (l.locked()) m_notify.programRunDone(array_size, site, serial, swbin, hwbin, pass, dsp_status);

    // Here we will delete the structures after usage per the spec.
    if (NULL != site)   { delete []site;   site = NULL;   }
    if (NULL != serial) { delete []serial; serial = NULL; }
    if (NULL != swbin)  { delete []swbin;  swbin = NULL;  }
    if (NULL != hwbin)  { delete []hwbin;  hwbin = NULL;  }
    if (NULL != pass)   { delete []pass;   pass = NULL;   }
}

void CevxaTester::CStateNotify::restartTester(void) 
{
    CSemLock l(sem(), timeout());
    if (!l.locked()) return;

    m_notify.restartTester();
}

void CevxaTester::CStateNotify::evtcDisconnected(void) 
{
    CSemLock l(sem(), timeout());
    if (!l.locked()) return;

    m_notify.evtcDisconnected();
}

void CevxaTester::CStateNotify::evtcConnected(void) 
{
    CSemLock l(sem(), timeout());
    if (!l.locked()) return;

    m_notify.evtcConnected();
}

void CevxaTester::CStateNotify::streamChange(void) 
{
    CSemLock l(sem(), timeout());
    if (!l.locked()) return;

    m_notify.streamChange();
}

void CevxaTester::CStateNotify::tcBooting(void) 
{
    CSemLock l(sem(), timeout());
    if (!l.locked()) return;

    m_notify.tcBooting();
}

void CevxaTester::CStateNotify::testerReady(void) 
{
    CSemLock l(sem(), timeout());
    if (!l.locked()) return;

    m_notify.testerReady();
}

void CevxaTester::CStateNotify::gemRunning(void) 
{
    CSemLock l(sem(), timeout());
    if (!l.locked()) return;

    m_notify.gemRunning();
}

void CevxaTester::CStateNotify::alarmChange(const EVX_ALARM_STATE alarm_state, const ALARM_TYPE alarm_type,
			       const signed int time_occurred, const char *description) 
{
    CSemLock l(sem(), timeout());
    if (!l.locked()) return;

    m_notify.alarmChange(alarm_state, alarm_type, time_occurred, description);
}

void CevxaTester::CStateNotify::testerStateChange(const EVX_TESTER_STATE tester_state) 
{
    CSemLock l(sem(), timeout());
    if (!l.locked()) return;

    m_notify.testerStateChange(tester_state);
}

void CevxaTester::CStateNotify::waferChange(const EVX_WAFER_STATE wafer_state, const char *wafer_id) 
{
    CSemLock l(sem(), timeout());
    if (!l.locked()) return;

    m_notify.waferChange(wafer_state, wafer_id);
}

void CevxaTester::CStateNotify::lotChange(const EVX_LOT_STATE lot_state, const char *lot_id) 
{
    CSemLock l(sem(), timeout());
    if (!l.locked()) return;

    m_notify.lotChange(lot_state, lot_id);
}

void CevxaTester::CStateNotify::datalogChange(const EVX_DATALOG_STATE state, const char *datalogType, const char *identifier, const char *interim) 
{
    CSemLock l(sem(), timeout());
    if (!l.locked()) return;

    m_notify.datalogChange(state, datalogType, identifier, interim);
}


///////////// CstreamClient ////////////////////

CevxaTester::CstreamClient::CstreamClient(char *tester_name, int headNum, ConChangeAPI& notify) : EvxioStreamsClient(tester_name, headNum), m_notify(notify) 
{
}

CevxaTester::CstreamClient::~CstreamClient() 
{
}

bool CevxaTester::CstreamClient::startListener(void) 
{ 
    return Cmonitor::startListener(this); 
}

bool CevxaTester::CstreamClient::run(signed int userID, void *pUserData) 
{
    struct timespec tv;
    fd_set l_watchFd;

    while (!stopRequested()) {
	tv.tv_sec = 0;
	tv.tv_nsec = 40000000; // 40ms
	FD_ZERO(&l_watchFd);
	FD_SET( EvxioStreamsClient::getEvxioSocketId(), &l_watchFd);

	if (1 == pselect((1+EvxioStreamsClient::getEvxioSocketId()), &l_watchFd, NULL, NULL, &tv, NULL)) {
	    if (EVXA::OK != EvxioStreamsClient::streamsRespond()) {
		fprintf(stdout, "Error EvxioStreamsClient::streamsRespond: %s\n", EvxioStreamsClient::getStatusBuffer());
		return false; // Terminates thread.
	    }
	}
    }
    return true;
}

void CevxaTester::CstreamClient::EvxioMessage(int responseNeeded, int responseAquired, char *evxio_msg) 
{
    CSemLock l(sem(), timeout());
    if (!l.locked()) return;

    m_notify.EvxioMessage(responseNeeded, responseAquired, evxio_msg);
}


///////////// CCommandNotify ////////////////////

CevxaTester::CCmdNotify::CCmdNotify(const TestheadConnection &thc, ConChangeAPI& notify) : CommandNotification(thc), m_notify(notify) 
{
}

CevxaTester::CCmdNotify::~CCmdNotify() 
{
}

bool CevxaTester::CCmdNotify::startListener(void) 
{ 
    return Cmonitor::startListener(this); 
}

bool CevxaTester::CCmdNotify::run(signed int userID, void *pUserData) 
{
    struct timespec tv;
    fd_set l_watchFd;

    while (!stopRequested()) {
	tv.tv_sec = 0;
	tv.tv_nsec = 40000000; // 40ms

	FD_ZERO(&l_watchFd);
	FD_SET( CommandNotification::getSocketId(), &l_watchFd);

	if (1 == pselect((1+CommandNotification::getSocketId()), &l_watchFd, NULL, NULL, &tv, NULL)) {
	    if (EVXA::OK != CommandNotification::respond(CommandNotification::getSocketId())) {
		fprintf(stdout, "Error CommandNotification::respond: %s\n", CommandNotification::getStatusBuffer());
		return false; // Terminates thread.
	    }
	}
    }
    return true;
}

void CevxaTester::CCmdNotify::RecipeDecodeAvailable(const char *recipe_text, bool &result) 
{
    CSemLock l(sem(), timeout());
    if (!l.locked()) return;

    m_notify.RecipeDecodeAvailable(recipe_text, result);
}

void CevxaTester::CCmdNotify::RecipeDecode(const char *recipe_text) 
{
    CSemLock l(sem(), timeout());
    if (!l.locked()) return;

    m_notify.RecipeDecode(recipe_text);
}

void CevxaTester::CCmdNotify::RecipeStatus(const EVX_RECIPE_PARSE_STATUS recipe_status) 
{
    CSemLock l(sem(), timeout());
    if (!l.locked()) return;

    m_notify.RecipeStatus(recipe_status);
}


///////////// CevxaTester ////////////////////

CevxaTester::CevxaTester() 
{ 
    commonInit();
}
CevxaTester::CevxaTester(const std::string& tester_name) 
{
    commonInit();
    m_testerName = tester_name;
}
  
CevxaTester::CevxaTester(const std::string& tester_name, int headNum) 
{ 
    commonInit();
    m_testerName = tester_name;
    m_headNumber = headNum;
}

CevxaTester::~CevxaTester() 
{
    disconnect(false);
}


void CevxaTester::commonInit(void) 
{
    m_testerName       = "";
    m_headNumber       = 1;
    m_testerConnect    = NULL;
    m_testHeadConnect  = NULL;
    m_internTHC        = NULL;
    m_programCtrl      = NULL;
    m_states           = NULL;
    m_evxioStreams     = NULL;
    m_cmdNotify        = NULL;
    m_clientNotify     = NULL;
}

bool CevxaTester::haveTestHeadConnection(void) 
{
    CSemLock l(m_sem, 1000);
    if (!l.locked()) {
	return false;
    }
    if (NULL != m_testHeadConnect) {
	m_testHeadConnect->clearStatus();
	int result = m_testHeadConnect->isTesterReady();
	if (EVXA::OK == m_testHeadConnect->getStatus()) {
	    return (result > 0);
	}
	return false;
    }

    return false;
}

bool CevxaTester::onChange(ConChangeAPI *onChange) 
{
    if (m_clientNotify) return false;
    m_clientNotify = onChange;
    return true;
}
    
bool CevxaTester::connect(const std::string tester, int head,  bool restartIfNeeded, const char *restartOptions) 
{
    CSemLock l(m_sem, 1000); // After disconnect, as it will try to grab the sem also.
    if (haveTesterConnection()) disconnect();

    if (tester.length() < 1) return false;
    if (head < 1) return false;

    m_testerName = tester;
    m_headNumber = head;
    bool ok = true;

    try { ok = (NULL != (m_testerConnect = new TesterConnection(m_testerName.c_str()))); }
    catch (...) { ok = false; }

    bool l_needRestart = false;
    if (ok) {
	int l_stat = m_testerConnect->getStatus();
	if (EVXA::OK != l_stat) {
	    if (EVXA::NOT_EXIST == l_stat) l_needRestart = true;
	    else if (!m_testerConnect->isTesterReady(head)) l_needRestart = true;
	    else {
		ok = false;
		delete m_testerConnect;
		m_testerConnect = NULL;
	    }
	}
    } 

    if (ok && l_needRestart) {
	if (restartIfNeeded) ok = (EVXA::OK == m_testerConnect->restartTester((restartOptions ? restartOptions : "")));
	else ok = false;
    }

    if (ok) ok = (NULL != (m_testHeadConnect = new TestheadConnection(*m_testerConnect, m_headNumber)));
    if (ok) ok = (NULL != (m_programCtrl = new ProgramControl(*m_testHeadConnect)));

    bool listenerOk = false;
    if (ok && (NULL != m_clientNotify)) listenerOk = startListener(m_clientNotify, false);
     
    return (ok && listenerOk);
}

bool CevxaTester::registerCommandNotification(const int arr_size, const EVX_NOTIFY_COMMANDS evx_cmd[]) 
{
    bool result = false;
    if (m_cmdNotify) {
	EVXAStatus status = m_cmdNotify->registerNotification(arr_size, evx_cmd);
	if (status == EVXA::OK)
	    result = true;
    }
    return result;
}

const char *CevxaTester::getRecipeParseStatusName(EVX_RECIPE_PARSE_STATUS state)
{
    if (m_cmdNotify) {
	return m_cmdNotify->getRecipeParseStatusName(state);
    }
    else
	return ""; 
}

bool CevxaTester::startListener(ConChangeAPI *onChange, bool needSemaphore) 
{
    CSemLock l(m_sem, 1000, needSemaphore);

    stopListener(false); // Stop if running.

    if (NULL != onChange) m_clientNotify = onChange; 

    if ((m_internTHC != NULL) || (m_evxioStreams != NULL)) return false;

    bool ok = (NULL != (m_internTHC = new TestheadConnection(m_testerName.c_str(), m_headNumber)));
     
    if (ok && (NULL != m_clientNotify)) {

	if (NULL != (m_states = new CStateNotify(*m_internTHC, *m_clientNotify))) {
	    ok = m_states->startListener();
	}

	if (ok) {
	    if (NULL != (m_cmdNotify = new CCmdNotify(*m_internTHC, *m_clientNotify))) {
		ok = m_cmdNotify->startListener();
	    }
	}
	if (ok) {
	    ok = false;

	    // Normally should not re-delcare from const char to char.
	    if (NULL != (m_evxioStreams = new CstreamClient((char *)m_testerName.c_str(), m_headNumber, *m_clientNotify))) {
		int pid = getpid();
		char name[64];
		sprintf(name, "client_%d", pid);
		if (m_evxioStreams->ConnectEvxioStreams(m_internTHC, name) == EVXA::OK) { 
		    ok = m_evxioStreams->startListener();
		}
		else {
		    delete m_evxioStreams;
		    m_evxioStreams = NULL;
		}
	    }
	}
    }

    return (NULL != m_states); // Streams not connecting is not fatal
}

bool CevxaTester::stopListener(bool needSemaphore) 
{
    CSemLock l(m_sem, 1000, needSemaphore);

    if (NULL != m_evxioStreams) {
	delete m_evxioStreams;
	m_evxioStreams = NULL;
    }

    if (NULL != m_cmdNotify) {
	delete m_cmdNotify;
	m_cmdNotify = NULL;
    }

    if (NULL != m_states) {
	delete m_states;
	m_states = NULL;
    }


    if (NULL != m_internTHC) {
	delete m_internTHC;
	m_internTHC = NULL;
    }

    return true;
}

bool CevxaTester::disconnect(bool needSemaphore) 
{        
    CSemLock l(m_sem, 1000, needSemaphore);

    stopListener(false);
    if (NULL != m_testHeadConnect) {
	delete m_testHeadConnect;
	m_testHeadConnect = NULL;
    }

    if (NULL != m_programCtrl) {
	delete m_programCtrl;
	m_programCtrl = NULL;
    }

    if (NULL != m_testerConnect) {
	delete m_testerConnect;
	m_testerConnect = NULL;
    }

    if (NULL != m_internTHC) {
	delete m_internTHC;
	m_internTHC = NULL;
    }

    return true;
}
