#pragma once

#include <string>
#include <vector>
#include <map>
#include "tinyxml2.h"

namespace tinyxtrf {

class GdrField {
public:
	std::string m_name;
	std::string m_type;
	std::string m_value;

	GdrField(const std::string& name, const std::string& type, const std::string& value) :
		m_name(name), m_type(type), m_value(value) {}
};

typedef std::vector< GdrField > GdrRecord;

class Xtrf {
private:
	std::map< std::string, std::string > m_stdFields; /** Map to store all fields, except GDR */
	std::vector< GdrRecord > m_gdrRecords;
	std::string m_errorStr;

public:
	const std::string get(const std::string& token);
	bool parse(const std::string& file);
	bool loadGnbTesterTable(const std::string& fileName, const std::string& testerName);
	bool dumpGdrs(const std::string& fileName);

	inline void addGdr(const GdrRecord& gdr) { m_gdrRecords.push_back(gdr); }
	inline const std::map< std::string, std::string >& get() { return m_stdFields; }
	inline const std::vector< GdrRecord >& gdrs() { return m_gdrRecords; }
	inline void clear() { m_stdFields.clear(); m_gdrRecords.clear(); }
	inline const std::string getError() { return m_errorStr; }

	static Xtrf* instance();

private:
	Xtrf():	m_stdFields(), m_gdrRecords(), m_errorStr() {}
	void set(const std::string& token, const std::string& value);
	void processRecord(const char* recordName, tinyxml2::XMLElement* stdfRecordElt);
	void fillDefaults();
};

} // namespace tinyxtrf
