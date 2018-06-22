#ifndef _EVXA_TESTER_HEADER_INCLUDED_
#define _EVXA_TESTER_HEADER_INCLUDED_

#include <cmos_curi_platform.h>
#include <cmos_PrecisionTimer.h>
#include <curi_utils_thread.h>


#include <evxa/ProgramControl.hxx>
#include <evxa/StateNotification.hxx>
#include <evxa/EvxioStreamsClient.hxx>
#include <evxa/CommandNotification.hxx>


#ifdef _USE_NEW_CURI_LOCK_
typedef curi::utilities::EntryLock<curi::utilities::CsemaphoreCtrl> CSemLock;
#endif

namespace evxaTester { // Make sure we do not step on any other classes

// Used to allow everything for the tester to be controlled in one class, CevxaTester.

class ConChangeAPI {
protected:
    ConChangeAPI *getPtr(void) { return this; }
    ConChangeAPI& getReference(void) { return *this; }
public:
    virtual ~ConChangeAPI() {}

    virtual void dlogChange(const EVX_DLOG_STATE state) {}
    virtual void expressionChange(const char *expr_obj_name) {}
    virtual void expressionChange(const char *expr_obj_name, const char *expr_name,
                                  const char *expr_value, int site) {}
    virtual void objectChange(const XClientMessageEvent xmsg) {}
    virtual void programChange(const EVX_PROGRAM_STATE state, const char *text_msg) {}
    virtual void modVarChange(const int id, char *value) {}

    virtual void programRunDone(const int array_size,
                            int site[],
                            int serial[],
                            int swbin[],
                            int hwbin[],
                            int pass[],
                            LWORD dsp_status
                            ) {}

    virtual void restartTester(void) {}
    virtual void evtcDisconnected(void) {}
    virtual void evtcConnected(void) {}
    virtual void streamChange(void) {}
    virtual void tcBooting(void) {}
    virtual void testerReady(void) {}
    virtual void gemRunning(void) {}
    virtual void alarmChange(const EVX_ALARM_STATE alarm_state, const ALARM_TYPE alarm_type,
                             const signed int time_occurred, const char *description) {}

    virtual void testerStateChange(const EVX_TESTER_STATE tester_state) {}
    virtual void waferChange(const EVX_WAFER_STATE wafer_state, const char *wafer_id) {}
    virtual void lotChange(const EVX_LOT_STATE lot_state, const char *lot_id) {}
    virtual void datalogChange(const EVX_DATALOG_STATE state, const char *datalogType, const char *identifier, const char *interim) {}

    virtual void EvxioMessage(int responseNeeded, int responseAquired, char *evxio_msg) {}

    virtual void RecipeDecodeAvailable(const char *recipe_text, bool &result) {}
    virtual void RecipeDecode(const char *recipe_text) {}
    virtual void RecipeStatus(const EVX_RECIPE_PARSE_STATUS recipe_status) {}

};

class Cmonitor : public curi::utilities::CThreadCallback {
private:
        curi::utilities::CThread m_thread;
        signed int m_timeout;
        curi::utilities::CsemaphoreCtrl m_sem;
public:

        Cmonitor();
        virtual ~Cmonitor();

        bool startListener(curi::utilities::CThreadCallback *cb);
        long timeout(long t);

        inline curi::utilities::CsemaphoreCtrl& sem(void) { return m_sem; }
        inline signed int timeout(void) const { return m_timeout; }
        inline curi::utilities::CThread& thread(void) { return m_thread; }

};

class CevxaTester : public ConChangeAPI {
private:

///////////// CStateNotify ////////////////////
//
// A Wrapper to StateNotification that handles the threading, locking and deleting of program run done info.
// A reference to ConChangeAPI is required for this class.
//
    class CStateNotify : public StateNotification, public Cmonitor {
   private:
        ConChangeAPI& m_notify;

        CStateNotify();
    public:

        CStateNotify(const TestheadConnection &thc, ConChangeAPI& notify);
	virtual ~CStateNotify();

        // Register our local run() function.
        bool startListener(void);

        // Thread listener
	bool run(signed int userID, void *pUserData);

	void dlogChange(const EVX_DLOG_STATE state);
	void expressionChange(const char *expr_obj_name);
	void expressionChange(const char *expr_obj_name, const char *expr_name,
                              const char *expr_value, int site);
        void objectChange(const XClientMessageEvent xmsg);
        void programChange(const EVX_PROGRAM_STATE state, const char *text_msg);
	void modVarChange(const int id, char *value);
	void programRunDone(const int array_size, int site[], int serial[], int swbin[],
			    int hwbin[], int pass[], LWORD dsp_status);
	void restartTester(void);
	void evtcDisconnected(void);
	void evtcConnected(void);
        void streamChange(void);
        void tcBooting(void);
        void testerReady(void);
        void gemRunning(void);
        void alarmChange(const EVX_ALARM_STATE alarm_state, const ALARM_TYPE alarm_type,
			 const signed int time_occurred, const char *description);
        void testerStateChange(const EVX_TESTER_STATE tester_state);
        void waferChange(const EVX_WAFER_STATE wafer_state, const char *wafer_id);
        void lotChange(const EVX_LOT_STATE lot_state, const char *lot_id);
        void datalogChange(const EVX_DATALOG_STATE state, const char *datalogType, const char *identifier, const char *interim);
    };


///////////// CstreamClient ////////////////////
//
// A Wrapper to EvxioStreamsClient that handles the threading and locking.
// A reference to ConChangeAPI is required for this class.
//
    class CstreamClient : public EvxioStreamsClient, public Cmonitor {
    private:
        ConChangeAPI& m_notify;

        CstreamClient();

    public:
	CstreamClient(char *tester_name, int headNum, ConChangeAPI& notify);
	virtual ~CstreamClient();

	// Register our local run() function.
        bool startListener(void);

        // Thread listener
        bool run(signed int userID, void *pUserData);
        void EvxioMessage(int responseNeeded, int responseAquired, char *evxio_msg);
    };

///////////// CCommandNotify ////////////////////
//
// A Wrapper to CommandNotification that handles the threading, locking and deleting command info if required.
// A reference to ConChangeAPI is required for this class.
//
    class CCmdNotify : public CommandNotification, public Cmonitor {
    private:
        ConChangeAPI& m_notify;

        CCmdNotify();
    public:

        ///////////////
        //
        //

        CCmdNotify(const TestheadConnection &thc, ConChangeAPI& notify);
        virtual ~CCmdNotify();

        // Register our local run() function.
        bool startListener(void);

        // Thread listener
        bool run(signed int userID, void *pUserData);
        void RecipeDecodeAvailable(const char *recipe_text, bool &result);
	void RecipeDecode(const char *recipe_text);
        void RecipeStatus(const EVX_RECIPE_PARSE_STATUS recipe_status);
    };

private:
    std::string m_testerName;
    int m_headNumber;
    TesterConnection *m_testerConnect;
    TestheadConnection *m_testHeadConnect, *m_internTHC;
    ProgramControl *m_programCtrl;
    CStateNotify *m_states;
    CstreamClient *m_evxioStreams;
    CCmdNotify *m_cmdNotify;
    ConChangeAPI *m_clientNotify;
    curi::utilities::CsemaphoreCtrl m_sem;

    void commonInit(void);

    // Semaphores are closly controlled, so make functions that use semaphore control internal.
    bool startListener(ConChangeAPI *onChange, bool needSemaphore);
    bool stopListener(bool needSemaphore);
    bool disconnect(bool needSemaphore);

public:
    CevxaTester();	
    CevxaTester(const std::string& tester_name);
    CevxaTester(const std::string& tester_name, int headNum);
    virtual ~CevxaTester();

    inline const std::string& testerName(void) { return m_testerName; }
    inline int head(void) { return m_headNumber; }
    inline bool haveProgramControl(void) { return (NULL != m_programCtrl); }
    inline ProgramControl *progCtrl(void) { return m_programCtrl; }

    inline bool haveTesterConnection(void) { return (NULL != m_testerConnect); }
    inline TesterConnection *evxaTester(void) { return m_testerConnect; }

    inline bool haveTestHead(void) { return (NULL != m_testHeadConnect); }
    inline TestheadConnection *testHead(void) { return m_testHeadConnect; }

    inline bool connect(bool restartIfNeeded = false, const char *restartOptions = NULL) {
        return connect(m_testerName, m_headNumber, restartIfNeeded, restartOptions);
    }
    inline bool connect(const std::string tester,  bool restartIfNeeded = false, const char *restartOptions = NULL) {
        return connect(tester, m_headNumber, restartIfNeeded, restartOptions);
    }
    inline bool disconnect(void) { return disconnect(true); }
    inline bool startListener(ConChangeAPI *onChange = NULL) { return startListener(onChange, true); }
    inline bool stopListener(void) { return stopListener(true); }

    bool haveTestHeadConnection(void);
    bool onChange(ConChangeAPI *onChange);
    bool connect(const std::string tester, int head,  bool restartIfNeeded = false, const char *restartOptions = NULL);
    bool registerCommandNotification(const int arr_size, const EVX_NOTIFY_COMMANDS evx_cmd[]);
    const char *getRecipeParseStatusName(EVX_RECIPE_PARSE_STATUS state);
};

}; // End namespace evxaTester

#endif // _EVXA_TESTER_HEADER_INCLUDED_
