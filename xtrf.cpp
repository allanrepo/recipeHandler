
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unistd.h>
#include <cstring> // for strerror
#include <cerrno>  // for errno

#include "xtrf.h"
#include "tinyxml2.h"

namespace tinyxtrf {

Xtrf* Xtrf::instance() {
	static Xtrf l_instance;
	return &l_instance;
}

void Xtrf::set(const std::string& token, const std::string& value) {
	m_stdFields[token] = value;
}

const std::string Xtrf::get(const std::string& token) {
	std::map<std::string, std::string>::const_iterator itr = m_stdFields.find(token);
	if(itr == m_stdFields.end()) {
		return "";
	}
	return itr->second;
}

void Xtrf::processRecord(const char* recordName, tinyxml2::XMLElement* stdfRecordElt) {
	// Try to get the first STDFfields.STDFfield in current testerRecipe.STDF.STDFrecord element
	tinyxml2::XMLElement* stdfFieldElt = stdfRecordElt->FirstChildElement("STDFfields");
	if(stdfFieldElt) stdfFieldElt = stdfFieldElt->FirstChildElement("STDFfield");
	//std::cout << " === " << recordName << " === " << std::endl;
	if(strncmp("GDR", recordName, 3) == 0) {
		GdrRecord myGdrRecord;
		// Iterate over all STDFfields.STDFfield elements
		for(;NULL != stdfFieldElt;stdfFieldElt = stdfFieldElt->NextSiblingElement("STDFfield")) {
			// If field name is not set, skip it
			const char* fieldName  = stdfFieldElt->Attribute("fieldName");
			if(NULL == fieldName) continue;
			// If field type is not set, skip it
			const char* dataType  = stdfFieldElt->Attribute("dataType");
			if(NULL == dataType) continue;
			// Try to get value, if there is a child. Empty childs will be ignored.
			tinyxml2::XMLNode* textNode = stdfFieldElt->FirstChild();
			std::string dataValue("");
			if(NULL != textNode && NULL != textNode->ToText()) {
				dataValue = textNode->ToText()->Value();
			}
			myGdrRecord.push_back(GdrField(fieldName, dataType, dataValue));
			//std::cout << dataType << " " <<dataValue << std::endl;
		}
		m_gdrRecords.push_back(myGdrRecord);
	}
	else {
		// Iterate over all STDFfields.STDFfield elements
		for(;NULL != stdfFieldElt;stdfFieldElt = stdfFieldElt->NextSiblingElement("STDFfield")) {
			// If field name is not set, skip it
			const char* fieldName = stdfFieldElt->Attribute("fieldName");
			if(NULL == fieldName) continue;
			// Try to get value, if there is a child. Empty childs will be ignored.
			tinyxml2::XMLNode* textNode = stdfFieldElt->FirstChild();
			if(NULL == textNode) continue;
			if(NULL == textNode->ToText()) continue;
			const std::string stdfFieldValue(textNode->ToText()->Value());
			std::ostringstream xtrfToken;
			xtrfToken << recordName << "." << fieldName;
			set(xtrfToken.str(), stdfFieldValue);
			//std::cout << recordName << "." << fieldName << " = " << stdfFieldValue << std::endl;
		}
	}
}

bool Xtrf::parse(const std::string& file) {
	// Load XTRF document
	tinyxml2::XMLDocument xtrfDoc;
	tinyxml2::XMLError errCode = xtrfDoc.LoadFile(file.c_str());
	m_errorStr = xtrfDoc.ErrorStr();
	if(errCode != tinyxml2::XML_SUCCESS) {
		return false;
	}
	// Try to get the first testerRecipe.STDF.STDFrecord element
	//tinyxml2::XMLElement* stdfRecordElt = xtrfDoc.FirstChildElement("testerRecipe");
	tinyxml2::XMLElement* stdfRecordElt = xtrfDoc.RootElement();
	if(stdfRecordElt) stdfRecordElt = stdfRecordElt->FirstChildElement("STDF");
	if(stdfRecordElt) stdfRecordElt = stdfRecordElt->FirstChildElement("STDFrecord");
	// Iterate over all testerRecipe.STDF.STDFrecord elements
	for(;NULL != stdfRecordElt; stdfRecordElt=stdfRecordElt->NextSiblingElement("STDFrecord")) {
		// Try to get the record name. If record name was not set, skip it.
		const char* recordName = stdfRecordElt->Attribute("recordName");
		if(NULL == recordName) continue;
		processRecord(recordName, stdfRecordElt);
	}
	return true;
}

bool Xtrf::loadGnbTesterTable(const std::string& fileName, const std::string& mytesterName) {
	// Fill defaults
	fillDefaults();
	// Open the file
	bool success = true;
	std::ifstream testerFile;
	testerFile.open(fileName.c_str(), std::ios_base::in);
	if(!testerFile.is_open()) {
		m_errorStr = "Failed to open "+fileName;
		return false;
	}
	// Get hostname, need uppercase
	std::string hostName(mytesterName);
	if(hostName.find("-t") != std::string::npos) {
		hostName = hostName.substr(0, hostName.find("-t"));
	}
	std::transform(hostName.begin(), hostName.end(), hostName.begin(), ::toupper);
	// Look for the testers in the file
	while(!testerFile.eof()) {
		std::string readLine, testerName, serialNum, testerModel, handlerTyp;
		std::getline(testerFile, readLine);
		std::stringstream rwStream;
		rwStream << readLine;
		rwStream >> testerName >> serialNum >> testerModel >> handlerTyp;
		if(testerName == hostName) {
			set("MIR.NODE_NAM", testerName);
			set("MIR.SERL_NUM", serialNum);
			set("MIR.TSTR_TYP", testerModel);
			set("SDR.HAND_TYP", handlerTyp);
			success = true;
			break;
		}
	}
	testerFile.close();
	if(!success) {
		m_errorStr = "Failed to find this tester data in "+fileName;
	}
	return success;
}

void Xtrf::fillDefaults() {
	// Default tester name
	std::string hostName(getenv("LTX_TESTER"));
	std::transform(hostName.begin(), hostName.end(), hostName.begin(), ::toupper);
	// Remove the trailing -T if any
	const size_t cutIdx(hostName.find("-T"));
	if(cutIdx != std::string::npos) {
		hostName = hostName.substr(0, cutIdx);
	}
	set("MIR.NODE_NAM", hostName);

	// Detect tester type
	std::ostringstream stream;
	stream << getenv("LTXHOME") << "/testers/" << getenv("LTX_TESTER") << "/user_data/utl_iu_data";
	std::ifstream utlIuData;
	utlIuData.open(stream.str().c_str());
	int exBp=0, dxThctl=0, d10Dibu=0, dxvVbp=0;
	while(!utlIuData.eof()) {
		std::string rawLine;
		std::getline(utlIuData, rawLine);
		if     (rawLine.find("LTXC_PHX_THCTL")    != std::string::npos) { ++dxThctl; }
		else if(rawLine.find("LTXC_EX_BACKPLANE") != std::string::npos) { ++exBp;    }
		else if(rawLine.find("LTXC_DMD_DIBU")     != std::string::npos) { ++d10Dibu; }
		else if(rawLine.find("LTXC_PHX_VBP")      != std::string::npos) { ++dxvVbp;  }
	}
	utlIuData.close();
	std::ostringstream tstrTypeStream;
	if     (dxThctl != 0) { tstrTypeStream << "DIAMONDX_" << dxThctl; }
	else if(dxvVbp  != 0) { tstrTypeStream << "DXV_" << dxvVbp; }
	else if(exBp != 0)    { tstrTypeStream << "FUSION_" << exBp; }
	else if(d10Dibu != 0) { tstrTypeStream << "DIAMOND10_" << d10Dibu; }
	else tstrTypeStream << "UNKNOWN";
	set("MIR.TSTR_TYP", tstrTypeStream.str());

	// Unknown serial number
	set("MIR.SERL_NUM", "");
	// Unknown handler type
	set("SDR.HAND_TYP", "");
}

bool Xtrf::dumpGdrs(const std::string& fileName) {
	// tinyxml2 can't generate XML files. Do it the ugly way...
	// Open destination file
	std::ofstream gdrXtrfFile;
	gdrXtrfFile.open(fileName.c_str(), std::ios_base::out);
	if(!gdrXtrfFile.good()) {
		m_errorStr = "Failed to write " + fileName + ". Error details:" + ::strerror(errno);
		return false;
	}
	gdrXtrfFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
	gdrXtrfFile << "<testerRecipe xmlns=\"urn:st-com:xsd:XTRF.V0150.Generic\">" << std::endl;
	gdrXtrfFile << "<STDF>" << std::endl;
	for(std::vector< GdrRecord >::const_iterator gdrRecord = m_gdrRecords.begin();
			gdrRecord != m_gdrRecords.end(); ++gdrRecord) {
		gdrXtrfFile << "<STDFrecord recordName=\"GDR\">" << std::endl << "<STDFfields>" << std::endl;
		for(GdrRecord::const_iterator gdrField = gdrRecord->begin();
				gdrField != gdrRecord->end(); ++gdrField) {
			gdrXtrfFile << "<STDFfield fieldName=\"" << gdrField->m_name << "\" ";
			gdrXtrfFile << "dataType=\"" << gdrField->m_type << "\" required=\"strict\">";
			gdrXtrfFile << gdrField->m_value << "</STDFfield>" << std::endl;
		}
		gdrXtrfFile << "</STDFfields>" << std::endl << "</STDFrecord>" << std::endl;
	}
	gdrXtrfFile << "</STDF>" << std::endl << "</testerRecipe>" << std::endl;
	gdrXtrfFile.close();
	return true;
}

} // namespace tinyxtrf
