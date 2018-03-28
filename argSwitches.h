#include <ostream>
#include <string>
#include <map>
#include <vector>

namespace utilities {
	namespace args {
		class argSwitches {	// May move into configClass later.
		private:
			class argOption {
			private:
				std::string m_shortOption;
				std::string m_longOption;
				std::string m_description;
				std::string m_strValue;
				std::string m_paramName;
				bool m_isSet;
				bool m_hasParam;
				bool m_hiddenArg;
				static unsigned long m_shortOptWidth;
				static unsigned long m_longOptWidth;
				static unsigned long m_paramDescWidth;
				static const std::string m_emptyStr;
			public:
				argOption();
				argOption(const std::string& shortOpt, const std::string& longOpt, const std::string& desc, bool m_hasParam = false, const std::string& paramName = std::string(""), bool hiddenArg = false);
				virtual ~argOption();

				void clear(void) {
					m_isSet = false;
					m_strValue = "";
				}

				bool setArg(void) {
					if (m_hasParam)	return false; // Cannot use this method to set an arg that needs a value
					m_isSet = true;
					return m_isSet;
				}

				bool setArg(const std::string& value) {
					m_hasParam = true;
					m_strValue = value;
					m_isSet = true;
					return m_isSet;
				}

				const std::string& description(void) { return m_description;}
				const std::string& description(const std::string& desc);

				const std::string& value(void) { if (m_hasParam && m_isSet)	return m_strValue;return m_emptyStr;}
				bool               bValue(void) { if (!m_hasParam) return m_isSet;return false;}
				bool               isSet(void) { return m_isSet;}

				bool               hasParam(void) { return m_hasParam;}
				bool               hasParam(bool b, const std::string& param);

				argOption *getThis(void); // { return this; }

				static const std::string& emptyString(void) { return m_emptyStr;}

				bool displayHelp(std::ostream& os, unsigned long outputWidth = 75);
			};

			typedef std::vector<argOption *> argOptionVec;
			typedef std::map<std::string, argOption *, std::less<std::string> > str2argOptionPtr;

		private:
			argOptionVec m_args;
			str2argOptionPtr m_longArgs;
			str2argOptionPtr m_shortArgs;
			bool m_initNeeded;
			unsigned long m_scannedCount;
			std::vector<std::string> m_unusedArgs;
			char **m_unusedArgsPtr;
			std::string m_cmd;
			std::vector<std::string> m_globalDescriptions;
			std::vector<std::pair<std::string, std::string> > m_globalExamples;
			std::vector<std::string> m_globalSeeAlso;
			std::string m_LastArgForScan;
			bool m_useLastForScan;
			bool m_lastScanArgIsLongForm;


			void common_init(void);
		public:
			argSwitches();

			virtual ~argSwitches();

			bool scanArgs(int argc, char **argv, bool replaceCommand = false, const std::string& replaceWith = "");

			bool setLastScanArg(const std::string& lastArg, bool useLongArg = true);

			virtual bool initArgList(void);
			virtual bool argUpdate(const std::string& shortOpt, const std::string& longOpt, const std::string& desc, bool hasValue = false, const std::string& value = std::string(""));

			unsigned long scannedOptions(void) { return m_scannedCount;}

			bool addArg(const std::string& shortOpt, const std::string& longOpt, const std::string& desc, bool expectParam = false, const std::string& paramName = std::string(""));

			// The following return the index number of the data added
			unsigned long addDescriptionHeader(const std::string& str);
			unsigned long addExample(const std::string& cmdOptions, const std::string& desc);
			unsigned long addSeeAlso(const std::string& str);

			bool displayHelp(std::ostream& os, unsigned long outputWidth = 75);

			// The following functions default to looking up the long form argument name, set useLongArg to false to lookup
			// the short form argument name. Reference should not include any "-" or "--"
			bool isSet(const std::string& ref, bool useLongArg = true);	// Command was set as a switch on the command line
			// If not set, the empty string "" will be returned.
			const std::string& getArg(const std::string& ref, bool useLongArg = true);
			// In the event of the argument not being set or found, the "value" parameter will be returned.
			bool getArg(const std::string& ref, bool& value, bool useLongArg);
			long getArg(const std::string& ref, long& value, bool useLongArg = true);
			double getArg(const std::string& ref, double& value, bool useLongArg = true);
			const std::string& getCommand(void) { return m_cmd;}


			const std::string& setArg(const std::string& ref, const char *value, bool useLongArg = true) { return setArg(ref, std::string(value), useLongArg);}
			const std::string& setArg(const std::string& ref, const std::string& value, bool useLongArg = true);
			bool setArg(const std::string& ref, bool useLongArg = true);


			// The following two functions returns a list of args and their count of those arguemnts that
			// were not processed by the scanArgs function.
			// 
			// The size will always be at least 1 after calling scanArgs() as the "program name" will be in argv[0]
			int argc(void) { return(int) m_unusedArgs.size();}
			char **argv(void);



		private:
			argOption *getOption(const std::string& ref, bool useLongArg);
			void clearUnused(void);

		};
	}
}

