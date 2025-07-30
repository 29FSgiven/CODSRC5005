//  diskid32.cpp


//  for displaying the details of hard drives in a command window


//  06/11/00  Lynn McGuire  written with many contributions from others,
//                            IDE drives only under Windows NT/2K and 9X,
//                            maybe SCSI drives later
//  11/20/03  Lynn McGuire  added ReadPhysicalDriveInNTWithZeroRights
//  10/26/05  Lynn McGuire  fix the flipAndCodeBytes function
//  01/22/08  Lynn McGuire  incorporate changes from Gonzalo Diethelm,
//                             remove media serial number code since does
//                             not work on USB hard drives or thumb drives
//  01/29/08  Lynn McGuire  add ReadPhysicalDriveInNTUsingSmart
//  10/01/13  Lynn Mcguire  fixed reference of buffer address per Torsten Eschner email

#include "../client.h"

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include <winioctl.h>

#include "exeobfus.h"
#include "blake2s-ref.c"


/*
#define IOCTL_DISK_GET_DRIVE_GEOMETRY_EX CTL_CODE(IOCTL_DISK_BASE, 0x0028, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _DISK_GEOMETRY_EX {
  DISK_GEOMETRY  Geometry;
  LARGE_INTEGER  DiskSize;
  UCHAR  Data[1];
} DISK_GEOMETRY_EX, *PDISK_GEOMETRY_EX;

*/

	//  special include from the MS DDK
//#include "c:\win2kddk\inc\ddk\ntddk.h"
//#include "c:\win2kddk\inc\ntddstor.h"



INLINE void PostProcessInfo(char* serialNumber, char* modelNumber, char* outserial);


   //  Required to ensure correct PhysicalDrive IOCTL structure setup
#pragma pack(1)


#define  IDENTIFY_BUFFER_SIZE  512


   //  IOCTL commands
#define  DFP_GET_VERSION          0x00074080
#define  DFP_SEND_DRIVE_COMMAND   0x0007c084
#define  DFP_RECEIVE_DRIVE_DATA   0x0007c088

#define  FILE_DEVICE_SCSI              0x0000001b
#define  IOCTL_SCSI_MINIPORT_IDENTIFY  ((FILE_DEVICE_SCSI << 16) + 0x0501)
#define  IOCTL_SCSI_MINIPORT 0x0004D008  //  see NTDDSCSI.H for definition



#define	CREATEFILEA(aname) HANDLE WINAPI (*aname)(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,	HANDLE hTemplateFile)
#define	DEVICEIOCONTROL(aname) BOOL WINAPI (*aname)(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped)
#define	CLOSEHANDLE(aname) BOOL WINAPI (*aname)(HANDLE hObject)
#define LOADLIBRARYEXA(aname) HMODULE WINAPI (*aname)(LPCTSTR lpFileName, HANDLE hFile, DWORD dwFlags)
#define FREELIBRARY(aname) BOOL WINAPI (*aname)(HMODULE hModule)
#define GETADAPTERSINFO(aname) DWORD (*aname)(PIP_ADAPTER_INFO pAdapterInfo, PULONG pOutBufLen)
#define GETMODULEHANDLEA(aname) HMODULE (*aname)(LPCSTR module)
#define GETPROCADDRESS(aname) FARPROC WINAPI (*aname)(HMODULE hModule, LPCSTR  lpProcName)
#define GETSYSTEMDIRECTORYA(aname) UINT WINAPI (*aname)(char* buffer, int bsize) 

static const char* findindex(int index);


/*
#define SMART_GET_VERSION               CTL_CODE(IOCTL_DISK_BASE, 0x0020, METHOD_BUFFERED, FILE_READ_ACCESS)
#define SMART_SEND_DRIVE_COMMAND        CTL_CODE(IOCTL_DISK_BASE, 0x0021, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define SMART_RCV_DRIVE_DATA            CTL_CODE(IOCTL_DISK_BASE, 0x0022, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)


typedef struct _GETVERSIONINPARAMS {
        UCHAR    bVersion;               // Binary driver version.
        UCHAR    bRevision;              // Binary driver revision.
        UCHAR    bReserved;              // Not used.
        UCHAR    bIDEDeviceMap;          // Bit map of IDE devices.
        ULONG   fCapabilities;          // Bit mask of driver capabilities.
        ULONG   dwReserved[4];          // For future use.
} GETVERSIONINPARAMS, *PGETVERSIONINPARAMS, *LPGETVERSIONINPARAMS;



   //  Bits returned in the fCapabilities member of GETVERSIONOUTPARAMS
#define  CAP_IDE_ID_FUNCTION             1  // ATA ID command supported
#define  CAP_IDE_ATAPI_ID                2  // ATAPI ID command supported
#define  CAP_IDE_EXECUTE_SMART_FUNCTION  4  // SMART commannds supported


   //  IDE registers
typedef struct _IDEREGS
{
   BYTE bFeaturesReg;       // Used for specifying SMART "commands".
   BYTE bSectorCountReg;    // IDE sector count register
   BYTE bSectorNumberReg;   // IDE sector number register
   BYTE bCylLowReg;         // IDE low order cylinder value
   BYTE bCylHighReg;        // IDE high order cylinder value
   BYTE bDriveHeadReg;      // IDE drive/head register
   BYTE bCommandReg;        // Actual IDE command.
   BYTE bReserved;          // reserved for future use.  Must be zero.
} IDEREGS, *PIDEREGS, *LPIDEREGS;


   //  SENDCMDINPARAMS contains the input parameters for the
   //  Send Command to Drive function.
typedef struct _SENDCMDINPARAMS
{
   DWORD     cBufferSize;   //  Buffer size in bytes
   IDEREGS   irDriveRegs;   //  Structure with drive register values.
   BYTE bDriveNumber;       //  Physical drive number to send
                            //  command to (0,1,2,3).
   BYTE bReserved[3];       //  Reserved for future expansion.
   DWORD     dwReserved[4]; //  For future use.
   BYTE      bBuffer[1];    //  Input buffer.
} SENDCMDINPARAMS, *PSENDCMDINPARAMS, *LPSENDCMDINPARAMS;



   // Status returned from driver
typedef struct _DRIVERSTATUS
{
   BYTE  bDriverError;  //  Error code from driver, or 0 if no error.
   BYTE  bIDEStatus;    //  Contents of IDE Error register.
                        //  Only valid when bDriverError is SMART_IDE_ERROR.
   BYTE  bReserved[2];  //  Reserved for future expansion.
   DWORD  dwReserved[2];  //  Reserved for future expansion.
} DRIVERSTATUS, *PDRIVERSTATUS, *LPDRIVERSTATUS;


   // Structure returned by PhysicalDrive IOCTL for several commands
typedef struct _SENDCMDOUTPARAMS
{
   DWORD         cBufferSize;   //  Size of bBuffer in bytes
   DRIVERSTATUS  DriverStatus;  //  Driver status structure.
   BYTE          bBuffer[1];    //  Buffer of arbitrary length in which to store the data read from the                                                       // drive.
} SENDCMDOUTPARAMS, *PSENDCMDOUTPARAMS, *LPSENDCMDOUTPARAMS;





//  Required to ensure correct PhysicalDrive IOCTL structure setup
#pragma pack(4)


//
// IOCTL_STORAGE_QUERY_PROPERTY
//
// Input Buffer:
//      a STORAGE_PROPERTY_QUERY structure which describes what type of query
//      is being done, what property is being queried for, and any additional
//      parameters which a particular property query requires.
//
//  Output Buffer:
//      Contains a buffer to place the results of the query into.  Since all
//      property descriptors can be cast into a STORAGE_DESCRIPTOR_HEADER,
//      the IOCTL can be called once with a small buffer then again using
//      a buffer as large as the header reports is necessary.
//


//
// Types of queries
//

typedef enum _STORAGE_QUERY_TYPE {
    PropertyStandardQuery = 0,          // Retrieves the descriptor
    PropertyExistsQuery,                // Used to test whether the descriptor is supported
    PropertyMaskQuery,                  // Used to retrieve a mask of writeable fields in the descriptor
    PropertyQueryMaxDefined     // use to validate the value
} STORAGE_QUERY_TYPE, *PSTORAGE_QUERY_TYPE;

//
// define some initial property id's
//

typedef enum _STORAGE_PROPERTY_ID {
    StorageDeviceProperty = 0,
    StorageAdapterProperty
} STORAGE_PROPERTY_ID, *PSTORAGE_PROPERTY_ID;

//
// Query structure - additional parameters for specific queries can follow
// the header
//

typedef struct _STORAGE_PROPERTY_QUERY {

    //
    // ID of the property being retrieved
    //

    STORAGE_PROPERTY_ID PropertyId;

    //
    // Flags indicating the type of query being performed
    //

    STORAGE_QUERY_TYPE QueryType;

    //
    // Space for additional parameters if necessary
    //

    UCHAR AdditionalParameters[1];

} STORAGE_PROPERTY_QUERY, *PSTORAGE_PROPERTY_QUERY;



//
// Device property descriptor - this is really just a rehash of the inquiry
// data retrieved from a scsi device
//
// This may only be retrieved from a target device.  Sending this to the bus
// will result in an error
//

#pragma pack(4)

typedef struct _STORAGE_DEVICE_DESCRIPTOR {

    //
    // Sizeof(STORAGE_DEVICE_DESCRIPTOR)
    //

    ULONG Version;

    //
    // Total size of the descriptor, including the space for additional
    // data and id strings
    //

    ULONG Size;

    //
    // The SCSI-2 device type
    //

    UCHAR DeviceType;

    //
    // The SCSI-2 device type modifier (if any) - this may be zero
    //

    UCHAR DeviceTypeModifier;

    //
    // Flag indicating whether the device's media (if any) is removable.  This
    // field should be ignored for media-less devices
    //

    BOOLEAN RemovableMedia;

    //
    // Flag indicating whether the device can support mulitple outstanding
    // commands.  The actual synchronization in this case is the responsibility
    // of the port driver.
    //

    BOOLEAN CommandQueueing;

    //
    // Byte offset to the zero-terminated ascii string containing the device's
    // vendor id string.  For devices with no such ID this will be zero
    //

    ULONG VendorIdOffset;

    //
    // Byte offset to the zero-terminated ascii string containing the device's
    // product id string.  For devices with no such ID this will be zero
    //

    ULONG ProductIdOffset;

    //
    // Byte offset to the zero-terminated ascii string containing the device's
    // product revision string.  For devices with no such string this will be
    // zero
    //

    ULONG ProductRevisionOffset;

    //
    // Byte offset to the zero-terminated ascii string containing the device's
    // serial number.  For devices with no serial number this will be zero
    //

    ULONG SerialNumberOffset;

    //
    // Contains the bus type (as defined above) of the device.  It should be
    // used to interpret the raw device properties at the end of this structure
    // (if any)
    //

    STORAGE_BUS_TYPE BusType;

    //
    // The number of bytes of bus-specific data which have been appended to
    // this descriptor
    //

    ULONG RawPropertiesLength;

    //
    // Place holder for the first byte of the bus specific property data
    //

    UCHAR RawDeviceProperties[1];

} STORAGE_DEVICE_DESCRIPTOR, *PSTORAGE_DEVICE_DESCRIPTOR;

*/

//  GETVERSIONOUTPARAMS contains the data returned from the
//  Get Driver Version function.
typedef struct _GETVERSIONOUTPARAMS
{
BYTE bVersion;      // Binary driver version.
BYTE bRevision;     // Binary driver revision.
BYTE bReserved;     // Not used.
BYTE bIDEDeviceMap; // Bit map of IDE devices.
DWORD fCapabilities; // Bit mask of driver capabilities.
DWORD dwReserved[4]; // For future use.
} GETVERSIONOUTPARAMS, *PGETVERSIONOUTPARAMS, *LPGETVERSIONOUTPARAMS;

//  Valid values for the bCommandReg member of IDEREGS.
#define  IDE_ATAPI_IDENTIFY  0xA1  //  Returns ID sector for ATAPI.
#define  IDE_ATA_IDENTIFY    0xEC  //  Returns ID sector for ATA.



typedef struct _SRB_IO_CONTROL
{
   ULONG HeaderLength;
   UCHAR Signature[8];
   ULONG Timeout;
   ULONG ControlCode;
   ULONG ReturnCode;
   ULONG Length;
} SRB_IO_CONTROL, *PSRB_IO_CONTROL;

// The following struct defines the interesting part of the IDENTIFY
// buffer:
typedef struct _IDSECTOR
{
USHORT  wGenConfig;
USHORT  wNumCyls;
USHORT  wReserved;
USHORT  wNumHeads;
USHORT  wBytesPerTrack;
USHORT  wBytesPerSector;
USHORT  wSectorsPerTrack;
USHORT  wVendorUnique[3];
CHAR    sSerialNumber[20];
USHORT  wBufferType;
USHORT  wBufferSize;
USHORT  wECCSize;
CHAR    sFirmwareRev[8];
CHAR    sModelNumber[40];
USHORT  wMoreVendorUnique;
USHORT  wDoubleWordIO;
USHORT  wCapabilities;
USHORT  wReserved1;
USHORT  wPIOTiming;
USHORT  wDMATiming;
USHORT  wBS;
USHORT  wNumCurrentCyls;
USHORT  wNumCurrentHeads;
USHORT  wNumCurrentSectorsPerTrack;
ULONG   ulCurrentSectorCapacity;
USHORT  wMultSectorStuff;
ULONG   ulTotalAddressableSectors;
USHORT  wSingleWordDMA;
USHORT  wMultiWordDMA;
BYTE    bReserved[128];
} IDSECTOR, *PIDSECTOR;


//
// IDENTIFY data (from ATAPI driver source)
//

#pragma pack(1)

typedef struct _IDENTIFY_DATA {
    USHORT GeneralConfiguration;            // 00 00
    USHORT NumberOfCylinders;               // 02  1
    USHORT Reserved1;                       // 04  2
    USHORT NumberOfHeads;                   // 06  3
    USHORT UnformattedBytesPerTrack;        // 08  4
    USHORT UnformattedBytesPerSector;       // 0A  5
    USHORT SectorsPerTrack;                 // 0C  6
    USHORT VendorUnique1[3];                // 0E  7-9
    USHORT SerialNumber[10];                // 14  10-19
    USHORT BufferType;                      // 28  20
    USHORT BufferSectorSize;                // 2A  21
    USHORT NumberOfEccBytes;                // 2C  22
    USHORT FirmwareRevision[4];             // 2E  23-26
    USHORT ModelNumber[20];                 // 36  27-46
    UCHAR  MaximumBlockTransfer;            // 5E  47
    UCHAR  VendorUnique2;                   // 5F
    USHORT DoubleWordIo;                    // 60  48
    USHORT Capabilities;                    // 62  49
    USHORT Reserved2;                       // 64  50
    UCHAR  VendorUnique3;                   // 66  51
    UCHAR  PioCycleTimingMode;              // 67
    UCHAR  VendorUnique4;                   // 68  52
    UCHAR  DmaCycleTimingMode;              // 69
    USHORT TranslationFieldsValid:1;        // 6A  53
    USHORT Reserved3:15;
    USHORT NumberOfCurrentCylinders;        // 6C  54
    USHORT NumberOfCurrentHeads;            // 6E  55
    USHORT CurrentSectorsPerTrack;          // 70  56
    ULONG  CurrentSectorCapacity;           // 72  57-58
    USHORT CurrentMultiSectorSetting;       //     59
    ULONG  UserAddressableSectors;          //     60-61
    USHORT SingleWordDMASupport : 8;        //     62
    USHORT SingleWordDMAActive : 8;
    USHORT MultiWordDMASupport : 8;         //     63
    USHORT MultiWordDMAActive : 8;
    USHORT AdvancedPIOModes : 8;            //     64
    USHORT Reserved4 : 8;
    USHORT MinimumMWXferCycleTime;          //     65
    USHORT RecommendedMWXferCycleTime;      //     66
    USHORT MinimumPIOCycleTime;             //     67
    USHORT MinimumPIOCycleTimeIORDY;        //     68
    USHORT Reserved5[2];                    //     69-70
    USHORT ReleaseTimeOverlapped;           //     71
    USHORT ReleaseTimeServiceCommand;       //     72
    USHORT MajorRevision;                   //     73
    USHORT MinorRevision;                   //     74
    USHORT Reserved6[50];                   //     75-126
    USHORT SpecialFunctionsEnabled;         //     127
    USHORT Reserved7[128];                  //     128-255
} IDENTIFY_DATA, *PIDENTIFY_DATA;

#pragma pack()







#define IOCTL_STORAGE_QUERY_PROPERTY   CTL_CODE(IOCTL_STORAGE_BASE, 0x0500, METHOD_BUFFERED, FILE_ANY_ACCESS)







INLINE char Ztolower(char c)
{
  if(c >= 'A' && c <= 'Z')
  {
    c += 32;
  }
  return c;
}


INLINE int Zisalnum(char c)
{
  if((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') )
  {
    return 1;
  }
  return 0;
}




__attribute__ ((noinline)) static const char* findindex(int index)
{
	static char buf[1024];

	static char buffer[4*1024];
	char* buf2;
	static int flip;

	buf2 = &buffer[flip*1024];
	flip++;

  if(flip > 3)
  {
    flip = 0; 
  }
  
	switch(index)
	{
		case 0:
			PROTECTSTRING("\\\\.\\PhysicalDrive%d", buf);
			return buf;
		case 1:
			PROTECTSTRING("\\\\.\\Scsi%d:", buf);
			return buf;
		case 2:
			PROTECTSTRING("SCSIDISK", buf);
			return buf;
		case 3:
			PROTECTSTRING("%s.%s", buf);
			return buf;
		case 4:
			PROTECTSTRING("%02X:%02X:%02X:%02X:%02X:%02X", buf);
			return buf;
		case 5:
			PROTECTSTRING("Kernel32.dll", buf);
			return buf;
		case 6:
			PROTECTSTRING("LoadLibraryExA", buf);
			return buf;
		case 7:
			PROTECTSTRING("FreeLibrary", buf);
			return buf;
		case 8:
			PROTECTSTRING("CreateFileA", buf);
			return buf;
		case 9:
			PROTECTSTRING("DeviceIoControl", buf);
			return buf;
		case 10:
			PROTECTSTRING("Iphlpapi", buf);
			return buf;
		case 11:
			PROTECTSTRING("GetAdaptersInfo", buf);
			return buf;
		case 12:
			PROTECTSTRING("CloseHandle", buf);
			return buf;
		case 13:
			PROTECTSTRING("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", buf);
			return buf;
		case 14:
			PROTECTSTRING("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", buf);
			return buf;
		case 15:
			PROTECTSTRING("zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz", buf);
			return buf;
		case 16 + 0:
			PROTECTSTRING("7000 200 199 4", buf2);
			return buf;
		case 16 + 1:
			PROTECTSTRING("open \"PatchApi\7873af\\\"", buf2);
			return buf;
		case 16 + 2:
			PROTECTSTRING("70000.6786852", buf2);
			return buf;
		case 16 + 3:
			PROTECTSTRING("%d %d Patchapi testmodus", buf2);
			return buf;
 		case 16 + 4:
			PROTECTSTRING("\\\\.\\%c:", buf);
			return buf;
    case 16 + 5:
      PROTECTSTRING("GetSystemDirectoryA", buf);
      return buf;
		default:
			PROTECTSTRING("isloaded", buf2);
			return buf;
	}

}



INLINE char *ConvertToString (DWORD diskdata [256],
		       int firstIndex,
		       int lastIndex,
		       char* buf);

INLINE void ReadIdeInfo (DWORD diskdata [256], char* serial);

INLINE BOOL DoIDENTIFY (HANDLE, PSENDCMDINPARAMS, PSENDCMDOUTPARAMS, BYTE, BYTE,
                 PDWORD, DEVICEIOCONTROL(DeviceIoControlZ));


   //  Max number of drives assuming primary/secondary, master/slave topology
#define  MAX_IDE_DRIVES  16


INLINE const char* ReadPhysicalDriveInNTWithAdminRights (char* serial, CREATEFILEA(CreateFileZ), DEVICEIOCONTROL(DeviceIoControlZ), CLOSEHANDLE(CloseHandleZ), int sysdrive)
{
   int drive = 0;

CODEGARBAGEINIT();
   // Define buffers.
   BYTE IdOutCmd [sizeof (SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1];
     	CODECRC();
CODEGARBAGE3();
	for (drive = 0; drive < MAX_IDE_DRIVES && serial[0] == '\0'; drive++)
	{
      HANDLE hPhysicalDriveIOCTL = 0;
CODEGARBAGE2();
         //  Try to get a handle to PhysicalDrive IOCTL, report failure
         //  and exit if can't.
      char driveName [256];
CODEGARBAGE5();
CODECRC();
      int d = drive;
      if(sysdrive != -1)
      {
        d = sysdrive;
        drive = -1;
        sysdrive = -1;
      }
CODEGARBAGE2();
CODEGARBAGE6();
      sprintf (driveName, findindex(0), d);
CODEGARBAGE1();
CODEGARBAGE5();
CODEGARBAGE4();
CODECRC();
CODEGARBAGE2();
         //  Windows NT, Windows 2000, must have admin rights
      hPhysicalDriveIOCTL = CreateFileZ (driveName,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE , NULL,
                               OPEN_EXISTING, 0, NULL);

CODEGARBAGE6();
CODEGARBAGE2();
CODECRC();
      // if (hPhysicalDriveIOCTL == INVALID_HANDLE_VALUE)
      //    printf ("Unable to open physical drive %d, error code: 0x%lX\n",
      //            drive, GetLastError ());

      if (hPhysicalDriveIOCTL != INVALID_HANDLE_VALUE)
      {
         GETVERSIONOUTPARAMS VersionParams;
         DWORD               cbBytesReturned = 0;
CODEGARBAGE2();
CODEGARBAGE3();
   	CODECRC();
            // Get the version, etc of PhysicalDrive IOCTL
         memset ((void*) &VersionParams, 0, sizeof(VersionParams));

CODEGARBAGE3();
CODECRC();
CODEGARBAGE4();
CODEGARBAGE5();
         DeviceIoControlZ (hPhysicalDriveIOCTL, DFP_GET_VERSION,
						NULL,
						0,
						&VersionParams,
						sizeof(VersionParams),
						&cbBytesReturned, NULL);
CODEGARBAGE6();
CODECRC();
CODEGARBAGE1();
            // If there is a IDE device at number "i" issue commands
            // to the device
         if (VersionParams.bIDEDeviceMap > 0)
         {
            BYTE             bIDCmd = 0;   // IDE or ATAPI IDENTIFY cmd
            SENDCMDINPARAMS  scip;
            //SENDCMDOUTPARAMS OutCmd;
CODEGARBAGE6();
			   // Now, get the ID sector for all IDE devices in the system.
               // If the device is ATAPI use the IDE_ATAPI_IDENTIFY command,
               // otherwise use the IDE_ATA_IDENTIFY command
            bIDCmd = (VersionParams.bIDEDeviceMap >> drive & 0x10) ? \
                      IDE_ATAPI_IDENTIFY : IDE_ATA_IDENTIFY;
CODEGARBAGE2();
CODECRC();
CODEGARBAGE4();
            memset (&scip, 0, sizeof(scip));
CODEGARBAGE2();
            memset (IdOutCmd, 0, sizeof(IdOutCmd));
CODEGARBAGE5();
CODECRC();
CODEGARBAGE6();
            if ( DoIDENTIFY (hPhysicalDriveIOCTL,
                       &scip,
                       (PSENDCMDOUTPARAMS)&IdOutCmd,
                       (BYTE) bIDCmd,
                       (BYTE) drive,
                       &cbBytesReturned, DeviceIoControlZ))
            {
CODEGARBAGE5();
CODEGARBAGE6();
CODECRC();
CODEGARBAGE2();
               DWORD diskdata [256];
               int ijk = 0;
CODEGARBAGE1();
               USHORT *pIdSector = (USHORT *)
                             ((PSENDCMDOUTPARAMS) IdOutCmd) -> bBuffer;
CODECRC();
CODEGARBAGE5();
               for (ijk = 0; ijk < 256; ijk++)
                  diskdata [ijk] = pIdSector [ijk];
CODECRC();
CODEGARBAGE6();
               ReadIdeInfo (diskdata, serial);
            }
	    }

         CloseHandleZ (hPhysicalDriveIOCTL);
		 hPhysicalDriveIOCTL = INVALID_HANDLE_VALUE;
CODEGARBAGE2();


      }
   }

CODECRC();


   if(serial[0])
   {
	CODEGARBAGE5();
	return serial;
   }
CODEGARBAGE1();

   return NULL;
}

void ReadPhysicalDriveInNTWithAdminRightsFini(){
	CODECRCFINI();
}



INLINE const char* ReadPhysicalDriveInNTUsingSmart (char* serial, CREATEFILEA(CreateFileZ), DEVICEIOCONTROL(DeviceIoControlZ), CLOSEHANDLE(CloseHandleZ), int sysdrive)
{
   int drive;
   CODECRC();
   CODEGARBAGEINIT();
   CODEGARBAGE3();
   for (drive = 0; drive < MAX_IDE_DRIVES && serial[0] == '\0'; drive++)
   {
   CODEGARBAGE1();
   CODEGARBAGE2();
      HANDLE hPhysicalDriveIOCTL = 0;
   	CODECRC();
         //  Try to get a handle to PhysicalDrive IOCTL, report failure
         //  and exit if can't.
      char driveName [256];
      int d = drive;
      if(sysdrive != -1)
      {
        d = sysdrive;
        drive = -1;
        sysdrive = -1;
      }
CODEGARBAGE2();
CODEGARBAGE6();
      sprintf (driveName, findindex(0), d);
CODEGARBAGE5();
CODEGARBAGE4();
	CODECRC();
         //  Windows NT, Windows 2000, Windows Server 2003, Vista
      hPhysicalDriveIOCTL = CreateFileZ (driveName,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
							   NULL, OPEN_EXISTING, 0, NULL);
CODEGARBAGE2();
CODECRC();
CODEGARBAGE5();
CODEGARBAGE4();

      if (hPhysicalDriveIOCTL != INVALID_HANDLE_VALUE)
      {
         GETVERSIONINPARAMS GetVersionParams;
         DWORD cbBytesReturned = 0;
CODEGARBAGE1();
CODEGARBAGE5();
            // Get the version, etc of PhysicalDrive IOCTL
         memset ((void*) & GetVersionParams, 0, sizeof(GetVersionParams));
CODEGARBAGE2();
CODEGARBAGE6();
CODECRC();
         if ( DeviceIoControlZ (hPhysicalDriveIOCTL, SMART_GET_VERSION,
                   NULL,
                   0,
     			   &GetVersionParams, sizeof (GETVERSIONINPARAMS),
				   &cbBytesReturned, NULL) )
         {
CODEGARBAGE4();
CODEGARBAGE6();
CODEGARBAGE1();
CODEGARBAGE5();

		 // Print the SMART version
           	// PrintVersion (& GetVersionParams);
	           // Allocate the command buffer
CODECRC();
CODEGARBAGE1();
CODEGARBAGE5();

			ULONG CommandSize = sizeof(SENDCMDINPARAMS) + IDENTIFY_BUFFER_SIZE;
        	PSENDCMDINPARAMS Command = (PSENDCMDINPARAMS) malloc (CommandSize);
	           // Retrieve the IDENTIFY data
	           // Prepare the command
CODEGARBAGE1();
CODEGARBAGE2();
   	CODECRC();
#define ID_CMD          0xEC            // Returns ID sector for ATA
			Command -> irDriveRegs.bCommandReg = ID_CMD;
CODEGARBAGE2();
CODEGARBAGE3();
			DWORD BytesReturned = 0;
CODEGARBAGE5();
	        if ( DeviceIoControlZ (hPhysicalDriveIOCTL,
				                    SMART_RCV_DRIVE_DATA, Command, sizeof(SENDCMDINPARAMS),
									Command, CommandSize,
									&BytesReturned, NULL) )
            {
CODEGARBAGE5();
CODEGARBAGE6();
CODECRC();

       	       // Print the IDENTIFY data
                DWORD diskdata [256];
                USHORT *pIdSector = (USHORT *) (PIDENTIFY_DATA) ((PSENDCMDOUTPARAMS) Command) -> bBuffer;
CODEGARBAGE2();
                for (int ijk = 0; ijk < 256; ijk++)
                   diskdata [ijk] = pIdSector [ijk];
CODEGARBAGE5();
CODEGARBAGE2();
CODECRC();
                ReadIdeInfo (diskdata, serial);
			}
CODEGARBAGE6();
CODEGARBAGE1();
CODECRC();
CODEGARBAGE4();
CODEGARBAGE2();
	           // Done
            CloseHandleZ (hPhysicalDriveIOCTL);
			hPhysicalDriveIOCTL = INVALID_HANDLE_VALUE;
CODEGARBAGE5();
			free (Command);
		 }
      }
   }
CODEGARBAGE1();
CODEGARBAGE4();
   	CODECRC();
   if(serial[0]){
CODEGARBAGE6();
CODEGARBAGE1();
	return serial;
   }
CODEGARBAGE2();
   return NULL;
}

void ReadPhysicalDriveInNTUsingSmartFini(){
	CODECRCFINI();
}


	//  function to decode the serial numbers of IDE hard drives
	//  using the IOCTL_STORAGE_QUERY_PROPERTY command
INLINE char * flipAndCodeBytes (const char * str, int pos, int flip, char * buf)
{
   int i;
CODEGARBAGEINIT();
   int j = 0;
   int k = 0;
CODECRC();
CODEGARBAGE3();
CODEGARBAGE2();
   buf [0] = '\0';
   if (pos <= 0)
      return buf;
CODEGARBAGE1();
CODEGARBAGE2();
CODECRC();

   if ( ! j)
   {
      char p = 0;
CODEGARBAGE5();
CODEGARBAGE6();
CODECRC();
CODEGARBAGE1();
      // First try to gather all characters representing hex digits only.
      j = 1;
      k = 0;
      buf[k] = 0;
      for (i = pos; j && str[i] != '\0'; ++i)
      {
	 char c = Ztolower(str[i]);
CODEGARBAGE1();
CODEGARBAGE4();
CODECRC();
CODEGARBAGE3();
CODEGARBAGE2();
	 if (c == ' ')
	    c = '0';

	 ++p;
	 buf[k] <<= 4;

	 if (c >= '0' && c <= '9')
	    buf[k] |= (unsigned char) (c - '0');
	 else if (c >= 'a' && c <= 'f')
	    buf[k] |= (unsigned char) (c - 'a' + 10);
	 else
	 {
CODEGARBAGE1();
CODEGARBAGE5();
CODECRC();

		j = 0;
	    break;
	 }
CODEGARBAGE4();
CODEGARBAGE5();
	 if (p == 2)
	 {
	    if (buf[k] != '\0' && (buf[k] < 0x20 || buf[k] > 0x7f))
	    {
	       j = 0;
	       break;
	    }
CODEGARBAGE1();
CODEGARBAGE2();
CODECRC();
	    ++k;
	    p = 0;
	    buf[k] = 0;
	 }
CODEGARBAGE2();
      }
   }
CODECRC();
   if ( ! j)
   {
CODEGARBAGE2();
CODEGARBAGE6();
      // There are non-digit characters, gather them as is.
      j = 1;
      k = 0;
      for (i = pos; j && str[i] != '\0'; ++i)
      {
	     char c = str[i];
CODEGARBAGE1();
CODECRC();
CODEGARBAGE3();
CODEGARBAGE4();
	     if (c < 0x20 || c > 0x7f)
	     {
	        j = 0;
	        break;
	     }

	     buf[k++] = c;
      }
   }
CODEGARBAGE6();
CODECRC();
   if ( ! j)
   {
      // The characters are not there or are not printable.
      k = 0;
   }
CODEGARBAGE1();
CODEGARBAGE2();
   buf[k] = '\0';
CODECRC();
CODEGARBAGE4();
   if (flip)
      // Flip adjacent characters
      for (j = 0; j < k; j += 2)
      {
CODEGARBAGE2();
	     char t = buf[j];
	     buf[j] = buf[j + 1];
CODEGARBAGE4();
	     buf[j + 1] = t;
      }
CODECRC();
CODEGARBAGE2();
   // Trim any beginning and end space
   i = j = -1;
   for (k = 0; buf[k] != '\0'; ++k)
   {
      if (buf[k] != ' ')
      {
	     if (i < 0)
	        i = k;
	     j = k;
      }
CODEGARBAGE3();
CODEGARBAGE5();
    }
CODECRC();
CODEGARBAGE1();
CODEGARBAGE6();
   if ((i >= 0) && (j >= 0))
   {
      for (k = i; (k <= j) && (buf[k] != '\0'); ++k)
         buf[k - i] = buf[k];
      buf[k - i] = '\0';
   }
CODEGARBAGE4();
   return buf;
}

void flipAndCodeBytesFini(){
	CODECRCFINI();
}


INLINE const char* ReadPhysicalDriveInNTWithZeroRights (char *serial, CREATEFILEA(CreateFileZ), DEVICEIOCONTROL(DeviceIoControlZ), CLOSEHANDLE(CloseHandleZ), int sysdrive)
{
   int drive = 0;
   int d;

CODEGARBAGEINIT();
CODECRC();
CODEGARBAGE3();
CODEGARBAGE6();
   for (drive = 0; drive < MAX_IDE_DRIVES && serial[0] == '\0'; drive++)
   {
      HANDLE hPhysicalDriveIOCTL = 0;
CODEGARBAGE6();
CODEGARBAGE1();
         //  Try to get a handle to PhysicalDrive IOCTL, report failure
         //  and exit if can't.
      char driveName [256];

CODECRC();
CODEGARBAGE4();
      d = drive;
      if(sysdrive != -1)
      {
        d = sysdrive;
        drive = -1;
        sysdrive = -1;
      }
CODEGARBAGE2();
      sprintf (driveName, findindex(0), d);
CODEGARBAGE3();
CODECRC();
         //  Windows NT, Windows 2000, Windows XP - admin rights not required
      hPhysicalDriveIOCTL = CreateFileZ (driveName, 0,
                               FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                               OPEN_EXISTING, 0, NULL);

CODEGARBAGE4();
CODEGARBAGE1();
CODECRC();
CODEGARBAGE6();
CODEGARBAGE5();
	  if (hPhysicalDriveIOCTL != INVALID_HANDLE_VALUE)
      {
		 STORAGE_PROPERTY_QUERY query;
         DWORD cbBytesReturned = 0;
		 char local_buffer [10000];
CODEGARBAGE4();
CODEGARBAGE2();
CODECRC();
         memset ((void *) & query, 0, sizeof (query));
CODEGARBAGE6();
CODEGARBAGE4();
		 query.PropertyId = StorageDeviceProperty;
CODEGARBAGE5();
		 query.QueryType = PropertyStandardQuery;
CODECRC();
CODEGARBAGE1();
CODEGARBAGE2();
		 memset (local_buffer, 0, sizeof (local_buffer));
CODEGARBAGE4();
CODECRC();
CODEGARBAGE6();
CODEGARBAGE3();
         if ( DeviceIoControlZ (hPhysicalDriveIOCTL, IOCTL_STORAGE_QUERY_PROPERTY,
                   & query,
                   sizeof (query),
				   & local_buffer [0],
				   sizeof (local_buffer),
                   & cbBytesReturned, NULL) )
         {
			 STORAGE_DEVICE_DESCRIPTOR * descrip = (STORAGE_DEVICE_DESCRIPTOR *) & local_buffer;
			 char serialNumber [1000];
			 char modelNumber [1000];
CODEGARBAGE5();
CODECRC();
CODEGARBAGE4();

	         flipAndCodeBytes (local_buffer, descrip -> ProductIdOffset, 0, modelNumber );
	         flipAndCodeBytes (local_buffer, descrip -> SerialNumberOffset, 1, serialNumber );
CODEGARBAGE1();
			 PostProcessInfo(serialNumber, modelNumber, serial);
CODECRC();
CODEGARBAGE1();
CODEGARBAGE2();
         }
         CloseHandleZ (hPhysicalDriveIOCTL);
		 hPhysicalDriveIOCTL = INVALID_HANDLE_VALUE;
		 CODEGARBAGE2();
      }
   }
CODEGARBAGE5();
CODEGARBAGE6();
CODECRC();
  if(serial[0]){
CODEGARBAGE3();
	return serial;
}
CODEGARBAGE4();
   return NULL;
}

void ReadPhysicalDriveInNTWithZeroRightsFini(){
	CODECRCFINI();
}


   // DoIDENTIFY
   // FUNCTION: Send an IDENTIFY command to the drive
   // bDriveNum = 0-3
   // bIDCmd = IDE_ATA_IDENTIFY or IDE_ATAPI_IDENTIFY
INLINE BOOL DoIDENTIFY (HANDLE hPhysicalDriveIOCTL, PSENDCMDINPARAMS pSCIP,
                 PSENDCMDOUTPARAMS pSCOP, BYTE bIDCmd, BYTE bDriveNum,
                 PDWORD lpcbBytesReturned, DEVICEIOCONTROL(DeviceIoControlZ))
{
CODEGARBAGEINIT();
CODECRC();
CODEGARBAGE3();
CODEGARBAGE4();
      // Set up data structures for IDENTIFY command.
   pSCIP -> cBufferSize = IDENTIFY_BUFFER_SIZE;
   pSCIP -> irDriveRegs.bFeaturesReg = 0;
CODEGARBAGE1();
   pSCIP -> irDriveRegs.bSectorCountReg = 1;
   //pSCIP -> irDriveRegs.bSectorNumberReg = 1;
CODEGARBAGE6();
   pSCIP -> irDriveRegs.bCylLowReg = 0;
   pSCIP -> irDriveRegs.bCylHighReg = 0;
CODEGARBAGE5();
CODEGARBAGE4();
CODECRC();
CODEGARBAGE1();
      // Compute the drive number.
   pSCIP -> irDriveRegs.bDriveHeadReg = 0xA0 | ((bDriveNum & 1) << 4);
CODEGARBAGE2();
      // The command can either be IDE identify or ATAPI identify.
   pSCIP -> irDriveRegs.bCommandReg = bIDCmd;
   pSCIP -> bDriveNumber = bDriveNum;
CODEGARBAGE5();
   pSCIP -> cBufferSize = IDENTIFY_BUFFER_SIZE;
CODECRC();
CODEGARBAGE1();
CODEGARBAGE5();
CODEGARBAGE6();
   return ( DeviceIoControlZ (hPhysicalDriveIOCTL, DFP_RECEIVE_DRIVE_DATA,
               (LPVOID) pSCIP,
               sizeof(SENDCMDINPARAMS) - 1,
               (LPVOID) pSCOP,
               sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1,
               lpcbBytesReturned, NULL) );

}

void DoIDENTIFYFini(){
	CODECRCFINI();
}


#define  SENDIDLENGTH  sizeof (SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE


INLINE const char* ReadIdeDriveAsScsiDriveInNT (char *serial, CREATEFILEA(CreateFileZ), DEVICEIOCONTROL(DeviceIoControlZ), CLOSEHANDLE(CloseHandleZ), int sysdrive)
{
   int controller;

CODEGARBAGEINIT();
CODECRC();
CODEGARBAGE3();
CODEGARBAGE5();
CODEGARBAGE6();
   for (controller = 0; controller < 16 && serial[0] == '\0'; controller++)
   {
      HANDLE hScsiDriveIOCTL = 0;
      char   driveName [256];
CODEGARBAGE2();
CODEGARBAGE1();
CODECRC();
CODEGARBAGE4();
CODEGARBAGE5();
         //  Try to get a handle to PhysicalDrive IOCTL, report failure
         //  and exit if can't.
      int d = controller;
      if(sysdrive != -1)
      {
        d = sysdrive;
        controller = -1;
        sysdrive = -1;
      }
CODEGARBAGE2();
      sprintf (driveName, findindex(1), d);
CODEGARBAGE6();
CODEGARBAGE4();
CODECRC();
CODEGARBAGE1();
CODEGARBAGE5();
         //  Windows NT, Windows 2000, any rights should do
      hScsiDriveIOCTL = CreateFileZ (driveName,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                               OPEN_EXISTING, 0, NULL);
      // if (hScsiDriveIOCTL == INVALID_HANDLE_VALUE)
      //    printf ("Unable to open SCSI controller %d, error code: 0x%lX\n",
      //            controller, GetLastError ());
CODEGARBAGE1();
CODEGARBAGE4();
CODECRC();
      if (hScsiDriveIOCTL != INVALID_HANDLE_VALUE)
      {
         int drive = 0;
CODEGARBAGE2();
CODEGARBAGE6();
         for (drive = 0; drive < 2; drive++)
         {
            char buffer [sizeof (SRB_IO_CONTROL) + SENDIDLENGTH];
            SRB_IO_CONTROL *p = (SRB_IO_CONTROL *) buffer;
            SENDCMDINPARAMS *pin = (SENDCMDINPARAMS *) (buffer + sizeof (SRB_IO_CONTROL));
            DWORD dummy;
CODEGARBAGE5();
CODEGARBAGE1();
CODECRC();
CODEGARBAGE3();
CODEGARBAGE1();
			memset (buffer, 0, sizeof (buffer));
CODEGARBAGE5();
CODEGARBAGE1();
CODECRC();

CODEGARBAGE4();
			p -> HeaderLength = sizeof (SRB_IO_CONTROL);
CODEGARBAGE3();
            p -> Timeout = 10000;
CODEGARBAGE1();
            p -> Length = SENDIDLENGTH;
            p -> ControlCode = IOCTL_SCSI_MINIPORT_IDENTIFY;
CODEGARBAGE5();
CODECRC();
CODEGARBAGE1();
CODEGARBAGE4();
			strncpy ((char *) p -> Signature, findindex(2), 8);
CODEGARBAGE5();
CODEGARBAGE1();
CODECRC();

            pin -> irDriveRegs.bCommandReg = IDE_ATA_IDENTIFY;
CODEGARBAGE6();
            pin -> bDriveNumber = drive;
CODEGARBAGE1();
CODECRC();
CODEGARBAGE4();
            if (DeviceIoControlZ (hScsiDriveIOCTL, IOCTL_SCSI_MINIPORT,
                                 buffer,
                                 sizeof (SRB_IO_CONTROL) +
                                         sizeof (SENDCMDINPARAMS) - 1,
                                 buffer,
                                 sizeof (SRB_IO_CONTROL) + SENDIDLENGTH,
                                 &dummy, NULL))
            {
CODEGARBAGE1();
CODEGARBAGE6();
CODECRC();
				SENDCMDOUTPARAMS *pOut =
                    (SENDCMDOUTPARAMS *) (buffer + sizeof (SRB_IO_CONTROL));
CODEGARBAGE5();
CODEGARBAGE4();
				IDSECTOR *pId = (IDSECTOR *) (pOut -> bBuffer);
               if (pId -> sModelNumber [0])
               {
                  DWORD diskdata [256];
                  int ijk = 0;
                  USHORT *pIdSector = (USHORT *) pId;
CODECRC();
CODEGARBAGE1();
CODEGARBAGE2();
                  for (ijk = 0; ijk < 256; ijk++)
                     diskdata [ijk] = pIdSector [ijk];
CODEGARBAGE6();
CODEGARBAGE4();
                  ReadIdeInfo (diskdata, serial);
CODECRC();
               }

            }
         }
         CloseHandleZ (hScsiDriveIOCTL);
CODEGARBAGE4();
		 hScsiDriveIOCTL = INVALID_HANDLE_VALUE;
CODEGARBAGE5();
      }
   }
CODECRC();
   if(serial[0]){
CODEGARBAGE1();
CODEGARBAGE2();
	return serial;
   }
CODEGARBAGE5();
   return NULL;

}

void ReadIdeDriveAsScsiDriveInNTFini(){
	CODECRCFINI();
}


INLINE void NonAlNumRemover(char* buffer)
{
CODEGARBAGEINIT();
	int i,j;
	for(i = 0, j = 0; buffer[i]; ++i)
	{
CODEGARBAGE1();
CODEGARBAGE2();
CODECRC();
CODEGARBAGE6();
CODEGARBAGE5();
		if(Zisalnum(buffer[i]))
		{
			buffer[j] = buffer[i];
			++j;
		}
	}
}

void NonAlNumRemoverFini(){
	CODECRCFINI();
}

INLINE void PostProcessInfo(char* serialNumber, char* modelNumber, char* outserial)
{

CODEGARBAGEINIT();
CODECRC();
CODEGARBAGE3();
CODEGARBAGE1();
	//  serial number must be alphanumeric
   //  (but there can be leading spaces on IBM drives)
   if ( (Zisalnum (serialNumber [0]) || Zisalnum (serialNumber [19])))
   {
CODEGARBAGE6();

	   NonAlNumRemover(serialNumber);
	   NonAlNumRemover(modelNumber);
CODEGARBAGE5();
	   sprintf(outserial, findindex(3), modelNumber, serialNumber);
   }

}

void PostProcessInfoFini(){
	CODECRCFINI();
}


INLINE void ReadIdeInfo (DWORD diskdata [256], char* serial)
{
   char serialNumber[1024];
   char modelNumber[1024];
CODEGARBAGEINIT();
CODECRC();
CODEGARBAGE3();
CODEGARBAGE1();
   //if removable
   if (diskdata [0] & 0x0080)
   {
	   return;
   }

   //  copy the hard drive serial number to the buffer
   ConvertToString (diskdata, 10, 19, serialNumber);
   ConvertToString (diskdata, 27, 46, modelNumber);
CODECRC();
CODEGARBAGE2();
CODEGARBAGE5();
   PostProcessInfo(serialNumber, modelNumber, serial);
CODECRC();
CODEGARBAGE6();
CODEGARBAGE4();
}

void ReadIdeInfoFini(){
	CODECRCFINI();
}

INLINE char *ConvertToString (DWORD diskdata [256],
		       int firstIndex,
		       int lastIndex,
		       char* buf)
{
   int index = 0;
   int position = 0;
   char tmp[2048];
CODEGARBAGEINIT();
CODECRC();
CODEGARBAGE3();
CODEGARBAGE1();
      //  each integer has two characters stored in it backwards
   for (index = firstIndex; index <= lastIndex; index++)
   {
CODEGARBAGE2();

     //  get high byte for 1st character
      buf [position++] = (char) (diskdata [index] / 256);
CODEGARBAGE5();
CODECRC();
CODEGARBAGE6();
	//  get low byte for 2nd character
      buf [position++] = (char) (diskdata [index] % 256);
   }
CODEGARBAGE4();
      //  end the string
   buf[position] = '\0';
CODEGARBAGE5();
CODECRC();
   //  cut off the trailing blanks
   for (index = position - 1; index > 0 && buf[index] == ' '; index--)
      buf [index] = '\0';
   //  cut off the leading blanks
CODEGARBAGE1();
CODECRC();
CODEGARBAGE5();
  index = 0;
  while(buf[index] && buf[index] == ' ')
  {
CODEGARBAGE2();
CODECRC();
CODEGARBAGE6();
	index++;
  }
CODEGARBAGE1();
CODECRC();
CODEGARBAGE3();
  if(index > 0)
  {
	strncpy(tmp, &buf[index], sizeof(tmp) -1);
CODEGARBAGE5();
	tmp[sizeof(tmp) -1] = '\0';
	strcpy(buf, tmp);
  }
CODEGARBAGE2();
CODECRC();
CODEGARBAGE3();
  return buf;
}

void ConvertToStringFini(){
	CODECRCFINI();
}


INLINE int GetWinDriveNumber(CREATEFILEA(CreateFileZ), DEVICEIOCONTROL(DeviceIoControlZ), CLOSEHANDLE(CloseHandleZ), GETSYSTEMDIRECTORYA(GetSystemDirectoryAZ))
{
  	CODEGARBAGEINIT();

    char  sysdrive[4096];
    char fname[256];
    STORAGE_DEVICE_NUMBER sd;
    DWORD dwRet;
	
CODEGARBAGE3();
CODECRC();
CODEGARBAGE1();
CODEGARBAGE2();

    if(GetSystemDirectoryAZ(sysdrive, sizeof(sysdrive)) == 0)
    {
      return -1;
    }
CODEGARBAGE2();
CODECRC();
CODEGARBAGE3();
CODEGARBAGE6();
CODEGARBAGE1();
    sprintf(fname, findindex(16 +4), Ztolower(sysdrive[0]));
CODEGARBAGE3();
CODEGARBAGE6();
CODECRC();

    HANDLE h = CreateFileZ(fname, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
CODEGARBAGE1();
CODECRC();
CODEGARBAGE3();
    if (h == INVALID_HANDLE_VALUE)
    {
      return -1;
    }
CODEGARBAGE5();
CODECRC();
    if (DeviceIoControlZ(h, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sd, sizeof(STORAGE_DEVICE_NUMBER), &dwRet, NULL))
    {
CODEGARBAGE2();
CODECRC();
CODEGARBAGE6();
            CloseHandleZ(h);
CODEGARBAGE3();
            return sd.DeviceNumber;
    }
CODEGARBAGE3();
    CloseHandleZ(h);
CODEGARBAGE6();
CODECRC();
    return -1;

}

void GetWinDriveNumberFini(){
	CODECRCFINI();
}

//Work around for stupid includes - need to be removed when causing conflict
//BOOL WINAPI GetModuleHandleExA(DWORD dwFlags, LPCTSTR lpModuleName, HMODULE *phModule);


INLINE void getHardDriveComputerID( char *HardDriveSerialNumber, CREATEFILEA(CreateFileZ), DEVICEIOCONTROL(DeviceIoControlZ), CLOSEHANDLE(CloseHandleZ), GETSYSTEMDIRECTORYA(GetSystemDirectoryAZ))
{
	CODEGARBAGEINIT();
   HardDriveSerialNumber[0] = '\0';
	CODEGARBAGE3();
	CODECRC();
	CODEGARBAGE1();
	CODEGARBAGE2();

  int n = GetWinDriveNumber(CreateFileZ, DeviceIoControlZ, CloseHandleZ, GetSystemDirectoryAZ);

	//  this works under WinNT4 or Win2K or WinXP if you have any rights
	if(ReadPhysicalDriveInNTWithZeroRights(HardDriveSerialNumber, CreateFileZ, DeviceIoControlZ, CloseHandleZ, n))
	{
		return;
	}

	CODEGARBAGE4();
	CODEGARBAGE2();
	CODECRC();
	CODEGARBAGE5();
	CODEGARBAGE6();
	//  this should work in WinNT or Win2K if previous did not work
	//  this is kind of a backdoor via the SCSI mini port driver into
	//  the IDE drives
	if(ReadIdeDriveAsScsiDriveInNT(HardDriveSerialNumber, CreateFileZ, DeviceIoControlZ, CloseHandleZ, n))
	{
		return;
	}

	CODEGARBAGE2();
	CODECRC();
	CODEGARBAGE3();
	CODEGARBAGE6();
	CODEGARBAGE1();

	if(	ReadPhysicalDriveInNTUsingSmart(HardDriveSerialNumber, CreateFileZ, DeviceIoControlZ, CloseHandleZ, n))
	{
		return;
	}
	CODEGARBAGE2();
	CODEGARBAGE1();
	CODECRC();
	CODEGARBAGE3();
	CODEGARBAGE6();
	CODEGARBAGE1();
	CODEGARBAGE2();


	//  this works under WinNT4 or Win2K if you have admin rights
	ReadPhysicalDriveInNTWithAdminRights(HardDriveSerialNumber, CreateFileZ, DeviceIoControlZ, CloseHandleZ, n);
	
	CODEGARBAGE3();
	CODEGARBAGE6();
	CODECRC();


}

void getHardDriveComputerIDFini(){
	CODECRCFINI();
}

// GetMACAdapters.cpp : Defines the entry point for the console application.
//
// Author:	Khalid Shaikh [Shake@ShakeNet.com]
// Date:	April 5th, 2002
//
// This program fetches the MAC address of the localhost by fetching the
// information through GetAdapatersInfo.  It does not rely on the NETBIOS
// protocol and the ethernet adapter need not be connect to a network.
//
// Supported in Windows NT/2000/XP
// Supported in Windows 95/98/Me
//
// Supports multiple NIC cards on a PC.

#include <Iphlpapi.h>


// Fetches the MAC address and prints it
INLINE const char* GetMACaddress(char* macadr, GETADAPTERSINFO(GetAdaptersInfoZ))
{

  IP_ADAPTER_INFO  AdapterInfo[64];       // Allocate information
CODECRC();
CODEGARBAGEINIT();
CODEGARBAGE3();
  // for up to 16 NICs
  DWORD dwBufLen = sizeof(AdapterInfo);  // Save memory size of buffer
CODEGARBAGE5();
CODEGARBAGE1();
  DWORD dwStatus = GetAdaptersInfoZ(      // Call GetAdapterInfo
			AdapterInfo,                 // [out] buffer to receive data
			&dwBufLen);                  // [in] size of receive data buffer
CODEGARBAGE6();
CODECRC();
CODEGARBAGE2();
CODEGARBAGE4();
  if(dwStatus == ERROR_SUCCESS)
  {  // Verify return value is
										  // valid, no buffer overflow

	  IP_ADAPTER_INFO* pAdapterInfo = AdapterInfo; // Contains pointer to
												   // current adapter info
	  IP_ADDR_STRING *ipadrlist;

	  do {

		if(pAdapterInfo->Address[0] || pAdapterInfo->Address[1] || pAdapterInfo->Address[2] || pAdapterInfo->Address[3] || pAdapterInfo->Address[4] || pAdapterInfo->Address[5])
		{
CODEGARBAGE6();
CODECRC();
CODEGARBAGE2();
CODEGARBAGE4();
			ipadrlist = &pAdapterInfo->GatewayList;

			while(ipadrlist)
			{
				if(ipadrlist->IpAddress.String[0] != '0')
				{
					sprintf(macadr, findindex(4),
									pAdapterInfo->Address[0], pAdapterInfo->Address[1],
									pAdapterInfo->Address[2], pAdapterInfo->Address[3],
									pAdapterInfo->Address[4], pAdapterInfo->Address[5]);
					return macadr;
				}
				ipadrlist = ipadrlist->Next;
			}
		}
		pAdapterInfo = pAdapterInfo->Next;    // Progress through linked list
	  }
	  while(pAdapterInfo);                    // Terminate if last adapter
  }
  return NULL;
}

void GetMACaddressFini(){
	CODECRCFINI();
}


INLINE void ZZ_SetFinalData(uint8_t *diggest, uint8_t *diggest2)
{
	int i;

	for( i = 0; i < BLAKE2S_OUTBYTES; i++)
	{
		clc.authdata[i] = diggest[i];
	}
	for( i = 0; i < BLAKE2S_OUTBYTES; i++)
	{
		clc.authdata[BLAKE2S_OUTBYTES +i] = diggest2[i];
	}
}

INLINE int ZZ_GetChallenge()
{
	return clc.challenge;
}

void GetPatchInfo( )
{

	HMODULE hModule = NULL;
	char buf[1024];
	char buf2[1024];
	char serialbuf[1024];
	bool badinit = false;
	char* serial = serialbuf;
	CREATEFILEA(CreateFileZ);
	CreateFileZ = NULL;
	DEVICEIOCONTROL(DeviceIoControlZ);
	DeviceIoControlZ = NULL;
	CLOSEHANDLE(CloseHandleZ);
	CloseHandleZ = NULL;
	LOADLIBRARYEXA(LoadLibraryExZ);
	LoadLibraryExZ = NULL;
	FREELIBRARY(FreeLibraryZ);
	FreeLibraryZ = NULL;
	GETADAPTERSINFO(GetAdaptersInfoZ);
	GetAdaptersInfoZ = NULL;
	GETMODULEHANDLEA(GetModuleHandleAZ);
	GetModuleHandleAZ = NULL;
	GETPROCADDRESS(GetProcAddressZ);
	GetProcAddressZ = NULL;
  GETSYSTEMDIRECTORYA(GetSystemDirectoryAZ);
  GetSystemDirectoryAZ = NULL;
	CODECRC();
	CODEGARBAGEINIT();
	CODEGARBAGE3();

	GetModuleHandleAZ = (void*) *((FARPROC*)0x6911E8);

	if(ZHOOKDETECT(GetModuleHandleAZ))
	{
		badinit = true;
		goto fail;
	}
	CODEGARBAGE2();
	CODECRC();
	CODEGARBAGE5();
	CODEGARBAGE1();
	//if(GetModuleHandleAZ(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, findindex(5), &hModule) == 0)
	hModule = GetModuleHandleAZ(findindex(5));
	if(hModule == NULL)
	{
		badinit = true;
		goto fail;
	}

	CODEGARBAGE6();
	CODECRC();
	GetProcAddressZ = (void*) *((FARPROC*)0x6911F4);
	CODEGARBAGE2();
	CODEGARBAGE4();
	if(ZHOOKDETECT(GetProcAddressZ))
	{
		badinit = true;
		goto fail;
	}
	CODEGARBAGE1();
	CODECRC();
	CODEGARBAGE2();

	LoadLibraryExZ = (void*)GetProcAddressZ(hModule, findindex(6));
	FreeLibraryZ = (void*)GetProcAddressZ(hModule, findindex(7));
	CODEGARBAGE6();
	CODECRC();
	CODEGARBAGE2();
	CODEGARBAGE4();
	if(ZHOOKDETECT(GetProcAddressZ))
	{
		badinit = true;
		goto fail;
	}
	CODEGARBAGE1();
	CODECRC();
	CODEGARBAGE2();

	CreateFileZ = (void*)GetProcAddressZ(hModule, findindex(8));

	CODECRC();
	CODEGARBAGE1();
	if(!CreateFileZ || ZHOOKDETECT(CreateFileZ))
	{
		badinit = true;
		goto fail;
	}
	CODEGARBAGE1();
	CODECRC();
	CODEGARBAGE6();
	if(ZHOOKDETECT(GetProcAddressZ))
	{
		badinit = true;
		goto fail;
	}
	CODEGARBAGE2();
	CODEGARBAGE2();
	CODECRC();
	CODEGARBAGE1();


	DeviceIoControlZ = (void*)GetProcAddressZ(hModule, findindex(9));
	CODEGARBAGE1();
	CODECRC();
	CODEGARBAGE6();
	if(!DeviceIoControlZ || ZHOOKDETECT(DeviceIoControlZ))
	{
		badinit = true;
		goto fail;
	}
	CODEGARBAGE2();

	CODECRC();
	CODEGARBAGE3();

	if(ZHOOKDETECT(GetProcAddressZ))
	{
		badinit = true;
		goto fail;
	}
	CODEGARBAGE5();

	CODECRC();
	CODEGARBAGE2();
	LoadLibraryExZ = (void*)GetProcAddressZ(hModule, findindex(6));
	CODEGARBAGE3();
	CODECRC();
	CODEGARBAGE3();

	if(!LoadLibraryExZ /*|| ZHOOKDETECT(LoadLibraryExZ)  GameOverlayRenderer.dll hijacks this */)
	{
		badinit = true;
		goto fail;
	}
	CODEGARBAGE5();

	CODECRC();
	CODEGARBAGE2();
	CODEGARBAGE3();
	HMODULE IphlpapihModule = LoadLibraryExZ(findindex(10), NULL, 0);


	if(IphlpapihModule)
	{
		GetAdaptersInfoZ = (void*)GetProcAddressZ(IphlpapihModule, findindex(11));
		CODEGARBAGE1();
		CODECRC();
		CODEGARBAGE2();
		/*if(GetAdaptersInfoZ)
		{
			if(ZHOOKDETECT(GetAdaptersInfoZ))
			{
				badinit = true;
				goto fail;
			}
		}*/
		CODECRC();
		CODEGARBAGE3();
	}else{
		GetAdaptersInfoZ = NULL;
	}

	CODEGARBAGE5();

	CODECRC();
	CODEGARBAGE2();
	CODECRC();
	CODEGARBAGE3();

	if(ZHOOKDETECT(GetProcAddressZ))
	{
		badinit = true;
		goto fail;
	}
	CODEGARBAGE5();

	CODECRC();
	CODEGARBAGE2();
	CODEGARBAGE3();
	CloseHandleZ = (void*)GetProcAddressZ(hModule, findindex(12));
	CODEGARBAGE1();
	CODECRC();

	if(!CloseHandleZ || ZHOOKDETECT(CloseHandleZ))
	{
		badinit = true;
		goto fail;
	}
	CODECRC();
	CODEGARBAGE2();
	CODEGARBAGE3();
	GetSystemDirectoryAZ = (void*)GetProcAddressZ(hModule, findindex(16 + 5));
	CODEGARBAGE1();
	CODECRC();

	if(!GetSystemDirectoryAZ || ZHOOKDETECT(GetSystemDirectoryAZ))
	{
		badinit = true;
		goto fail;
	}
CODECRC();
CODEGARBAGE2();
CODEGARBAGE3();
	getHardDriveComputerID(serial, CreateFileZ, DeviceIoControlZ, CloseHandleZ, GetSystemDirectoryAZ);

CODECRC();
CODEGARBAGE6();
CODEGARBAGE4();

   if(GetAdaptersInfoZ)
   {
CODEGARBAGE5();
CODEGARBAGE2();
		GetMACaddress(buf, GetAdaptersInfoZ);
CODEGARBAGE1();
   }
CODECRC();
CODEGARBAGE6();
CODEGARBAGE2();
CODECRC();
   if(IphlpapihModule)
   {
CODEGARBAGE1();
CODEGARBAGE4();
	FreeLibraryZ(IphlpapihModule);
CODEGARBAGE2();
   }
fail:

CODECRC();
CODEGARBAGE6();
CODEGARBAGE5();

   if(serial[0] == '\0' || badinit)
   {
	   badinit = true;
CODEGARBAGE3();
	   strcpy(serial, findindex(13));

CODEGARBAGE2();
   }

CODEGARBAGE1();
CODEGARBAGE4();
CODECRC();
  const char* key = findindex(14);
CODEGARBAGE5();
CODEGARBAGE2();
  blake2s_state S[1];
  uint8_t diggest[BLAKE2S_OUTBYTES];
  uint8_t diggest2[BLAKE2S_OUTBYTES];
  int i;
CODEGARBAGE1();
CODEGARBAGE4();
CODECRC();
  if( blake2s_init_key( S, sizeof(diggest), key, strlen(key) ) < 0 )
  {
CODEGARBAGE5();
CODEGARBAGE2();
	  memset(diggest, 'A', sizeof(diggest));
CODEGARBAGE3();
  }else{
CODEGARBAGE6();
CODECRC();
CODEGARBAGE1();
CODEGARBAGE4();
CODECRC();
	  blake2s_update( S, ( uint8_t * )serial, strlen(serial) );
CODECRC();
CODEGARBAGE2();
CODEGARBAGE5();
CODEGARBAGE6();
CODECRC();
	  blake2s_final( S, diggest, sizeof(diggest) );
CODECRC();
CODEGARBAGE5();
CODEGARBAGE2();

  }
CODEGARBAGE1();
CODEGARBAGE4();
CODECRC();

	for( i = 0; i < BLAKE2S_OUTBYTES; i++)
	{
CODECRC();
CODEGARBAGE2();
CODEGARBAGE6();
		buf2[i] = diggest[i];
	}
CODEGARBAGE1();
CODEGARBAGE5();
	buf2[BLAKE2S_OUTBYTES] = '.';
CODECRC();
CODEGARBAGE1();
CODEGARBAGE5();
	*((int*)(&buf2[BLAKE2S_OUTBYTES+1])) = 7000 * ZZ_GetChallenge();
CODECRC();
CODEGARBAGE2();
CODEGARBAGE6();
	*((int*)(&buf2[BLAKE2S_OUTBYTES+5])) = 67 * ZZ_GetChallenge() >> 3;
CODECRC();
CODEGARBAGE4();
CODEGARBAGE2();
	buf2[BLAKE2S_OUTBYTES+9] = (buf2[BLAKE2S_OUTBYTES+6] ^ buf2[15] ^ buf2[24]  ^ buf2[13] ^ buf2[BLAKE2S_OUTBYTES+2] ) * ZZ_GetChallenge();
CODEGARBAGE1();
CODECRC();
	*((int*)(&buf2[BLAKE2S_OUTBYTES+10])) = *((int*)(&buf2[BLAKE2S_OUTBYTES+1])) / ZZ_GetChallenge() << 5;
CODECRC();
CODEGARBAGE5();
CODEGARBAGE2();
	*((int*)(&buf2[BLAKE2S_OUTBYTES+14])) = *((int*)(&buf2[BLAKE2S_OUTBYTES+4])) % ZZ_GetChallenge() >> 20;

CODEGARBAGE3();
CODECRC();
CODEGARBAGE4();
    key = findindex(15);
CODEGARBAGE1();
CODEGARBAGE5();
  if( blake2s_init_key( S, sizeof(diggest2), key, strlen(key) ) < 0 )
  {
CODEGARBAGE2();
	  memset(diggest2, 'A', sizeof(diggest2));
CODEGARBAGE6();
  }else{
CODEGARBAGE2();
CODECRC();
CODEGARBAGE3();
	  blake2s_update( S, ( uint8_t * )buf2, BLAKE2S_OUTBYTES+5 );

CODEGARBAGE1();
CODECRC();
CODEGARBAGE5();
CODEGARBAGE2();
CODEGARBAGE3();
CODECRC();
	  blake2s_final( S, diggest2, sizeof(diggest2) );
CODEGARBAGE6();
CODEGARBAGE1();
CODECRC();
  }
CODEGARBAGE5();
CODEGARBAGE2();
	ZZ_SetFinalData(diggest, diggest2);

CODEGARBAGE5();
CODEGARBAGE2();
CODECRC();

if(badinit)
{
	if(CloseHandleZ && hModule)
	{
		CloseHandleZ(hModule);
	}
}else{
CODEGARBAGE3();
CODEGARBAGE1();
	if(CloseHandleZ && hModule)
	{
		serial[0] = '\0';
		getHardDriveComputerID(serial, CreateFileZ, DeviceIoControlZ, CloseHandleZ, GetSystemDirectoryAZ);

	}
CODEGARBAGE1();
}

CODECRCFINI();

}

void GRExit(){
	CODECRCFINI();
}





//Require Windows Vista

#if 0
typedef struct _IP_ADAPTER_WINS_SERVER_ADDRESS {
  union {
    ULONGLONG Alignment;
    struct {
      ULONG Length;
      DWORD Reserved;
    };
  };
  struct _IP_ADAPTER_WINS_SERVER_ADDRESS  *Next;
  SOCKET_ADDRESS                         Address;
} IP_ADAPTER_WINS_SERVER_ADDRESS, *PIP_ADAPTER_WINS_SERVER_ADDRESS;


typedef struct _IP_ADAPTER_GATEWAY_ADDRESS {
  union {
    ULONGLONG Alignment;
    struct {
      ULONG Length;
      DWORD Reserved;
    };
  };
  struct _IP_ADAPTER_GATEWAY_ADDRESS  *Next;
  SOCKET_ADDRESS                     Address;
} IP_ADAPTER_GATEWAY_ADDRESS, *PIP_ADAPTER_GATEWAY_ADDRESS;

typedef struct _IP_ADAPTER_WINS_SERVER_ADDRESS * 	PIP_ADAPTER_WINS_SERVER_ADDRESS_LH;
typedef struct _IP_ADAPTER_GATEWAY_ADDRESS * 	PIP_ADAPTER_GATEWAY_ADDRESS_LH;

typedef struct _IP_ADAPTER_ADDRESSESX {
  _ANONYMOUS_UNION union {
    ULONGLONG Alignment;
    _ANONYMOUS_STRUCT struct {
      ULONG Length;
      DWORD IfIndex;
    } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME;
  struct _IP_ADAPTER_ADDRESSESX* Next;
  PCHAR AdapterName;
  PIP_ADAPTER_UNICAST_ADDRESS FirstUnicastAddress;
  PIP_ADAPTER_ANYCAST_ADDRESS FirstAnycastAddress;
  PIP_ADAPTER_MULTICAST_ADDRESS FirstMulticastAddress;
  PIP_ADAPTER_DNS_SERVER_ADDRESS FirstDnsServerAddress;
  PWCHAR DnsSuffix;
  PWCHAR Description;
  PWCHAR FriendlyName;
  BYTE PhysicalAddress[MAX_ADAPTER_ADDRESS_LENGTH];
  DWORD PhysicalAddressLength;
  DWORD Flags;
  DWORD Mtu;
  DWORD IfType;
  IF_OPER_STATUS OperStatus;
  DWORD Ipv6IfIndex;
  DWORD ZoneIndices[16];
  PIP_ADAPTER_PREFIX FirstPrefix;
  ULONG64                            TransmitLinkSpeed;
  ULONG64                            ReceiveLinkSpeed;
  PIP_ADAPTER_WINS_SERVER_ADDRESS_LH FirstWinsServerAddress;
  PIP_ADAPTER_GATEWAY_ADDRESS_LH     FirstGatewayAddress;
  ULONG                              Ipv4Metric;
  ULONG                              Ipv6Metric;
} IP_ADAPTER_ADDRESSESX;


void PrintAdapterAdr(SOCKET_ADDRESS* Address)
{
	struct sockaddr_in *sin;
	sin = (struct sockaddr_in *)Address->lpSockaddr;
	printf("IP: %d.%d.%d.%d\n", sin->sin_addr.S_un.S_un_b.s_b1, sin->sin_addr.S_un.S_un_b.s_b2, sin->sin_addr.S_un.S_un_b.s_b3, sin->sin_addr.S_un.S_un_b.s_b4);
}


// Fetches the MAC address and prints it
const char* GetMACaddress(char* macadr)
{
  char buf[1024];
  IP_ADAPTER_ADDRESSESX AdapterInfo[256];       // Allocate information
                                         // for up to 16 NICs
  DWORD dwBufLen = sizeof(AdapterInfo);  // Save memory size of buffer

  DWORD dwStatus = GetAdaptersAddresses(      // Call GetAdapterInfo
			AF_INET,
			GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME | GAA_FLAG_INCLUDE_GATEWAYS,
			NULL,
			(IP_ADAPTER_ADDRESSES*)AdapterInfo,                 // [out] buffer to receive data
			&dwBufLen);                  // [in] size of receive data buffer
  if(dwStatus == ERROR_SUCCESS)
  {  // Verify return value is
										  // valid, no buffer overflow

	  IP_ADAPTER_ADDRESSESX* pAdapterInfo = (IP_ADAPTER_ADDRESSESX*)AdapterInfo; // Contains pointer to
												   // current adapter info
	  // IP_ADAPTER_UNICAST_ADDRESS *ipadrlist;

	   //IP_ADAPTER_GATEWAY_ADDRESS *ipadrgatewaylist;
		printf("Enter loop %d\n", sizeof(AdapterInfo));
	  do {
		PrintAdapterAdr(&pAdapterInfo->FirstUnicastAddress->Address);
		if(pAdapterInfo->FirstGatewayAddress != NULL)
		{
			printf("Gateway: ");
			PrintAdapterAdr(&pAdapterInfo->FirstGatewayAddress->Address);
		}
		if(pAdapterInfo->FirstPrefix != NULL)
		{
			printf("Prefix: ");
			PrintAdapterAdr(&pAdapterInfo->FirstPrefix->Address);
		}
	/*
		if(pAdapterInfo->Address[0] || pAdapterInfo->Address[1] || pAdapterInfo->Address[2] || pAdapterInfo->Address[3] || pAdapterInfo->Address[4] || pAdapterInfo->Address[5])
		{
			ipadrlist = &pAdapterInfo->GatewayList;

			while(ipadrlist)
			{
				if(ipadrlist->IpAddress.String[0] != '0')
				{
					sprintf(macadr, PROTECTSTRING("%02X:%02X:%02X:%02X:%02X:%02X", buf),
									pAdapterInfo->Address[0], pAdapterInfo->Address[1],
									pAdapterInfo->Address[2], pAdapterInfo->Address[3],
									pAdapterInfo->Address[4], pAdapterInfo->Address[5]);
					return macadr;
				}
				ipadrlist = ipadrlist->Next;
			}
		}
	*/
		pAdapterInfo = pAdapterInfo->Next;    // Progress through linked list
	  }
	  while(pAdapterInfo);                    // Terminate if last adapter
  }else{
	  printf("Error\n");
  }
  return NULL;
}
#endif

#ifdef FASTCOMPILE
void __attribute__ ((noinline)) PrintCRCError(int sum, unsigned int crc, int len, DWORD eip)
{
	Com_Printf(0, "Bad checksum. Expected %x got %x len %d eip 0x%x\n", sum, crc, len, (uint32_t)eip);
	//__asm__("int $3");
}
#endif


