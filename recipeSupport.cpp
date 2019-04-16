#include "recipeSupport.h"
#include <iostream>

///////////////////////////////////////////////////////
// recipeConfig

CRecipeConfig::CRecipeConfig(): RemoteLocation(), LocalLocation(), ProgLocation(), PackageType(), ConfigurationName(), S10F1()
{
}

CRecipeConfig::CRecipeConfig(const CRecipeConfig& ot): RemoteLocation(ot. RemoteLocation), 
						       LocalLocation(ot.LocalLocation), 
						       ProgLocation(ot.ProgLocation), 
						       PackageType(ot.PackageType),
						       ConfigurationName(ot.ConfigurationName),
							S10F1(ot.S10F1)
{
}

CRecipeConfig& CRecipeConfig::operator=(const CRecipeConfig& ot)
{
    RemoteLocation = ot.RemoteLocation;
    LocalLocation = ot.LocalLocation;
    ProgLocation = ot.ProgLocation;
    PackageType = ot.ProgLocation;
    ConfigurationName = ot.ConfigurationName;
    S10F1 = ot.S10F1;
    return *this;
}

CRecipeConfig::~CRecipeConfig()
{
}

bool CRecipeConfig::clearConfigParams()
{
	RemoteLocation.clear();
	LocalLocation.clear();
	ProgLocation.clear();
	PackageType.clear();
	ConfigurationName.clear();
	S10F1.clear();
	return true;   
}

bool CRecipeConfig::checkConfigParams()
{
    bool result = true;
    if (RemoteLocation.empty() == true) {
	std::cout << "[ERROR] RemoteLocation is empty, correct configuration file." << std::endl;
	result = false;
    }
    if (LocalLocation.empty() == true) {
	std::cout << "[ERROR] LocalLocation is empty, correct configuration file." << std::endl;
	result = false;
    }
    if (ProgLocation.empty() == true) {
	std::cout << "[ERROR] ProgLocation is empty, correct configuration file." << std::endl;
	result = false;
    }
    if (PackageType.empty() == true) {
	std::cout << "[ERROR] PackageType is empty, correct configuration file." << std::endl;
	result = false;
    }
    if (ConfigurationName.empty() == true) {
	std::cout << "[ERROR] ConfigurationName is empty, correct configuration file." << std::endl;
	result = false;
    }
    if (S10F1.empty() == true) {
	std::cout << "[ERROR] S10F1 is empty, correct configuration file." << std::endl;
	result = false;
    }

    return result;
}

bool CRecipeConfig::printConfigParams()
{
    m_Log << "Printing active RecipeHandler Configuration [" << ConfigurationName << "]..." << CUtil::CLog::endl;
    m_Log << "Remote Location to download test program package from [RemoteLocation]: " << RemoteLocation << CUtil::CLog::endl;
    m_Log << "Location in Tester PC to download test program file package [LocalLocation]: " << LocalLocation << CUtil::CLog::endl;
    m_Log << "Location in tester PC to unpack/install test program [ProgLocation]: " << ProgLocation << CUtil::CLog::endl;
    m_Log << "Expected test program package file type [PackageType]: " << PackageType << CUtil::CLog::endl;
    m_Log << "Enable S10F1 event/error messaging to host [S10F1]: " << S10F1 << CUtil::CLog::endl;
    return true;
}


///////////////////////////////////////////////////////
// CTestProgArgs

CTestProgArgs::CTestProgArgs() : ReloadStrategy(),DownloadStrategy(), BackToIdleStrategy(), TPName(),TPPath(),TPFile(),RcpFileSupport(),
				 Flow(),Salestype(),Temperature(),Product(),Parallelism(),
				 DieCode(),CmodCode(),LotId(),FlowId(),GrossDie(),ActiveController(),
				 CurrentProgName(),FullTPName(),EndWaferEnable(),StartLotEnable(),
				 StartWaferEnable()
{  
}

CTestProgArgs::CTestProgArgs(const CTestProgArgs& ot) : ReloadStrategy(ot.ReloadStrategy), DownloadStrategy(ot.DownloadStrategy), 
		BackToIdleStrategy(ot.BackToIdleStrategy), TPName(ot.TPName),TPPath(ot.TPPath),TPFile(ot.TPFile),RcpFileSupport(ot.RcpFileSupport),
	       Flow(ot.Flow), Salestype(ot.Salestype),Temperature(ot.Temperature),Product(ot.Product), 
               Parallelism(ot.Parallelism),DieCode(ot.DieCode),CmodCode(ot.CmodCode),LotId(ot.LotId), 
               FlowId(ot.FlowId),GrossDie(ot.GrossDie),ActiveController(ot.ActiveController),
	       CurrentProgName(ot.CurrentProgName),FullTPName(ot.FullTPName),EndWaferEnable(ot.EndWaferEnable),
               StartLotEnable(ot.StartLotEnable),StartWaferEnable(ot.StartWaferEnable)
{
}

CTestProgArgs& CTestProgArgs::operator=(const CTestProgArgs& ot)
{
    ReloadStrategy = ot.ReloadStrategy;
    DownloadStrategy = ot.DownloadStrategy;
    BackToIdleStrategy = ot.BackToIdleStrategy;
    TPName = ot.TPName; 
    TPPath = ot.TPPath; 
    TPFile = ot.TPFile; 
    RcpFileSupport = ot.RcpFileSupport;
    Flow = ot.Flow;
    Salestype= ot.Salestype;
    Temperature = ot.Temperature;
    Product = ot.Product; 
    Parallelism = ot.Parallelism;
    DieCode= ot.DieCode;
    CmodCode = ot.CmodCode;
    LotId = ot.LotId;
    FlowId = ot.FlowId;
    GrossDie = ot.GrossDie;
    ActiveController = ot.ActiveController;
    CurrentProgName = ot.CurrentProgName;
    FullTPName = ot.FullTPName;
    EndWaferEnable = ot.EndWaferEnable;
    StartLotEnable = ot.StartLotEnable;
    StartWaferEnable = ot.StartWaferEnable;
    return *this;
}

CTestProgArgs::~CTestProgArgs()
{
}

bool CTestProgArgs::clearParams()
{
    ReloadStrategy.clear();
    DownloadStrategy.clear();
    BackToIdleStrategy.clear();
    TPName.clear();
    TPPath.clear();
    TPFile.clear();
    RcpFileSupport.clear();
    Flow.clear();
    Salestype.clear();
    Temperature.clear();
    Product.clear();
    Parallelism.clear();
    DieCode.clear();
    CmodCode.clear();
    LotId.clear();
    FlowId.clear();
    GrossDie.clear();
    ActiveController.clear();
    CurrentProgName.clear();
    FullTPName.clear();
    EndLotEnable.clear();
    EndWaferEnable.clear();
    StartLotEnable.clear();
    StartWaferEnable.clear();
    return true;
}


///////////////////////////////////////////////////////
// CMIRArgs

CMIRArgs::CMIRArgs()
{
}

CMIRArgs::CMIRArgs(const CMIRArgs& ot)
{
	LotId = ot.LotId;
	CmodCod = ot.CmodCod;
	FlowId = ot.FlowId;
	DsgnRev = ot.DsgnRev;
	DateCod = ot.DateCod;
	OperFrq = ot.OperFrq;
	OperNam = ot.OperNam;
	NodeNam = ot.NodeNam;
	PartTyp = ot.PartTyp;
	EngId = ot.EngId;
	TestTmp = ot.TestTmp;
	FacilId = ot.FacilId;
	FloorId = ot.FloorId;
	StatNum = ot.StatNum;
	ProcId = ot.ProcId;
	ModCod = ot.ModCod;
	FamilyId = ot.FamilyId;	
	PkgTyp = ot.PkgTyp;
	SblotId = ot.SblotId;
	JobNam = ot.JobNam;
	SetupId = ot.SetupId;
	JobRev = ot.JobRev;
	ExecTyp = ot.ExecTyp;
	ExecVer = ot.ExecVer;
	AuxFile = ot.AuxFile;
	RtstCod = ot.RtstCod;
	TestCod = ot.TestCod;
	UserText = ot.UserText;
	RomCod = ot.RomCod;
	SerlNum = ot.SerlNum;
	SpecNam = ot.SpecNam;
	TstrTyp = ot.TstrTyp;
	SuprNam = ot.SuprNam;
	SpecVer = ot.SpecVer;
	ProtCod = ot.ProtCod;
}

CMIRArgs& CMIRArgs::operator=(const CMIRArgs& ot)
{
	LotId = ot.LotId;
	CmodCod = ot.CmodCod;
	FlowId = ot.FlowId;
	DsgnRev = ot.DsgnRev;
	DateCod = ot.DateCod;
	OperFrq = ot.OperFrq;
	OperNam = ot.OperNam;
	NodeNam = ot.NodeNam;
	PartTyp = ot.PartTyp;
	EngId = ot.EngId;
	TestTmp = ot.TestTmp;
	FacilId = ot.FacilId;
	FloorId = ot.FloorId;
	StatNum = ot.StatNum;
	ProcId = ot.ProcId;
	ModCod = ot.ModCod;
	FamilyId = ot.FamilyId;	
	PkgTyp = ot.PkgTyp;
	SblotId = ot.SblotId;
	JobNam = ot.JobNam;
	SetupId = ot.SetupId;
	JobRev = ot.JobRev;
	ExecTyp = ot.ExecTyp;
	ExecVer = ot.ExecVer;
	AuxFile = ot.AuxFile;
	RtstCod = ot.RtstCod;
	TestCod = ot.TestCod;
	UserText = ot.UserText;
	RomCod = ot.RomCod;
	SerlNum = ot.SerlNum;
	SpecNam = ot.SpecNam;
	TstrTyp = ot.TstrTyp;
	SuprNam = ot.SuprNam;
	SpecVer = ot.SpecVer;
	ProtCod = ot.ProtCod;
	return *this;
}

CMIRArgs::~CMIRArgs()
{
}

bool CMIRArgs::clear()
{
	LotId.clear();
	CmodCod.clear();
	DsgnRev.clear();
	DateCod.clear();
	OperFrq.clear();
	OperNam.clear();
	NodeNam.clear();
	PartTyp.clear();
	EngId.clear();
	TestTmp.clear();
	FacilId.clear();
	FloorId.clear();
	StatNum.clear();
	ProcId.clear();
	ModCod.clear();
	FamilyId.clear();
	PkgTyp.clear();
	SblotId.clear();
	JobNam.clear();
	JobRev.clear();
	ExecTyp.clear();
	ExecVer.clear();
	AuxFile.clear();
	RtstCod.clear();
	TestCod.clear();
	SetupId.clear();
	UserText.clear();
	RomCod.clear();
	SerlNum.clear();
	SpecNam.clear();
	TstrTyp.clear();
	SuprNam.clear();
	SpecVer.clear();
	ProtCod.clear();
	return true;
}


///////////////////////////////////////////////////////
// CSDRArgs

CSDRArgs::CSDRArgs() 
{
}

CSDRArgs::CSDRArgs(const CSDRArgs& ot)
{
	HandTyp = ot.HandTyp; 
    	CardId = ot.CardId;
    	LoadId = ot.LoadId;
    	PHId = ot.PHId;  
	DibTyp = ot.DibTyp;
	CableId = ot.CableId;
	ContTyp = ot.ContTyp;
	LoadTyp = ot.LoadTyp;
	LaserTyp = ot.LaserTyp;
	LaserId = ot.LaserId;
	ExtrTyp = ot.ExtrTyp;
	ExtrId = ot.ExtrId;
	ContId = ot.ContId;
	DibId = ot.DibId;
	CardTyp = ot.CardTyp;
	CableTyp = ot.CableTyp; 
}

CSDRArgs& CSDRArgs::operator=(const CSDRArgs& ot)
{
	HandTyp = ot.HandTyp; 
    	CardId = ot.CardId;
    	LoadId = ot.LoadId;
    	PHId = ot.PHId;  
	DibTyp = ot.DibTyp;
	CableId = ot.CableId;
	ContTyp = ot.ContTyp;
	LoadTyp = ot.LoadTyp;
	LaserTyp = ot.LaserTyp;
	LaserId = ot.LaserId;
	ExtrTyp = ot.ExtrTyp;
	ExtrId = ot.ExtrId;
	ContId = ot.ContId;
	DibId = ot.DibId;
	CardTyp = ot.CardTyp;
	CableTyp = ot.CableTyp;
    	return *this;
}

CSDRArgs::~CSDRArgs() 
{
}

bool CSDRArgs::clear()
{
	HandTyp.clear(); 
	CardId.clear();
	LoadId.clear();
	PHId.clear();
	DibTyp.clear();
	CableId.clear();
	ContTyp.clear();
	LoadTyp.clear();
	LaserTyp.clear();
	LaserId.clear();
	ExtrTyp.clear();
	ExtrId.clear();
	ContId.clear();
	DibId.clear();
	CardTyp.clear();
	CableTyp.clear();
    	return true;
}

CGDR::CGDR()
{
}

CGDR::CGDR(const CGDR& ot)
{
	auto_nam.value = ot.auto_nam.value; 
	auto_ver.value = ot.auto_ver.value;
	trf_xtrf.value = ot.trf_xtrf.value;
	sg_status.value = ot.sg_status.value;
	sg_nam.value = ot.sg_nam.value;
	sg_rev.value = ot.sg_rev.value;
	api_nam.value = ot.api_nam.value;
	api_rev.value =  ot.api_rev.value;
	gui_nam.value = ot.gui_nam.value;
	gui_rev.value =  ot.gui_rev.value;
	stdf_frm.value =  ot.gui_rev.value;
	drv_rev.value = ot.drv_rev.value;
	drv_nam.value = ot.drv_nam.value;
}

CGDR& CGDR::operator=(const CGDR& ot)
{
	auto_nam.value = ot.auto_nam.value; 
	auto_ver.value = ot.auto_ver.value;
	trf_xtrf.value = ot.trf_xtrf.value;
	sg_status.value = ot.sg_status.value;
	sg_nam.value = ot.sg_nam.value;
	sg_rev.value = ot.sg_rev.value;
	api_nam.value = ot.api_nam.value;
	api_rev.value =  ot.api_rev.value;
	gui_nam.value = ot.gui_nam.value;
	gui_rev.value =  ot.gui_rev.value;
	stdf_frm.value =  ot.gui_rev.value;
	drv_rev.value = ot.drv_rev.value;
	drv_nam.value = ot.drv_nam.value;	
	return *this;
}

CGDR::~CGDR()
{
	clear();
}

void CGDR::clear()
{
	auto_nam.clear(); 
	auto_ver.clear();
	trf_xtrf.clear();
	sg_status.clear();
	sg_nam.clear();
	sg_rev.clear();
	api_nam.clear();
	api_rev.clear();
	gui_nam.clear();
	gui_rev.clear();
	stdf_frm.clear();
	drv_rev.clear();
	drv_nam.clear();
	customs.clear();
}

void CGDR::addCustom(const std::string& name, const std::string& v, const std::string& r, const std::string o)
{
	// find this customName in our list.
	int bFound = -1;
	for (unsigned int i = 0; i < customs.size(); i++)
	{
		if( name.compare( customs[i].name ) == 0 )
		{
			bFound = i;
			break;
		}
	}	

	// if this customName already exist in our list, let's add the field to it
	if (bFound >= 0)
	{
		param field;
		field.set(v, r, o);
		customs[bFound].fields.push_back(field);
	}
	else
	{
		// add this customName to our list
		custom cgdr;
		cgdr.name = name;
		
		// now add the field to custom list
		param field;
		field.set(v, r, o);
		cgdr.fields.push_back(field);

		customs.push_back(cgdr);
	}

}






