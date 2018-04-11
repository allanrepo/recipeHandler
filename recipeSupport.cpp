#include "recipeSupport.h"
#include <iostream>

///////////////////////////////////////////////////////
// recipeConfig

CRecipeConfig::CRecipeConfig(): RemoteLocation(), LocalLocation(), ProgLocation(), PackageType(), ConfigurationName()
{
}

CRecipeConfig::CRecipeConfig(const CRecipeConfig& ot): RemoteLocation(ot. RemoteLocation), 
						       LocalLocation(ot.LocalLocation), 
						       ProgLocation(ot.ProgLocation), 
						       PackageType(ot.PackageType),
						       ConfigurationName(ot.ConfigurationName)
{
}

CRecipeConfig& CRecipeConfig::operator=(const CRecipeConfig& ot)
{
    RemoteLocation = ot.RemoteLocation;
    LocalLocation = ot.LocalLocation;
    ProgLocation = ot.ProgLocation;
    PackageType = ot.ProgLocation;
    ConfigurationName = ot.ConfigurationName;
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
    return result;
}

bool CRecipeConfig::printConfigParams()
{
    std::cout << "Printing active RecipeHandler Configuration [" << ConfigurationName << "]..." << std::endl;
    std::cout << "[RemoteLocation]: " << RemoteLocation << std::endl;
    std::cout << "[LocalLocation]: " << LocalLocation << std::endl;
    std::cout << "[ProgLocation]: " << ProgLocation << std::endl;
    std::cout << "[PackageType]: " << PackageType << std::endl;
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

CMIRArgs::CMIRArgs() : LotId(),CmodCod(),FlowId(),DsgnRev(),DateCod(),OperFrq(),OperNam(),NodeNam(),
	  PartTyp(),EngId(),TestTmp(),FacilId(),FloorId(),StatNum(),ProcId(),ModCod(),
	  FamilyId(),PkgTyp(),SblotId(),JobNam(),SetupId(),JobRev(),ExecTyp(),ExecVer(),AuxFile(),
	  RtstCod(),TestCod(),UserText(),RomCod(),SerlNum(),SpecNam(),TstrTyp(),SuprNam(),SpecVer(), ProtCod()
{
}

CMIRArgs::CMIRArgs(const CMIRArgs& ot) : LotId(ot.LotId),CmodCod(ot.CmodCod),FlowId(ot.FlowId),
          DsgnRev(ot.DsgnRev),DateCod(ot.DateCod),OperFrq(ot.OperFrq),OperNam(ot.OperNam),
          NodeNam(ot.NodeNam),PartTyp(ot.PartTyp),EngId(ot.EngId),TestTmp(ot.TestTmp),FacilId(ot.FacilId),
          FloorId(ot.FloorId),StatNum(ot.StatNum),ProcId(ot.ProcId),ModCod(ot.ModCod),
	  FamilyId(ot.FamilyId),PkgTyp(ot.PkgTyp),SblotId(ot.SblotId),
	  JobNam(ot.JobNam),SetupId(ot.SetupId),JobRev(ot.JobRev),ExecTyp(ot.ExecTyp),
	  ExecVer(ot.ExecVer),AuxFile(ot.AuxFile),RtstCod(ot.RtstCod),TestCod(ot.TestCod),
	  UserText(ot.UserText),RomCod(ot.RomCod),SerlNum(ot.SerlNum),SpecNam(ot.SpecNam),TstrTyp(ot.TstrTyp),
	  SuprNam(ot.SuprNam),SpecVer(ot.SpecVer), ProtCod(ot.ProtCod)	
{
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

bool CMIRArgs::clearParams()
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

CSDRArgs::CSDRArgs() : HandTyp(),CardId(),LoadId(),PHId(),DibTyp(),CableId(),ContTyp(),LoadTyp(),ContId(),LaserTyp(),LaserId(),ExtrTyp(),ExtrId(),DibId(),CardTyp(),CableTyp()
{
}

CSDRArgs::CSDRArgs(const CSDRArgs& ot) : HandTyp(ot.HandTyp),CardId(ot.CardId),LoadId(ot.LoadId),PHId(ot.PHId),
					 DibTyp(ot.DibTyp),CableId(ot.CableId),ContTyp(ot.ContTyp),LoadTyp(ot.LoadTyp),ContId(ot.ContId),LaserTyp(ot.LaserTyp),LaserId(ot.LaserId),ExtrTyp(ot.ExtrTyp),ExtrId(ot.ExtrId),
					 DibId(ot.DibId),CardTyp(ot.CardTyp),CableTyp(ot.CableTyp)
{
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

bool CSDRArgs::clearParams()
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
