/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * $Id: DevRCFenix.h,v 1.0 2022/01/10 10:35:29 root Exp root $
 */

//__Version_1.0____________________________________________Vladimir Fux 12/2015_

//__________________________________________________________compilation_control_

#ifndef __DevRCFenix_H_
#define __DevRCFenix_H_

//_____________________________________________________________________includes_

#include "devLX.h"
#include "devLX_EOS_ERA.h"
#include "dlgTools.h"
#include "Dialogs.h"
#include "Parser.h"
#include "WindowControls.h"


//______________________________________________________________________defines_


//___________________________________________________________class_declarations_

// #############################################################################
// *****************************************************************************
//
//   DevRCFenix
//
// *****************************************************************************
// #############################################################################

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



          
  

bool FenixBlockReceived(void);
class DevRCFenix : public DevLX_EOS_ERA
{
  //----------------------------------------------------------------------------
  public:


  //----------------------------------------------------------------------------
  protected:

    /// task declaration structure for device
    class Decl;

    /// competition class
    class Class;

    //..........................................................................

    /// Protected only constructor - class should not be instantiated.
    DevRCFenix() {}

    /// Installs device specific handlers.
    static void Install(PDeviceDescriptor_t d);

    /// Returns device name (max length is @c DEVNAMESIZE).
    static constexpr
    const TCHAR* GetName() {
      return _T("RC Fenix");  
    }
    static BOOL FormatTP( TCHAR* DeclStrings, int num, int total,const WAYPOINT *wp);
    /// Writes declaration into the logger.
    static BOOL DeclareTask(PDeviceDescriptor_t d,const Declaration_t* lkDecl, unsigned errBufSize, TCHAR errBuf[]);


    /// Send one line of ceclaration to logger
   static bool SendDecl(PDeviceDescriptor_t d, unsigned row, unsigned n_rows, TCHAR content[], unsigned errBufSize, TCHAR errBuf[]);

   static BOOL ParseNMEA(PDeviceDescriptor_t d, TCHAR* sentence, NMEA_INFO* info);
   static BOOL FenixParseStream(DeviceDescriptor_t *d, char *String, int len, NMEA_INFO *GPS_INFO);
   
   static BOOL Config(PDeviceDescriptor_t d);
   static void OnCloseClicked(WndButton* pWnd);

   static void OnValuesClicked(WndButton* pWnd);

   static BOOL RCDT(PDeviceDescriptor_t d, const TCHAR* sentence, NMEA_INFO* info);
   static BOOL LXBC(PDeviceDescriptor_t d, const TCHAR* sentence, NMEA_INFO* info);
   static BOOL SENS(PDeviceDescriptor_t d, const TCHAR* sentence, NMEA_INFO* info, int ParNo);
   static BOOL SetupFenix_Sentence(PDeviceDescriptor_t d);
   static BOOL PutTarget(PDeviceDescriptor_t d);


   static void GetDirections(WndButton* pWnd);

   static BOOL FenixRadioEnabled(PDeviceDescriptor_t d) { return false;};
   static BOOL FenixPutMacCready(PDeviceDescriptor_t d, double MacCready);
   static BOOL FenixPutBallast(PDeviceDescriptor_t d, double Ballast);
   static BOOL FenixPutBugs(PDeviceDescriptor_t d, double Bugs);

}; // DevRCFenix


// #############################################################################
// *****************************************************************************
//
//   DevRCFenix::Decl
//
// *****************************************************************************
// #############################################################################

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// LX task declaration data.
/// This data are byte-by-byte sent to device.
///



class DevRCFenix::Decl
{
  //----------------------------------------------------------------------------
  public:

    /// class constants
    enum Consts
    {
      min_wp_count =  4,  ///< minimum waypoint count
      max_wp_count = 12,  ///< maximum waypoint count
    }; // Consts

    /// String member ID (all explicitly defined ids here must have value <0)
    enum StrId
    {
      fl_pilot    = -1,
      fl_glider   = -2,
      fl_reg_num  = -3,
      fl_cmp_num  = -4,
      fl_observer = -5,
      fl_gps      = -6,
    }; // StrId

    /// Competitions Class
    enum Class
    {
      cls_standard    = 0,
      cls_15_meter    = 1,
      cls_open        = 2,
      cls_18_meter    = 3,
      cls_world       = 4,
      cls_double      = 5,
      cls_motor_gl    = 6,
      cls_textdef     = 7, ///< class will be written with PKT_CCWRITE)
    }; // Class

    /// waypoint type
    enum WpType
    {
      tp_undef   = 0, ///< waypoint will be ignored
      tp_regular = 1, ///< Start, TP, Finish
      tp_landing = 2,
      tp_takeoff = 3,
    }; // WpType

    //..........................................................................

    /// Constructor - sets all data to 0.
    Decl();

}; // DevRCFenix::Decl



// #############################################################################
// *****************************************************************************
//
//   DevRCFenix::Class
//
// *****************************************************************************
// #############################################################################

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// Fenix task declaration data - competition class.
/// This data are byte-by-byte sent to device.
///
class DevRCFenix::Class
{
  //----------------------------------------------------------------------------
  public:

    /// competition class name
    char  name[9];

    //..........................................................................

    /// Constructor - sets all data to 0.
    Class();

    /// Sets the value of @c name member.



}; // DevRCFenix::Class

//______________________________________________________________________________

#endif // __DevRCFenix_H_
