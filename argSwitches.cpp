#include "argSwitches.h"
#include <sstream>
#include <string.h>
#include <stdio.h>

using namespace utilities::args;

unsigned long argSwitches::argOption::m_shortOptWidth = 0;
unsigned long argSwitches::argOption::m_longOptWidth = 0;
unsigned long argSwitches::argOption::m_paramDescWidth = 0;
const std::string argSwitches::argOption::m_emptyStr("");


unsigned long formatStringToVectorList(const std::string& str, std::vector<std::string>& formatVecList, unsigned long width);

unsigned long formatStringToVectorList(const std::string& str, std::vector<std::string>& formatVecList, unsigned long width)
{
	if (width) {
		// Lets split the string up
		if (str.length() > width) {
			std::istringstream ds(str);
			std::vector<std::string> wordList;
			while (!ds.eof()) {
				std::string nextStr;
				ds >> nextStr;
				wordList.push_back(nextStr);
			}

			// Rebuild the strings based on the lenght now.
			std::string nextLine("");
			for (size_t idx = 0; idx < wordList.size(); idx++) {
				if ((nextLine.length() + wordList[idx].length() + 1) > width) {	// +1 for space
					if (nextLine.length() < 1) formatVecList.push_back(wordList[idx]); // This is a long word, so add and push
					else { // Push back and reset nextLine.
						formatVecList.push_back(nextLine);
						nextLine = wordList[idx]; // Full reset.
					}
				} else {
					// Add word to last line.
					if (nextLine.length() > 0) nextLine += std::string(" ");
					nextLine += wordList[idx];
				}
			}
			// We still have the last line, add it.
			formatVecList.push_back(nextLine);
		} else formatVecList.push_back(str);
	} else formatVecList.push_back(str);


	return(unsigned long) formatVecList.size();
}

argSwitches::argOption::argOption()
{
	m_shortOption = "";
	m_longOption = "";
	m_paramName = "";
	m_description = "";
	m_strValue = "";
	m_isSet = false;
	m_hasParam = false;
	m_hiddenArg = false;

}

argSwitches::argOption::argOption(const std::string& shortOpt, const std::string& longOpt, const std::string& desc, bool hasParam, const std::string& paramName, bool hiddenArg)
{
	m_shortOption = shortOpt;
	m_longOption = longOpt;
	m_description = desc;
	m_strValue = "";
	m_isSet = false;
	m_hasParam = hasParam;
	m_hiddenArg = hiddenArg;
	if (m_hasParam)	m_paramName = paramName;
	else m_paramName = "";

	if (m_shortOption.length() > m_shortOptWidth) m_shortOptWidth = (unsigned long) m_shortOption.length();
	if (m_longOption.length() > m_longOptWidth)	m_longOptWidth = (unsigned long) m_longOption.length();
	if (m_paramName.length() > m_paramDescWidth) m_paramDescWidth = (unsigned long) m_paramName.length();
}

argSwitches::argOption::~argOption()
{
	m_shortOption.clear();
	m_longOption.clear();
	m_description.clear();
	m_paramName.clear();
	m_isSet = false;
}


const std::string& argSwitches::argOption::description(const std::string& desc)
{
	m_description = desc;
	return m_description;
}

bool argSwitches::argOption::hasParam(bool b, const std::string& param)
{
	m_hasParam = b; 
	if (m_hasParam) {
		m_paramName = param;
		if (m_paramName.length() > m_paramDescWidth) m_paramDescWidth = (unsigned long) m_paramName.length();
	}
	return m_hasParam; 
}

bool argSwitches::argOption::displayHelp(std::ostream& os, unsigned long outputWidth)
{
	// just dump for the moment
	if (m_hiddenArg) return true; // Early abort

	unsigned int sw, lw, pw, dw, gw;
	sw = m_shortOptWidth + 4;
	lw = m_longOptWidth + 3;
	pw = m_paramDescWidth + 3;
	gw = sw + lw + pw;
	if (gw < outputWidth) dw = outputWidth - gw;
	else dw	= 0;

	// Get the current left/right allignment to restore after this function.

	std::ostream::fmtflags currentStreamFlags = os.flags();

	os.width(sw);
	if (m_shortOption.length() > 0) {
		std::string outStr("-");
		outStr += m_shortOption + std::string(",");
		os << std::right << outStr;
	} else os << "";

	os.width(lw);
	if (m_longOption.length() > 0) {
		std::string outStr(" --");
		outStr += m_longOption;
		os <<  std::left << outStr;
	} else os << "";

	os.width(pw);
	if (m_paramName.length() > 0) {
		std::string outStr(" <");
		outStr += m_paramName + std::string(">");
		os << outStr;
	} else os << "";

	// Now need to check descripion and line wrapping.
	std::vector<std::string> descLines;

	formatStringToVectorList(m_description, descLines, dw);

	for (size_t idx = 0; idx < descLines.size(); idx++) {
		if (idx) { // Not first line
			os.width(gw);
			os << ""; // Justify
		}
		os << " " << descLines[idx] << std::endl;
	}

	// restore the flags now
	os.flags(currentStreamFlags);

	return true;
}

//////////////////
// 
// 
// 

void argSwitches::common_init(void)
{
	try {
		m_args.clear();
	} catch (...) {
	}
	try {
		m_longArgs.clear();
	} catch (...) {
	}
	try {
		m_shortArgs.clear();
	} catch (...) {
	}
	m_cmd = "";
	try {
		m_globalDescriptions.clear();
	} catch (...) {
	}
	try {
		m_globalExamples.clear();
	} catch (...) {
	}
	try {
		m_globalSeeAlso.clear();
	} catch (...) {
	}
	try {
		m_unusedArgs.clear();
	} catch (...) {
	};
	m_unusedArgsPtr = NULL;

	m_initNeeded = true;
	m_scannedCount = 0;
	m_useLastForScan = false;
	m_LastArgForScan = "";
	m_lastScanArgIsLongForm = true;
}

argSwitches::argSwitches()
{
	common_init(); // clears all the lists.
}
void argSwitches::clearUnused(void)
{
	if (m_unusedArgsPtr != NULL) {
		for (size_t idx = 0; idx < m_unusedArgs.size(); idx++) {
			delete m_unusedArgsPtr[idx];
		}
		delete m_unusedArgsPtr;
		m_unusedArgsPtr = NULL;
	}
	try {
		m_unusedArgs.clear();
	} catch (...) {
	};
}

bool  argSwitches::setLastScanArg(const std::string& lastArg, bool useLongArg)
{
	if (lastArg.length() < 1) {
		m_useLastForScan = false;
	} else {
		m_useLastForScan = true;
		m_LastArgForScan = lastArg;
		m_lastScanArgIsLongForm = useLongArg;
	}
	return m_useLastForScan;
}


char **argSwitches::argv(void)
{
	if (m_unusedArgsPtr == NULL) {
		if (m_unusedArgs.size() == 0) return NULL; // scanArgs was never called, break out now.

		m_unusedArgsPtr = new char * [m_unusedArgs.size()];

		for (size_t idx = 0; idx < m_unusedArgs.size(); idx++) {
			size_t len = m_unusedArgs[idx].length();
			m_unusedArgsPtr[idx] = new char [len+1];
			memset(m_unusedArgsPtr[idx], 0, len+1);
			memcpy(m_unusedArgsPtr[idx], m_unusedArgs[idx].c_str(), len);
		}
	}

	return m_unusedArgsPtr;
}

argSwitches::~argSwitches()
{
	clearUnused();
	// Delete the args

	try {
		for (size_t idx = 0; idx < m_args.size() ; idx++) {
			delete m_args[idx];
		}
		m_args.clear();
	} catch (...) {
	}
	common_init(); // clears all the lists.
}

bool argSwitches::initArgList(void)
{
	return true; // User can override this function to trigger on arg events.
}

bool argSwitches::argUpdate(const std::string& shortOpt, const std::string& longOpt, const std::string& desc, bool hasValue, const std::string& value)
{
	return true; // User can override this function to trigger on arg events.
}


bool argSwitches::scanArgs(int argc, char **argv, bool replaceCommand, const std::string& replaceWith)
{
	if ((argc == 0) || (argv == NULL)) return false;

	clearUnused();

	if (replaceCommand) {
		m_unusedArgs.push_back(replaceWith); // Grab the command line.
	} else {
		m_unusedArgs.push_back(std::string(argv[0])); // Grab the command line.
	}

	// put the command line into the member variable for later use in the help menus.
	size_t fnameStart = m_unusedArgs[0].find_last_of("/\\");
	if (fnameStart != std::string::npos) m_cmd = m_unusedArgs[0].substr(fnameStart+1);
	else m_cmd = m_unusedArgs[0];

	if (m_initNeeded) {
		initArgList(); m_initNeeded = false;
	}

	str2argOptionPtr::iterator testArg;

	char *p;
	bool found = false;
	bool earlyBreak = false;
	str2argOptionPtr::iterator testEarly = m_longArgs.end();
	if (m_useLastForScan) {
		if (m_lastScanArgIsLongForm) testEarly = m_longArgs.find(m_LastArgForScan);
		else testEarly = m_shortArgs.find(m_LastArgForScan);
	}

	for (int i = 1; i < argc; i++, found = false) {
		if (earlyBreak)	 m_unusedArgs.push_back(std::string(argv[i]));
		else {
			if ((argv[i][1] == '-') && (argv[i][0] == '-')) { // look at 1 first to check for long form
				p = argv[i]; p+= 2;
				testArg = m_longArgs.find(p);
				if (testArg != m_longArgs.end()) found = true;
			} else if (argv[i][0] == '-') {	// Fast break from above could indicate short form.
				p = argv[i]; p+= 1;
				testArg = m_shortArgs.find(p);
				if (testArg != m_shortArgs.end()) found = true;
			}

			if (!found)	m_unusedArgs.push_back(std::string(argv[i]));
			else {
				// We found the argument, see if we need to add a parameter.
				if (testArg->second->hasParam()) {
					i++;
					if (i < argc) {	// Just to be sure
						testArg->second->setArg(argv[i]);
						m_scannedCount++;
					}
				} else {
					testArg->second->setArg();
					m_scannedCount++;
				}
			}
			if (m_useLastForScan) {
				if (testEarly != m_longArgs.end()) earlyBreak = testEarly->second->isSet();
			}
		}
	}

	return true;
}

argSwitches::argOption *argSwitches::argOption::getThis(void)
{
	return this;
}


bool argSwitches::addArg(const std::string& shortOpt, const std::string& longOpt, const std::string& desc, bool expectParam, const std::string& paramName)
{
	if ((shortOpt.length() < 1) && (longOpt.length() < 1)) return false; // need some way to find the arg. 
	bool l_hidden = (shortOpt.compare("-") == 0);
	str2argOptionPtr::iterator sopt = m_shortArgs.end();
	if (!l_hidden) sopt = m_shortArgs.find(shortOpt);
	str2argOptionPtr::iterator lopt = m_longArgs.find(longOpt);
	if (sopt != m_shortArgs.end()) { // Updating discription/has value
		sopt->second->description(desc);
		sopt->second->hasParam(expectParam, paramName);
		if (lopt == m_longArgs.end()) {
			// Add the new reference.
			if (longOpt.length() > 0) m_longArgs[longOpt] = sopt->second->getThis();
		}
	} else if (lopt != m_longArgs.end()) { // Updating discription/has value
		lopt->second->description(desc);
		lopt->second->hasParam(expectParam, paramName);
		if (sopt == m_longArgs.end()) {
			// Add the new reference.
			if (shortOpt.length() > 0) m_shortArgs[shortOpt] = lopt->second->getThis();
		}
	} else {// adding new arg
		m_args.push_back(new argOption(shortOpt, longOpt, desc, expectParam, paramName, l_hidden));
		size_t lastLoc = m_args.size() -1;
		if ((shortOpt.length() > 0) && !l_hidden) m_shortArgs[shortOpt] = m_args[lastLoc]->getThis();
		if (longOpt.length() > 0) m_longArgs[longOpt] = m_args[lastLoc]->getThis();
	}
	return true;
}

unsigned long argSwitches::addDescriptionHeader(const std::string& str)
{
	m_globalDescriptions.push_back(str);
	return(unsigned long) (m_globalDescriptions.size() -1);
}

unsigned long argSwitches::addExample(const std::string& cmdOptions, const std::string& desc)
{
	m_globalExamples.push_back(std::pair<std::string, std::string>(cmdOptions, desc));
	return(unsigned long) (m_globalExamples.size() -1);
}

unsigned long argSwitches::addSeeAlso(const std::string& str)
{
	m_globalSeeAlso.push_back(str);
	return(unsigned long) (m_globalSeeAlso.size() -1);
}


bool argSwitches::displayHelp(std::ostream& os, unsigned long outputWidth)
{
	// First Display the usage statement
	os << "Usage: " << m_cmd;
	if (m_args.size() > 0) {
		os << " [OPTIONS]" << std::endl << std::endl;
	}

	if (m_globalDescriptions.size() > 0) {
		os << "Description:" << std::endl;
		for (size_t idx = 0; idx < m_globalDescriptions.size(); idx++) {
			std::vector<std::string> fmtList;
			unsigned long nLines = formatStringToVectorList(m_globalDescriptions[idx], fmtList, 60);
			for (unsigned long ln = 0; ln < nLines; ln++) {
				os << "    " << fmtList[ln] << std::endl;
			}
			os << std::endl; // Paragraph
		}
	}

	if (m_args.size() > 0) {
		os << "OPTIONS" << std::endl;
	}

	for (size_t idx = 0; idx < m_args.size(); idx++) {
		if (!m_args[idx]->displayHelp(os, outputWidth))	return false;
	}

	if (m_globalExamples.size() > 0) {
		os << std::endl;
		for (size_t idx = 0; idx < m_globalExamples.size(); idx++) {
			os << "Example " << idx+1 << ":" << std::endl;
			os << "   " << m_cmd << " " << m_globalExamples[idx].first << std::endl << std::endl;

			std::vector<std::string> fmtList;
			unsigned long nLines = formatStringToVectorList(m_globalExamples[idx].second, fmtList, 60);
			for (unsigned long ln = 0; ln < nLines; ln++) {
				os << "        " << fmtList[ln] << std::endl;
			}
			os << std::endl; // Paragraph
		}
	}

	if (m_globalSeeAlso.size() > 0) {
		os << "See Also:" << std::endl;
		for (size_t idx = 0; idx < m_globalSeeAlso.size(); idx++) {
			std::vector<std::string> fmtList;
			unsigned long nLines = formatStringToVectorList(m_globalSeeAlso[idx], fmtList, 60);
			for (unsigned long ln = 0; ln < nLines; ln++) {
				os << "    " << fmtList[ln] << std::endl;
			}
			os << std::endl; // Paragraph
		}
	}



	return true;
}

argSwitches::argOption *argSwitches::getOption(const std::string& ref, bool useLongArg)
{
	str2argOptionPtr::iterator iter;
	if (useLongArg) {
		iter = m_longArgs.find(ref);
		if (iter == m_longArgs.end()) return NULL;
	} else {
		iter = m_shortArgs.find(ref);
		if (iter == m_shortArgs.end()) return NULL;
	}

	return iter->second->getThis();
}

bool argSwitches::getArg(const std::string& ref, bool& value, bool useLongArg)
{
	argOption *ap = getOption(ref, useLongArg);
	if (ap != NULL)	value = ap->bValue();

	return value;
}


const std::string&  argSwitches::getArg(const std::string& ref, bool useLongArg)
{
	argOption *ap = getOption(ref, useLongArg);
	if (ap == NULL)	return argSwitches::argOption::emptyString();

	return ap->value();
}

long argSwitches::getArg(const std::string& ref, long& value, bool useLongArg)
{
	argOption *ap = getOption(ref, useLongArg);
	if (ap == NULL)	return value;

	long l = 0;
	if (sscanf(ap->value().c_str(), "%ld", &l) == 1) value = l;
	return value;
}
double argSwitches::getArg(const std::string& ref, double& value, bool useLongArg)
{
	argOption *ap = getOption(ref, useLongArg);
	if (ap == NULL)	return value;

	double lf = 0;
	if (sscanf(ap->value().c_str(), "%lf", &lf) == 1) value = lf;
	return value;
}


bool argSwitches::isSet(const std::string& ref, bool useLongArg)
{
	argOption *ap = getOption(ref, useLongArg);
	if (ap == NULL)	return false;

	return ap->isSet();
}


const std::string& argSwitches::setArg(const std::string& ref, const std::string& value, bool useLongArg)
{
	argOption *ap = getOption(ref, useLongArg);
	if (ap != NULL) {
		if (ap->setArg(value)) return value;
	}

	return argSwitches::argOption::emptyString();
}

bool argSwitches::setArg(const std::string& ref, bool useLongArg)
{
	argOption *ap = getOption(ref, useLongArg);
	if (ap == NULL)	return false;

	return ap->setArg();
}

