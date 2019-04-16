#ifndef _RECIPE_SUPPORT_HEADER_INCLUDED_
#define _RECIPE_SUPPORT_HEADER_INCLUDED_

#include <string>
#include <iostream>
#include <vector>
#include <utility.h>

class CRecipeConfig
{ 
private:
	CUtil::CLog m_Log;
 public:
    CRecipeConfig();
    CRecipeConfig(const CRecipeConfig& ot);
    CRecipeConfig& operator=(const CRecipeConfig& ot);
    ~CRecipeConfig();

    bool clearConfigParams();
    bool checkConfigParams();
    bool printConfigParams();
		     
    std::string RemoteLocation;  // Remote Location of package to be downloaded.
    std::string LocalLocation;   // Local Location of package to be downloaded to.
    std::string ProgLocation;   // Local Location of test program be downloaded extracted to.
    std::string PackageType;     // package type of download.  Supported values: tar.
    std::string ConfigurationName;
    std::string S10F1;

	bool checkProgLocation()
	{ 
		if (ProgLocation.empty()) { std::cout << "[ERROR] ProgLocation is empty." << std::endl; return false; }
		else return true;
    	} 
	
};

class CTestProgArgs
{
public:
    CTestProgArgs();
    CTestProgArgs(const CTestProgArgs& ot);
    CTestProgArgs& operator=(const CTestProgArgs& ot);
    ~CTestProgArgs();

    bool clearParams();

    std::string ReloadStrategy; // testPrgmLoader and testPrgmCopierLoaderScript
    std::string DownloadStrategy; // testPrgmLoader and testPrgmCopierLoaderScript
    std::string BackToIdleStrategy; // testPrgmLoader and testPrgmCopierLoaderScript
    std::string TPName;		// testPrgmLoader and testPrgmCopierLoaderScript
    std::string TPPath;		// testPrgmLoader and testPrgmCopierLoaderScript
    std::string TPFile;		// testPrgmLoader and testPrgmCopierLoaderScript
    std::string RcpFileSupport; // testPrgmCopierLoaderScript
    std::string Flow;		// testPrgmLoader and testPrgmCopierLoaderScript
    std::string Salestype;	// testPrgmCopierLoaderScript
    std::string Temperature;	// testPrgmLoader and testPrgmCopierLoaderScript
    std::string Product;	// testPrgmLoader and testPrgmCopierLoaderScript
    std::string Parallelism;	// testPrgmLoader and testPrgmCopierLoaderScript
    std::string DieCode;	// testPrgmLoader 
    std::string CmodCode;	// testPrgmLoader 
    std::string LotId;		// testPrgmLoader
    std::string FlowId;		// testPrgmLoader
    std::string GrossDie;	// testPrgmLoader
    std::string ActiveController; // testPrgmLoader
    std::string CurrentProgName;  // Current Program name that is loaded if there is one.  
    std::string FullTPName;       // full name of Test Program to load including path.
    std::string EndLotEnable;	  // Added as example as attribute to testPrgmLoader and testPrgmCopierLoaderScript
    std::string EndWaferEnable;	  // Added as example as attribute to testPrgmLoader and testPrgmCopierLoaderScript
    std::string StartLotEnable;	  // Added as example as attribute to testPrgmLoader and testPrgmCopierLoaderScript
    std::string StartWaferEnable;	  // Added as example as attribute to testPrgmLoader and testPrgmCopierLoaderScript
   // std::string ConfigurationName;
};

struct param
{
	std::string value;
	std::string required;
	std::string override;
	long state;

	bool empty(){ return value.empty(); }
	const char* c_str() const { return value.c_str(); }
	const std::string& str() const { return value; }
	param& operator=(const param& ot)
	{
		value = ot.value;
		required = ot.required;
		override = ot.override;
		state = ot.state;
		return *this;
	}  

	param& operator=(const std::string& ot)
	{
		value = ot;
		return *this;
	}

	void clear()
	{
		value.clear();
		required.clear();
		override.clear();		
	}

	void set(const std::string& v, const std::string& r, const std::string o)
	{
		value = v;
		required = r;
		override = o;
	}
};

class CGDR
{
	struct custom
	{
		std::string name;
		std::vector< param > fields;
	};

public:
	param auto_nam;
	param auto_ver;
	param trf_xtrf;
	param sg_status;
	param sg_nam;
	param sg_rev;
	param api_nam;
	param api_rev;
	param gui_nam;
	param gui_rev;
	param stdf_frm;
	param drv_rev;
	param drv_nam;

	std::vector< custom > customs;
	

public:
	CGDR();
	CGDR(const CGDR& ot);
	CGDR& operator=(const CGDR& ot);
	virtual ~CGDR();
	void clear();	

	void addCustom(const std::string& name, const std::string& v, const std::string& r, const std::string o); 
};


class CMIRArgs
{
public:
    CMIRArgs();
    CMIRArgs(const CMIRArgs& ot);
    CMIRArgs& operator=(const CMIRArgs& ot);
    ~CMIRArgs();

    bool clear();

    param LotId;		// TestProgData.LotId
    param CmodCod;	// TestProgData.CommandMode
    param FlowId;		// TestProgData.ActiveFlowName
    param DsgnRev;	// TestProgData.DesignRev
    param DateCod;	// TestProgData.DateCode
    param OperFrq;	// TestProgData.OperFreq
    param OperNam;	// TestProgData.Operator
    param NodeNam;	// TestProgData.TcName
    param PartTyp;	// TestProgData.Device
    param EngId;		// TestProgData.EngrLotId
    param TestTmp;	// TestProgData.TestTemp
    param FacilId;	// TestProgData.TestFacility
    param FloorId;	// TestProgData.TestFloor
    param StatNum;	// TestProgData.Head (int)	
    param ProcId;		// TestProgData.FabId
    param ModCod;		// TestProgData.TestMode
    param FamilyId;	// TestProgData.ProdId
    param PkgTyp;		// TestProgData.Package
    param SblotId;	// TestProgData.SublotId
    param JobNam;		// TestProgData.ObjName        //// exists in TestProgData class but cannot be set in setLotInformation()
    param SetupId;	// TestProgData.TestSetup
    param JobRev;		// TestProgData.FileNameRev
    param ExecTyp;	// TestProgData.SystemName
    param ExecVer;	// TestProgData.TargetName
    param AuxFile;	// TestProgData.AuxDataFile
    param RtstCod;	// TestProgData.LotStatusTest  //// exists in TestProgData class but cannot be set in setLotInformation()
    param TestCod;	// TestProgData.TestPhase
    param UserText;	// TestProgData.UserText
    param RomCod;		// TestProgData.RomCode
    param SerlNum;	// TestProgData.TesterSerNum
    param SpecNam;	// TestProgData.TestSpecName
    param TstrTyp;	// TestProgData.TesterType
    param SuprNam;	// TestProgData.Supervisor
    param SpecVer;	// TestProgData.TestSpecRev
    param ProtCod;	// TestProgData.ProtectionCode

};


class CSDRArgs
{
public:
    CSDRArgs();
    CSDRArgs(const CSDRArgs& ot);
    CSDRArgs& operator=(const CSDRArgs& ot);
    ~CSDRArgs();

    bool clear();
    
    param HandTyp;   	// TestProgData.HandlerType
    param CardId;	// TestProgData.CardId
    param LoadId;	// TestProgData.LoadboardId
    param PHId;		// TestProgData.PHID
    param DibTyp;	// TestProgData.DIBType
    param CableId;	// TestProgData.IfCableId
    param ContTyp;	// TestProgData.ContactorType
    param LoadTyp;	// TestProgData.LoadBrdType
    param ContId;	// TestProgData.ContactorId
    param LaserTyp;	// TestProgData.LaserType
    param LaserId;	// TestProgData.LasterId
    param ExtrTyp;	// TestProgData.ExtEquipType
    param ExtrId;	// TestProgData.ExtEquipId
    param DibId;	// TestProgData.ActiveLoadBrdName
    param CardTyp;	// TestProgData.CardType
    param CableTyp;	// TestProgData.LotIfCableType
};

#endif // _RECIPE_SUPPORT_HEADER_INCLUDED_
