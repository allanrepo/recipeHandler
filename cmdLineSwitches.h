#ifndef _CMD_LINE_SWITCHES_HEADER_INCLUDED_

#include <argSwitches.h>

class CuserCmdSwitches : public utilities::args::argSwitches {
private:

	void commonInit(void) {
	}
public:
	CuserCmdSwitches() { commonInit(); }
	CuserCmdSwitches(int argc, char **argv) { commonInit(); scanArgs(argc, argv); }
	virtual ~CuserCmdSwitches() {}

	bool useArgs(int argc, char **argv) { return scanArgs(argc, argv); }

	bool initArgList(void) {
            addArg("t", "tester", "Specifies the target tester, if not set, the environment variable LTX_TESTER will be checked, followed by <username>_sim", true, "tester name");
            addArg("hd", "head", "Specifies the target test head, if not set, default to 1", true, "test head");
            addArg("d", "debug", "Enable debug logic", false, "debug");
            addArg("dm", "", "Enable debug logic", false, "dm");
            addArg("v", "version", "Display the version and exit", false, "version");
            addArg("c", "continue", "Continue to monitor after tester goes down for next time tester comes up (no exit)");
            addArg("r", "recipe", "Program recipe to load", true, "recipe");
            addArg("h", "help", "Print these messages");
            addArg("conf", "config", "config file and path", true, "config");
            addArg("s10f1", "s10f1", "enable sending S10F1 event and error reporting", false, "s10f1");
            return true;
	}

	bool haveTesterName(void) { return isSet("tester"); }
	const std::string& testerName(const std::string& tName) { return setArg("tester", tName); }
	const std::string& testerName(void) { return getArg("tester"); }

	bool haveTestHead(void) { return isSet("head"); }
	unsigned long testHead(void) { unsigned long ul = 1; if (1 != sscanf(getArg("head").c_str(), "%lud", &ul)) ul = 1; return ul; }
        unsigned long testHead(unsigned long head) { char buf[128]; sprintf(buf, "%lud", head); setArg("head", buf); return testHead(); }

	bool haveConfig(void) { return isSet("config"); }
	const std::string& config(const std::string& tName) { return setArg("config", tName); }
	const std::string& config(void) { return getArg("config"); }

	bool debug(void) { return isSet("debug"); }
	bool debug(bool b) { return setArg("debug"); }
	bool showVersion(void) { return isSet("version"); }
	bool showVersion(bool b) { return setArg("version"); }
	bool s10f1() { return isSet("s10f1"); }


};

#define _CMD_LINE_SWITCHES_HEADER_INCLUDED_
#endif // _CMD_LINE_SWITCHES_HEADER_INCLUDED_
