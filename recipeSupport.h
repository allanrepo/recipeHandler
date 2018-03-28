#ifndef _RECIPE_SUPPORT_HEADER_INCLUDED_
#define _RECIPE_SUPPORT_HEADER_INCLUDED_

#include <string>
#include <iostream>

class CRecipeConfig
{
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

class CMIRArgs
{
public:
    CMIRArgs();
    CMIRArgs(const CMIRArgs& ot);
    CMIRArgs& operator=(const CMIRArgs& ot);
    ~CMIRArgs();

    bool clearParams();

    std::string LotId;		// TestProgData.LotId
    std::string CmodCod;	// TestProgData.CommandMode
    std::string FlowId;		// TestProgData.ActiveFlowName
    std::string DsgnRev;	// TestProgData.DesignRev
    std::string DateCod;	// TestProgData.DateCode
    std::string OperFrq;	// TestProgData.OperFreq
    std::string OperNam;	// TestProgData.Operator
    std::string NodeNam;	// TestProgData.TcName
    std::string PartTyp;	// TestProgData.Device
    std::string EngId;		// TestProgData.EngrLotId
    std::string TestTmp;	// TestProgData.TestTemp
    std::string FacilId;	// TestProgData.TestFacility
    std::string FloorId;	// TestProgData.TestFloor
    std::string StatNum;	// TestProgData.Head (int)	
    std::string ProcId;		// TestProgData.FabId
    std::string ModCod;		// TestProgData.TestMode
    std::string FamilyId;	// TestProgData.ProdId
    std::string PkgTyp;		// TestProgData.Package
    std::string SblotId;	// TestProgData.SublotId
    std::string JobNam;		// TestProgData.ObjName        //// exists in TestProgData class but cannot be set in setLotInformation()
    std::string SetupId;	// TestProgData.TestSetup
    std::string JobRev;		// TestProgData.FileNameRev
    std::string ExecTyp;	// TestProgData.SystemName
    std::string ExecVer;	// TestProgData.TargetName
    std::string AuxFile;	// TestProgData.AuxDataFile
    std::string RtstCod;	// TestProgData.LotStatusTest  //// exists in TestProgData class but cannot be set in setLotInformation()
    std::string TestCod;	// TestProgData.TestPhase
    std::string UserText;	// TestProgData.UserText
    std::string RomCod;		// TestProgData.RomCode
    std::string SerlNum;	// TestProgData.TesterSerNum
    std::string SpecNam;	// TestProgData.TestSpecName
    std::string TstrTyp;	// TestProgData.TesterType
    std::string SuprNam;	// TestProgData.Supervisor
    std::string SpecVer;	// TestProgData.TestSpecRev
};

class CSDRArgs
{
public:
    CSDRArgs();
    CSDRArgs(const CSDRArgs& ot);
    CSDRArgs& operator=(const CSDRArgs& ot);
    ~CSDRArgs();

    bool clearParams();
    
    std::string HandTyp;   	// TestProgData.HandlerType
    std::string CardId;		// TestProgData.CardId
    std::string LoadId;		// TestProgData.LoadboardId
    std::string PHId;		// TestProgData.PHID
    std::string DibTyp;		// TestProgData.DIBType
    std::string CableId;	// TestProgData.IfCableId
    std::string ContTyp;	// TestProgData.ContactorType
    std::string LoadTyp;	// TestProgData.LoadBrdType
    std::string ContId;		// TestProgData.ContactorId
    std::string LaserTyp;	// TestProgData.LaserType
    std::string LaserId;	// TestProgData.LasterId
    std::string ExtrTyp;	// TestProgData.ExtEquipType
    std::string ExtrId;		// TestProgData.ExtEquipId
    std::string DibId;		// TestProgData.ActiveLoadBrdName
    std::string CardTyp;	// TestProgData.CardType
    std::string CableTyp;	// TestProgData.LotIfCableType


};

#endif // _RECIPE_SUPPORT_HEADER_INCLUDED_
