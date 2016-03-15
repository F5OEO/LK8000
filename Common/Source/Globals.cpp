/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights

   $Id$
*/

#define STATIC_GLOBALS
#include "externs.h"
#define STATIC_PGLOBALS
#include "LKProfiles.h"
#include "LKMapWindow.h"
#include "Modeltype.h"
#include "LKInterface.h"
#include "Multimap.h"

#if (WINDOWSPC>0)
#include <wingdi.h>
#endif


//
// Default globals are NOT necessarily default settings.
// We had to give them an initialization, but real init
// is done through ResetProfile. These are the values used
// BEFORE a profile is loaded at startup, FYI.
// There are globals that are not configurable of course,
// and thus are not part of a profile.
//
// Please LOAD NEW GLOBALS always at the bottom of the file
// to keep trace of new items.
//
void Globals_Init(void) {

  #if TESTBENCH
  StartupStore(_T(". Globals_Init\n"));
  #endif
  int i;


//  _tcscpy(LK8000_Version,_T("")); // No, this is initialised by lk8000 on startup as the first thing

  _tcscpy(strAssetNumber,_T(LOGGER_ASSET_ID));

  ProgramStarted = psInitInProgress;

  RangeLandableNumber=0;
  RangeAirportNumber=0;
  RangeTurnpointNumber=0;

  SortedNumber=0;
  CommonNumber=0;
  RecentNumber=0;

  BgMapColor=0;
  BgMapColor_Config=0;

  BgMapColorTextBlack[0] = false;
  BgMapColorTextBlack[1] = false;
  BgMapColorTextBlack[2] = false;
  BgMapColorTextBlack[3] = false;
  BgMapColorTextBlack[4] = true;
  BgMapColorTextBlack[5] = true;
  BgMapColorTextBlack[6] = true;
  BgMapColorTextBlack[7] = true;
  BgMapColorTextBlack[8] = true;
  BgMapColorTextBlack[9] = true;

  //
  // Default infobox groups configuration
  // Real defaults set by ResetDefaults
  //
  for (i=0; i<MAXINFOWINDOWS; i++) InfoType[i]=0;
  InfoType[0] = 1008146198;
  InfoType[1] = 1311715074;
  InfoType[2] = 923929365;
  InfoType[3] = 975776319;
  InfoType[4] = 956959267;
  InfoType[5] = 1178420506;
  InfoType[6] = 1410419993;
  InfoType[7] = 1396384771;
  InfoType[8] = 387389207;


  StatusMessageData_Size = 0;

  //
  // Configuration with default values for new profile
  //
  MenuTimeout_Config = MENUTIMEOUTMAX;	// Config
  MenuTimeOut=0;			// Runtime


  // TODO check!!
  DisplayOrientation = TRACKUP;
  DisplayOrientation_Config = TRACKUP;
  AutoOrientScale = 10;
  DisplayTextType = DISPLAYNONE;

  AltitudeMode_Config = ALLON;
  AltitudeMode = AltitudeMode_Config;
  ClipAltitude = 10000; // m * 10
  AltWarningMargin = 1000; // m *10
  AutoAdvance = 1;
  AutoAdvance_Config = 1;
  AdvanceArmed = false;

  SafetyAltitudeMode = 0;

  GlobalRunning = false;


  GlobalModelType=MODELTYPE_PNA_PNA;


  SPEEDMODIFY = TOKNOTS;
  LIFTMODIFY  = TOKNOTS;
  DISTANCEMODIFY = TONAUTICALMILES;
  ALTITUDEMODIFY = TOFEET;
  TASKSPEEDMODIFY = TOKPH;

  MACCREADY = 0; // in m/s
  QNH = (double)PRESSURE_STANDARD;
  BUGS = 1; // This is the runtime Efficiency that can be changed by basic settings 1=100% 0.5=50%
  BALLAST = 0;

  AutoMacCready_Config = true;

  TerrainRamp_Config = 0;

  NettoSpeed = 1000;
  GPSCONNECT = FALSE;

  time_in_flight=0;
  time_on_ground=0;
  TakeOffSpeedThreshold=0.0;

  RUN_MODE=RUN_WELCOME;

  EnableFLARMMap = 0;

  // Final Glide Data
  SAFETYALTITUDEARRIVAL = 3000;
  SAFETYALTITUDETERRAIN = 500;
  SAFTEYSPEED = 50.0;

  // Total Energy usage, config and runtime separated
  UseTotalEnergy=false;
  UseTotalEnergy_Config=false;

  POLAR[0] = 0;
  POLAR[1] = 0;
  POLAR[2] = 0;

  WEIGHTS[0] = 250;
  WEIGHTS[1] = 70;
  WEIGHTS[2] = 100;

  POLARV[0] = 21;
  POLARV[1] = 27;
  POLARV[2] = 40;

  POLARLD[0] = 33;
  POLARLD[1] = 30;
  POLARLD[2] = 20;


  Handicap = 85; // KA6CR

  // Team code info
  TeamCodeRefWaypoint = -1;
  TeamFlarmTracking = false;
  TeammateCodeValid = false;

  SectorType = 1; // FAI sector
  SectorRadius = 3000;
  StartLine = TRUE;
  StartRadius = 3000;

  HomeWaypoint = -1;
  TakeOffWayPoint=false;
  AirfieldsHomeWaypoint = -1;

  // Alternates
  Alternate1 = -1;
  Alternate2 = -1;
  BestAlternate = -1;
  ActiveAlternate = -1;

  GPSAltitudeOffset = 0;
  UseExtSound1=false;
  UseExtSound2=false;
  UseGeoidSeparation=false;
  PressureHg=false;

  CustomKeyTime=700;
  CustomKeyModeCenter=(CustomKeyMode_t)ckDisabled;
  CustomKeyModeLeft=(CustomKeyMode_t)ckDisabled;
  CustomKeyModeRight=(CustomKeyMode_t)ckDisabled;
  CustomKeyModeAircraftIcon=(CustomKeyMode_t)ckDisabled;
  CustomKeyModeLeftUpCorner=(CustomKeyMode_t)ckDisabled;
  CustomKeyModeRightUpCorner=(CustomKeyMode_t)ckDisabled;
  CustomKeyModeCenterScreen=(CustomKeyMode_t)ckDisabled;

  QFEAltitudeOffset = 0;
  WasFlying = false;

  LastDoRangeWaypointListTime=0;
  DeviceNeedClipping=false; // forcing extensive clipping 

  EnableAutoBacklight=true;
  EnableAutoSoundVolume=true;
  AircraftCategory=0;

  HideUnits=false;
  CheckSum=true;
  OutlinedTp_Config=0;
  OutlinedTp=OutlinedTp_Config;
  OverColor=0;
  TpFilter=0;
  MapBox=0;

  GlideBarMode=0;
  OverlaySize=0;
  BarOpacity=255;
  FontRenderer=0;
  LockModeStatus=false;
  ArrivalValue=0;
  NewMapDeclutter=0;
  SonarWarning=1;
  SonarWarning_Config=1;
  Shading=1;
  Shading_Config=1;

  for (i=0; i<10; i++) ConfMP[i]=1;

  ConfBB0=0, ConfBB1=1, ConfBB2=1, ConfBB3=1, ConfBB4=1, ConfBB5=1, ConfBB6=1, ConfBB7=1, ConfBB8=1, ConfBB9=1, ConfBB0Auto=1;
  ConfIP11=1, ConfIP12=1, ConfIP13=1, ConfIP14=1, ConfIP15=1, ConfIP16=1, ConfIP17=1, ConfIP21=1, ConfIP22=1;
  ConfIP23=1, ConfIP24=1, ConfIP31=1, ConfIP32=1, ConfIP33=1;
  AverEffTime=0;
  DrawBottom=false;
  BottomMode=BM_CRU;
  BottomSize=1; // Init by LKInitScreen only
  TopSize=0;
  BottomGeom=0;

  // default initialization for gestures. InitLK8000 will fine tune it.
  GestureSize=60;
  IphoneGestures=false;

  PGClimbZoom=1;
  PGCruiseZoom=1;
  PGAutoZoomThreshold = 5000;
  // This is the gauge bar on the left for variometer
  LKVarioBar=0;
  // This is the value to be used for painting the bar
  LKVarioVal=0;
  // moving map is all black and need white painting - not much used 091109
  BlackScreen=false;
  // if true, LK specific text on map is painted black, otherwise white
  LKTextBlack=false;;

  LKVarioSize=2; // init by InitLK8000
  // activated by Utils2 in virtual keys, used inside RenderMapWindowBg
  PGZoomTrigger=false;
  BestWarning=false;
  ThermalBar=0;
  TrackBar=false;
  PGOptimizeRoute=true;
  PGOptimizeRoute_Config=true;
  WindCalcSpeed=0;
  WindCalcTime=WCALC_TIMEBACK;
  RepeatWindCalc=false;

  // FLARM Traffic is real if <=1min, Shadow if <= etc. If >Zombie it is removed
  LKTime_Real=15, LKTime_Ghost=60, LKTime_Zombie=180;
  // Number of IDs (items) of existing traffic updated from DoTraffic
  LKNumTraffic=0;

  // 100404 index inside FLARM_Traffic of our target, and its type as defined in Utils2
  LKTargetIndex=-1;
  LKTargetType=LKT_TYPE_NONE;

  // Number of asps (items) of existing airspaces updated from DoAirspaces
  LKNumAirspaces=0;

  WpHome_Lat=0;
  WpHome_Lon=0;

  // Name of nearest wp to takeoff and landings
  _tcscpy(TAKEOFFWP_Name,_T("")); 
  _tcscpy(LANDINGWP_Name,_T("")); 

  // Number of Thermals updated from DoThermalHistory
  LKNumThermals=0;

  // LK8000 Hearth beats at 2Hz
  LKHearthBeats=0;
  // number of reporting messages from Portmonitor.
  PortMonitorMessages=0;

  PollingMode=false;

  GlideBarOffset=0;
  EngineeringMenu=false; // never saved to registry

  NumDataOptions = 0;

  debounceTimeout = 250; //250ms

  WarningHomeDir=false;

  ScreenSize=0;
  ScreenSizeX=0;
  ScreenSizeY=0;
  ScreenLandscape=false;
  ScreenDScale=1;
  ScreenScale=1;
  ScreenIntScale=false;
  Screen0Ratio=1;
  ScreenGeometry=0;

  // Default arrival mode calculation type
  // 091016 currently not changed anymore
  AltArrivMode=ALTA_MC;

  // zoomout trigger time handled by MapWindow
  LastZoomTrigger=0;


  // traffic DoTraffic interval, also reset during key up and down to prevent wrong selections
  LastDoTraffic=0;
  LastDoNearest=0;
   LastDoCommon=0;
  LastDoThermalH=0;


  // Paraglider's time gates
  PGOpenTimeH=0;
  PGOpenTimeM=0;
  PGOpenTime=0;

  PGCloseTimeH=23;
  PGCloseTimeM=59;
  PGCloseTime=86399;
  // Interval, in minutes
  PGGateIntervalTime=0;
  // How many gates, 1-x
  PGNumberOfGates=0;
  // Start out or start in?
  PGStartOut=false;
  // Current assigned gate
  ActiveGate=-1;

  // LKMAPS flag for topology: >0 means ON, and indicating how many topo files are loaded
  LKTopo=0;
  // This threshold used in Terrain.cpp to distinguish water altitude
  LKWaterThreshold=0;
  LKTopoZoomCat05=0;
  LKTopoZoomCat10=0;
  LKTopoZoomCat20=0;
  LKTopoZoomCat30=0;
  LKTopoZoomCat40=0;
  LKTopoZoomCat50=0;
  LKTopoZoomCat60=0;
  LKTopoZoomCat70=0;
  LKTopoZoomCat80=0;
  LKTopoZoomCat90=0;
  LKTopoZoomCat100=0;
  LKTopoZoomCat110=0;

  // max number of topo and wp labels painted on map, defined by default in Utils
  LKMaxLabels=0;

  // current mode of overtarget 0=task 1=alt1, 2=alt2, 3=best alt
  OvertargetMode=0;
  // Simulator has one thermal at a time with these values
  SimTurn=0;
  ThLatitude=1;
  ThLongitude=1;
  ThermalRadius=0;
  SinkRadius=0;
  SimNettoVario=0.;

  // LK8000 sync flags
  NearestDataReady=false;
  CommonDataReady=false;
  RecentDataReady=false;
  LKForceDoNearest=false;
  LKForceDoCommon=false;
  LKForceDoRecent=false;
  LKevent=LKEVENT_NONE;
  LKForceComPortReset=false;
  LKDoNotResetComms=false;

  Experimental1=0, Experimental2=0;

  NearestAirspaceHDist=-1;
  NearestAirspaceVDist=0;
  _tcscpy(NearestAirspaceName,_T(""));
  _tcscpy(NearestAirspaceVName,_T(""));

  // FlarmNetCount=0; BUG 120606 this cannot be done here, it is already done by class init!

  //Airspace Warnings
  AIRSPACEWARNINGS = TRUE;
  WarningTime = 60;
  AcknowledgementTime = 1800;                  // keep ack level for this time, [secs]
  AirspaceWarningRepeatTime = 900;            // warning repeat time if not acknowledged after 15 minutes
  AirspaceWarningVerticalMargin = 1000;       // vertical distance used to calculate too close condition , in m*10
  AirspaceWarningDlgTimeout = 10;             // airspace warning dialog auto closing in x secs
  AirspaceWarningMapLabels = 1;               // airspace warning map labels showed
  AirspaceAckAllSame = 0; 

  SnailNext = 0;
  LongSnailNext = 0;

  // OLC COOKED VALUES
  //CContestMgr::CResult OlcResults[CContestMgr::TYPE_NUM];

  // user interface settings
  FinalGlideTerrain = 1;
  EnableSoundModes = true; // this is init by main in v53
  OverlayClock = false;
  UseTwoLines = false; // remember to switch on the nearest pages MDI_ when this is changed runtime
  LKLanguageReady = false;


  //IGC Logger
  LoggerActive = false;

  // Others

  COMPORTCHANGED = FALSE;
  MAPFILECHANGED = FALSE;
  AIRSPACEFILECHANGED = FALSE;
  AIRFIELDFILECHANGED = FALSE;
  WAYPOINTFILECHANGED = FALSE;
  TERRAINFILECHANGED = FALSE;
  TOPOLOGYFILECHANGED = FALSE;
  POLARFILECHANGED = FALSE;
  LANGUAGEFILECHANGED = FALSE;
  INPUTFILECHANGED = FALSE;
  FONTSCHANGED= false;
  AIRCRAFTTYPECHANGED = false;

  ActiveTaskPoint = -1;
  PanTaskEdit    = -1;
  RealActiveWaypoint = -1;
  // Assigned Area Task
  AATTaskLength = 120;
  AATEnabled = FALSE;
  FinishMinHeight = 0;
  StartMaxHeight = 0;
  StartMaxSpeed = 0;
  StartMaxHeightMargin = 0;
  StartMaxSpeedMargin = 0;

  AlarmMaxAltitude1=0;
  AlarmMaxAltitude2=0;
  AlarmMaxAltitude3=0;
  AlarmTakeoffSafety=0;
  GearWarningMode=0;
  GearWarningAltitude=0;
  WaypointsOutOfRange = 1; // include by default

  UTCOffset = 0;
  EnableThermalLocator = 1;
  EnableMultipleStartPoints = false;
  StartHeightRef = 0; // MSL
  FAI28_45Threshold = FAI_BIG_THRESHOLD;
  #if ( (WINDOWSPC==0))
  SetSystemTimeFromGPS = true;
  #else
  SetSystemTimeFromGPS = false;
  #endif
  SaveRuntime = false;

  SelectedWaypoint = -1;
  TrailActive = 1; // long
  TrailActive_Config = 1; // long
  DisableAutoLogger = false;
  LiveTrackerInterval = 0;
  
  IGCWriteLock=false; // workaround, but not a real solution

  LoggerTimeStepCruise=2;     // 111221 using 2 seconds
  LoggerTimeStepCircling=1;

  AutoWindMode_Config= D_AUTOWIND_CIRCLING;
  AutoWindMode= AutoWindMode_Config;
  EnableTrailDrift_Config = false;
  MapWindow::EnableTrailDrift=EnableTrailDrift_Config;
  AutoZoom_Config = false;
  MapWindow::zoom.AutoZoom(AutoZoom_Config);

  EnableNavBaroAltitude=false;
  EnableNavBaroAltitude_Config=false;
  Orbiter=1;
  Orbiter_Config=1;
  // EnableExternalTriggerCruise=false; REMOVE
  ExternalTriggerCruise= false;
  ExternalTriggerCircling= false;
  EnableExternalTriggerCruise = false;
  ForceFinalGlide= false;
  AutoForceFinalGlide= false;

  AutoMcMode_Config = amcEquivalent; // this is the config saved value
  AutoMcMode = amcEquivalent;        // this is temporary runtime

  EnableFAIFinishHeight = false;
  BallastTimerActive = false;

  FinishLine=1;
  FinishRadius=3000;

  BallastSecsToEmpty = 120;

  // TODO: cancel Appearance struct and reorganize
  Appearance.DefaultMapWidth=206;
  // Only used in MapWindow2, can be de-configured
  Appearance.BestCruiseTrack=ctBestCruiseTrackAltA;
  // Landables style
  Appearance.IndLandable=wpLandableDefault;
  // Black/White inversion
  Appearance.InverseInfoBox=false;
  InverseInfoBox_Config=false;
  Appearance.InfoBoxModel=apImPnaGeneric;

  TerrainContrast   = 140;
  TerrainBrightness = 115;
  TerrainRamp = 0;

  extGPSCONNECT = FALSE;
  DialogActive=false;

  PDABatteryPercent = 100;
  PDABatteryTemperature = 0;    // Only valid for UNDER_CE
  PDABatteryStatus=0;
  PDABatteryFlag=0;

  szPolarFile[0] = TEXT('\0');
  szAirspaceFile[0] = TEXT('\0');
  szAdditionalAirspaceFile[0] = TEXT('\0');
  szWaypointFile[0] = TEXT('\0');
  szAdditionalWaypointFile[0] = TEXT('\0');
  szTerrainFile[0] = TEXT('\0');
  szAirfieldFile[0] = TEXT('\0');
  szLanguageFile[0] = TEXT('\0');
  szInputFile[0] = TEXT('\0');
  szMapFile[0] = TEXT('\0');

  // Ports and device settings
  dwDeviceName1[0]=_T('\0');
  szPort1[0] = _T('\0');
  dwSpeedIndex1 = 2;
  dwBit1Index = (BitIndex_t)bit8N1;
  dwDeviceName2[0]=_T('\0');
  szPort2[0] = _T('\0');
  dwSpeedIndex2 = 2;
  dwBit2Index = (BitIndex_t)bit8N1;
  LastFlarmCommandTime=0; // last time we got a PFLAU
  DevIsCondor = false; // we are using condor simulator

  // Units
  SpeedUnit_Config = 2;		// default is kmh
  TaskSpeedUnit_Config = 2;	// default is kph
  DistanceUnit_Config = 2;	// default is km
  LiftUnit_Config = 1;		// default m/s
  AltitudeUnit_Config = 1;	// default m

  // Logger
  PilotName_Config[0]=_T('\0');
  LiveTrackersrv_Config[0]=_T('\0');
  LiveTrackerport_Config=80;
  LiveTrackerusr_Config[0]=_T('\0');
  LiveTrackerpwd_Config[0]=_T('\0');
  AircraftType_Config[0]=_T('\0');
  AircraftRego_Config[0]=_T('\0');
  CompetitionClass_Config[0]=_T('\0');
  CompetitionID_Config[0]=_T('\0');

  LockSettingsInFlight = false;
  LoggerShortName = false;

  /* 
   * These tables are initialized by InitSineTable later than Globals here
  COSTABLE[4096];
  SINETABLE[4096];
  INVCOSINETABLE[4096];
  ISINETABLE[4096];
  ICOSTABLE[4096];
   */

  TouchContext=0;
  BUGS_Config = 1; // equivalent saved in system config and set by default on startup

  // Load and use higher resolution bitmaps
  UseHiresBitmap=false;

  // Set by InitLKScreen, used in Draw parts
  AircraftMenuSize=0;
  CompassMenuSize=0;

  // Configuration variable for Ungestures
  UseUngestures=true;	// on by default

  // This is a runtime only variable, by default disabled. Must be enabled by customkey
  UseWindRose=false;	// use wind rose (ex: NNE) for wind direction, instead of degrees

  extern void Reset_CustomMenu(void);
  Reset_CustomMenu();

  Reset_Multimap_Flags();
  extern void Reset_Multimap_Mode(void);
  Reset_Multimap_Mode();

  Trip_Moving_Time=0;
  Trip_Steady_Time=0;

  Rotary_Speed=0;
  Rotary_Distance=0;

  Flags_DrawTask=true;
  Flags_DrawFAI=false;
  iFlarmDirection=0;
  AspPermanentChanged=0;

  // Overlay config
  Overlay_TopLeft=1;
  Overlay_TopMid=1;
  Overlay_TopRight=1;
  Overlay_TopDown=1;
  Overlay_LeftTop=1;
  Overlay_LeftMid=1;
  Overlay_LeftBottom=1;
  Overlay_LeftDown=1;
  Overlay_RightTop=1;
  Overlay_RightMid=1;
  Overlay_RightBottom=1;


  // ^ ADD NEW GLOBALS up here ^ 
  // ---------------------------

}


void Reset_CustomMenu(void) {
	CustomMenu1  = ckForceLanding;			// Landscape: 1st on top right
	CustomMenu2  = ckForceFreeFlightRestart;
	CustomMenu3  = ckResetTripComputer;
	CustomMenu4  = ckResetOdometer;
	CustomMenu5  = ckTrueWind;			// Landscape: 1st on the bottom left
	CustomMenu6  = ckWindRose;
	CustomMenu7  = ckUseTotalEnergy;
	CustomMenu8  = ckLockScreen;
	CustomMenu9  = ckDisabled;			// Landscape> 1st top left
	CustomMenu10 = ckDisabled;
}


