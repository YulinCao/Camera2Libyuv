#include <stdio.h>
#include "swordfish.h"
#ifdef _WINDOWS
#include <Winsock2.h>
#include <Windows.h>
#include <time.h>
#pragma comment(lib, "ws2_32.lib")
#include <process.h>
//#include "lusb0_usb.h"
/*enum libusb_endpoint_direction
{
	LIBUSB_ENDPOINT_IN = 0x80,

	LIBUSB_ENDPOINT_OUT = 0x00
};*/
#else
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#ifdef USEDLOPENLIBUSB
#include <dlfcn.h>
#endif
#include <sys/time.h>
#endif

#include <libusb.h>
#include "utils.h"
#include "ring_queue.h"
//#include <vector>
#include <stdlib.h>
#ifdef USEIMU
#ifndef USEYAMLCPP
#include <yaml.h>
#endif
#endif

#include "log.h"
#include <stdarg.h>
#pragma pack(8)

#define SWORDFISH_VID 0x4E56
#define SWORDFISH_PID 0x5055
#define SWORDFISH_ENDPOINT_IN (LIBUSB_ENDPOINT_IN + 1)
#define SWORDFISH_ENDPOINT_OUT (LIBUSB_ENDPOINT_OUT + 1)

#define USB_START_REQUEST 0x01
#define USB_START_RESPONSE 0x02

#define USB_STOP_REQUEST 0x03
#define USB_STOP_RESPONSE 0x04

#define USB_RESET_REQUEST 0x05
#define USB_RESET_RESPONSE 0x06
/*
#include <android/log.h>
#define LOG_TAG "MediaRecorder"
#define MYPRINT(...) __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG, __VA_ARGS__)

#ifdef DEBUGLOG
//#define PRINTF printf
#include <android/log.h>
#define LOG_TAG "MediaRecorder"
#define PRINTF(...) __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG, __VA_ARGS__)
#else
#define PRINTF(A, ...)
#endif
*/

#ifdef _WINDOWS

#include <time.h>
const __int64 DELTA_EPOCH_IN_MICROSECS = 11644473600000000;

/* IN UNIX the use of the timezone struct is obsolete;
I don't know why you use it. See http://linux.about.com/od/commands/l/blcmdl2_gettime.htm
But if you want to use this structure to know about GMT(UTC) diffrence from your local time
it will be next: tz_minuteswest is the real diffrence in minutes from GMT(UTC) and a tz_dsttime is a flag
indicates whether daylight is now in use
*/
struct timezone2
{
	__int32 tz_minuteswest; /* minutes W of Greenwich */
	BOOL tz_dsttime;		/* type of dst correction */
};

struct timeval2
{
	__int32 tv_sec;	 /* seconds */
	__int32 tv_usec; /* microseconds */
};

int gettimeofday(struct timeval2 *tv /*in*/, struct timezone2 *tz /*in*/)
{
	FILETIME ft;
	__int64 tmpres = 0;
	TIME_ZONE_INFORMATION tz_winapi;
	int rez = 0;

	ZeroMemory(&ft, sizeof(ft));
	ZeroMemory(&tz_winapi, sizeof(tz_winapi));

	GetSystemTimeAsFileTime(&ft);

	tmpres = ft.dwHighDateTime;
	tmpres <<= 32;
	tmpres |= ft.dwLowDateTime;

	/*converting file time to unix epoch*/
	tmpres /= 10; /*convert into microseconds*/
	tmpres -= DELTA_EPOCH_IN_MICROSECS;
	tv->tv_sec = (__int32)(tmpres * 0.000001);
	tv->tv_usec = (tmpres % 1000000);

	if (tz != NULL)
	{
		//_tzset(),don't work properly, so we use GetTimeZoneInformation
		rez = GetTimeZoneInformation(&tz_winapi);
		tz->tz_dsttime = (rez == 2) ? TRUE : FALSE;
		tz->tz_minuteswest = tz_winapi.Bias + ((rez == 2) ? tz_winapi.DaylightBias : 0);
	}
	return 0;
}
#endif

#ifdef _WINDOWS
// typedef usb_dev_handle *USBHANDLE;
typedef HANDLE THREADHANDLE;
typedef HANDLE SEM_T;
typedef struct timeval2 MYTIMEVAL;
#else
typedef int SOCKET;
typedef pthread_t THREADHANDLE;
typedef sem_t SEM_T;
typedef struct timeval MYTIMEVAL;
#endif
typedef libusb_device_handle *USBHANDLE;

typedef struct _pkt_info_custom_t
{
	unsigned char buffer[USB_PACKET_MAX_SIZE];
	int len;
} pkt_info_custom_t;

// static Ring_Queue *otherpktqueue = NULL;
// static Ring_Queue *depthpktqueue = NULL;
// static Ring_Queue *irpktqueue = NULL;
// static Ring_Queue *rgbpktqueue = NULL;
// static Ring_Queue *imupktqueue = NULL;
// static Ring_Queue *savepktqueue = NULL;
// static Ring_Queue *sendqueue = NULL;
////////////////////
#include <math.h>
#include <libgen.h>
const unsigned char colortable[256][3] = {{16, 128, 128},
										  {26, 170, 122},
										  {26, 173, 121},
										  {27, 176, 121},
										  {28, 178, 120},
										  {28, 181, 120},
										  {29, 184, 119},
										  {29, 186, 119},
										  {30, 189, 119},
										  {30, 192, 118},
										  {31, 194, 118},
										  {32, 196, 118},
										  {32, 197, 117},
										  {32, 199, 117},
										  {33, 201, 117},
										  {33, 203, 117},
										  {33, 204, 116},
										  {34, 206, 116},
										  {34, 208, 116},
										  {35, 209, 116},
										  {35, 211, 115},
										  {35, 213, 115},
										  {36, 215, 114},
										  {37, 217, 115},
										  {36, 218, 114},
										  {37, 220, 114},
										  {37, 222, 114},
										  {38, 223, 113},
										  {38, 225, 113},
										  {39, 226, 113},
										  {39, 229, 113},
										  {39, 231, 112},
										  {40, 233, 112},
										  {40, 235, 111},
										  {40, 236, 111},
										  {41, 238, 111},
										  {41, 239, 110},
										  {41, 240, 110},
										  {43, 239, 109},
										  {45, 238, 107},
										  {47, 237, 106},
										  {49, 235, 104},
										  {51, 234, 103},
										  {53, 233, 101},
										  {55, 232, 100},
										  {57, 231, 98},
										  {59, 230, 97},
										  {61, 228, 96},
										  {63, 227, 94},
										  {65, 226, 93},
										  {67, 225, 92},
										  {69, 224, 90},
										  {71, 222, 89},
										  {73, 221, 87},
										  {75, 220, 86},
										  {77, 219, 84},
										  {79, 218, 84},
										  {81, 217, 81},
										  {83, 216, 80},
										  {86, 214, 78},
										  {88, 213, 77},
										  {90, 212, 75},
										  {92, 211, 74},
										  {94, 210, 72},
										  {96, 208, 71},
										  {98, 207, 69},
										  {100, 206, 68},
										  {102, 205, 66},
										  {104, 204, 65},
										  {106, 203, 63},
										  {108, 202, 63},
										  {110, 200, 60},
										  {112, 199, 59},
										  {114, 198, 57},
										  {116, 197, 56},
										  {118, 196, 54},
										  {120, 194, 53},
										  {122, 193, 51},
										  {124, 192, 50},
										  {126, 191, 48},
										  {128, 190, 47},
										  {130, 189, 45},
										  {131, 188, 44},
										  {134, 186, 43},
										  {136, 185, 41},
										  {138, 184, 40},
										  {140, 183, 39},
										  {142, 182, 37},
										  {144, 181, 36},
										  {146, 180, 34},
										  {148, 178, 33},
										  {150, 177, 31},
										  {152, 176, 30},
										  {154, 175, 28},
										  {156, 174, 27},
										  {158, 173, 25},
										  {160, 171, 24},
										  {162, 170, 22},
										  {164, 169, 21},
										  {166, 168, 19},
										  {168, 167, 18},
										  {170, 166, 16},
										  {171, 164, 18},
										  {171, 162, 20},
										  {172, 159, 22},
										  {173, 157, 25},
										  {173, 155, 26},
										  {174, 153, 28},
										  {174, 150, 31},
										  {175, 148, 32},
										  {176, 146, 34},
										  {177, 143, 37},
										  {177, 141, 39},
										  {178, 138, 41},
										  {179, 136, 43},
										  {179, 134, 45},
										  {180, 131, 47},
										  {180, 129, 49},
										  {181, 127, 51},
										  {182, 124, 54},
										  {182, 122, 55},
										  {183, 119, 57},
										  {183, 117, 59},
										  {184, 115, 61},
										  {184, 112, 63},
										  {185, 110, 65},
										  {186, 108, 67},
										  {187, 106, 69},
										  {187, 103, 71},
										  {188, 101, 73},
										  {189, 99, 75},
										  {189, 96, 77},
										  {190, 94, 79},
										  {190, 91, 82},
										  {191, 89, 84},
										  {191, 87, 85},
										  {191, 85, 86},
										  {192, 83, 88},
										  {192, 81, 90},
										  {193, 78, 91},
										  {193, 76, 93},
										  {193, 74, 94},
										  {194, 72, 96},
										  {194, 70, 98},
										  {194, 67, 99},
										  {195, 65, 101},
										  {196, 66, 102},
										  {197, 68, 103},
										  {199, 72, 103},
										  {201, 76, 104},
										  {203, 80, 105},
										  {204, 84, 105},
										  {207, 92, 105},
										  {209, 96, 106},
										  {211, 100, 106},
										  {213, 107, 106},
										  {214, 107, 108},
										  {215, 106, 109},
										  {215, 106, 110},
										  {216, 105, 112},
										  {217, 105, 113},
										  {218, 104, 114},
										  {219, 104, 116},
										  {220, 103, 118},
										  {221, 103, 120},
										  {222, 102, 121},
										  {223, 101, 123},
										  {224, 101, 125},
										  {225, 100, 127},
										  {226, 100, 128},
										  {227, 99, 130},
										  {228, 98, 132},
										  {228, 98, 133},
										  {227, 93, 134},
										  {226, 88, 134},
										  {225, 84, 135},
										  {224, 79, 136},
										  {223, 74, 137},
										  {222, 69, 138},
										  {221, 64, 138},
										  {220, 60, 139},
										  {219, 55, 140},
										  {218, 50, 141},
										  {210, 17, 146},
										  {208, 18, 148},
										  {207, 19, 149},
										  {205, 19, 150},
										  {204, 21, 151},
										  {202, 21, 152},
										  {200, 22, 153},
										  {199, 22, 154},
										  {198, 24, 155},
										  {196, 25, 156},
										  {195, 26, 157},
										  {193, 26, 159},
										  {192, 27, 160},
										  {190, 28, 161},
										  {188, 29, 162},
										  {187, 29, 163},
										  {185, 31, 164},
										  {184, 32, 165},
										  {182, 32, 166},
										  {181, 33, 167},
										  {179, 34, 168},
										  {178, 35, 170},
										  {176, 36, 171},
										  {175, 37, 172},
										  {173, 38, 173},
										  {172, 39, 174},
										  {170, 39, 175},
										  {169, 40, 176},
										  {167, 41, 177},
										  {166, 42, 178},
										  {164, 43, 180},
										  {163, 44, 181},
										  {161, 45, 182},
										  {160, 46, 183},
										  {158, 46, 184},
										  {157, 47, 185},
										  {155, 48, 186},
										  {154, 49, 187},
										  {152, 50, 188},
										  {151, 51, 189},
										  {149, 52, 191},
										  {148, 53, 192},
										  {146, 53, 193},
										  {145, 54, 194},
										  {143, 55, 195},
										  {142, 56, 196},
										  {140, 57, 197},
										  {139, 58, 198},
										  {137, 59, 199},
										  {136, 60, 200},
										  {134, 60, 202},
										  {133, 61, 203},
										  {131, 62, 204},
										  {130, 63, 205},
										  {128, 64, 206},
										  {126, 65, 207},
										  {125, 66, 208},
										  {123, 67, 209},
										  {122, 67, 210},
										  {120, 68, 212},
										  {115, 71, 216},
										  {109, 75, 220},
										  {104, 78, 224},
										  {98, 81, 228},
										  {93, 84, 232},
										  {92, 85, 233},
										  {91, 85, 233},
										  {90, 86, 234},
										  {89, 87, 235},
										  {88, 87, 235},
										  {87, 88, 236},
										  {86, 88, 237},
										  {16, 128, 128}};

extern int update_result;

//#ifndef _WINDOWS
typedef ssize_t (*LIBUSB_GET_DEVICE_LIST)(libusb_context *ctx,
										  libusb_device ***list);

static LIBUSB_GET_DEVICE_LIST _LIBUSB_GET_DEVICE_LIST;

typedef int (*LIBUSB_GET_DEVICE_DESCRIPTOR)(libusb_device *dev,
											struct libusb_device_descriptor *desc);
static LIBUSB_GET_DEVICE_DESCRIPTOR _LIBUSB_GET_DEVICE_DESCRIPTOR;

typedef void (*LIBUSB_CLOSE)(libusb_device_handle *dev_handle);
static LIBUSB_CLOSE _LIBUSB_CLOSE;

typedef void (*LIBUSB_EXIT)(libusb_context *ctx);
static LIBUSB_EXIT _LIBUSB_EXIT;

typedef int (*LIBUSB_BULK_TRANSFER)(libusb_device_handle *dev_handle,
									unsigned char endpoint, unsigned char *data, int length,
									int *actual_length, unsigned int timeout);
static LIBUSB_BULK_TRANSFER _LIBUSB_BULK_TRANSFER;

typedef int (*LIBUSB_INIT)(libusb_context **ctx);
static LIBUSB_INIT _LIBUSB_INIT;

typedef int (*LIBUSB_CLAIM_INTERFACE)(libusb_device_handle *dev_handle,
									  int interface_number);
static LIBUSB_CLAIM_INTERFACE _LIBUSB_CLAIM_INTERFACE;

typedef uint8_t (*LIBUSB_GET_BUS_NUMBER)(libusb_device *dev);
static LIBUSB_GET_BUS_NUMBER _LIBUSB_GET_BUS_NUMBER;
typedef uint8_t (*LIBUSB_GET_PORT_NUMBER)(libusb_device *dev);
static LIBUSB_GET_PORT_NUMBER _LIBUSB_GET_PORT_NUMBER;

typedef int (*LIBUSB_GET_PORT_NUMBERS)(libusb_device *dev,
									   uint8_t *port_numbers, int port_numbers_len);
static LIBUSB_GET_PORT_NUMBERS _LIBUSB_GET_PORT_NUMBERS;

typedef uint8_t (*LIBUSB_GET_DEVICE_ADDRESS)(libusb_device *dev);
static LIBUSB_GET_DEVICE_ADDRESS _LIBUSB_GET_DEVICE_ADDRESS;

typedef void (*LIBUSB_FREE_DEVICE_LIST)(libusb_device **list,
										int unref_devices);
static LIBUSB_FREE_DEVICE_LIST _LIBUSB_FREE_DEVICE_LIST;

typedef int (*LIBUSB_OPEN)(libusb_device *dev,
						   libusb_device_handle **dev_handle);
static LIBUSB_OPEN _LIBUSB_OPEN;

typedef libusb_device_handle *(*LIBUSB_OPEN_DEVICE_WITH_VID_PID)(
	libusb_context *ctx, uint16_t vendor_id, uint16_t product_id);
static LIBUSB_OPEN_DEVICE_WITH_VID_PID _LIBUSB_OPEN_DEVICE_WITH_VID_PID;
#ifdef USEDLOPENLIBUSB
static void *libusbhandle = NULL;
#endif
//#endif


static log_t * g_loghandle=NULL;

void swordfish_debug_printf(const char* fmt,...){
	if(NULL==g_loghandle)return;
	char buffer[512];
	va_list args;
	va_start(args,fmt);
	vsnprintf(buffer,511,fmt,args);
    _log_debug(g_loghandle,buffer);//fmt,##__VA_ARGS__)
	va_end(args); 
} 
void swordfish_info_printf(const char* fmt,...){
	if(NULL==g_loghandle)return;
	char buffer[512];
	va_list args;
	va_start(args,fmt);
	vsnprintf(buffer,511,fmt,args);
    _log_info(g_loghandle,buffer);//fmt,##__VA_ARGS__)
	va_end(args); 
} 
void swordfish_warn_printf(const char* fmt,...){
	if(NULL==g_loghandle)return;
	char buffer[512];
	va_list args;
	va_start(args,fmt);
	vsnprintf(buffer,511,fmt,args);
    _log_warn(g_loghandle,buffer);//fmt,##__VA_ARGS__)
	va_end(args); 
} 
void swordfish_error_printf(const char* fmt,...){
	if(NULL==g_loghandle)return;
	char buffer[512];
	va_list args;
	va_start(args,fmt);
	vsnprintf(buffer,511,fmt,args);
    _log_error(g_loghandle,buffer);//fmt,##__VA_ARGS__)
	va_end(args); 
} 

int swordfish_init(const char* logfile,int maxlogfilesize)
{
        /////////////////initial log module
	if(g_loghandle==NULL){
		void* sh1 = stream_handle_create(ERROR_STDERR);
		sh1 = set_stream_param(sh1, LOG_ERROR, FRED, NULL, NULL);
		sh1 = set_stream_param(sh1, LOG_DEBUG, FGREEN, NULL,NULL);// BLINK);
		sh1 = set_stream_param(sh1, LOG_WARN, NULL, BGCYAN, UNDERLINE);
		g_loghandle = add_to_handle_list(NULL, sh1);//fh1);
		if(logfile!=NULL){	
			void *fh1 = create_log_file(logfile, maxlogfilesize,1, 256);
			//printf("got fh1:0x%X\n",(int)fh1);	
			g_loghandle = add_to_handle_list(g_loghandle,fh1);// sh1);
		}
		_log_start(g_loghandle);
	}
        //////////////////////////////////////////
	swordfish_debug_printf("init in feynman!\n");
#ifdef _WINDOWS
	//�����׽��ֿ�
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	swordfish_debug_printf("This is a Client side application!\n");
	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		// Tell the user that we could not find a usable WinSock Dll.
		swordfish_debug_printf("WSAStartup() called failed!\n");
		return -1;
	}
	else
	{
		swordfish_debug_printf("WSAStartup called successful!\n");
	}
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		// Tell the user that we could not find a usable WinSock Dll.
		WSACleanup();
		return -1;
	}
#endif
#ifdef USEDLOPENLIBUSB
	/*1. �򿪿��ļ�*/
	libusbhandle = dlopen("libusb1.0.so", RTLD_LAZY);
	if (libusbhandle == NULL)
	{
		swordfish_debug_printf("���ʧ��!\n");
		return -1;
	}
	/*2. ��ȡָ���ĺ�����ַ*/
	_LIBUSB_GET_DEVICE_LIST = (LIBUSB_GET_DEVICE_LIST)dlsym(libusbhandle, "libusb_get_device_list");
	if (_LIBUSB_GET_DEVICE_LIST == NULL)
	{
		swordfish_debug_printf("������ַ��ȡʧ��!\n");
		return -1;
	}

	_LIBUSB_GET_DEVICE_DESCRIPTOR = (LIBUSB_GET_DEVICE_DESCRIPTOR)dlsym(libusbhandle, "libusb_get_device_descriptor");
	if (_LIBUSB_GET_DEVICE_DESCRIPTOR == NULL)
	{
		swordfish_debug_printf("������ַ��ȡʧ��!\n");
		return -1;
	}

	_LIBUSB_CLOSE = (LIBUSB_CLOSE)dlsym(libusbhandle, "libusb_close");
	if (_LIBUSB_CLOSE == NULL)
	{
		swordfish_debug_printf("������ַ��ȡʧ��!\n");
		return -1;
	}

	_LIBUSB_EXIT = (LIBUSB_EXIT)dlsym(libusbhandle, "libusb_exit");
	if (_LIBUSB_EXIT == NULL)
	{
		swordfish_debug_printf("������ַ��ȡʧ��!\n");
		return -1;
	}

	_LIBUSB_BULK_TRANSFER = (LIBUSB_BULK_TRANSFER)dlsym(libusbhandle, "libusb_bulk_transfer");
	if (_LIBUSB_BULK_TRANSFER == NULL)
	{
		swordfish_debug_printf("������ַ��ȡʧ��!\n");
		return -1;
	}

	_LIBUSB_INIT = (LIBUSB_INIT)dlsym(libusbhandle, "libusb_init");
	if (_LIBUSB_INIT == NULL)
	{
		swordfish_debug_printf("������ַ��ȡʧ��!\n");
		return -1;
	}
	_LIBUSB_CLAIM_INTERFACE = (LIBUSB_CLAIM_INTERFACE)dlsym(libusbhandle, "libusb_claim_interface");
	if (_LIBUSB_CLAIM_INTERFACE == NULL)
	{
		swordfish_debug_printf("������ַ��ȡʧ��!\n");
		return -1;
	}

	_LIBUSB_GET_BUS_NUMBER = (LIBUSB_GET_BUS_NUMBER)dlsym(libusbhandle, "libusb_get_bus_number");
	if (_LIBUSB_GET_BUS_NUMBER == NULL)
	{
		swordfish_debug_printf("������ַ��ȡʧ��!\n");
		return -1;
	}

	_LIBUSB_GET_PORT_NUMBER = (LIBUSB_GET_PORT_NUMBER)dlsym(libusbhandle, "libusb_get_port_number");
	if (_LIBUSB_GET_PORT_NUMBER == NULL)
	{
		swordfish_debug_printf("������ַ��ȡʧ��!\n");
		return -1;
	}
	_LIBUSB_GET_PORT_NUMBERS = (LIBUSB_GET_PORT_NUMBERS)dlsym(libusbhandle, "libusb_get_port_numbers");
	if (_LIBUSB_GET_PORT_NUMBERS == NULL)
	{
		swordfish_debug_printf("������ַ��ȡʧ��!\n");
		return -1;
	}

	_LIBUSB_GET_DEVICE_ADDRESS = (LIBUSB_GET_DEVICE_ADDRESS)dlsym(libusbhandle, "libusb_get_device_address");
	if (_LIBUSB_GET_DEVICE_ADDRESS == NULL)
	{
		swordfish_debug_printf("������ַ��ȡʧ��!\n");
		return -1;
	}
	_LIBUSB_FREE_DEVICE_LIST = (LIBUSB_FREE_DEVICE_LIST)dlsym(libusbhandle, "libusb_free_device_list");
	if (_LIBUSB_FREE_DEVICE_LIST == NULL)
	{
		swordfish_debug_printf("������ַ��ȡʧ��!\n");
		return -1;
	}

	_LIBUSB_OPEN = (LIBUSB_OPEN)dlsym(libusbhandle, "libusb_open");
	if (_LIBUSB_OPEN == NULL)
	{
		swordfish_debug_printf("������ַ��ȡʧ��!\n");
		return -1;
	}

	_LIBUSB_OPEN_DEVICE_WITH_VID_PID = (LIBUSB_OPEN_DEVICE_WITH_VID_PID)dlsym(libusbhandle, "libusb_open_device_with_vid_pid");
	if (_LIBUSB_OPEN_DEVICE_WITH_VID_PID == NULL)
	{
		swordfish_debug_printf("������ַ��ȡʧ��!\n");
		return -1;
	}

//���ú���
// swordfish_debug_printf("sum=%d\n", p(12, 12));
#else
	_LIBUSB_GET_DEVICE_LIST = libusb_get_device_list;
	_LIBUSB_GET_DEVICE_DESCRIPTOR = libusb_get_device_descriptor;
	_LIBUSB_CLOSE = libusb_close;
	_LIBUSB_EXIT = libusb_exit;
	_LIBUSB_BULK_TRANSFER = libusb_bulk_transfer;
	_LIBUSB_INIT = libusb_init;
	_LIBUSB_CLAIM_INTERFACE = libusb_claim_interface;
	_LIBUSB_GET_BUS_NUMBER = libusb_get_bus_number;
	_LIBUSB_GET_PORT_NUMBER = libusb_get_port_number;
	_LIBUSB_GET_PORT_NUMBERS = libusb_get_port_numbers;
	_LIBUSB_GET_DEVICE_ADDRESS = libusb_get_device_address;
	_LIBUSB_FREE_DEVICE_LIST = libusb_free_device_list;
	_LIBUSB_OPEN = libusb_open;
	_LIBUSB_OPEN_DEVICE_WITH_VID_PID = libusb_open_device_with_vid_pid;
#endif
	return 0;
}
void swordfish_deinit()
{
	swordfish_debug_printf("deinit in feynman!\n");
#ifdef USEDLOPENLIBUSB
	/*3. �رտ��ļ�*/
	dlclose(libusbhandle);
#endif
	if(g_loghandle!=NULL)
	{
		log_destory(g_loghandle);
		g_loghandle=NULL;	
	}	
}


int swordfish_getyuvfromindex(int index, unsigned char *py, unsigned char *pu, unsigned char *pv)
{
	if (index < 255 && index >= 0)
	{
		*py = colortable[index][0];
		*pu = colortable[index][1];
		*pv = colortable[index][2];
		return 0;
	}
	else
	{
		return -1;
	}
}
//////////////////////////////////////

// int g_thread_running_flag = 0;
// static uint8_t *g_usb_buf = NULL;

// struct libusb_device_handle *g_h_dev_usb;

// static void *connectuserdata;
/*
typedef struct
{
	SWORDFISH_STATUS status;
	SWORDFISH_INTERFACE_TYPE interface_type;
	USBHANDLE device_usb_handle;
	SOCKET device_net_socket;
	grouppkt_info_custom_t groupbuffer;

	pthread_mutex_t sendmutex;

	int thread_running_flag;
	int global_time_enabled;
	int irwidth;
	int irheight;
	int rgbwidth;
	int rgbheight;
	int rgb_rectify_mode;
	EVENTCALLBACK eventcallback;
	void *connectuserdata;
	//	Ring_Queue *sendqueue;
	Ring_Queue *responsequeue;
	SWORDFISH_LIST_NODE *responselisthead;
	FRAMECALLBACK framecallback;
	Ring_Queue *otherpktqueue;
	Ring_Queue *depthpktqueue;
	Ring_Queue *leftirpktqueue;
	Ring_Queue *rightirpktqueue;
	Ring_Queue *rgbpktqueue;
	Ring_Queue *grouppktqueue;
	Ring_Queue *imupktqueue;
	Ring_Queue *goodfeaturepktqueue;
	Ring_Queue *savepktqueue;
	THREADHANDLE recvthread;
	THREADHANDLE depththread;
	THREADHANDLE rgbthread;
	THREADHANDLE groupthread;
	THREADHANDLE imuthread;
	THREADHANDLE goodfeaturethread;
	THREADHANDLE otherthread;
	THREADHANDLE leftirthread;
	THREADHANDLE rightirthread;
	THREADHANDLE savethread;
	//	THREADHANDLE sendthread;
	uint8_t *usb_buf;
	uint8_t *tmpbuf;
	SEM_T depthsem;
	SEM_T rgbsem;
	SEM_T groupsem;
	SEM_T leftirsem;
	SEM_T rightirsem;
	SEM_T imusem;
	SEM_T goodfeaturesem;
	SEM_T othersem;
	SEM_T postconnectsem;
	FRAMECALLBACK depthcallback;
	FRAMECALLBACK rgbcallback;
	FRAMECALLBACK groupcallback;
	FRAMECALLBACK leftircallback;
	FRAMECALLBACK rightircallback;
	FRAMECALLBACK savecallback;
	FRAMECALLBACK othercallback;
	FRAMECALLBACK imucallback;
	FRAMECALLBACK goodfeaturecallback;
	s_swordfish_device_info *camera_info;
} SWORDFISH_INSTANCE;*/
#pragma pack()
static void hal_close(SWORDFISH_DEVICE_HANDLE handle)
{
	SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)handle;
	if (inst->interface_type == SWORDFISH_USB_INTERFACE)
	{
		/*#ifdef _WINDOWS
				usb_release_interface(inst->device_usb_handle, 0); // ע���ӿڣ��ͷ���Դ����usb_claim_interface����ʹ�á�
				usb_close(inst->device_usb_handle);
		#else*/
		_LIBUSB_CLOSE(inst->device_usb_handle);
		_LIBUSB_EXIT(NULL);
		//#endif
	}
	else
	{ // network
#ifdef _WINDOWS
		closesocket(inst->device_net_socket);
#else
		swordfish_debug_printf("will shut device_net_socket!!!\n");
		shutdown(inst->device_net_socket, SHUT_RDWR);
		close(inst->device_net_socket);

#endif
	}
}
BOOL swordfish_hasconnect(SWORDFISH_DEVICE_HANDLE handle)
{
	//	swordfish_debug_printf("in swordfish_hasconnect\n");
	SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)handle;
	return inst->status >= CONNECTED && inst->status != STOPPED;
}
int hal_write(SWORDFISH_DEVICE_HANDLE handle, uint8_t *data, int len, int *bytesTransffered, int32_t timeout)
{
	if (swordfish_hasconnect(handle))
	{
		SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)handle;
		if (inst->interface_type == SWORDFISH_USB_INTERFACE)
		{
			/*#ifdef _WINDOWS
						int32_t ret = usb_bulk_write(inst->device_usb_handle, SWORDFISH_ENDPOINT_OUT, (char *)data, len, timeout);
						*bytesTransffered = ret;
			#else*/
			int32_t ret = _LIBUSB_BULK_TRANSFER(inst->device_usb_handle, SWORDFISH_ENDPOINT_OUT, (unsigned char *)data, len, bytesTransffered, timeout);
			//#endif
			return ret;
		}
		else
		{
			int failtimes = 0;
			int index = 0;
			while (index < len && failtimes < 5)
			{
				int ret = send(inst->device_net_socket, (char *)data + index, len - index, 0);
				if (ret > 0)
					index += ret;
				else if (ret == 0)
				{
					failtimes++;
					swordfish_debug_printf("just write timeout!!!\n");
				}
				else
				{
					if (inst->eventcallback != NULL)
						inst->eventcallback(inst->connectuserdata);
					return -1;
				}
			}
			*bytesTransffered = index;
			if (index == len)
				return 0;
			else
				return -1;
		}
	}
	else
	{
		return -1;
	}
}
int send_one_packet(SWORDFISH_DEVICE_HANDLE handle, uint16_t type, uint16_t sub_type, uint32_t len, const uint8_t *pData, int timeout)
{
	SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)handle;
	pthread_mutex_lock(&inst->sendmutex);	
	int bytesTransffered;
	SWORDFISH_USBHeaderDataPacket *usbPacket = (SWORDFISH_USBHeaderDataPacket *)calloc(1, sizeof(SWORDFISH_USBHeaderDataPacket) + len);
	int ret = 0;
	uint32_t checksum = 0;

	if (len > ONE_PACKET_DATA_MAX_SIZE)
	{
		free(usbPacket);
		swordfish_debug_printf("packet data too long\r\n");
		pthread_mutex_unlock(&inst->sendmutex);	
		return -1;
	}
	//	memset(&usbPacket, 0, sizeof(usbPacket));

	usbPacket->magic[0] = 'N';
	usbPacket->magic[1] = 'E';
	usbPacket->magic[2] = 'X';
	usbPacket->magic[3] = 'T';
	usbPacket->magic[4] = '_';
	usbPacket->magic[5] = 'V';
	usbPacket->magic[6] = 'P';
	usbPacket->magic[7] = 'U';
	usbPacket->type = type;
	usbPacket->sub_type = sub_type;
	usbPacket->len = len;

	if (len > 0)
	{
		for (uint32_t i = 0; i < len; i++)
		{
			checksum += pData[i];
		}
		memcpy((void *)usbPacket->data, pData, len);
	}
	usbPacket->checksum = checksum;

	ret = hal_write(handle, (uint8_t *)usbPacket, sizeof(SWORDFISH_USBHeaderDataPacket) + len, &bytesTransffered, timeout);
	free(usbPacket);
	if (ret < 0 || ((uint32_t)bytesTransffered) != (sizeof(SWORDFISH_USBHeaderDataPacket) + len))
	{
		swordfish_debug_printf("[%s]: ret = %d\n", __FUNCTION__, ret);
		pthread_mutex_unlock(&inst->sendmutex);	
		return -1;
	}
	pthread_mutex_unlock(&inst->sendmutex);	
	return 0;
}
/*
void swordfish_setpointcloudtransfer(SWORDFISH_DEVICE_HANDLE handle, int willtransfer)
{
	if (swordfish_hasconnect(handle))
	{
		s_swordfish_img_pointcloud_transfer tmptransfer;
		tmptransfer.status = willtransfer;

		send_one_packet(handle, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_USB_IMG_POINTCLOUD_TRANSFER_COMMAND, sizeof(s_swordfish_img_pointcloud_transfer), (const uint8_t *)&tmptransfer, 2000);
	}
}*/

static int swordfish_transferrgb(SWORDFISH_DEVICE_HANDLE handle, int enable)
{
	if (swordfish_hasconnect(handle))
	{
		s_swordfish_img_depth_rgb_transfer tmptransfer;
		tmptransfer.status = enable;

		return send_one_packet(handle, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_USB_IMG_DEPTH_RGB_TRANSFER_COMMAND, sizeof(s_swordfish_img_depth_rgb_transfer), (const uint8_t *)&tmptransfer, 2000);
	}
	return -1;
}
static int swordfish_transferir(SWORDFISH_DEVICE_HANDLE handle, int enable)
{
	if (swordfish_hasconnect(handle))
	{
		s_swordfish_img_depth_ir_transfer cmd;
		cmd.status = enable;
		return send_one_packet(handle, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_USB_IMG_DEPTH_IR_TRANSFER_COMMAND, sizeof(s_swordfish_img_depth_depth_transfer), (const uint8_t *)&cmd, 2000);
	}
	return -1;
}

static int swordfish_transferdepth(SWORDFISH_DEVICE_HANDLE handle, int mode)
{ // 0.off 1.depth 2.disparity
	if (swordfish_hasconnect(handle))
	{
		if (mode == 0)
		{
			s_swordfish_img_depth_depth_transfer cmd;
			cmd.status = 0;
			return send_one_packet(handle, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_USB_IMG_DEPTH_DEPTH_TRANSFER_COMMAND, sizeof(s_swordfish_img_depth_depth_transfer), (const uint8_t *)&cmd, 2000);
		}
		else if (mode == 1)
		{
			swordfish_debug_printf("will real set depth stream tarnsfer.....\n");
			s_swordfish_img_depth_depth_transfer cmd;
			cmd.status = 1;
			int ret = send_one_packet(handle, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_USB_IMG_DEPTH_DEPTH_TRANSFER_COMMAND, sizeof(s_swordfish_img_depth_depth_transfer), (const uint8_t *)&cmd, 2000);

			if (ret >= 0)
			{
				s_swordfish_depth_img_source srccmd;
				srccmd.source = 1;
				return send_one_packet(handle, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_DEPTH_DATA_SOURCE_COMMAND, sizeof(s_swordfish_depth_img_source), (const uint8_t *)&srccmd, 2000);
			}
			return ret;
		}
		else if (mode == 2)
		{
			s_swordfish_img_depth_depth_transfer cmd;
			cmd.status = 1;
			int ret = send_one_packet(handle, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_USB_IMG_DEPTH_DEPTH_TRANSFER_COMMAND, sizeof(s_swordfish_img_depth_depth_transfer), (const uint8_t *)&cmd, 2000);

			if (ret >= 0)
			{
				s_swordfish_depth_img_source srccmd;
				srccmd.source = 0;
				send_one_packet(handle, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_DEPTH_DATA_SOURCE_COMMAND, sizeof(s_swordfish_depth_img_source), (const uint8_t *)&srccmd, 2000);
			}
			return ret;
		}
		return -1;
	}
	return -1;
}
SWORDFISH_RESULT swordfish_senduserdata(SWORDFISH_DEVICE_HANDLE handle,void* data,int size)
{
	if (swordfish_hasconnect(handle))
	{
		int ret=send_one_packet(handle, SWORDFISH_USER_DATA, SWORDFISH_USER_DATA_TO_BOARD, size, (const uint8_t *)data, 2000);
		if(ret>=0)return SWORDFISH_OK;
		else
			return SWORDFISH_FAILED;	
	}
	return SWORDFISH_FAILED;
}


void swordfish_setfactory(SWORDFISH_DEVICE_HANDLE handle, int status)
{
	if (swordfish_hasconnect(handle))
	{
		s_swordfish_switch_to_factory cmd;
		cmd.status = status;
		send_one_packet(handle, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SWITCH_TO_FACTORY_COMMAND, sizeof(s_swordfish_switch_to_factory), (const uint8_t *)&cmd, 2000);
	}
}

static int swordfish_imuenable(SWORDFISH_DEVICE_HANDLE handle, int enable)
{
	if (enable)
	{
		s_swordfish_imu_transfer tmptransfer;
		tmptransfer.status = 1;

		return send_one_packet(handle, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_USB_IMU_TRANSFER_COMMAND, sizeof(s_swordfish_imu_transfer), (const uint8_t *)&tmptransfer, 2000);
	}
	else
	{
		s_swordfish_imu_transfer tmptransfer;
		tmptransfer.status = 0;
		return send_one_packet(handle, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_USB_IMU_TRANSFER_COMMAND, sizeof(s_swordfish_imu_transfer), (const uint8_t *)&tmptransfer, 2000);
	}
}
//#ifndef _WINDOWS
static libusb_device_handle *my_open_device_with_vid_pid(
	libusb_context *ctx, uint16_t vendor_id, uint16_t product_id, int bus, char *path)
{
	struct libusb_device **devs;
	struct libusb_device *found = NULL;
	struct libusb_device *dev;
	struct libusb_device_handle *dev_handle = NULL;
	size_t i = 0;
	int r, j;

	swordfish_debug_printf("will get device list!\n");
	if (_LIBUSB_GET_DEVICE_LIST(ctx, &devs) < 0)
	{
		swordfish_debug_printf("fail to get device list!\n");
		return NULL;
	}

	swordfish_debug_printf("will while device list!\n");
	while ((dev = devs[i++]) != NULL)
	{
		struct libusb_device_descriptor desc;
		r = _LIBUSB_GET_DEVICE_DESCRIPTOR(dev, &desc);
		if (r < 0)
			goto out;
		int tmpbus = _LIBUSB_GET_BUS_NUMBER(dev);

		char tmppath[64];
		memset(&tmppath[0], 0, sizeof(tmppath));
		swordfish_debug_printf("will get port numbers!\n");
		uint8_t pathcontains[8];
		r = _LIBUSB_GET_PORT_NUMBERS(dev, pathcontains, sizeof(pathcontains));
		if (r > 0)
		{
			sprintf(tmppath, "%d", pathcontains[0]);
			// printf(" path: %d", pathcontains[0]);
			for (j = 1; j < r; j++)
			{
				char subpath[16];
				sprintf(subpath, ".%d", pathcontains[j]);
				strcat(tmppath, subpath);
			}
		}
		else
		{
			swordfish_debug_printf("fail to get port numbers!\n");
		}

		swordfish_debug_printf("will get:path:%s!\n", tmppath);
		if (0 == strcmp(tmppath, path) && tmpbus == bus && desc.idVendor == vendor_id && desc.idProduct == product_id)
		{
			found = dev;
			break;
		}
	}

	if (found)
	{
		r = _LIBUSB_OPEN(found, &dev_handle);
		if (r < 0)
		{
			swordfish_debug_printf("fail to open dev_handle!%d\n", r);
			dev_handle = NULL;
		}
	}

out:
	_LIBUSB_FREE_DEVICE_LIST(devs, 1);
	swordfish_debug_printf("to the end,return dev_handle!\n");
	return dev_handle;
}
/*
char *getportid(libusb_context *ctx, int bus, int device)
{
	struct libusb_device **devs;
	struct libusb_device *found = NULL;
	struct libusb_device *dev;
	//	struct libusb_device_handle *dev_handle = NULL;
	size_t i = 0;
	char *retportnumbers = (char *)calloc(1, 128);

	if (_LIBUSB_GET_DEVICE_LIST(ctx, &devs) < 0)
	{
		swordfish_debug_printf("fail to get device list!\n");
		return NULL;
	}

	while ((dev = devs[i++]) != NULL)
	{
		struct libusb_device_descriptor desc;
		int r = _LIBUSB_GET_DEVICE_DESCRIPTOR(dev, &desc);
		if (r < 0)
			goto out;
		int tmpbus = _LIBUSB_GET_BUS_NUMBER(dev);
		int tmpdevice = _LIBUSB_GET_DEVICE_ADDRESS(dev);
		if (tmpdevice == device && tmpbus == bus)
		{
			found = dev;
			break;
		}
	}

	if (found)
	{
		unsigned char nums[8];
		int ret = _LIBUSB_GET_PORT_NUMBERS(found, nums, sizeof(nums));
		if (ret > 0)
		{
			sprintf(retportnumbers, "%d-", bus);
			for (int i = 0; i < ret; i++)
			{
				char tmp[8];
				if (i == 0)
				{
					sprintf(tmp, "%u", nums[i]);
				}
				else
				{
					sprintf(tmp, ".%u", nums[i]);
				}
				strcat(retportnumbers, tmp);
			}
		}
	}

out:
	_LIBUSB_FREE_DEVICE_LIST(devs, 1);
	swordfish_debug_printf("to the end,return dev_handle!\n");
	return retportnumbers;
}
*/
//#endif

void swordfish_freedevices(SWORDFISH_DEVICE_INFO *thedevice)
{
	if (thedevice == NULL)
		return;

	SWORDFISH_DEVICE_INFO *tmpdevice = thedevice;
	SWORDFISH_DEVICE_INFO *nextdevice = NULL;
	do
	{
		SWORDFISH_DEVICE_INFO *nextdevice = tmpdevice->next;
		free(tmpdevice);
		tmpdevice = nextdevice;
	} while (nextdevice != NULL);
}
SWORDFISH_RESULT swordfish_enum_device(int *ptotal, SWORDFISH_DEVICE_INFO **ppdevices)
{
	int countdevices = 0;
	SWORDFISH_DEVICE_INFO *tmpdevice = NULL;
/*	if (NULL != tmpdevice)
	{
		swordfish_freedevices(tmpdevice);
		tmpdevice = NULL;
	}*/
	/*
#ifdef _WINDOWS
	struct usb_bus *bus;
	struct usb_device *dev;
	int device_number = 0;

	static BOOL hasusbinit = FALSE;
	if (hasusbinit == FALSE)
		usb_init();
	usb_find_busses();
	usb_find_devices();

	// memset(swordfish_device, 0, sizeof(swordfish_device));

	for (bus = usb_get_busses(); bus; bus = bus->next)
	{
		// swordfish_debug_printf("bus:%p, first_device:%p\r\n", bus, bus->devices);
		for (dev = bus->devices; dev; dev = dev->next)
		{
			// swordfish_debug_printf("VID[0x%x],PID[0x%x]\r\n", dev->descriptor.idVendor, dev->descriptor.idProduct);
			if (dev->descriptor.idVendor == SWORDFISH_VID && dev->descriptor.idProduct == SWORDFISH_PID)
			{
				//	static unsigned char *tmpbuf = 0;
				//	swordfish_device[device_number].dev = dev;
				//	swordfish_device[device_number].dev_index = device_number;
				//	sprintf(swordfish_device[device_number].dev_desc, "swordfish_%02d", device_number);
				//	sprintf((char *)tmpbuf, "feynman-%d-%d",bus->location, dev->devnum);

				SWORDFISH_DEVICE_INFO *thedevice = (SWORDFISH_DEVICE_INFO *)calloc(1, sizeof(SWORDFISH_DEVICE_INFO));
				thedevice->next = tmpdevice;
				sprintf((char *)thedevice->usb_camera_name, "feynman-%d-%d", bus->location, dev->devnum);
				tmpdevice = thedevice;

				//	device_number++;
				// swordfish_debug_printf("find device, device_number\r\n", device_number);
			}
		}
	}
	// return device_number;

#else*/
	libusb_device **devs;
	int r;
	ssize_t cnt;

	r = _LIBUSB_INIT(NULL);
	if (r < 0)
	{
		swordfish_debug_printf("fail to init libusb!\n");
		return SWORDFISH_FAILED; // r;
	}
	cnt = _LIBUSB_GET_DEVICE_LIST(NULL, &devs);
	if (cnt < 0)
	{
		swordfish_debug_printf("fail to get usb device list!\n");
		_LIBUSB_EXIT(NULL);
		return SWORDFISH_FAILED; // (int)cnt;
	}
	swordfish_debug_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	libusb_device *dev;
	int i = 0, j = 0;
	uint8_t path[8];

	while ((dev = devs[i++]) != NULL)
	{
		struct libusb_device_descriptor desc;
		int r = _LIBUSB_GET_DEVICE_DESCRIPTOR(dev, &desc);
		if (r < 0)
		{
			fprintf(stderr, "failed to get device descriptor");
			_LIBUSB_EXIT(NULL);
			return SWORDFISH_FAILED;
		}
		int bus = _LIBUSB_GET_BUS_NUMBER(dev);
		// int device = _LIBUSB_GET_DEVICE_ADDRESS(dev);
		// int portnumber = _LIBUSB_GET_PORT_NUMBER(dev);

		char tmppath[64];
		memset(&tmppath[0], 0, sizeof(tmppath));
		r = _LIBUSB_GET_PORT_NUMBERS(dev, path, sizeof(path));
		if (r > 0)
		{
			sprintf(tmppath, "%d", path[0]);
			for (j = 1; j < r; j++)
			{
				char thepath[16];
				sprintf(thepath, ".%d", path[j]);
				strcat(tmppath, thepath);
			}
		}
		swordfish_debug_printf("%04x:%04x (bus %d, path %s)\n",
			   desc.idVendor, desc.idProduct, bus, tmppath);

		if (desc.idVendor == SWORDFISH_VID && desc.idProduct == SWORDFISH_PID)
		{
			/*	libusb_device_handle* handler = libusb_open_device_with_vid_pid(NULL, 0x0525, 0xa4a2);
			if (handler == NULL) {
			swordfish_debug_printf("fail to open usb device!\n");
			}
			else {
			swordfish_debug_printf("open usb device ok!");
			}*/
			// if (0 == ret) {
			/*	static unsigned char *tmpbuf = 0;
				if (0 == tmpbuf)
					tmpbuf = (unsigned char *)malloc(128);
				struct libusb_device_descriptor des;
			//	struct libusb_device_handle *dev_handle = libusb_open_device_with_vid_pid(NULL, SWORDFISH_VID, SWORDFISH_PID);
	*/
			//	libusb_device_get_string_descriptor_ascii(dev_handle, desc.iProduct, tmpbuf, 128);

			swordfish_debug_printf("got feynman device!\n");
			SWORDFISH_DEVICE_INFO *thedevice = (SWORDFISH_DEVICE_INFO *)calloc(1, sizeof(SWORDFISH_DEVICE_INFO));
			thedevice->next = tmpdevice;
			sprintf((char *)thedevice->usb_camera_name, "feynman-%d-%s", bus, tmppath);
			thedevice->type = SWORDFISH_USB_INTERFACE;
			tmpdevice = thedevice;
			countdevices++;

			//	callback((const char *)tmpbuf, userdata);
			//	_LIBUSB_CLOSE(dev_handle);
			//		_LIBUSB_CLOSE(handler);

			//	}
			//	else {
			//		swordfish_debug_printf("open usb ret:%d\n", ret);
			//	}
		}
	}
	swordfish_debug_printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
	_LIBUSB_FREE_DEVICE_LIST(devs, 1);

	_LIBUSB_EXIT(NULL);
	//#endif
	if (countdevices > 0)
	{
		*ptotal = countdevices;
		*ppdevices = tmpdevice;
		return SWORDFISH_OK;
	}
	else
		return SWORDFISH_FAILED;
}
/*void swordfish_refresh(DEVICECALLBACK callback, void *userdata)
{
#ifdef _WINDOWS
	struct usb_bus *bus;
	struct usb_device *dev;
	int device_number = 0;

	static bool hasusbinit = FALSE;
	if (hasusbinit == FALSE)
		usb_init();
	usb_find_busses();
	usb_find_devices();

	// memset(swordfish_device, 0, sizeof(swordfish_device));

	for (bus = usb_get_busses(); bus; bus = bus->next)
	{
		// swordfish_debug_printf("bus:%p, first_device:%p\r\n", bus, bus->devices);
		for (dev = bus->devices; dev; dev = dev->next)
		{
			// swordfish_debug_printf("VID[0x%x],PID[0x%x]\r\n", dev->descriptor.idVendor, dev->descriptor.idProduct);
			if (dev->descriptor.idVendor == SWORDFISH_VID && dev->descriptor.idProduct == SWORDFISH_PID)
			{
				static unsigned char *tmpbuf = 0;
				if (0 == tmpbuf)
					tmpbuf = (unsigned char *)malloc(128);
				//	swordfish_device[device_number].dev = dev;
				//	swordfish_device[device_number].dev_index = device_number;
				//	sprintf(swordfish_device[device_number].dev_desc, "swordfish_%02d", device_number);
				sprintf((char *)tmpbuf, "feynman-%d-%d", bus->location, dev->devnum);
				callback((const char *)tmpbuf, userdata);
				//	device_number++;
				// swordfish_debug_printf("find device, device_number\r\n", device_number);
			}
		}
	}
	// return device_number;

#else
	libusb_device **devs;
	int r;
	ssize_t cnt;

	r = _LIBUSB_INIT(NULL);
	if (r < 0)
		return; // r;
	cnt = _LIBUSB_GET_DEVICE_LIST(NULL, &devs);
	if (cnt < 0)
	{
		_LIBUSB_EXIT(NULL);
		return; // (int)cnt;
	}
	swordfish_debug_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	libusb_device *dev;
	int i = 0, j = 0;
	uint8_t path[8];

	while ((dev = devs[i++]) != NULL)
	{
		struct libusb_device_descriptor desc;
		int r = _LIBUSB_GET_DEVICE_DESCRIPTOR(dev, &desc);
		if (r < 0)
		{
			fprintf(stderr, "failed to get device descriptor");
			_LIBUSB_EXIT(NULL);
			return;
		}
		int bus = _LIBUSB_GET_BUS_NUMBER(dev);
		int device = _LIBUSB_GET_DEVICE_ADDRESS(dev);
		if (desc.idVendor == SWORDFISH_VID && desc.idProduct == SWORDFISH_PID)
		{
			// if (0 == ret) {
			static unsigned char *tmpbuf = 0;
			if (0 == tmpbuf)
				tmpbuf = (unsigned char *)malloc(128);
			//	struct libusb_device_descriptor des;
			struct libusb_device_handle *dev_handle = _LIBUSB_OPEN_DEVICE_WITH_VID_PID(NULL, SWORDFISH_VID, SWORDFISH_PID);

			//	libusb_device_get_string_descriptor_ascii(dev_handle, desc.iProduct, tmpbuf, 128);

			sprintf((char *)tmpbuf, "feynman-%d-%d", bus, device);
			callback((const char *)tmpbuf, userdata);
			_LIBUSB_CLOSE(dev_handle);
			//		_LIBUSB_CLOSE(handler);

			//	}
			//	else {
			//		swordfish_debug_printf("open usb ret:%d\n", ret);
			//	}
		}
		int portnumber = _LIBUSB_GET_PORT_NUMBER(dev);
		swordfish_debug_printf("portnumbers:%d,%04x:%04x (bus %d, device %d)\n", portnumber,
			   desc.idVendor, desc.idProduct, bus, device);

		r = _LIBUSB_GET_PORT_NUMBERS(dev, path, sizeof(path));
		if (r > 0)
		{
			swordfish_debug_printf(" path: %d", path[0]);
			for (j = 1; j < r; j++)
				swordfish_debug_printf(".%d", path[j]);
		}
		swordfish_debug_printf("\n");
	}
	swordfish_debug_printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
	_LIBUSB_FREE_DEVICE_LIST(devs, 1);

	_LIBUSB_EXIT(NULL);
#endif
}*/
static int hal_read(SWORDFISH_DEVICE_HANDLE handle, uint8_t *data, int len, int *bytesTransffered)
{
	SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)handle;

	if (inst->interface_type == SWORDFISH_USB_INTERFACE)
	{
		int32_t timeout = 2000;
		/*#ifdef _WINDOWS
				int ret = usb_bulk_read(inst->device_usb_handle, SWORDFISH_ENDPOINT_IN, (char *)data, len, timeout);
				*bytesTransffered = ret;
				return ret;
		#else*/

		int ret = _LIBUSB_BULK_TRANSFER(inst->device_usb_handle, SWORDFISH_ENDPOINT_IN, (unsigned char *)data, len, bytesTransffered, timeout);

		// swordfish_debug_printf("has recv:%d bytes\n", *bytesTransffered);
		if (*bytesTransffered > 0)
		{
			ret = *bytesTransffered;
		}
		//	if(ret<0)swordfish_debug_printf("hal_read ret:%d\n",ret);
		return ret;
		//#endif
	}
	else
	{
		//	swordfish_debug_printf("will recv from network!!!,len:%d\n",len);
		int retlen = recv(inst->device_net_socket, (char *)data, len, 0);
#ifdef _WINDOWS
		if (retlen == SOCKET_ERROR)
		{
			int ret = WSAGetLastError();
			if (ret != WSAETIMEDOUT)
			{
				swordfish_debug_printf("recv failed with:%d\n", ret);
				return -1;
			}
			return 0; // timeout
		}
#else
		if (retlen == -1)
		{
			swordfish_debug_printf("recv failed with:%d\n", errno);
			return -1;
		}

#endif
		if (retlen == 0)
		{
			return -1;
		}
		*bytesTransffered = retlen;
		return retlen;
	}
}
static int isgroup(uint64_t tm1, uint64_t tm2, uint64_t tm3, uint64_t tm4)
{
	if (tm1 != 0 && tm2 != 0 && tm3 != 0 && tm4 != 0)
	{
		const int64_t offset = 20000;
		swordfish_debug_printf("offset:%ld,%ld,%ld,%ld\n", labs(tm1 - tm2), labs(tm2 - tm3), labs(tm3 - tm4), labs(tm4 - tm1));
		if (labs((int64_t)(tm1 - tm2)) < offset && labs((int64_t)(tm2 - tm3)) < offset && labs((int64_t)(tm3 - tm4)) < offset && labs((int64_t)(tm4 - tm1)) < offset)
		{
			return 1;
		}
	}
	return 0;
}
static void on_usb_data_receive(SWORDFISH_DEVICE_HANDLE handle, uint8_t *data, unsigned long len)
{
	// PRINTF("enter on_usb_data_receive!\n");
	SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)handle;

	if (NULL == inst->responsequeue)
	{
		inst->responsequeue = Create_Ring_Queue(9, sizeof(sendcmd_t));
	}
	if (len < sizeof(SWORDFISH_USBHeaderDataPacket))
	{
		swordfish_debug_printf("data too short:%lu<=%lu\n", len, sizeof(SWORDFISH_USBHeaderDataPacket));
		return;
	}

	if (0 != strncmp("NEXT_VPU", (const char *)data, strlen("NEXT_VPU")))
	{
		swordfish_debug_printf("first 8 character is not NEXT_VPU!\n");
		return;
	}

	SWORDFISH_USBHeaderDataPacket *tmppack = (SWORDFISH_USBHeaderDataPacket *)data;
	if (tmppack->len != (len - sizeof(SWORDFISH_USBHeaderDataPacket)))
	{
		swordfish_debug_printf("len not equal:%lu,%d\n", len - sizeof(SWORDFISH_USBHeaderDataPacket), tmppack->len);
		return;
	}

	swordfish_debug_printf("current type:%d,sub_type:%d,datalen:%u\n", tmppack->type, tmppack->sub_type,tmppack->len);

/*	if (inst->status == CONNECTED)// && tmppack->type == SWORDFISH_LOG_DATA)
	{
	//	if (swordfish_transferdepth(inst, 0) >= 0 && swordfish_transferir(inst, 0) >= 0 && swordfish_transferrgb(inst, 0) >= 0 && swordfish_imuenable(inst, 0) >= 0)
		{
			swordfish_debug_printf("enter post connected!\n");
			inst->status = POSTCONNECTED;
			SEM_POST(&inst->postconnectsem);
			return;
		}
	}

	swordfish_debug_printf("test postconnected and image data\n");
if(inst->status == POSTCONNECTED && tmppack->type==SWORDFISH_IMAGE_DATA){
		return;
	}

	swordfish_debug_printf("set started\n");
	inst->status = STARTED;
	SEM_POST(&inst->postconnectsem);
	if(inst->status == CONFIGURED && tmppack->type==SWORDFISH_IMAGE_DATA){
		//test if width and height equal to inst->imgheight....,or reset pipeline
		if (tmppack->type == SWORDFISH_IMAGE_DATA)
		{ // here,we will save data to rgb/depth/ir/save queue
			SWORDFISH_USB_IMAGE_HEADER *tmpimgheader = (SWORDFISH_USB_IMAGE_HEADER *)(tmppack->data);
			if(tmpimgheader->width!=(uint32_t)inst->irwidth||tmpimgheader->height!=(uint32_t)inst->irheight){
				s_swordfish_pipeline_cmd value;
				value.status = 3;
				int ret4 = send_one_packet(handle, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_USB_PIPELINE_COMMAND, sizeof(s_swordfish_pipeline_cmd), (const uint8_t *)&value, 2000);
				if(ret4>=0){
					inst->status = STARTED;
					SEM_POST(&inst->postconnectsem);
					return;
				}
			}else{
				inst->status = STARTED;
				SEM_POST(&inst->postconnectsem);
			}	
		}
		return;
	}
*/	
	/////////////////////////get all from responsequeue to responselisthead
	//	if(inst->sendthread!=NULL){
	{
		//	sendcmd_t tmpcmd;
		sendcmd_t *responsenode = 0;
		while (1)
		{
			responsenode = (sendcmd_t *)SOLO_Read(inst->responsequeue);
			if (responsenode)
			{
				SWORDFISH_LIST_NODE *tmpnode = (SWORDFISH_LIST_NODE *)calloc(1, sizeof(SWORDFISH_LIST_NODE));
				tmpnode->data = *responsenode;
				tmpnode->next = inst->responselisthead;
				inst->responselisthead = tmpnode;
				SOLO_Read_Over(inst->responsequeue);
			}
			else
			{
				break;
			}
		}
	}

	swordfish_debug_printf("process respond\n");
	{
		//	swordfish_debug_printf("curernt sub_type:%d\n",tmppack->sub_type);
		//	swordfish_debug_printf("vector size:%d\n",inst->responselisthead->size());
		// std::vector<sendcmd_t>::iterator iter;
		SWORDFISH_LIST_NODE *tmpnode = inst->responselisthead;
		SWORDFISH_LIST_NODE *prevnode = NULL;
		while (tmpnode != NULL)
		// for (iter = inst->responselisthead->begin(); iter != inst->responselisthead->end();)
		{
			sendcmd_t tmpcmd = tmpnode->data;

			MYTIMEVAL tmpval;
			gettimeofday(&tmpval, NULL);

			swordfish_debug_printf("current sub_type:%d,want sub_type:%d\n", tmppack->sub_type, tmpcmd.responsesubtype);
			///////////////1.û���ڣ�����һ��
			if ((tmppack->type == SWORDFISH_COMMAND_DATA || tmppack->type == SWORDFISH_DOWNLOAD_DATA) &&
				((tmpval.tv_sec - tmpcmd.starttm.tv_sec) * 1000 + (tmpval.tv_usec - tmpcmd.starttm.tv_usec) / 1000) < tmpcmd.timeout && tmppack->sub_type == tmpcmd.responsesubtype)
			{
				swordfish_debug_printf("found reponse,copy data....\n");
				memcpy(tmpcmd.responsedata, tmppack->data, tmppack->len);
				*tmpcmd.responselen = tmppack->len;
				if (SWORDFISH_COMMAND_GET_CAM_PARAM_RETURN == tmpcmd.responsesubtype)
				{
					if (tmppack->len == (sizeof(s_swordfish_cam_param) - 4))
					{ // old firmware version
						((s_swordfish_cam_param *)tmpcmd.responsedata)->cam_param_valid = 1;
					}
				}
				swordfish_debug_printf("SEM_POST!!!\n");
				*tmpcmd.hastimeout = FALSE;
				SEM_POST(tmpcmd.waitsem);
				// iter = inst->responselisthead->erase(iter);
				SWORDFISH_LIST_NODE *tobedelete = tmpnode;
				tmpnode = tmpnode->next;
				if (prevnode != NULL)
					prevnode->next = tmpnode;
				else
					inst->responselisthead = tmpnode;
				free(tobedelete);
			}
			else if (((tmpval.tv_sec - tmpcmd.starttm.tv_sec) * 1000 + (tmpval.tv_usec - tmpcmd.starttm.tv_usec) / 1000) >= tmpcmd.timeout)
			{
				swordfish_debug_printf("timeout,just abanden!!!\n");
				*tmpcmd.hastimeout = TRUE;
				SEM_POST(tmpcmd.waitsem);
				// iter = inst->responselisthead->erase(iter);
				SWORDFISH_LIST_NODE *tobedelete = tmpnode;
				tmpnode = tmpnode->next;
				if (prevnode != NULL)
					prevnode->next = tmpnode;
				else
					inst->responselisthead = tmpnode;
				free(tobedelete);
			}
			else
			{
				prevnode = tmpnode;
				tmpnode = tmpnode->next;
			}
		}
	}
	//	}
	/*	if (tmppack->type == SWORDFISH_COMMAND_DATA)
	{ //&&tmppack->sub_type== SWORDFISH_COMMAND_GET_DEVICE_SN_RETURN) {
		if (tmppack->sub_type == SWORDFISH_COMMAND_GET_DEPTH_CONFIGURATION_RETURN ||
			tmppack->sub_type == SWORDFISH_COMMAND_SET_DEPTH_CONFIGURATION_RETURN)
		{
			s_depthconfig = *(s_swordfish_depth_config *)tmppack->data;
		}

		callback(tmppack, connectuserdata);
	}
	else if (tmppack->type == SWORDFISH_LOG_DATA)
	{
		callback(tmppack, connectuserdata);
	}
	else if (tmppack->type == SWORDFISH_CNN_DATA)
	{
		callback(tmppack, connectuserdata);
	}
	else */
	swordfish_debug_printf("handle status:%d,data type:%d,subtype:%d\n", inst->status, tmppack->type, tmppack->sub_type);
	if (tmppack->type == SWORDFISH_IMAGE_DATA && inst->status == STARTED)
	{ 
		SWORDFISH_USB_IMAGE_HEADER *tmpimgheader = (SWORDFISH_USB_IMAGE_HEADER *)(tmppack->data);

		
	}
	if(inst->resultcallback!=NULL){
		inst->resultcallback(tmppack,inst->connectuserdata);
	}

}

// pthread_cond_t  g_cond  = PTHREAD_COND_INITIALIZER;
/*
#ifdef _WINDOWS
static unsigned __stdcall saveprocessthread(void *param)
{
#else
static void *saveprocessthread(void *param)
{
#endif
	SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)param;
	while (inst->thread_running_flag && NULL == inst->savepktqueue)
	{
#ifdef _WINDOWS
		Sleep(100);
#else
		usleep(100 * 1000);
#endif
	}

#ifdef _WINDOWS
	HANDLE waitevent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
#endif
	while (inst->thread_running_flag)
	{
		pkt_info_custom_t *p = 0;
		p = (pkt_info_custom_t *)inst->savepktqueue->SOLO_Read();
		if (p)
		{
			int len = p->len;
			uint8_t *tmpbuf = (uint8_t *)malloc(1280 * 800 * 3);
			memcpy(tmpbuf, (unsigned char *)p->buffer, len);
			inst->savepktqueue->SOLO_Read_Over();

			inst->savecallback(tmpbuf, inst->connectuserdata);
			free(tmpbuf);
		}
		else
		{
#ifdef _WINDOWS
			WaitForSingleObject(waitevent, 5);
#else

			usleep(10000 * 1);
#endif
		}
	}

#ifdef _WINDOWS
	CloseHandle(waitevent);
#endif
	return 0;
}
*/

#ifdef _WINDOWS
static unsigned __stdcall imuprocessthread(void *param)
{
#else
static void *imuprocessthread(void *param)
{
#endif
	SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)param;
	while (inst->thread_running_flag && NULL == inst->imupktqueue)
	{
#ifdef _WINDOWS
		Sleep(100);
#else
		usleep(100 * 1000);
#endif
	}

	while (inst->thread_running_flag)
	{
		pkt_info_custom_t *p = 0;
		p = (pkt_info_custom_t *)SOLO_Read(inst->imupktqueue);
		if (p)
		{
			int len = p->len;
			uint8_t *tmpbuf = (uint8_t *)malloc(1280 * 800);
			memcpy(tmpbuf, (unsigned char *)p->buffer, len);
			SOLO_Read_Over(inst->imupktqueue);

			inst->imucallback(tmpbuf, inst->connectuserdata);
			free(tmpbuf);
		}
		else
		{
#ifdef _WINDOWS
			Sleep(10);
#else
			/*	MYTIMEVAL  now;
			struct timespec outtime;
			gettimeofday(&now, NULL);
			outtime.tv_sec = now.tv_sec + 5;
			outtime.tv_nsec = now.tv_usec * 1000;
			int ret = pthread_cond_timedwait(&g_cond,NULL, &outtime);*/
			usleep(10000 * 1);
#endif
		}
	}
	if (inst->status == STOPPED)
		pthread_detach(pthread_self());
	return 0;
}

#ifdef _WINDOWS
static unsigned __stdcall goodfeatureprocessthread(void *param)
{
#else
static void *goodfeatureprocessthread(void *param)
{
#endif
	SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)param;
	while (inst->thread_running_flag && NULL == inst->goodfeaturepktqueue)
	{
#ifdef _WINDOWS
		Sleep(100);
#else
		usleep(100 * 1000);
#endif
	}

	while (inst->thread_running_flag)
	{
		pkt_info_custom_t *p = 0;
		p = (pkt_info_custom_t *)SOLO_Read(inst->goodfeaturepktqueue);
		if (p)
		{
			int len = p->len;
			uint8_t *tmpbuf = (uint8_t *)malloc(1280 * 800);
			memcpy(tmpbuf, (unsigned char *)p->buffer, len);
			SOLO_Read_Over(inst->goodfeaturepktqueue);

			inst->goodfeaturecallback(tmpbuf, inst->connectuserdata);
			free(tmpbuf);
		}
		else
		{
#ifdef _WINDOWS
			Sleep(10);
#else
			/*	MYTIMEVAL  now;
			struct timespec outtime;
			gettimeofday(&now, NULL);
			outtime.tv_sec = now.tv_sec + 5;
			outtime.tv_nsec = now.tv_usec * 1000;
			int ret = pthread_cond_timedwait(&g_cond,NULL, &outtime);*/
			usleep(10000 * 1);
#endif
		}
	}
	if (inst->status == STOPPED)
		pthread_detach(pthread_self());
	return 0;
}
#ifdef _WINDOWS
static unsigned __stdcall groupprocessthread(void *param)
{
#else
static void *
groupprocessthread(void *param)
{
#endif
	SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)param;
	while (inst->thread_running_flag && NULL == inst->grouppktqueue)
	{
#ifdef _WINDOWS
		Sleep(100);
#else
		usleep(100 * 1000);
#endif
	}
	while (inst->thread_running_flag)
	{
		grouppkt_info_custom_t *p = 0;
		p = (grouppkt_info_custom_t *)SOLO_Read(inst->grouppktqueue);
		if (p)
		{
			int len = p->len;
			uint8_t *tmpbuf = (uint8_t *)malloc(USB_PACKET_MAX_SIZE * 4);
			memcpy((void *)tmpbuf, (const void *)p->buffer, (size_t)len);
			SOLO_Read_Over(inst->grouppktqueue);

			if (inst->groupcallback != NULL)
				inst->groupcallback(tmpbuf, inst->connectuserdata);
			free(tmpbuf);
		}
		else
		{
#ifdef _WINDOWS
			Sleep(10);
#else
			/*	MYTIMEVAL  now;
			struct timespec outtime;
			gettimeofday(&now, NULL);
			outtime.tv_sec = now.tv_sec + 5;
			outtime.tv_nsec = now.tv_usec * 1000;
			int ret = pthread_cond_timedwait(&g_cond,NULL, &outtime);*/
			usleep(10000 * 1);
#endif
		}
	}
	if (inst->status == STOPPED)
		pthread_detach(pthread_self());
	return 0;
}

#ifdef _WINDOWS
static unsigned __stdcall rgbprocessthread(void *param)
{
#else
static void *
rgbprocessthread(void *param)
{
#endif
	SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)param;
	while (inst->thread_running_flag && NULL == inst->rgbpktqueue)
	{
#ifdef _WINDOWS
		Sleep(100);
#else
		usleep(100 * 1000);
#endif
	}
	while (inst->thread_running_flag)
	{
		pkt_info_custom_t *p = 0;
		p = (pkt_info_custom_t *)SOLO_Read(inst->rgbpktqueue);
		if (p)
		{
			int len = p->len;
			uint8_t *tmpbuf = (uint8_t *)malloc(1280 * 800 * 3);
			memcpy((void *)tmpbuf, (const void *)p->buffer, (size_t)len);
			SOLO_Read_Over(inst->rgbpktqueue);

			if (inst->rgbcallback != NULL)
				inst->rgbcallback(tmpbuf, inst->connectuserdata);
			free(tmpbuf);
		}
		else
		{
#ifdef _WINDOWS
			Sleep(10);
#else
			/*	MYTIMEVAL  now;
			struct timespec outtime;
			gettimeofday(&now, NULL);
			outtime.tv_sec = now.tv_sec + 5;
			outtime.tv_nsec = now.tv_usec * 1000;
			int ret = pthread_cond_timedwait(&g_cond,NULL, &outtime);*/
			usleep(10000 * 1);
#endif
		}
	}
	if (inst->status == STOPPED)
		pthread_detach(pthread_self());
	return 0;
}

#ifdef _WINDOWS
static unsigned __stdcall leftirprocessthread(void *param)
{
#else
static void *
leftirprocessthread(void *param)
{
#endif
	SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)param;

	while (inst->thread_running_flag && NULL == inst->leftirpktqueue)
	{
#ifdef _WINDOWS
		Sleep(100);
#else
		usleep(100 * 1000);
#endif
	}
	while (inst->thread_running_flag)
	{
		pkt_info_custom_t *p = 0;
		p = (pkt_info_custom_t *)SOLO_Read(inst->leftirpktqueue);
		if (p)
		{
			int len = p->len;
			uint8_t *tmpbuf = (uint8_t *)malloc(1280 * 800 * 2);
			memcpy(tmpbuf, (unsigned char *)p->buffer, len);
			SOLO_Read_Over(inst->leftirpktqueue);

			if (inst->leftircallback != NULL)
				inst->leftircallback(tmpbuf, inst->connectuserdata);
			free(tmpbuf);
		}
		else
		{
#ifdef _WINDOWS
			Sleep(10);
#else
			/*	MYTIMEVAL  now;
			struct timespec outtime;
			gettimeofday(&now, NULL);
			outtime.tv_sec = now.tv_sec + 5;
			outtime.tv_nsec = now.tv_usec * 1000;
			int ret = pthread_cond_timedwait(&g_cond,NULL, &outtime);*/
			usleep(10000 * 1);
#endif
		}
	}
	if (inst->status == STOPPED)
		pthread_detach(pthread_self());
	return 0;
}

#ifdef _WINDOWS
static unsigned __stdcall rightirprocessthread(void *param)
{
#else
static void *
rightirprocessthread(void *param)
{
#endif
	SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)param;

	while (inst->thread_running_flag && NULL == inst->rightirpktqueue)
	{
#ifdef _WINDOWS
		Sleep(100);
#else
		usleep(100 * 1000);
#endif
	}

	while (inst->thread_running_flag)
	{
		pkt_info_custom_t *p = 0;
		p = (pkt_info_custom_t *)SOLO_Read(inst->rightirpktqueue);
		if (p)
		{
			int len = p->len;
			uint8_t *tmpbuf = (uint8_t *)malloc(1280 * 800 * 2);
			memcpy(tmpbuf, (unsigned char *)p->buffer, len);
			SOLO_Read_Over(inst->rightirpktqueue);

			if (inst->rightircallback != NULL)
				inst->rightircallback(tmpbuf, inst->connectuserdata);
			free(tmpbuf);
		}
		else
		{
#ifdef _WINDOWS
			Sleep(10);
#else
			/*	MYTIMEVAL  now;
			struct timespec outtime;
			gettimeofday(&now, NULL);
			outtime.tv_sec = now.tv_sec + 5;
			outtime.tv_nsec = now.tv_usec * 1000;
			int ret = pthread_cond_timedwait(&g_cond,NULL, &outtime);*/
			usleep(10000 * 1);
#endif
		}
	}
	if (inst->status == STOPPED)
		pthread_detach(pthread_self());
	return 0;
}
#ifdef _WINDOWS
static unsigned __stdcall depthprocessthread(void *param)
{
#else
static void *depthprocessthread(void *param)
{
#endif
	swordfish_debug_printf("in depthprocessthread,wait for depthpktqueue\n");
	SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)param;
	while (inst->thread_running_flag && NULL == inst->depthpktqueue)
	{
#ifdef _WINDOWS
		Sleep(100);
#else
		usleep(100 * 1000);
#endif
	}

	while (inst->thread_running_flag)
	{
		//	swordfish_debug_printf("wait for depthpktqueue to be ready!\n");
		int timeout = 1000;
		/*struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += timeout / 1000;
		ts.tv_nsec += (timeout % 1000) * 1000000;*/
		int ret = SEM_TIMEDWAIT(&inst->depthsem, timeout); // &ts);
		if (ret == -1)
		{
			//	swordfish_debug_printf("read depth frame timeout.....\n");
			continue;
		}

		//	swordfish_debug_printf("in depthprocessthread,read depthpktqueue\n");
		pkt_info_custom_t *p = 0;
		p = (pkt_info_custom_t *)SOLO_Read(inst->depthpktqueue);
		if (p)
		{
			int len = p->len;
			uint8_t *tmpbuf = (uint8_t *)malloc(1280 * 800 * 3);
			memcpy(tmpbuf, (unsigned char *)p->buffer, len);
			SOLO_Read_Over(inst->depthpktqueue);
                           int width=((SWORDFISH_USB_IMAGE_HEADER *)((unsigned char *)tmpbuf + sizeof(SWORDFISH_USBHeaderDataPacket)))->width;
                           int height=((SWORDFISH_USB_IMAGE_HEADER *)((unsigned char *)tmpbuf + sizeof(SWORDFISH_USBHeaderDataPacket)))->height;
			if (inst->depthcallback != NULL&&width==inst->irwidth&&height==inst->irheight)
				inst->depthcallback(tmpbuf, inst->connectuserdata);
			free(tmpbuf);
		}
		else
		{
#ifdef _WINDOWS
			Sleep(10);
#else
			/*	MYTIMEVAL  now;
			struct timespec outtime;
			gettimeofday(&now, NULL);
			outtime.tv_sec = now.tv_sec + 5;
			outtime.tv_nsec = now.tv_usec * 1000;
			int ret = pthread_cond_timedwait(&g_cond,NULL, &outtime);*/
			usleep(10000 * 1);
#endif
		}
	}

	if (inst->status == STOPPED)
		pthread_detach(pthread_self());
	return 0;
}

#ifdef _WINDOWS
static unsigned __stdcall otherprocessthread(void *param)
{
#else
static void *otherprocessthread(void *param)
{
#endif
	swordfish_debug_printf("in otherprocessthread,wait for otherpktqueue\n");
	SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)param;
	while (inst->thread_running_flag && NULL == inst->otherpktqueue)
	{
#ifdef _WINDOWS
		Sleep(100);
#else
		usleep(100 * 1000);
#endif
	}

	while (inst->thread_running_flag)
	{
		//	swordfish_debug_printf("wait for otherpktqueue to be ready!\n");
		int timeout = 1000;
		/*struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += timeout / 1000;
		ts.tv_nsec += (timeout % 1000) * 1000000;*/
		int ret = SEM_TIMEDWAIT(&inst->othersem, timeout); // &ts);
		if (ret == -1)
		{
			//	swordfish_debug_printf("read depth frame timeout.....\n");
			continue;
		}

		//	swordfish_debug_printf("in depthprocessthread,read depthpktqueue\n");
		pkt_info_custom_t *p = 0;
		p = (pkt_info_custom_t *)SOLO_Read(inst->otherpktqueue);
		if (p)
		{
			int len = p->len;
			uint8_t *tmpbuf = (uint8_t *)malloc(1024 * 1024 * 5);
			memcpy(tmpbuf, (unsigned char *)p->buffer, len);
			SOLO_Read_Over(inst->otherpktqueue);
			if (inst->othercallback != NULL)
				inst->othercallback(tmpbuf, inst->connectuserdata);
			free(tmpbuf);
		}
		else
		{
#ifdef _WINDOWS
			Sleep(10);
#else
			/*	MYTIMEVAL  now;
			struct timespec outtime;
			gettimeofday(&now, NULL);
			outtime.tv_sec = now.tv_sec + 5;
			outtime.tv_nsec = now.tv_usec * 1000;
			int ret = pthread_cond_timedwait(&g_cond,NULL, &outtime);*/
			usleep(10000 * 1);
#endif
		}
	}
	if (inst->status == STOPPED)
		pthread_detach(pthread_self());
	return 0;
}

/*
#ifdef _WINDOWS
static unsigned __stdcall loop_send(void *pParam)
{
#else
static void *loop_send(void *pParam)
{
#endif
	SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)pParam;
	if (NULL == inst->sendqueue)
	{
		inst->sendqueue = Create_Ring_Queue(9, sizeof(sendcmd_t));
	}
	if (NULL == inst->responsequeue)
	{
		inst->responsequeue = Create_Ring_Queue(9, sizeof(sendcmd_t));
	}

	while (inst->thread_running_flag)
	{
		sendcmd_t *p = 0;
		p = (sendcmd_t *)inst->sendqueue->SOLO_Read();
		if (p)
		{
			if (swordfish_hasconnect(inst))
			{
				send_one_packet(inst, SWORDFISH_COMMAND_DATA, p->cmdsubtype, p->len, (const uint8_t *)p->data, p->timeout);
				/////////////////////������ɺ���Ҫ�����µĽڵ㸽�ӵ�response���У���loop_recv�����
				sendcmd_t *responsenode = 0;
				responsenode = (sendcmd_t *)inst->responsequeue->SOLO_Write();
				if (responsenode)
				{
					memcpy(responsenode, p, sizeof(sendcmd_t));
					inst->responsequeue->SOLO_Write_Over();
				}
				else
				{
					swordfish_debug_printf("fail to get node from responsequeue!\n");
				}
			}
			if (p->data != NULL)
				free(p->data);
			inst->sendqueue->SOLO_Read_Over();
		}
#ifdef _WINDOWS
		Sleep(1000);
#else
		usleep(1000 * 1000);
#endif
	}
	swordfish_debug_printf("will empty sendqueue!!!\n");
	sendcmd_t *sendqueuenode = 0;
	while (1)
	{
		sendqueuenode = (sendcmd_t *)inst->sendqueue->SOLO_Read();
		if (sendqueuenode)
		{
			*sendqueuenode->hastimeout = TRUE;
			SEM_POST(sendqueuenode->waitsem);
			inst->sendqueue->SOLO_Read_Over();
		}
		else
		{
			break;
		}
	}
	return 0;
}
*/
#ifdef _WINDOWS
static unsigned __stdcall loop_recv(void *pParam)
{
#else
static void *loop_recv(void *pParam)
{
#endif
	swordfish_debug_printf("enter loop_recv thread\n");
	SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)pParam;
	// FRAMECALLBACK callback = (FRAMECALLBACK)inst->framecallback;
	// depthtool* toolobj = (depthtool*)pParam;
	if (inst->usb_buf == NULL)
		inst->usb_buf = (uint8_t *)malloc(sizeof(uint8_t) * USB_PACKET_MAX_SIZE);
	// swordfish_debug_printf("after malloc usb_buf!\n");
	//	static unsigned char *tmpbuf = NULL;
	if (NULL == inst->tmpbuf)
		inst->tmpbuf = (unsigned char *)malloc(USB_PACKET_MAX_SIZE);
	// swordfish_debug_printf("after malloc tmpbuf!\n");

	if (inst->interface_type == SWORDFISH_USB_INTERFACE)
	{
		swordfish_debug_printf("USB type interface!\n");
		unsigned long index = 0;
		int currentpacketlen = 0;

		while (inst->thread_running_flag)
		{
			//	swordfish_debug_printf("running recv loop!\n");
			int bytesTransffered = 0;
			int ret = hal_read(inst, inst->tmpbuf, REAL_USB_PACKET_MAX_SIZE /*sizeof(toolobj->g_usb_buf)*/, &bytesTransffered);
#ifdef _WINDOWS

			if (ret <= 0)
			{
				continue;
			}
#else
			if (ret < 0)
			{
				swordfish_debug_printf("in recv,hal_read ret:%d\n", ret);
			}

			if (ret == 0)
			{
				swordfish_debug_printf("received nothing!!!!\n");
				continue;
			}
			else if (ret == LIBUSB_ERROR_TIMEOUT)
			{
				swordfish_debug_printf("read usb just timeout!!!\n");
				continue;
			}
			else if (ret < 0)
			{
				swordfish_debug_printf("ret:%d and not equal LIBUSB_ERROR_TIMEOUT,something bad happened!\n", ret);
				inst->status = STOPPED;
				inst->thread_running_flag = 0;
				pthread_detach(pthread_self());

				continue;
			}
#endif
			//	swordfish_debug_printf("recv something from usb!\n");
			if (((long)bytesTransffered >= (long)sizeof(SWORDFISH_USBHeaderDataPacket)) && (index == 0) && (0 == strncmp("NEXT_VPU", (const char *)inst->tmpbuf, 8)))
			{ //
				memcpy(inst->usb_buf, inst->tmpbuf, bytesTransffered);
				index += bytesTransffered;
				currentpacketlen = ((SWORDFISH_USBHeaderDataPacket *)inst->usb_buf)->len;
				if (index == (currentpacketlen + sizeof(SWORDFISH_USBHeaderDataPacket)))
				{

					on_usb_data_receive(inst, inst->usb_buf, index);
					index = 0;
				}
			}
			else if (index >= sizeof(SWORDFISH_USBHeaderDataPacket))
			{
				if ((index + bytesTransffered) <= USB_PACKET_MAX_SIZE)
				{
					memcpy(inst->usb_buf + index, inst->tmpbuf, bytesTransffered);
					index += bytesTransffered;
					if (index == (currentpacketlen + sizeof(SWORDFISH_USBHeaderDataPacket)))
					{
						on_usb_data_receive(inst, inst->usb_buf, index);
						index = 0;
					}
					else if (index > (currentpacketlen + sizeof(SWORDFISH_USBHeaderDataPacket)))
					{
						swordfish_debug_printf("fail data!!!\n");
						index = 0;
					}
				}
				else
				{
					swordfish_debug_printf("false data!\n");
					index = 0;
				}
			}
			else
			{
				swordfish_debug_printf("false data,give up....\n");
			}
		}

		swordfish_debug_printf("usb recv thread exit....\n");
	}
	else
	{

		while (inst->thread_running_flag)
		{
			//ͷ������ͷ��
			//��ʼ����һ�����϶��ǰ�ͷ
			int bytesTransffered = 0;
			int32_t ret = 0;
			unsigned long index = 0;
			while (index < sizeof(SWORDFISH_USBHeaderDataPacket))
			{
				ret = hal_read(inst, inst->tmpbuf + index, sizeof(SWORDFISH_USBHeaderDataPacket) - index, &bytesTransffered);
				if (ret > 0)
				{
					index += ret;
				}
				else if (ret == 0)
				{
					swordfish_debug_printf("just read timeout!!!\n");
				}
				else
				{
					swordfish_debug_printf("network failed\n");
					inst->thread_running_flag = 0;
					if (inst->eventcallback != NULL)
						inst->eventcallback(inst->connectuserdata);
					break;
				}
			}
			if (inst->thread_running_flag == 0)
			{
				continue;
			}
			/*	if (ret <= 0 || ret<sizeof(SWORDFISH_USBHeaderDataPacket))
				{
					swordfish_debug_printf("recv head packet failed....exit\n");
					g_thread_running_flag = 0;
					continue;
				}*/

			if (0 != memcmp(inst->tmpbuf, "NEXT_VPU", 8))
			{
				swordfish_debug_printf("magic error,recv head packet failed....exit\n");
				inst->thread_running_flag = 0;
				continue;
			}
			/*	else
				{
					swordfish_debug_printf("got NEXT_VPU,maintype:%d,subtype:%d,len:%d\n", ((SWORDFISH_USBHeaderDataPacket *)inst->tmpbuf)->type,
						   ((SWORDFISH_USBHeaderDataPacket *)inst->tmpbuf)->sub_type, ((SWORDFISH_USBHeaderDataPacket *)inst->tmpbuf)->len);
				}
			*/
			int toberecv = ((SWORDFISH_USBHeaderDataPacket *)inst->tmpbuf)->len;
			index = ret;
			//��ͷȷ����������ж�������
			while (toberecv > 0)
			{
				ret = hal_read(inst, inst->tmpbuf + index, toberecv, &bytesTransffered);
				if (ret > 0)
				{
					index += ret;
					toberecv -= ret;
				}
				else if (ret == 0)
				{
					swordfish_debug_printf("just read timeout!!!\n");
				}
				else
				{
					swordfish_debug_printf("network failed\n");
					inst->thread_running_flag = 0;

					if (inst->eventcallback != NULL)
						inst->eventcallback(inst->connectuserdata);
					break;
				}
			}
			if (inst->thread_running_flag == 0)
			{
				continue;
			}
			//	swordfish_debug_printf("has recv a frame,size:%d\n",index);
			on_usb_data_receive(inst, inst->tmpbuf, index);
		}
	}

	if (inst->responsequeue != NULL)
	{
		swordfish_debug_printf("will empty responsequeue!!!\n");
		sendcmd_t *responsenode = 0;
		while (1)
		{
			responsenode = (sendcmd_t *)SOLO_Read(inst->responsequeue);
			if (responsenode)
			{
				*responsenode->hastimeout = TRUE;
				SEM_POST(responsenode->waitsem);
				SOLO_Read_Over(inst->responsequeue);
			}
			else
			{
				break;
			}
		}
	}
	swordfish_debug_printf("will empty responselisthead!\n");
	SWORDFISH_LIST_NODE *tmpnode = inst->responselisthead;
	while (tmpnode != NULL)
	{
		*tmpnode->data.hastimeout = TRUE;
		SEM_POST(tmpnode->data.waitsem);
		SWORDFISH_LIST_NODE *tobedelete = tmpnode;
		tmpnode = tmpnode->next;
		free(tobedelete);
	}
	inst->responselisthead = NULL;
	SEM_POST(&inst->depthsem);
	if (inst->thread_running_flag == 0)
	{

		hal_close(inst);
	}
	swordfish_debug_printf("usb recv thread exit....\n");
	return 0;
}

static int usb_hal_open(SWORDFISH_INSTANCE *inst, int bus, char *path)
{
	/*#ifdef _WINDOWS
		struct usb_bus *tmpbus;
		struct usb_device *tmpdev;
		usb_find_busses();
		usb_find_devices();

		// memset(swordfish_device, 0, sizeof(swordfish_device));

		for (tmpbus = usb_get_busses(); tmpbus; tmpbus = tmpbus->next)
		{
			// swordfish_debug_printf("bus:%p, first_device:%p\r\n", bus, bus->devices);
			for (tmpdev = tmpbus->devices; tmpdev; tmpdev = tmpdev->next)
			{
				// swordfish_debug_printf("VID[0x%x],PID[0x%x]\r\n", dev->descriptor.idVendor, dev->descriptor.idProduct);
				if (tmpdev->descriptor.idVendor == SWORDFISH_VID && tmpdev->descriptor.idProduct == SWORDFISH_PID && 0==strcmp(tmpdev->filename , path))
				{
					inst->device_usb_handle = usb_open(tmpdev);

					if (usb_set_configuration(inst->device_usb_handle, 1) < 0)
					{
						//	errMsg = "Could not set configuration";
						usb_close(inst->device_usb_handle);
						return -1;
					}
					if (usb_claim_interface(inst->device_usb_handle, 0) < 0) // claim_interface 0 ע�������ϵͳͨ�ŵĽӿ� 0
					{
						//	errMsg = "Could not claim interface";
						usb_close(inst->device_usb_handle);
						return -1;
					}
					return 0;
				}
			}
		}
		return -1;

	#else*/
	int ret;
	char str[64];
	memset(str, 0, sizeof(str));
	ret = _LIBUSB_INIT(NULL);
	if (ret < 0)
	{
		swordfish_debug_printf("usb init failed");
		return -1;
	}
	swordfish_debug_printf("will open bus:%d,path:%s\n", bus, path);
	inst->device_usb_handle = my_open_device_with_vid_pid(NULL, SWORDFISH_VID, SWORDFISH_PID, bus, path);
	if (inst->device_usb_handle == NULL)
	{
		swordfish_debug_printf("open device failed");
		_LIBUSB_EXIT(NULL);
		return -1;
	}
	ret = _LIBUSB_CLAIM_INTERFACE(inst->device_usb_handle, 0);
	if (ret < 0)
	{
		swordfish_debug_printf("claim interface failed:%d,errno:%d\n", ret, errno);
		_LIBUSB_EXIT(NULL);
		return -1;
	}
	return 0;
	//#endif
}
// static BOOL s_hasconnect = FALSE;
/*
#ifdef _WINDOWS
static HANDLE recvthread, processthread;
static HANDLE sendthread;
#else
static pthread_t recvthread, depththread, rgbthread, imuthread, irthread, savethread, sendthread;
#endif
*/
static void swordfish_releasehandle(SWORDFISH_DEVICE_HANDLE handle)
{
	SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)handle;
	free(inst->tmpbuf);
	free(inst->usb_buf);
	if (NULL != inst->otherpktqueue)
	{
		Destroy_Ring_Queue(inst->otherpktqueue);
	}
	if (NULL != inst->depthpktqueue)
	{
		Destroy_Ring_Queue(inst->depthpktqueue);
	}
	if(inst->tminfo!=NULL)free(inst->tminfo);
	if (NULL != inst->leftirpktqueue)
	{
		Destroy_Ring_Queue(inst->leftirpktqueue);
	}
	if (NULL != inst->rightirpktqueue)
	{
		Destroy_Ring_Queue(inst->rightirpktqueue);
	}
	if (NULL != inst->grouppktqueue)
	{
		Destroy_Ring_Queue(inst->grouppktqueue);
	}
	if (NULL != inst->rgbpktqueue)
	{
		Destroy_Ring_Queue(inst->rgbpktqueue);
	}
	if (NULL != inst->imupktqueue)
	{
		Destroy_Ring_Queue(inst->imupktqueue);
	}
	if (NULL != inst->goodfeaturepktqueue)
	{
		Destroy_Ring_Queue(inst->goodfeaturepktqueue);
	}
	if (NULL != inst->savepktqueue)
	{
		Destroy_Ring_Queue(inst->savepktqueue);
	}

	SEM_DESTROY(&inst->postconnectsem);
	SEM_DESTROY(&inst->depthsem);
	SEM_DESTROY(&inst->othersem);
	SEM_DESTROY(&inst->rgbsem);
	SEM_DESTROY(&inst->groupsem);
	SEM_DESTROY(&inst->leftirsem);
	SEM_DESTROY(&inst->rightirsem);
	SEM_DESTROY(&inst->imusem);
	SEM_DESTROY(&inst->goodfeaturesem);

	free(inst);
}
static void swordfish_waitfordisconnect(SWORDFISH_DEVICE_HANDLE handle)
{
	SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)handle;
	if (swordfish_hasconnect(handle))
	{
#ifdef _WINDOWS
		swordfish_debug_printf("in swordfish_waitfordisconnect...\n");
		DWORD retfirst = WaitForSingleObject(inst->recvthread, INFINITE);

		swordfish_debug_printf("will join leftirthread...\n");
		DWORD retsecond = WaitForSingleObject(inst->leftirthread, INFINITE);

		swordfish_debug_printf("will join rightirthread...\n");
		retsecond = WaitForSingleObject(inst->rightirthread, INFINITE);

		swordfish_debug_printf("will join rgbthread...\n");
		DWORD retthird = WaitForSingleObject(inst->rgbthread, INFINITE);
		swordfish_debug_printf("will join groupthread...\n");
		DWORD retthird = WaitForSingleObject(inst->groupthread, INFINITE);

		swordfish_debug_printf("will join imuthread...\n");
		DWORD retfourth = WaitForSingleObject(inst->imuthread, INFINITE);

		swordfish_debug_printf("will join depththread...\n");
		DWORD retfifth = WaitForSingleObject(inst->depththread, INFINITE);
		swordfish_debug_printf("will join otherthread...\n");
		DWORD retfifthdotfive = WaitForSingleObject(inst->otherthread, INFINITE);

		swordfish_debug_printf("will join savethread...\n");
		DWORD retsixth = WaitForSingleObject(inst->savethread, INFINITE);
#else
		swordfish_debug_printf("in swordfish_waitfordisconnect...\n");
		pthread_join(inst->recvthread, NULL);
		pthread_mutex_destroy(&inst->sendmutex);

		if (inst->leftirthread != (THREADHANDLE)NULL)
		{
			swordfish_debug_printf("will join leftirthread...\n");
			pthread_join(inst->leftirthread, NULL);
		}
		if (inst->rightirthread != (THREADHANDLE)NULL)
		{
			swordfish_debug_printf("will join rightirthread...\n");
			pthread_join(inst->rightirthread, NULL);
		}
		if (inst->rgbthread != (THREADHANDLE)NULL)
		{
			swordfish_debug_printf("will join rgbthread...\n");
			pthread_join(inst->rgbthread, NULL);
		}
		if (inst->imuthread != (THREADHANDLE)NULL)
		{
			swordfish_debug_printf("will join imuthread...\n");
			pthread_join(inst->imuthread, NULL);
		}
		if (inst->groupthread != (THREADHANDLE)NULL)
		{
			swordfish_debug_printf("will join groupthread...\n");
			pthread_join(inst->groupthread, NULL);
		}
		if (inst->depththread != (THREADHANDLE)NULL)
		{
			swordfish_debug_printf("will join depththread...\n");
			pthread_join(inst->depththread, NULL);
		}
		if (inst->otherthread != (THREADHANDLE)NULL)
		{
			swordfish_debug_printf("will join otherthread...\n");
			pthread_join(inst->otherthread, NULL);
		}
		if (inst->savethread != (THREADHANDLE)NULL)
		{
			swordfish_debug_printf("will join savethread...\n");
			pthread_join(inst->savethread, NULL);
		}
#endif
		swordfish_releasehandle(inst);
	}
	else
	{
		swordfish_debug_printf("feynman not connected,so waitfordisconnect return instantly!\n");
	}
}

void swordfish_close(SWORDFISH_DEVICE_HANDLE handle)
{
	if (swordfish_hasconnect(handle))
	{
		s_swordfish_pipeline_cmd value;
		value.status = 2;
		swordfish_set(handle, SWORDFISH_PIPELINE, (void *)&value, sizeof(s_swordfish_pipeline_cmd), 2000);
		sleep(1);
		SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)handle;
		inst->thread_running_flag = 0;
		/*#ifdef _WINDOWS
		DWORD retfirst = WaitForSingleObject(recvthread, INFINITE);
		DWORD retsecond = WaitForSingleObject(processthread, INFINITE);
		swordfish_debug_printf("ret:%d:%d\n", retfirst, retsecond, retthird);
#else
		pthread_join(recvthread, NULL);
		pthread_join(irthread, NULL);
		pthread_join(rgbthread, NULL);
		pthread_join(imuthread, NULL);
		pthread_join(depththread, NULL);
		pthread_join(savethread, NULL);
#endif
*/
		swordfish_waitfordisconnect(handle);
	}
	else
	{
		swordfish_releasehandle(handle);
		swordfish_debug_printf("feynman not connected,so disconnectcamera has no effect!\n");
	}
}

static void c_split(char *src, const char *separator, int maxlen, char **dest, int *num)
{
	char *pNext;
	int count = 0;
	if (src == NULL || strlen(src) == 0)
		return;
	if (separator == NULL || strlen(separator) == 0)
		return;
	pNext = strtok(src, separator);
	while (pNext != NULL && count < maxlen)
	{
		*dest++ = pNext;
		++count;
		pNext = strtok(NULL, separator);
	}
	*num = count;
}
/*static char *id_data_process(void *data)
{
	SWORDFISH_USBHeaderDataPacket *packet = (SWORDFISH_USBHeaderDataPacket *)data;
	if (packet->type == SWORDFISH_DEVICE_DATA && packet->sub_type == SWORDFISH_DEVICE_DATA_ALL)
	{
		s_swordfish_device_info *tmpinfo = (s_swordfish_device_info *)packet->data;
		if (tmpinfo->device_sn.sn[15] != 0)
		{
			char *tmpstr = (char *)calloc(1, 17);
			memcpy(tmpstr, &tmpinfo->device_sn.sn[0], 16);
			return tmpstr;
		}
		else
		{
			return strdup((char *)tmpinfo->device_sn.sn);
		}
	}
	return NULL;
}
*/
static int net_hal_open(const char *deviceip)
{
#ifdef _WINDOWS
	SOCKET tmpsock = socket(AF_INET, SOCK_STREAM, 0);
	if (tmpsock == INVALID_SOCKET)
	{
		swordfish_debug_printf("socket() called failed!, error code is %d\n", WSAGetLastError());
		return -1;
	}
	else
	{
		swordfish_debug_printf("socket() called successful!\n");
	}
	//��Ҫ���ӵķ�����׽��ֽṹ��Ϣ
	SOCKADDR_IN addrServer;
	//�趨������IP
	addrServer.sin_addr.S_un.S_addr = inet_addr(deviceip);
	addrServer.sin_family = AF_INET;
	//�趨�������Ķ˿ں�(ʹ�������ֽ���)
	addrServer.sin_port = htons((unsigned short)8000);
	//�������������������
	swordfish_debug_printf("will connect to:%s:%d\n", deviceip, 8000);

	//���÷�������ʽ����
	unsigned long ul = 1;
	int ret = ioctlsocket(tmpsock, FIONBIO, (unsigned long *)&ul);
	if (ret == SOCKET_ERROR)
		return -1;

	connect(tmpsock, (SOCKADDR *)&addrServer, sizeof(SOCKADDR));

	// select ģ�ͣ������ó�ʱ
	fd_set r;
	FD_ZERO(&r);
	FD_SET(tmpsock, &r);

	struct timeval timeout;
	timeout.tv_sec = 3; //���ӳ�ʱ3��
	timeout.tv_usec = 0;
	ret = select(0, 0, &r, 0, &timeout);
	if (ret <= 0)
	{
		closesocket(tmpsock);
		return -1;
	}

	ul = 0;
	//�������ģʽ
	ret = ioctlsocket(tmpsock, FIONBIO, (unsigned long *)&ul);
	if (ret == SOCKET_ERROR)
	{
		closesocket(tmpsock);
		return -1;
	}

#else
	int re = -1;
	unsigned long ul;
	swordfish_debug_printf("device ip:%s\n", deviceip);

	// ���� socket
	int tmpsock;
	//	swordfish_debug_printf("create socket...\n");
	tmpsock = socket(AF_INET, SOCK_STREAM, 0);
	if (tmpsock < 0)
	{
		perror("create socket error");
		return -1;
	}

	//	swordfish_debug_printf("set nonblock socket...\n");
	// ���÷�����
	ul = 1;
	ioctl(tmpsock, FIONBIO, &ul);

	// ���� socket
	struct sockaddr_in cliaddr;

	// ���socket��IP��˿�
	memset(&cliaddr, 0,sizeof(struct sockaddr));
	cliaddr.sin_family = AF_INET;
	cliaddr.sin_port = htons(8000);
	cliaddr.sin_addr.s_addr = inet_addr(deviceip);

	//	swordfish_debug_printf("will connect to...\n");
	// �ͻ�������
	re = connect(tmpsock, (struct sockaddr *)&cliaddr, sizeof(struct sockaddr));
	if (re >= 0)
	{
		swordfish_debug_printf("just connected!\n");
		return 1;
	}

	//	swordfish_debug_printf("set sock to block...\n");
	if (re == 1)
	{
		// ����Ϊ����
		ul = 0;
		ioctl(tmpsock, FIONBIO, &ul);
		return tmpsock;
	}

	// ���ó�ʱʱ��
	re = 0;
	fd_set set;
	MYTIMEVAL tm;

	int len;
	int error = -1;

	tm.tv_sec = 3;
	tm.tv_usec = 0;
	FD_ZERO(&set);
	FD_SET(tmpsock, &set);
	// swordfish_debug_printf("will select!!!\n");

	re = select(tmpsock + 1, NULL, &set, NULL, &tm);
	len = sizeof(int);
	if (re > 0)
	{

		//	swordfish_debug_printf("after select!!!\n");

		// ��ȡsocket״̬
		getsockopt(tmpsock, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len);
		if (error != 0)
		{
			swordfish_debug_printf("socket status not valid!!!\n");
			close(tmpsock);
			return -1;
		}
	}

	//	swordfish_debug_printf("set to block!!!\n");
	// ����Ϊ����
	ul = 0;
	ioctl(tmpsock, FIONBIO, &ul);

#endif

#ifdef _WINDOWS
	int recvTimeout = 4 * 1000; // 30s
	int sendTimeout = 4 * 1000; // 30s
	setsockopt(tmpsock, SOL_SOCKET, SO_RCVTIMEO, (char *)&recvTimeout, sizeof(int));
	setsockopt(tmpsock, SOL_SOCKET, SO_SNDTIMEO, (char *)&sendTimeout, sizeof(int));
#else

	//	swordfish_debug_printf("set send and recv timeout!!!\n");
	MYTIMEVAL recvTimeout = {4, 0};
	MYTIMEVAL sendTimeout = {4, 0};
	setsockopt(tmpsock, SOL_SOCKET, SO_RCVTIMEO, (char *)&recvTimeout, sizeof(struct timeval));
	setsockopt(tmpsock, SOL_SOCKET, SO_SNDTIMEO, (char *)&sendTimeout, sizeof(struct timeval));

#endif
	return tmpsock;
}

/*
SWORDFISH_DEVICE_HANDLE swordfish_connectcamerasync(SWORDFISH_INTERFACE_TYPE dt, const char *devicename)
{
	EVENTCALLBACK eventcallback = NULL;
	FRAMECALLBACK imucallback = NULL;
	FRAMECALLBACK savecallback = NULL;
	FRAMECALLBACK depthcallback = NULL;
	FRAMECALLBACK ircallback = NULL;
	FRAMECALLBACK rgbcallback = NULL;
	FRAMECALLBACK othercallback = NULL;

	if (dt ==SWORDFISH_NETWORK_INTERFACE)
	{

		int ret = -1;

		int tmpsocket;
		if ((tmpsocket = net_hal_open(devicename)) >= 0)
		{
			SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)calloc(1, sizeof(SWORDFISH_INSTANCE));
			inst->interface_type =SWORDFISH_NETWORK_INTERFACE;
			inst->device_net_socket = tmpsocket;
			inst->hasconnect = TRUE;
			inst->depthcallback = NULL;
			inst->imucallback = NULL;
			inst->ircallback = NULL;
			inst->othercallback = NULL;
			inst->rgbcallback = NULL;
			inst->savecallback = NULL;
			inst->eventcallback = NULL;
			inst->connectuserdata = NULL;

			SEM_INIT(&inst->depthsem, 0, 0);

			ret = 1;

			inst->thread_running_flag = 1;

#ifdef _WINDOWS
			unsigned int threadID;
			if (depthcallback != NULL)
				inst->recvthread = (HANDLE)_beginthreadex(NULL, 0, loop_recv, (LPVOID)inst, 0, &threadID);
			if (ircallback != NULL)
				inst->depththread = (HANDLE)_beginthreadex(NULL, 0, depthprocessthread, (LPVOID)inst, 0, &threadID);
			if (ircallback != NULL)
				inst->irthread = (HANDLE)_beginthreadex(NULL, 0, irprocessthread, (LPVOID)inst, 0, &threadID);
			if (rgbcallback != NULL)
				inst->rgbthread = (HANDLE)_beginthreadex(NULL, 0, rgbprocessthread, (LPVOID)inst, 0, &threadID);
			if (imucallback != NULL)
				inst->imuthread = (HANDLE)_beginthreadex(NULL, 0, imuprocessthread, (LPVOID)inst, 0, &threadID);
			if (savecallback != NULL)
				inst->savethread = (HANDLE)_beginthreadex(NULL, 0, saveprocessthread, (LPVOID)inst, 0, &threadID);
#else

			pthread_create(&inst->recvthread, 0, loop_recv, (void *)inst);
			if (depthcallback != NULL)
				pthread_create(&inst->depththread, 0, depthprocessthread, (void *)inst);
			if (ircallback != NULL)
				pthread_create(&inst->irthread, 0, irprocessthread, (void *)inst);
			if (rgbcallback != NULL)
				pthread_create(&inst->rgbthread, 0, rgbprocessthread, (void *)inst);
			if (imucallback != NULL)
				pthread_create(&inst->imuthread, 0, imuprocessthread, (void *)inst);
			if (savecallback != NULL)
				pthread_create(&inst->savethread, 0, saveprocessthread, (void *)inst);
#endif
			return inst;
		}
		swordfish_debug_printf("fail to open connect to:%s\n", devicename);
		return NULL;
	}
	else
	{
		int ret = -1;

		char *msg = strdup(devicename);
		int num = 0;
		int bus, device;
		char *split_buf[512] = {0};
		int i = 0;
		swordfish_debug_printf("will split devicename!%s\n", msg);
		c_split((char *)msg, "-", 512, split_buf, &num);
		if (num < 3)
		{
			swordfish_debug_printf("fail to split devicename!%s\n", devicename);
			free(msg);
			return NULL;
		}
		bus = atoi(split_buf[1]);
		device = atoi(split_buf[2]);
		free(msg);

		SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)calloc(1, sizeof(SWORDFISH_INSTANCE));
		inst->interface_type = USB;
		if (usb_hal_open(inst, bus, device) >= 0)
		{

			inst->hasconnect = TRUE;
			inst->depthcallback = NULL;
			inst->imucallback = NULL;
			inst->ircallback = NULL;
			inst->othercallback = NULL;
			inst->rgbcallback = NULL;
			inst->savecallback = NULL;
			inst->eventcallback = NULL;
			inst->connectuserdata = NULL;

			ret = 1;

			inst->thread_running_flag = 1;

#ifdef _WINDOWS
			unsigned int threadID;

			inst->recvthread = (HANDLE)_beginthreadex(NULL, 0, loop_recv, (LPVOID)inst, 0, &threadID);

			if (depthcallback != NULL)
				inst->depththread = (HANDLE)_beginthreadex(NULL, 0, depthprocessthread, (LPVOID)inst, 0, &threadID);
			if (ircallback != NULL)
				inst->irthread = (HANDLE)_beginthreadex(NULL, 0, irprocessthread, (LPVOID)inst, 0, &threadID);
			if (rgbcallback != NULL)
				inst->rgbthread = (HANDLE)_beginthreadex(NULL, 0, rgbprocessthread, (LPVOID)inst, 0, &threadID);

			if (imucallback != NULL)
				inst->imuthread = (HANDLE)_beginthreadex(NULL, 0, imuprocessthread, (LPVOID)inst, 0, &threadID);

			if (savecallback != NULL)
				inst->savethread = (HANDLE)_beginthreadex(NULL, 0, saveprocessthread, (LPVOID)inst, 0, &threadID);

#else
			pthread_create(&inst->recvthread, 0, loop_recv, (void *)inst);
			if (depthcallback != NULL)
				pthread_create(&inst->depththread, 0, depthprocessthread, (void *)inst);
			if (ircallback != NULL)
				pthread_create(&inst->irthread, 0, irprocessthread, (void *)inst);
			if (rgbcallback != NULL)
				pthread_create(&inst->rgbthread, 0, rgbprocessthread, (void *)inst);
			if (imucallback != NULL)
				pthread_create(&inst->imuthread, 0, imuprocessthread, (void *)inst);
			if (savecallback != NULL)
				pthread_create(&inst->savethread, 0, saveprocessthread, (void *)inst);
#endif
			return inst;
		}
		return NULL;
	}
}
*/
// 1.connect camera
// 2.shutdown streams including depth/ir/rgb/imu
// 3.return handle
SWORDFISH_DEVICE_HANDLE swordfish_open(SWORDFISH_DEVICE_INFO *dev_info)
{
	if (dev_info->type == SWORDFISH_NETWORK_INTERFACE)
	{
		// int ret = -1;

		int tmpsocket;
		if ((tmpsocket = net_hal_open(dev_info->net_camera_ip)) >= 0)
		{
			SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)calloc(1, sizeof(SWORDFISH_INSTANCE));
			inst->interface_type = SWORDFISH_NETWORK_INTERFACE;
			inst->device_net_socket = tmpsocket;
			pthread_mutex_init(&inst->sendmutex, NULL);
			inst->tminfo=(TIMESTAMPINFO*)calloc(1,sizeof(TIMESTAMPINFO));
			inst->depthcallback = NULL;
			inst->imucallback = NULL;
			inst->leftircallback = NULL;
			inst->rightircallback = NULL;
			inst->othercallback = NULL;
			inst->rgbcallback = NULL;
			inst->savecallback = NULL;
			inst->eventcallback = NULL;
			inst->connectuserdata = NULL;
			SEM_INIT(&inst->depthsem, 0, 0);
			SEM_INIT(&inst->othersem, 0, 0);
			SEM_INIT(&inst->rgbsem, 0, 0);
			SEM_INIT(&inst->groupsem, 0, 0);
			SEM_INIT(&inst->leftirsem, 0, 0);
			SEM_INIT(&inst->rightirsem, 0, 0);
			SEM_INIT(&inst->imusem, 0, 0);
			SEM_INIT(&inst->goodfeaturesem, 0, 0);
			SEM_INIT(&inst->postconnectsem, 0, 0);
			inst->status = CONNECTED;

			// ret = 1;

			inst->thread_running_flag = 1;

#ifdef _WINDOWS
			unsigned int threadID;
			//		if (depthcallback != NULL)
			inst->recvthread = (HANDLE)_beginthreadex(NULL, 0, loop_recv, (LPVOID)inst, 0, &threadID);
			/*struct timespec ts;
			clock_gettime(CLOCK_REALTIME, &ts);
			ts.tv_sec += 4000 / 1000;
			ts.tv_nsec += (4000 % 1000) * 1000000;*/
			while (1)
			{
				int ret = SEM_TIMEDWAIT(&inst->postconnectsem, 4000); // &ts);
				if (ret == -1 && errno == ETIMEDOUT)
				{
					swordfish_debug_printf("connectcamera wait for postconnect timeout.....\n");
					swordfish_close(inst);
					// swordfish_releasehandle(inst);
					//	free(inst);
					return NULL;
				}
				else if (ret == -1 && errno != ETIMEDOUT)
				{
					swordfish_debug_printf("connectcamera wait for postconnect,some else err happened:%d\n", errno);
				}
				else if (ret == 0)
				{
					break;
				}
			}
			swordfish_debug_printf("connect ok!!!!\n");

			/*		if (ircallback != NULL)
						inst->depththread = (HANDLE)_beginthreadex(NULL, 0, depthprocessthread, (LPVOID)inst, 0, &threadID);
					if (ircallback != NULL)
						inst->irthread = (HANDLE)_beginthreadex(NULL, 0, irprocessthread, (LPVOID)inst, 0, &threadID);
					if (rgbcallback != NULL)
						inst->rgbthread = (HANDLE)_beginthreadex(NULL, 0, rgbprocessthread, (LPVOID)inst, 0, &threadID);
					if (imucallback != NULL)
						inst->imuthread = (HANDLE)_beginthreadex(NULL, 0, imuprocessthread, (LPVOID)inst, 0, &threadID);
					if (savecallback != NULL)
						inst->savethread = (HANDLE)_beginthreadex(NULL, 0, saveprocessthread, (LPVOID)inst, 0, &threadID);
						*/
#else
			pthread_create(&inst->recvthread, 0, loop_recv, (void *)inst);

			/*	struct timespec ts;
				clock_gettime(CLOCK_REALTIME, &ts);
				ts.tv_sec += 4000 / 1000;
				ts.tv_nsec += (4000 % 1000) * 1000000;*/
			while (1)
			{
				int ret = SEM_TIMEDWAIT(&inst->postconnectsem, 4000);
				if (ret == -1 && errno == ETIMEDOUT)
				{
					swordfish_debug_printf("connectcamera wait for postconnect timeout.....\n");
					swordfish_close(inst);
					// swordfish_releasehandle(inst);
					//	free(inst);
					return NULL;
				}
				else if (ret == -1 && errno != ETIMEDOUT)
				{
					swordfish_debug_printf("connectcamera wait for postconnect,some else err happened:%d\n", errno);
				}
				else if (ret == 0)
				{
					break;
				}
			}
			swordfish_debug_printf("connect ok!!!!\n");

			/*		if (depthcallback != NULL)
						pthread_create(&inst->depththread, 0, depthprocessthread, (void *)inst);
					if (ircallback != NULL)
						pthread_create(&inst->irthread, 0, irprocessthread, (void *)inst);
					if (rgbcallback != NULL)
						pthread_create(&inst->rgbthread, 0, rgbprocessthread, (void *)inst);
					if (imucallback != NULL)
						pthread_create(&inst->imuthread, 0, imuprocessthread, (void *)inst);
					if (savecallback != NULL)
						pthread_create(&inst->savethread, 0, saveprocessthread, (void *)inst);
						*/
#endif
			return inst;
		}
		swordfish_debug_printf("fail to open connect to:%s\n", dev_info->net_camera_ip);
		return NULL;
	}
	else
	{
		// int ret = -1;
		char *msg = strdup(dev_info->usb_camera_name);
		int num = 0;
		int bus;
		char *path;
		char *split_buf[512] = {0};
		//	int i = 0;
		swordfish_debug_printf("will split devicename!%s\n", msg);
		c_split((char *)msg, "-", 512, split_buf, &num);
		if (num < 3)
		{
			swordfish_debug_printf("fail to split devicename!%s\n", dev_info->usb_camera_name);
			free(msg);
			return NULL;
		}
		bus = atoi(split_buf[1]);
		path = strdup(split_buf[2]);
		free(msg);
		SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)calloc(1, sizeof(SWORDFISH_INSTANCE));
		inst->interface_type = SWORDFISH_USB_INTERFACE;
		if (usb_hal_open(inst, bus, path) >= 0)
		{
			struct timeval tv;
			gettimeofday(&tv,NULL);
			//printf("after open:%ld\n",tv.tv_sec*1000000+tv.tv_usec);	
			free(path);
			inst->depthcallback = NULL;
			pthread_mutex_init(&inst->sendmutex, NULL);
			inst->imucallback = NULL;
			inst->leftircallback = NULL;
			inst->rightircallback = NULL;
			inst->othercallback = NULL;
			inst->rgbcallback = NULL;
			inst->savecallback = NULL;
			inst->eventcallback = NULL;
			inst->connectuserdata = NULL;
			
			inst->tminfo=(TIMESTAMPINFO*)calloc(1,sizeof(TIMESTAMPINFO));

			SEM_INIT(&inst->depthsem, 0, 0);
			SEM_INIT(&inst->othersem, 0, 0);
			SEM_INIT(&inst->rgbsem, 0, 0);
			SEM_INIT(&inst->groupsem, 0, 0);
			SEM_INIT(&inst->leftirsem, 0, 0);
			SEM_INIT(&inst->rightirsem, 0, 0);
			SEM_INIT(&inst->imusem, 0, 0);
			SEM_INIT(&inst->goodfeaturesem, 0, 0);
			SEM_INIT(&inst->postconnectsem, 0, 0);
			inst->status = STARTED;//CONNECTED;

			// ret = 1;

			inst->thread_running_flag = 1;

#ifdef _WINDOWS
			unsigned int threadID;

			inst->recvthread = (HANDLE)_beginthreadex(NULL, 0, loop_recv, (LPVOID)inst, 0, &threadID);

			/*	if (depthcallback != NULL)
					inst->depththread = (HANDLE)_beginthreadex(NULL, 0, depthprocessthread, (LPVOID)inst, 0, &threadID);
				if (ircallback != NULL)
					inst->irthread = (HANDLE)_beginthreadex(NULL, 0, irprocessthread, (LPVOID)inst, 0, &threadID);
				if (rgbcallback != NULL)
					inst->rgbthread = (HANDLE)_beginthreadex(NULL, 0, rgbprocessthread, (LPVOID)inst, 0, &threadID);

				if (imucallback != NULL)
					inst->imuthread = (HANDLE)_beginthreadex(NULL, 0, imuprocessthread, (LPVOID)inst, 0, &threadID);

				if (savecallback != NULL)
					inst->savethread = (HANDLE)_beginthreadex(NULL, 0, saveprocessthread, (LPVOID)inst, 0, &threadID);
					*/

#else
			pthread_create(&inst->recvthread, 0, loop_recv, (void *)inst);
			/*	if (depthcallback != NULL)
					pthread_create(&inst->depththread, 0, depthprocessthread, (void *)inst);
				if (ircallback != NULL)
					pthread_create(&inst->irthread, 0, irprocessthread, (void *)inst);
				if (rgbcallback != NULL)
					pthread_create(&inst->rgbthread, 0, rgbprocessthread, (void *)inst);
				if (imucallback != NULL)
					pthread_create(&inst->imuthread, 0, imuprocessthread, (void *)inst);
				if (savecallback != NULL)
					pthread_create(&inst->savethread, 0, saveprocessthread, (void *)inst);
					*/
#endif

			/*	struct timespec ts;
				clock_gettime(CLOCK_REALTIME, &ts);
				ts.tv_sec += 4000 / 1000;
				ts.tv_nsec += (4000 % 1000) * 1000000;*/
		/*	while (1)
			{
				int ret = SEM_TIMEDWAIT(&inst->postconnectsem, 4000); // &ts);
				if (ret == -1 && errno == ETIMEDOUT)
				{
					swordfish_debug_printf("connectcamera wait for postconnect timeout.....\n");
					swordfish_close(inst);
					//	swordfish_releasehandle(inst);
					// free(inst);
					return NULL;
				}
				else if (ret == -1 && errno != ETIMEDOUT)
				{
					swordfish_debug_printf("connectcamera wait for postconnect,some else err happened:%d\n", errno);
				}
				else if (ret == 0)
				{
					break;
				}
			}*/
			//for some reason, camera not start pipeline by default
			/*s_swordfish_pipeline_cmd param;
			param.status = 1;
			SWORDFISH_RESULT ret = swordfish_set(inst, SWORDFISH_PIPELINE, &param, sizeof(param), 2000);
		  	(void)ret;	*/
			swordfish_debug_printf("connect ok!!!!\n");
			return inst;
		}
		else
		{
			free(path);
		}
		free(inst);
		return NULL;
	}
}

int getwidthofres(SWORDFISH_SENSOR_RESOLUTION_TYPE res)
{
	switch (res)
	{
	case SWORDFISH_RESOLUTION_1280_800:
		return 1280;
	case SWORDFISH_RESOLUTION_1280_720:
		return 1280;
	case SWORDFISH_RESOLUTION_640_480:
		return 640;
	case SWORDFISH_RESOLUTION_640_400:
		return 640;
	case SWORDFISH_RESOLUTION_320_200:
		return 320;
	case SWORDFISH_RESOLUTION_640_360:
		return 640;
	default:
		return 0;
	}
	return 0;
}
int getheightofres(SWORDFISH_SENSOR_RESOLUTION_TYPE res)
{
	switch (res)
	{
	case SWORDFISH_RESOLUTION_1280_800:
		return 800;
	case SWORDFISH_RESOLUTION_1280_720:
		return 720;
	case SWORDFISH_RESOLUTION_640_480:
		return 480;
	case SWORDFISH_RESOLUTION_640_400:
		return 400;
	case SWORDFISH_RESOLUTION_320_200:
		return 200;
	case SWORDFISH_RESOLUTION_640_360:
		return 360;
	default:
		return 0;
	}
	return 0;
}
SWORDFISH_RESULT swordfish_config(SWORDFISH_DEVICE_HANDLE handle, SWORDFISH_CONFIG *config)
{
	if (swordfish_hasconnect(handle))
	{
		SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)handle;
		if (inst->status < POSTCONNECTED)
		{
			swordfish_debug_printf("not reach postconnected,config stream failed!\n");
			return SWORDFISH_NOTREACHPOSTCONNECTED;
		}
		s_swordfish_sensor_resolution_fps_infor info;
		info.ir_resolution = config->irres;
		info.rgb_resolution = config->rgbres;
		info.fps = (config->fps == FPS_15 ? 15 : 30);
		//	swordfish_debug_printf("will send one packet to camera\n");
		int ret1 = send_one_packet(handle, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_IR_IMAGE_SENSOR_RESOLUTION_FPS_COMMAND, sizeof(s_swordfish_sensor_resolution_fps_infor), (const uint8_t *)&info, 2000);
		int ret2 = send_one_packet(handle, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_RGB_IMAGE_SENSOR_RESOLUTION_FPS_COMMAND, sizeof(s_swordfish_sensor_resolution_fps_infor), (const uint8_t *)&info, 2000);

		int ret3 = send_one_packet(handle, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_FRAME_RATE_PARAM, sizeof(int), (const uint8_t *)&config->fpsratio, 2000);
		//	swordfish_debug_printf("has send one packet to camera\n");
	/*	s_swordfish_pipeline_cmd value;
		value.status = 3;
		int ret4 = send_one_packet(handle, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_USB_PIPELINE_COMMAND, sizeof(s_swordfish_pipeline_cmd), (const uint8_t *)&value, 2000);
*/
		if (ret1 >= 0 && ret2 >= 0 && ret3 >= 0)// && ret4 >= 0)
		{
			inst->status = CONFIGURED;
			inst->irwidth = getwidthofres(config->irres);
			inst->irheight = getheightofres(config->irres);
			inst->rgbwidth = getwidthofres(config->rgbres);
			inst->rgbheight = getheightofres(config->rgbres);
			while (1)
			{
				int ret = SEM_TIMEDWAIT(&inst->postconnectsem, 4000);
				if (ret == -1 && errno == ETIMEDOUT)
				{
					swordfish_debug_printf("connectcamera wait for postconnect timeout.....\n");
					swordfish_close(inst);
					// swordfish_releasehandle(inst);
					//	free(inst);
					return SWORDFISH_FAILED;
				}
				else if (ret == -1 && errno != ETIMEDOUT)
				{
					swordfish_debug_printf("connectcamera wait for postconnect,some else err happened:%d\n", errno);
				}
				else if (ret == 0)
				{
					break;
				}
			}
	
			return SWORDFISH_OK;
		}
		else
		{
			swordfish_debug_printf("fail to send cmd to camera!\n");
			return SWORDFISH_FAILED;
		}
	}
	swordfish_debug_printf("feynman not connected,fail to config stream!\n");
	return SWORDFISH_NOTCONNECT;
}
SWORDFISH_RESULT swordfish_setcallback(SWORDFISH_DEVICE_HANDLE handle, FRAMECALLBACK callback, void *userdata)
{
	SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)handle;
	inst->resultcallback = callback;
	inst->connectuserdata = userdata;
	return SWORDFISH_OK;	
}

SWORDFISH_RESULT swordfish_start(SWORDFISH_DEVICE_HANDLE handle, SWORDFISH_STREAM_TYPE_E stream_type, FRAMECALLBACK callback, void *userdata)
{
	SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)handle;
	inst->connectuserdata = userdata;
	if (swordfish_hasconnect(handle) && inst->status >= CONFIGURED)
	{
		if (stream_type == SWORDFISH_STREAM_DEPTH)
		{
			swordfish_debug_printf("will start depth stream transfer!!\n");
			inst->depthcallback = callback;
			swordfish_transferdepth(handle, 1);
			if (callback != NULL)
			{
				if (inst->depththread == (THREADHANDLE)NULL)
				{
#ifdef _WINDOWS
					DWORD threadID;
					inst->depththread = (THREADHANDLE)_beginthreadex(NULL, 0, depthprocessthread, (LPVOID)inst, 0, &threadID);
#else
					pthread_create(&inst->depththread, 0, depthprocessthread, (void *)inst);
#endif
				}
			}
		}
		else if (stream_type == SWORDFISH_STREAM_DEPTH_LEFTIR)
		{
			swordfish_debug_printf("will leftir stream transfer!!\n");
			inst->leftircallback = callback;
			swordfish_transferir(handle, 1);
			if (callback != NULL)
			{
				if (inst->leftirthread == (THREADHANDLE)NULL)
				{
#ifdef _WINDOWS
					DWORD threadID;
					inst->leftirthread = (THREADHANDLE)_beginthreadex(NULL, 0, leftirprocessthread, (LPVOID)inst, 0, &threadID);
#else
					pthread_create(&inst->leftirthread, 0, leftirprocessthread, (void *)inst);
#endif
				}
			}
		}
		else if (stream_type == SWORDFISH_STREAM_DEPTH_RIGHTIR)
		{
			swordfish_debug_printf("will start rightir stream transfer!!\n");
			inst->rightircallback = callback;
			swordfish_transferir(handle, 1);
			if (callback != NULL)
			{
				if (inst->rightirthread == (THREADHANDLE)NULL)
				{
#ifdef _WINDOWS
					DWORD threadID;
					inst->rightirthread = (THREADHANDLE)_beginthreadex(NULL, 0, rightirprocessthread, (LPVOID)inst, 0, &threadID);
#else
					pthread_create(&inst->rightirthread, 0, rightirprocessthread, (void *)inst);
#endif
				}
			}
		}
		else if (stream_type == SWORDFISH_STREAM_RGB)
		{
			swordfish_debug_printf("will start rgb stream transfer!!\n");
			inst->rgbcallback = callback;
			swordfish_transferrgb(handle, 1);
			if (callback != NULL)
			{
				if (inst->rgbthread == (THREADHANDLE)NULL)
				{
#ifdef _WINDOWS
					DWORD threadID;
					inst->rgbthread = (THREADHANDLE)_beginthreadex(NULL, 0, rgbprocessthread, (LPVOID)inst, 0, &threadID);
#else
					pthread_create(&inst->rgbthread, 0, rgbprocessthread, (void *)inst);
#endif
				}
			}
		}
		else if (stream_type == SWORDFISH_STREAM_GROUPIMAGES)
		{
			swordfish_debug_printf("will start group images stream transfer!!\n");
			inst->groupcallback = callback;
			swordfish_transferir(handle, 1);
			swordfish_transferrgb(handle, 1);
			swordfish_transferdepth(handle, 1);
			if (callback != NULL)
			{
				if (inst->groupthread == (THREADHANDLE)NULL)
				{
#ifdef _WINDOWS
					DWORD threadID;
					inst->groupthread = (THREADHANDLE)_beginthreadex(NULL, 0, groupprocessthread, (LPVOID)inst, 0, &threadID);
#else
					pthread_create(&inst->groupthread, 0, groupprocessthread, (void *)inst);
#endif
				}
			}
		}
		else if (stream_type == SWORDFISH_STREAM_IMU)
		{
			swordfish_debug_printf("will start imu stream transfer!!\n");
			inst->imucallback = callback;
			swordfish_imuenable(handle, 1);
			if (callback != NULL)
			{
				if (inst->imuthread == (THREADHANDLE)NULL)
				{
#ifdef _WINDOWS
					DWORD threadID;
					inst->imuthread = (THREADHANDLE)_beginthreadex(NULL, 0, imuprocessthread, (LPVOID)inst, 0, &threadID);
#else
					pthread_create(&inst->imuthread, 0, imuprocessthread, (void *)inst);
#endif
				}
			}
		}
		else if (stream_type == SWORDFISH_STREAM_OTHER)
		{
			swordfish_debug_printf("will start other stream transfer!!\n");
			inst->othercallback = callback;
			//	swordfish_imuenable(handle, 1);
			if (callback != NULL)
			{
				if (inst->otherthread == (THREADHANDLE)NULL)
				{
#ifdef _WINDOWS
					DWORD threadID;
					inst->otherthread = (THREADHANDLE)_beginthreadex(NULL, 0, otherprocessthread, (LPVOID)inst, 0, &threadID);
#else
					pthread_create(&inst->otherthread, 0, otherprocessthread, (void *)inst);
#endif
				}
			}
		}
		else if (stream_type == SWORDFISH_STREAM_GOODFEATURE)
		{
			swordfish_debug_printf("will start good feature stream transfer!!\n");
			inst->goodfeaturecallback = callback;
			//	swordfish_imuenable(handle, 1);
			if (callback != NULL)
			{
				if (inst->goodfeaturethread == (THREADHANDLE)NULL)
				{
#ifdef _WINDOWS
					DWORD threadID;
					inst->goodfeaturethread = (THREADHANDLE)_beginthreadex(NULL, 0, goodfeatureprocessthread, (LPVOID)inst, 0, &threadID);
#else
					pthread_create(&inst->goodfeaturethread, 0, goodfeatureprocessthread, (void *)inst);
#endif
				}
			}
		}
		else
		{
			swordfish_debug_printf("unknown stream type!\n");
			return SWORDFISH_FAILED;
		}

		inst->status = STARTED;

		return SWORDFISH_OK;
	}
	return SWORDFISH_FAILED;
}
SWORDFISH_RESULT swordfish_stop(SWORDFISH_DEVICE_HANDLE handle, SWORDFISH_STREAM_TYPE_E stream_type)
{
	SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)handle;
	if (swordfish_hasconnect(handle) && inst->status >= CONFIGURED)
	{
		if (stream_type == SWORDFISH_STREAM_DEPTH)
		{
			swordfish_transferdepth(handle, 0);
			// inst->depthcallback = NULL;
		}
		else if (stream_type == SWORDFISH_STREAM_DEPTH_LEFTIR)
		{
			swordfish_transferir(handle, 0);
			// inst->leftircallback = NULL;
		}
		else if (stream_type == SWORDFISH_STREAM_DEPTH_RIGHTIR)
		{
			swordfish_transferir(handle, 0);
			// inst->rightircallback = NULL;
		}
		else if (stream_type == SWORDFISH_STREAM_RGB)
		{
			swordfish_transferrgb(handle, 0);
			// inst->rgbcallback = NULL;
		}
		else if (stream_type == SWORDFISH_STREAM_IMU)
		{
			swordfish_imuenable(handle, 0);
			// inst->imucallback = NULL;
		}
		else
		{
			return SWORDFISH_FAILED;
		}

		return SWORDFISH_OK;
	}
	return SWORDFISH_FAILED;
}
SWORDFISH_RESULT swordfish_get_frame(SWORDFISH_DEVICE_HANDLE handle, SWORDFISH_STREAM_TYPE_E steamtype, SWORDFISH_USBHeaderDataPacket **ppframe, int timeout)
{
	SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)handle;
	if (steamtype == SWORDFISH_STREAM_DEPTH)
	{
		if (NULL == inst->depthpktqueue)
		{
#ifdef _WINDOWS
			Sleep(500);
#else
			usleep(500 * 1000);
#endif
			return SWORDFISH_NOTREADY;
		}
		/*struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += timeout / 1000;
		ts.tv_nsec += (timeout % 1000) * 1000000;*/
		int ret = SEM_TIMEDWAIT(&inst->depthsem, timeout); // &ts);
		if (ret == -1 && errno == ETIMEDOUT)
		{
			swordfish_debug_printf("swordfish_get_frame_sync timeout.....,for depth\n");
			if (inst->thread_running_flag)
				return SWORDFISH_TIMEOUT;
			else
				return SWORDFISH_NETFAILED;
		}
		else if (ret == -1 && errno != ETIMEDOUT)
		{
			swordfish_debug_printf("some else err happened:%d\n", errno);
		}

		if (inst->thread_running_flag)
		{
			pkt_info_custom_t *p = 0;
			p = (pkt_info_custom_t *)SOLO_Read(inst->depthpktqueue);
			if (p)
			{
				static SWORDFISH_USBHeaderDataPacket *tmpframe = NULL;
				if (NULL == tmpframe)
					tmpframe = (SWORDFISH_USBHeaderDataPacket *)calloc(1, 1280 * 800 * 2 + 1024);
				memcpy(tmpframe, &p->buffer[0], p->len);
				SOLO_Read_Over(inst->depthpktqueue);
				*ppframe = tmpframe;
				return SWORDFISH_OK;
			}
		}
		else
		{
			return SWORDFISH_NOTCONNECT;
		}
		return SWORDFISH_UNKNOWN;
	}
	else if (steamtype == SWORDFISH_STREAM_GROUPIMAGES)
	{
		if (NULL == inst->grouppktqueue)
		{
#ifdef _WINDOWS
			Sleep(500);
#else
			usleep(500 * 1000);
#endif
			return SWORDFISH_NOTREADY;
		}
		/*struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += timeout / 1000;
		ts.tv_nsec += (timeout % 1000) * 1000000;*/
		int ret = SEM_TIMEDWAIT(&inst->groupsem, timeout); // &ts);
		if (ret == -1 && errno == ETIMEDOUT)
		{
			swordfish_debug_printf("swordfish_get_frame_sync timeout.....for groupimages\n");
			if (inst->thread_running_flag)
				return SWORDFISH_TIMEOUT;
			else
				return SWORDFISH_NETFAILED;
		}
		else if (ret == -1 && errno != ETIMEDOUT)
		{
			swordfish_debug_printf("some else err happened:%d\n", errno);
		}

		if (inst->thread_running_flag)
		{
			pkt_info_custom_t *p = 0;
			p = (pkt_info_custom_t *)SOLO_Read(inst->grouppktqueue);
			if (p)
			{
				static SWORDFISH_USBHeaderDataPacket *tmpframe = NULL;
				if (NULL == tmpframe)
					tmpframe = (SWORDFISH_USBHeaderDataPacket *)calloc(1, 4 * (1280 * 800 * 2 + 1024));
				memcpy(tmpframe, &p->buffer[0], p->len);
				SOLO_Read_Over(inst->grouppktqueue);
				*ppframe = tmpframe;
				return SWORDFISH_OK;
			}
		}
		else
		{
			return SWORDFISH_NOTCONNECT;
		}
		return SWORDFISH_UNKNOWN;
	}
	else if (steamtype == SWORDFISH_STREAM_RGB)
	{
		if (NULL == inst->rgbpktqueue)
		{
#ifdef _WINDOWS
			Sleep(500);
#else
			usleep(500 * 1000);
#endif
			return SWORDFISH_NOTREADY;
		}
		/*struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += timeout / 1000;
		ts.tv_nsec += (timeout % 1000) * 1000000;*/
		int ret = SEM_TIMEDWAIT(&inst->rgbsem, timeout); // &ts);
		if (ret == -1 && errno == ETIMEDOUT)
		{
			swordfish_debug_printf("swordfish_get_frame_sync timeout.....for rgb\n");
			if (inst->thread_running_flag)
				return SWORDFISH_TIMEOUT;
			else
				return SWORDFISH_NETFAILED;
		}
		else if (ret == -1 && errno != ETIMEDOUT)
		{
			swordfish_debug_printf("some else err happened:%d\n", errno);
		}

		if (inst->thread_running_flag)
		{
			pkt_info_custom_t *p = 0;
			p = (pkt_info_custom_t *)SOLO_Read(inst->rgbpktqueue);
			if (p)
			{
				static SWORDFISH_USBHeaderDataPacket *tmpframe = NULL;
				if (NULL == tmpframe)
					tmpframe = (SWORDFISH_USBHeaderDataPacket *)calloc(1, 1280 * 800 * 2 + 1024);
				memcpy(tmpframe, &p->buffer[0], p->len);
				SOLO_Read_Over(inst->rgbpktqueue);
				*ppframe = tmpframe;
				return SWORDFISH_OK;
			}
		}
		else
		{
			return SWORDFISH_NOTCONNECT;
		}
		return SWORDFISH_UNKNOWN;
	}
	else if (steamtype == SWORDFISH_STREAM_DEPTH_LEFTIR)
	{
		if (NULL == inst->leftirpktqueue)
		{
#ifdef _WINDOWS
			Sleep(500);
#else
			usleep(500 * 1000);
#endif
			return SWORDFISH_NOTREADY;
		}
		/*struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += timeout / 1000;
		ts.tv_nsec += (timeout % 1000) * 1000000;*/
		int ret = SEM_TIMEDWAIT(&inst->leftirsem, timeout); // &ts);
		if (ret == -1 && errno == ETIMEDOUT)
		{
			swordfish_debug_printf("swordfish_get_frame_sync timeout.....for leftir\n");
			if (inst->thread_running_flag)
				return SWORDFISH_TIMEOUT;
			else
				return SWORDFISH_NETFAILED;
		}
		else if (ret == -1 && errno != ETIMEDOUT)
		{
			swordfish_debug_printf("some else err happened:%d\n", errno);
		}

		if (inst->thread_running_flag)
		{
			pkt_info_custom_t *p = 0;
			p = (pkt_info_custom_t *)SOLO_Read(inst->leftirpktqueue);
			if (p)
			{
				static SWORDFISH_USBHeaderDataPacket *tmpframe = NULL;
				if (NULL == tmpframe)
					tmpframe = (SWORDFISH_USBHeaderDataPacket *)calloc(1, 1280 * 800 * 2 + 1024);
				memcpy(tmpframe, &p->buffer[0], p->len);
				SOLO_Read_Over(inst->leftirpktqueue);
				*ppframe = tmpframe;
				return SWORDFISH_OK;
			}
		}
		else
		{
			return SWORDFISH_NOTCONNECT;
		}
		return SWORDFISH_UNKNOWN;
	}
	else if (steamtype == SWORDFISH_STREAM_DEPTH_RIGHTIR)
	{
		if (NULL == inst->rightirpktqueue)
		{
#ifdef _WINDOWS
			Sleep(500);
#else
			usleep(500 * 1000);
#endif
			return SWORDFISH_NOTREADY;
		}
		/*	struct timespec ts;
			clock_gettime(CLOCK_REALTIME, &ts);
			ts.tv_sec += timeout / 1000;
			ts.tv_nsec += (timeout % 1000) * 1000000;*/
		int ret = SEM_TIMEDWAIT(&inst->rightirsem, timeout); // &ts);
		if (ret == -1 && errno == ETIMEDOUT)
		{
			swordfish_debug_printf("swordfish_get_frame_sync timeout.....for rightir\n");
			if (inst->thread_running_flag)
				return SWORDFISH_TIMEOUT;
			else
				return SWORDFISH_NETFAILED;
		}
		else if (ret == -1 && errno != ETIMEDOUT)
		{
			swordfish_debug_printf("some else err happened:%d\n", errno);
		}

		if (inst->thread_running_flag)
		{
			pkt_info_custom_t *p = 0;
			p = (pkt_info_custom_t *)SOLO_Read(inst->rightirpktqueue);
			if (p)
			{
				static SWORDFISH_USBHeaderDataPacket *tmpframe = NULL;
				if (NULL == tmpframe)
					tmpframe = (SWORDFISH_USBHeaderDataPacket *)calloc(1, 1280 * 800 * 2 + 1024);
				memcpy(tmpframe, &p->buffer[0], p->len);
				SOLO_Read_Over(inst->rightirpktqueue);
				*ppframe = tmpframe;
				return SWORDFISH_OK;
			}
		}
		else
		{
			return SWORDFISH_NOTCONNECT;
		}
		return SWORDFISH_UNKNOWN;
	}
	else if (steamtype == SWORDFISH_STREAM_IMU)
	{
		if (NULL == inst->imupktqueue)
		{
#ifdef _WINDOWS
			Sleep(500);
#else
			usleep(500 * 1000);
#endif
			return SWORDFISH_NOTREADY;
		}
		/*struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += timeout / 1000;
		ts.tv_nsec += (timeout % 1000) * 1000000;*/
		int ret = SEM_TIMEDWAIT(&inst->imusem, timeout); // &ts);
		if (ret == -1 && errno == ETIMEDOUT)
		{
			swordfish_debug_printf("swordfish_get_frame_sync timeout.....for imu\n");
			if (inst->thread_running_flag)
				return SWORDFISH_TIMEOUT;
			else
				return SWORDFISH_NETFAILED;
		}
		else if (ret == -1 && errno != ETIMEDOUT)
		{
			swordfish_debug_printf("some else err happened:%d\n", errno);
		}

		if (inst->thread_running_flag)
		{
			pkt_info_custom_t *p = 0;
			p = (pkt_info_custom_t *)SOLO_Read(inst->imupktqueue);
			if (p)
			{
				static SWORDFISH_USBHeaderDataPacket *tmpframe = NULL;
				if (NULL == tmpframe)
					tmpframe = (SWORDFISH_USBHeaderDataPacket *)calloc(1, 1280 * 800 * 2 + 1024);
				memcpy(tmpframe, &p->buffer[0], p->len);
				SOLO_Read_Over(inst->imupktqueue);
				*ppframe = tmpframe;
				return SWORDFISH_OK;
			}
		}
		else
		{
			return SWORDFISH_NOTCONNECT;
		}
		return SWORDFISH_UNKNOWN;
	}
	else if (steamtype == SWORDFISH_STREAM_GOODFEATURE)
	{
		if (NULL == inst->goodfeaturepktqueue)
		{
#ifdef _WINDOWS
			Sleep(500);
#else
			usleep(500 * 1000);
#endif
			return SWORDFISH_NOTREADY;
		}
		/*struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += timeout / 1000;
		ts.tv_nsec += (timeout % 1000) * 1000000;*/
		int ret = SEM_TIMEDWAIT(&inst->goodfeaturesem, timeout); // &ts);
		if (ret == -1 && errno == ETIMEDOUT)
		{
			swordfish_debug_printf("swordfish_get_frame_sync timeout.....for imu\n");
			if (inst->thread_running_flag)
				return SWORDFISH_TIMEOUT;
			else
				return SWORDFISH_NETFAILED;
		}
		else if (ret == -1 && errno != ETIMEDOUT)
		{
			swordfish_debug_printf("some else err happened:%d\n", errno);
		}

		if (inst->thread_running_flag)
		{
			pkt_info_custom_t *p = 0;
			p = (pkt_info_custom_t *)SOLO_Read(inst->goodfeaturepktqueue);
			if (p)
			{
				static SWORDFISH_USBHeaderDataPacket *tmpframe = NULL;
				if (NULL == tmpframe)
					tmpframe = (SWORDFISH_USBHeaderDataPacket *)calloc(1, 1280 * 800 * 2 + 1024);
				memcpy(tmpframe, &p->buffer[0], p->len);
				SOLO_Read_Over(inst->goodfeaturepktqueue);
				*ppframe = tmpframe;
				return SWORDFISH_OK;
			}
		}
		else
		{
			return SWORDFISH_NOTCONNECT;
		}
		return SWORDFISH_UNKNOWN;
	}
	else if (steamtype == SWORDFISH_STREAM_OTHER)
	{
		if (NULL == inst->otherpktqueue)
		{
#ifdef _WINDOWS
			Sleep(500);
#else
			usleep(500 * 1000);
#endif
			return SWORDFISH_NOTREADY;
		}
		/*struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += timeout / 1000;
		ts.tv_nsec += (timeout % 1000) * 1000000;*/
		int ret = SEM_TIMEDWAIT(&inst->othersem, timeout); // &ts);
		if (ret == -1 && errno == ETIMEDOUT)
		{
			swordfish_debug_printf("swordfish_get_frame_sync timeout.....for other\n");
			if (inst->thread_running_flag)
				return SWORDFISH_TIMEOUT;
			else
				return SWORDFISH_NETFAILED;
		}
		else if (ret == -1 && errno != ETIMEDOUT)
		{
			swordfish_debug_printf("some else err happened:%d\n", errno);
		}

		if (inst->thread_running_flag)
		{
			pkt_info_custom_t *p = 0;
			p = (pkt_info_custom_t *)SOLO_Read(inst->otherpktqueue);
			if (p)
			{
				static SWORDFISH_USBHeaderDataPacket *tmpframe = NULL;
				if (NULL == tmpframe)
					tmpframe = (SWORDFISH_USBHeaderDataPacket *)calloc(1, 1280 * 800 * 2 + 1024);
				memcpy(tmpframe, &p->buffer[0], p->len);
				SOLO_Read_Over(inst->otherpktqueue);
				*ppframe = tmpframe;
				return SWORDFISH_OK;
			}
		}
		else
		{
			return SWORDFISH_NOTCONNECT;
		}
		return SWORDFISH_UNKNOWN;
	}

	else
	{
		return SWORDFISH_NOTSUPPORTED;
	}
}

SWORDFISH_RESULT swordfish_set(SWORDFISH_DEVICE_HANDLE handle, SWORDFISH_VALUE_TYPE valuetype, void *value, int datalen, int timeout)
{
	SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)handle;
	if (NULL == inst->responsequeue)
	{
		return SWORDFISH_NOTREADY;
	}
	if (swordfish_hasconnect(handle))
	{
		if (valuetype == SWORDFISH_PROJECTOR_STATUS)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_PROJECTOR_CURRENT_COMMAND, datalen, (const uint8_t *)value, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				int *tmpparam = (int *)calloc(1, sizeof(int));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_SET_PROJECTOR_CURRENT_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(30);
#else
						usleep(30 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("swordfish_set timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv result projector param:%d\n", *tmpparam);
							if (*tmpparam == *((int *)value))
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							else
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_FAILED;
							}
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_RGB_ELSE_PARAM)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_RGB_ELSE_PARAM, datalen, (const uint8_t *)value, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_rgb_else_param *tmpparam = (s_swordfish_rgb_else_param *)calloc(1, sizeof(s_swordfish_rgb_else_param));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_SET_RGB_ELSE_PARAM_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("swordfish_set timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							//	swordfish_debug_printf("recv result imuconconfig param:%d\n", tmpparam);
							//	*(s_swordfish_imu_info *)value = *tmpparam;
							//	if (tmpparam->trigger_source == ((s_swordfish_trigger_source *)value)->trigger_source)
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							/*	else
								{
									SEM_DESTROY(tmpsem);
									free(tmpsem);
									free(tmpparam);
free(responselen);
									free(hastimeout);
									return SWORDFISH_FAILED;
								}*/
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}

		else if (valuetype == SWORDFISH_HIGH_PRECISION)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_HIGH_PRECISION_PARAM, datalen, (const uint8_t *)value, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_high_precision *tmpparam = (s_swordfish_high_precision *)calloc(1, sizeof(s_swordfish_high_precision));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_SET_HIGH_PRECISION_PARAM_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("swordfish_set timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							//	swordfish_debug_printf("recv result imuconconfig param:%d\n", tmpparam);
							//	*(s_swordfish_imu_info *)value = *tmpparam;
							//	if (tmpparam->trigger_source == ((s_swordfish_trigger_source *)value)->trigger_source)
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							/*	else
								{
									SEM_DESTROY(tmpsem);
									free(tmpsem);
									free(tmpparam);
free(responselen);
									free(hastimeout);
									return SWORDFISH_FAILED;
								}*/
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_GOODFEATURE_PARAM)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_GOOD_FEATURE_PARAM, datalen, (const uint8_t *)value, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_good_feature *tmpparam = (s_swordfish_good_feature *)calloc(1, sizeof(s_swordfish_good_feature));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_SET_GOOD_FEATURE_PARAM_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("swordfish_set timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							//	swordfish_debug_printf("recv result imuconconfig param:%d\n", tmpparam);
							//	*(s_swordfish_imu_info *)value = *tmpparam;
							//	if (tmpparam->trigger_source == ((s_swordfish_trigger_source *)value)->trigger_source)
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							/*	else
								{
									SEM_DESTROY(tmpsem);
									free(tmpsem);
									free(tmpparam);
free(responselen);
									free(hastimeout);
									return SWORDFISH_FAILED;
								}*/
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_LK_PARAM)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_LK_PARAM, datalen, (const uint8_t *)value, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_lk *tmpparam = (s_swordfish_lk *)calloc(1, sizeof(s_swordfish_lk));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_SET_LK_PARAM_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("swordfish_set timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							//	swordfish_debug_printf("recv result imuconconfig param:%d\n", tmpparam);
							//	*(s_swordfish_imu_info *)value = *tmpparam;
							//	if (tmpparam->trigger_source == ((s_swordfish_trigger_source *)value)->trigger_source)
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							/*	else
								{
									SEM_DESTROY(tmpsem);
									free(tmpsem);
									free(tmpparam);
free(responselen);
									free(hastimeout);
									return SWORDFISH_FAILED;
								}*/
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_IMU_CONFIG)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_IMU_CONFIGURATION_COMMAND, datalen, (const uint8_t *)value, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_imu_info *tmpparam = (s_swordfish_imu_info *)calloc(1, sizeof(s_swordfish_imu_info));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_SET_IMU_CONFIGURATION_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("swordfish_set timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv result imuconconfig param:%d\n", tmpparam);
							*(s_swordfish_imu_info *)value = *tmpparam;
							//	if (tmpparam->trigger_source == ((s_swordfish_trigger_source *)value)->trigger_source)
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							/*	else
								{
									SEM_DESTROY(tmpsem);
									free(tmpsem);
									free(tmpparam);
free(responselen);
									free(hastimeout);
									return SWORDFISH_FAILED;
								}*/
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_TRIGGER_SRC)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_TRIGGER_SOURCE_COMMNAD, datalen, (const uint8_t *)value, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_trigger_source *tmpparam = (s_swordfish_trigger_source *)calloc(1, sizeof(s_swordfish_trigger_source));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_SET_TRIGGER_SOURCE_COMMNAD_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("swordfish_set timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
						//	swordfish_debug_printf("recv result trigger src param:%d\n", (int)tmpparam);
							if (tmpparam->trigger_source == ((s_swordfish_trigger_source *)value)->trigger_source)
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							else
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_FAILED;
							}
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_TRIGGER_SOFT)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_SOFTWARE_TRIGGER_COMMNAD, datalen, (const uint8_t *)value, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_software_trigger *tmpparam = (s_swordfish_software_trigger *)calloc(1, sizeof(s_swordfish_software_trigger));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_SET_SOFTWARE_TRIGGER_COMMNAD_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("swordfish_set timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							//swordfish_debug_printf("recv result trigger src param:%d\n", (int)(tmpparam));
							if (tmpparam->trigger_delay == ((s_swordfish_software_trigger *)value)->trigger_delay &&
								tmpparam->trigger_image_number == ((s_swordfish_software_trigger *)value)->trigger_image_number)
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							else
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_FAILED;
							}
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_SAVE_CONFIG)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SAVE_CONFIGURATION_COMMAND, datalen, (const uint8_t *)value, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				int *tmpparam = (int *)calloc(1, sizeof(int));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_SAVE_CONFIGURATION_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("save config timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv save config:%d\n", *tmpparam);
							//	if (*tmpparam == *((int *)value))
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							/*	else
								{
									SEM_DESTROY(tmpsem);
									free(tmpsem);
									free(tmpparam);
free(responselen);
									free(hastimeout);
									return SWORDFISH_FAILED;
								}*/
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_PIPELINE)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_USB_PIPELINE_COMMAND, datalen, (const uint8_t *)value, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_pipeline_cmd *tmpparam = (s_swordfish_pipeline_cmd *)calloc(1, sizeof(s_swordfish_pipeline_cmd));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_USB_PIPELINE_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("set pipeline timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv pipeline:%d\n", tmpparam->status);
							//	if (*tmpparam == *((int *)value))
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							/*	else
								{
									SEM_DESTROY(tmpsem);
									free(tmpsem);
									free(tmpparam);
free(responselen);
									free(hastimeout);
									return SWORDFISH_FAILED;
								}*/
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_CAMERA_IP)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_IP_ADDRESS_PARAM, datalen, (const uint8_t *)value, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				char *tmpparam = (char *)calloc(1, 24);
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_SET_IP_ADDRESS_PARAM_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("set ip timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv ip:%s\n", (char *)tmpparam);
							//	if (*tmpparam == *((int *)value))
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							/*	else
								{
									SEM_DESTROY(tmpsem);
									free(tmpsem);
									free(tmpparam);
free(responselen);
									free(hastimeout);
									return SWORDFISH_FAILED;
								}*/
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_SWITCH_DEPTH_DISPARITY)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_DEPTH_DATA_SOURCE_COMMAND, datalen, (const uint8_t *)value, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_depth_img_source *tmpparam = (s_swordfish_depth_img_source *)calloc(1, sizeof(s_swordfish_depth_img_source));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_SET_DEPTH_DATA_SOURCE_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("set switch depth disparity timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv result switch depth disparity:%d\n", tmpparam->source);
							//	if (*tmpparam == *((int *)value))
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							/*	else
								{
									SEM_DESTROY(tmpsem);
									free(tmpsem);
									free(tmpparam);
free(responselen);
									free(hastimeout);
									return SWORDFISH_FAILED;
								}*/
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_SWITCH_VI_VPSS)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_IR_DATA_SOURCE_COMMAND, datalen, (const uint8_t *)value, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_ir_img_source *tmpparam = (s_swordfish_ir_img_source *)calloc(1, sizeof(s_swordfish_ir_img_source));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_SET_IR_DATA_SOURCE_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("set switch vi vpss timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv result witch vi vpss:%d\n", tmpparam->source);
							//	if (*tmpparam == *((int *)value))
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							/*	else
								{
									SEM_DESTROY(tmpsem);
									free(tmpsem);
									free(tmpparam);
free(responselen);
									free(hastimeout);
									return SWORDFISH_FAILED;
								}*/
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_REPEAT_TEXTURE_FILTER)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_REPEATED_TEXTURE_PARAM, datalen, (const uint8_t *)value, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_repeated_texture_filter *tmpparam = (s_swordfish_repeated_texture_filter *)calloc(1, sizeof(s_swordfish_repeated_texture_filter));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_SET_REPEATED_TEXTURE_PARAM_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("set repeat texture filter timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv result repeat texture filter:%d\n", tmpparam->enable);
							//	if (*tmpparam == *((int *)value))
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							/*	else
								{
									SEM_DESTROY(tmpsem);
									free(tmpsem);
									free(tmpparam);
free(responselen);
									free(hastimeout);
									return SWORDFISH_FAILED;
								}*/
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}

		else if (valuetype == SWORDFISH_EDGE_FILTER)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_EDGE_FILTER_PARAM, datalen, (const uint8_t *)value, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_edge_filter_cmd *tmpparam = (s_swordfish_edge_filter_cmd *)calloc(1, sizeof(s_swordfish_edge_filter_cmd));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_SET_EDGE_FILTER_PARAM_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("set edge filter timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv result edge filter:%u\n", tmpparam->enable);
							//	if (*tmpparam == *((int *)value))
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							/*	else
								{
									SEM_DESTROY(tmpsem);
									free(tmpsem);
									free(tmpparam);
free(responselen);
									free(hastimeout);
									return SWORDFISH_FAILED;
								}*/
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_BAD_FILTER)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_BAD_FILTER_PARAM, datalen, (const uint8_t *)value, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_bad_filter *tmpparam = (s_swordfish_bad_filter *)calloc(1, sizeof(s_swordfish_bad_filter));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_SET_BAD_FILTER_PARAM_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("set bad filter timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv result bad filter:%u\n", tmpparam->enable);
							//	if (*tmpparam == *((int *)value))
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							/*	else
								{
									SEM_DESTROY(tmpsem);
									free(tmpsem);
									free(tmpparam);
free(responselen);
									free(hastimeout);
									return SWORDFISH_FAILED;
								}*/
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_LIGHT_FILTER)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_LIGHT_FILTER_PARAM, datalen, (const uint8_t *)value, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_light_filter *tmpparam = (s_swordfish_light_filter *)calloc(1, sizeof(s_swordfish_light_filter));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_SET_LIGHT_FILTER_PARAM_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("set light filter timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv result light filter:%u,%f\n", tmpparam->enable, tmpparam->sigma);
							//	if (*tmpparam == *((int *)value))
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							/*	else
								{
									SEM_DESTROY(tmpsem);
									free(tmpsem);
									free(tmpparam);
free(responselen);
									free(hastimeout);
									return SWORDFISH_FAILED;
								}*/
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_CONFIDENCE)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_CONFIDENCE_PARAM, datalen, (const uint8_t *)value, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_confidence *tmpparam = (s_swordfish_confidence *)calloc(1, sizeof(s_swordfish_confidence));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_SET_CONFIDENCE_PARAM_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("set confidence timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv result confidence:%u,%f\n", tmpparam->enable, tmpparam->sigma);
							//	if (*tmpparam == *((int *)value))
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							/*	else
								{
									SEM_DESTROY(tmpsem);
									free(tmpsem);
									free(tmpparam);
free(responselen);
									free(hastimeout);
									return SWORDFISH_FAILED;
								}*/
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_DEPTH_CONFIG)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_DEPTH_CONFIGURATION_COMMAND, datalen, (const uint8_t *)value, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_depth_config *tmpparam = (s_swordfish_depth_config *)calloc(1, sizeof(s_swordfish_depth_config));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_SET_DEPTH_CONFIGURATION_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("set depth mode timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv result depth mode:%d\n", tmpparam->app_depth_mode);
							//	if (*tmpparam == *((int *)value))
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							/*	else
								{
									SEM_DESTROY(tmpsem);
									free(tmpsem);
									free(tmpparam);
free(responselen);
									free(hastimeout);
									return SWORDFISH_FAILED;
								}*/
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_RUN_MODE)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_RUN_CONFIGURATION_COMMAND, datalen, (const uint8_t *)value, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_run_config *tmpparam = (s_swordfish_run_config *)calloc(1, sizeof(s_swordfish_run_config));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_SET_RUN_CONFIGURATION_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("set run mode timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv result run mode:%d\n", tmpparam->app_run_mode);
							//	if (*tmpparam == *((int *)value))
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							/*	else
								{
									SEM_DESTROY(tmpsem);
									free(tmpsem);
									free(tmpparam);
free(responselen);
									free(hastimeout);
									return SWORDFISH_FAILED;
								}*/
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_IR_EXPOSURE)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_IR_IMAGE_SENSOR_EXPOSURE_COMMAND, datalen, (const uint8_t *)value, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_ir_sensor_exposure_infor *tmpparam = (s_swordfish_ir_sensor_exposure_infor *)calloc(1, sizeof(s_swordfish_ir_sensor_exposure_infor));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_SET_IR_IMAGE_SENSOR_EXPOSURE_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("set ir exposure timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv result ir exposure mode:%d\n", tmpparam->exposure_mode);
							//	if (*tmpparam == *((int *)value))
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							/*	else
								{
									SEM_DESTROY(tmpsem);
									free(tmpsem);
									free(tmpparam);
free(responselen);
									free(hastimeout);
									return SWORDFISH_FAILED;
								}*/
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_IR_EXPOSURE)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_IR_IMAGE_SENSOR_EXPOSURE_COMMAND, datalen, (const uint8_t *)value, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_ir_sensor_exposure_infor *tmpparam = (s_swordfish_ir_sensor_exposure_infor *)calloc(1, sizeof(s_swordfish_ir_sensor_exposure_infor));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_SET_IR_IMAGE_SENSOR_EXPOSURE_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("set ir exposure timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv result ir exposure mode:%d\n", tmpparam->exposure_mode);
							//	if (*tmpparam == *((int *)value))
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							/*	else
								{
									SEM_DESTROY(tmpsem);
									free(tmpsem);
									free(tmpparam);
free(responselen);
									free(hastimeout);
									return SWORDFISH_FAILED;
								}*/
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_RGB_EXPOSURE)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_RGB_IMAGE_SENSOR_EXPOSURE_COMMAND, datalen, (const uint8_t *)value, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_rgb_sensor_exposure_infor *tmpparam = (s_swordfish_rgb_sensor_exposure_infor *)calloc(1, sizeof(s_swordfish_rgb_sensor_exposure_infor));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_SET_RGB_IMAGE_SENSOR_EXPOSURE_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("set rgb exposure timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv result rgb exposure mode:%d\n", tmpparam->exposure_mode);
							//	if (*tmpparam == *((int *)value))
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							/*	else
								{
									SEM_DESTROY(tmpsem);
									free(tmpsem);
									free(tmpparam);
free(responselen);
									free(hastimeout);
									return SWORDFISH_FAILED;
								}*/
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_TIME_SYNC)
		{
			s_swordfish_time_ret *tmpvalue = (s_swordfish_time_ret *)value;
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_SYS_TIME_COMMAND, datalen, (const uint8_t *)&tmpvalue->pc_time, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_time_ret *tmpparam = (s_swordfish_time_ret *)calloc(1, sizeof(s_swordfish_time_ret));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_SET_SYS_TIME_COMMAND_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("swordfish_set timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							// swordfish_debug_printf("recv result time sync,camera local time calibbed:%llu\n", tmpparam->localtime_calib);
							*((s_swordfish_time_ret *)value) = *tmpparam;
							//	if (*tmpparam == *((NVP_U64 *)value))
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							/*else
							{
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
free(responselen);
								free(hastimeout);
								return SWORDFISH_FAILED;
							}*/
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else
		{
			return SWORDFISH_NOTSUPPORTED;
		}
	}
	else
	{
		swordfish_debug_printf("not connected,just return SWORDFISH_NOTCONNECT\n");
		return SWORDFISH_NOTCONNECT;
	}
}
SWORDFISH_RESULT swordfish_send_update_tftp(SWORDFISH_DEVICE_HANDLE handle)
{
	if (!swordfish_hasconnect(handle))
	{
		return SWORDFISH_NOTCONNECT;
	}

	if (0 <= send_one_packet(handle, SWORDFISH_UPDATE_TFTP, 0, 0, (const uint8_t *)NULL, 2000))
	{
		return SWORDFISH_OK;
	}
	return SWORDFISH_FAILED;
}
SWORDFISH_RESULT swordfish_get_cam_param_by_resolution(SWORDFISH_DEVICE_HANDLE handle, int width, int height, s_swordfish_cam_param *tmpparam)
{
	if (!swordfish_hasconnect(handle))
	{
		return SWORDFISH_NOTCONNECT;
	}
	SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)handle;
	// 1.download txt file
	char parampath[256];
	if (inst->rgb_rectify_mode == 1)
	{
		sprintf(parampath, "/nextvpu/example/media/feynman/wide-angle/param_rgb_keep_black/%dx%d/cam_param.txt", width, height);
	}
	else if (inst->rgb_rectify_mode == 2)
	{
		sprintf(parampath, "/nextvpu/example/media/feynman/wide-angle/param_rgb_undistort/%dx%d/cam_param.txt", width, height);
	}
	else
	{
		sprintf(parampath, "/nextvpu/example/media/feynman/wide-angle/param/%dx%d/cam_param.txt", width, height);
	}
	s_swordfish_download_bidata *pdata1 = (s_swordfish_download_bidata *)malloc(sizeof(s_swordfish_download_bidata) + MAXDOWNLOADFILESIZE);
	strcpy(pdata1->downloaddata.file_path, parampath);

	if (SWORDFISH_OK == swordfish_get(handle, SWORDFISH_DOWNLOAD_FILE, pdata1, 2000)) // timeout 2000milliseconds
	{
		// parse cam_param.txt to get s_swordfish_cam_param reference
		char *buffer = (char *)calloc(1, pdata1->len + 1);
		memcpy(buffer, pdata1->data, pdata1->len);

		char *p = NULL;
		p = strtok(buffer, "\r\n");
		int index = 0;
		while (p != NULL)
		{
			char *tmpp = strdup(p);
			switch (index)
			{
			case 0: // widht height ??
			{
				if (2 == sscanf(tmpp, "%d %d %*d", &tmpparam->img_width, &tmpparam->img_height))
				{
					swordfish_debug_printf("width:%d,height:%d,parsed!\n", tmpparam->img_width, tmpparam->img_height);
				}
				else
				{
					swordfish_debug_printf("fail to parse width height ??:%s\n", tmpp);
					free(tmpp);
					free(buffer);
					free(pdata1);
					return SWORDFISH_FAILED;
				}
			}
			break;
			case 1:
			{
				if (1 == sscanf(tmpp, "%s", tmpparam->camera_model))
				{
					swordfish_debug_printf("camera_model:%s\n", tmpparam->camera_model);
				}
				else
				{
					swordfish_debug_printf("fail to parse camera_model:%s\n", tmpp);
					free(tmpp);
					free(buffer);
					free(pdata1);
					return SWORDFISH_FAILED;
				}
			}
			break;
			case 2:
			{
				if (1 == sscanf(tmpp, "%d", &tmpparam->camera_number))
				{
					swordfish_debug_printf("camera_number:%d\n", tmpparam->camera_number);
				}
				else
				{
					swordfish_debug_printf("fail to parse camera_number:%s\n", tmpp);
					free(tmpp);
					free(buffer);
					free(pdata1);
					return SWORDFISH_FAILED;
				}
			}
			break;
			case 3:
			{
				if (4 == sscanf(tmpp, "%f %f %f %f", &tmpparam->left_sensor_focus[0], &tmpparam->left_sensor_focus[1], &tmpparam->left_sensor_photocenter[0], &tmpparam->left_sensor_photocenter[1]))
				{
					swordfish_debug_printf("left sensor:fx:%f fy:%f pcx:%f pcy:%f\n",
						   tmpparam->left_sensor_focus[0], tmpparam->left_sensor_focus[1],
						   tmpparam->left_sensor_photocenter[0], tmpparam->left_sensor_photocenter[1]);
				}
				else
				{
					swordfish_debug_printf("fail to parse left sensor f and pc:%s\n", tmpp);
					free(tmpp);
					free(buffer);
					free(pdata1);
					return SWORDFISH_FAILED;
				}
			}
			break;
			case 4:
			{
				if (4 == sscanf(tmpp, "%f %f %f %f", &tmpparam->right_sensor_focus[0], &tmpparam->right_sensor_focus[1], &tmpparam->right_sensor_photocenter[0], &tmpparam->right_sensor_photocenter[1]))
				{
					swordfish_debug_printf("right sensor:fx:%f fy:%f pcx:%f pcy:%f\n",
						   tmpparam->right_sensor_focus[0], tmpparam->right_sensor_focus[1],
						   tmpparam->right_sensor_photocenter[0], tmpparam->right_sensor_photocenter[1]);
				}
				else
				{
					swordfish_debug_printf("fail to parse right sensor f and pc:%s\n", tmpp);
					free(tmpp);
					free(buffer);
					free(pdata1);
					return SWORDFISH_FAILED;
				}
			}
			break;
			case 5:
			{
				if(tmpparam->camera_number==3){	
					if (4 == sscanf(tmpp, "%f %f %f %f", &tmpparam->rgb_sensor_focus[0], &tmpparam->rgb_sensor_focus[1], &tmpparam->rgb_sensor_photocenter[0], &tmpparam->rgb_sensor_photocenter[1]))
					{
						swordfish_debug_printf("rgb sensor:fx:%f fy:%f pcx:%f pcy:%f\n",
							   tmpparam->rgb_sensor_focus[0], tmpparam->rgb_sensor_focus[1],
							   tmpparam->rgb_sensor_photocenter[0], tmpparam->rgb_sensor_photocenter[1]);
					}
					else
					{
						swordfish_debug_printf("fail to parse rgb sensor f and pc:%s\n", tmpp);
						free(tmpp);
						free(buffer);
						free(pdata1);
						return SWORDFISH_FAILED;
					}
				}else{
					if (12 == sscanf(tmpp, "%f %f %f %f %f %f %f %f %f %f %f %f",
									 &tmpparam->left2right_extern_param[0],
									 &tmpparam->left2right_extern_param[1],
									 &tmpparam->left2right_extern_param[2],
									 &tmpparam->left2right_extern_param[3],
									 &tmpparam->left2right_extern_param[4],
									 &tmpparam->left2right_extern_param[5],
									 &tmpparam->left2right_extern_param[6],
									 &tmpparam->left2right_extern_param[7],
									 &tmpparam->left2right_extern_param[8],
									 &tmpparam->left2right_extern_param[9],
									 &tmpparam->left2right_extern_param[10],
									 &tmpparam->left2right_extern_param[11]))
					{
						swordfish_debug_printf("left2right_extern_param:%f %f %f %f %f %f %f %f %f %f %f %f\n",
							   tmpparam->left2right_extern_param[0],
							   tmpparam->left2right_extern_param[1],
							   tmpparam->left2right_extern_param[2],
							   tmpparam->left2right_extern_param[3],
							   tmpparam->left2right_extern_param[4],
							   tmpparam->left2right_extern_param[5],
							   tmpparam->left2right_extern_param[6],
							   tmpparam->left2right_extern_param[7],
							   tmpparam->left2right_extern_param[8],
							   tmpparam->left2right_extern_param[9],
							   tmpparam->left2right_extern_param[10],
							   tmpparam->left2right_extern_param[11]);
					}
					else
					{
						swordfish_debug_printf("fail to parse left2right_extern_param:%s\n", tmpp);
						free(tmpp);
						free(buffer);
						free(pdata1);
						return SWORDFISH_FAILED;
					}

				}
			}
			break;
			case 6:
			{
				if(tmpparam->camera_number==3){	
					if (12 == sscanf(tmpp, "%f %f %f %f %f %f %f %f %f %f %f %f",
									 &tmpparam->left2right_extern_param[0],
									 &tmpparam->left2right_extern_param[1],
									 &tmpparam->left2right_extern_param[2],
									 &tmpparam->left2right_extern_param[3],
									 &tmpparam->left2right_extern_param[4],
									 &tmpparam->left2right_extern_param[5],
									 &tmpparam->left2right_extern_param[6],
									 &tmpparam->left2right_extern_param[7],
									 &tmpparam->left2right_extern_param[8],
									 &tmpparam->left2right_extern_param[9],
									 &tmpparam->left2right_extern_param[10],
									 &tmpparam->left2right_extern_param[11]))
					{
						swordfish_debug_printf("left2right_extern_param:%f %f %f %f %f %f %f %f %f %f %f %f\n",
							   tmpparam->left2right_extern_param[0],
							   tmpparam->left2right_extern_param[1],
							   tmpparam->left2right_extern_param[2],
							   tmpparam->left2right_extern_param[3],
							   tmpparam->left2right_extern_param[4],
							   tmpparam->left2right_extern_param[5],
							   tmpparam->left2right_extern_param[6],
							   tmpparam->left2right_extern_param[7],
							   tmpparam->left2right_extern_param[8],
							   tmpparam->left2right_extern_param[9],
							   tmpparam->left2right_extern_param[10],
							   tmpparam->left2right_extern_param[11]);
					}
					else
					{
						swordfish_debug_printf("fail to parse left2right_extern_param:%s\n", tmpp);
						free(tmpp);
						free(buffer);
						free(pdata1);
						return SWORDFISH_FAILED;
					}
				}else{


				}
			}
			break;
			case 7:
			{
				if(tmpparam->camera_number==3){	
					if (12 == sscanf(tmpp, "%f %f %f %f %f %f %f %f %f %f %f %f",
									 &tmpparam->left2rgb_extern_param[0],
									 &tmpparam->left2rgb_extern_param[1],
									 &tmpparam->left2rgb_extern_param[2],
									 &tmpparam->left2rgb_extern_param[3],
									 &tmpparam->left2rgb_extern_param[4],
									 &tmpparam->left2rgb_extern_param[5],
									 &tmpparam->left2rgb_extern_param[6],
									 &tmpparam->left2rgb_extern_param[7],
									 &tmpparam->left2rgb_extern_param[8],
									 &tmpparam->left2rgb_extern_param[9],
									 &tmpparam->left2rgb_extern_param[10],
									 &tmpparam->left2rgb_extern_param[11]))
					{
						swordfish_debug_printf("left2rgb_extern_param:%f %f %f %f %f %f %f %f %f %f %f %f\n",
							   tmpparam->left2rgb_extern_param[0],
							   tmpparam->left2rgb_extern_param[1],
							   tmpparam->left2rgb_extern_param[2],
							   tmpparam->left2rgb_extern_param[3],
							   tmpparam->left2rgb_extern_param[4],
							   tmpparam->left2rgb_extern_param[5],
							   tmpparam->left2rgb_extern_param[6],
							   tmpparam->left2rgb_extern_param[7],
							   tmpparam->left2rgb_extern_param[8],
							   tmpparam->left2rgb_extern_param[9],
							   tmpparam->left2rgb_extern_param[10],
							   tmpparam->left2rgb_extern_param[11]);
					}
					else
					{
						swordfish_debug_printf("fail to parse left2rgb_extern_param:%s\n", tmpp);
						free(tmpp);
						free(buffer);
						free(pdata1);
						return SWORDFISH_FAILED;
					}
				}else{

				}
			}
			break;
			}

			index++;
			p = strtok(NULL, "\r\n");
			free(tmpp);
		}
		free(buffer);
		free(pdata1);

		return SWORDFISH_OK;
	}
	else
	{
		free(pdata1);
		//	printf("get imuinternalref timeout!!!\n");
		return SWORDFISH_FAILED;
	}
}
SWORDFISH_RESULT swordfish_get(SWORDFISH_DEVICE_HANDLE handle, SWORDFISH_VALUE_TYPE valuetype, void *value, int timeout)
{
	SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)handle;
	if (NULL == inst->responsequeue)
	{
		return SWORDFISH_NOTREADY;
	}
	if (swordfish_hasconnect(handle))
	{
		if (valuetype == SWORDFISH_CAMERA_PARAM)
		{
			s_swordfish_cam_param tmpirparam, tmprgbparam;
			// printf("will get ir:%dx%d and rgb:%dx%d cam param!\n",inst->irwidth,inst->irheight,inst->rgbwidth,inst->rgbheight);
			if (SWORDFISH_OK == swordfish_get_cam_param_by_resolution(handle, inst->irwidth, inst->irheight, &tmpirparam) &&
				SWORDFISH_OK == swordfish_get_cam_param_by_resolution(handle, inst->rgbwidth, inst->rgbheight, &tmprgbparam))
			{
				memcpy(value, &tmpirparam, sizeof(s_swordfish_cam_param));
				s_swordfish_cam_param *tmpret = (s_swordfish_cam_param *)value;
				tmpret->rgb_sensor_focus[0] = tmprgbparam.rgb_sensor_focus[0];
				tmpret->rgb_sensor_focus[1] = tmprgbparam.rgb_sensor_focus[1];
				tmpret->rgb_sensor_photocenter[0] = tmprgbparam.rgb_sensor_photocenter[0];
				tmpret->rgb_sensor_photocenter[1] = tmprgbparam.rgb_sensor_photocenter[1];
				tmpret->cam_param_valid = 1;
				tmpret->rgb_distortion_valid = 0;
				return SWORDFISH_OK;
			}
			return SWORDFISH_FAILED;
			/*	if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND, 0, (const uint8_t *)NULL, timeout))
					swordfish_debug_printf("send ok!!!\n");
				else
				{
					return SWORDFISH_NETFAILED;
				}

				sendcmd_t *p = 0;
				p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
				if (p)
				{
					BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
					*hastimeout = FALSE;
					SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
					SEM_INIT(tmpsem, 0, 0);

					memset(p, 0, sizeof(sendcmd_t));

					s_swordfish_cam_param *tmpparam = (s_swordfish_cam_param *)calloc(1, sizeof(s_swordfish_cam_param));
					// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
					int *responselen = (int *)calloc(1, sizeof(int));
					p->responselen = responselen;
					p->responsesubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_RETURN;
					p->responsedata = tmpparam;
					p->hastimeout = hastimeout;
					p->waitsem = tmpsem;
					// p->data = NULL;
					// p->len = 0;
					MYTIMEVAL tmpstarttm;
					gettimeofday(&tmpstarttm, NULL);
					p->starttm = tmpstarttm;
					p->timeout = timeout;
					SOLO_Write_Over(inst->responsequeue);
					//	int countmilliseoncds = 0;
					for (;;)
					{
						int retvalue = SEM_TRYWAIT(tmpsem);
						if (retvalue == -1)
						{
							swordfish_debug_printf("still no answer!\n");
	#ifdef _WINDOWS
							Sleep(500);
	#else
							usleep(500 * 1000);
	#endif
						}
						else
						{
							if (*hastimeout)
							{ // timeout
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								swordfish_debug_printf("swordfish_getcamparamsync timeout.....\n");
								return SWORDFISH_TIMEOUT;
							}
							else
							{
								if (tmpparam->cam_param_valid)
								{
									swordfish_debug_printf("recv result cam param:fx:%f,baseline:%f\n", tmpparam->left_sensor_focus[0], tmpparam->left2right_extern_param[9]);
									memcpy(value, tmpparam, sizeof(s_swordfish_cam_param));
									SEM_DESTROY(tmpsem);
									free(tmpsem);
									free(tmpparam);
									free(responselen);
									free(hastimeout);
									return SWORDFISH_OK;
								}
								else
								{
									swordfish_debug_printf("recv result cam param not valid:fx:%f,baseline:%f\n", tmpparam->left_sensor_focus[0], tmpparam->left2right_extern_param[9]);
									memcpy(value, tmpparam, sizeof(s_swordfish_cam_param));
									SEM_DESTROY(tmpsem);
									free(tmpsem);
									free(tmpparam);
									free(responselen);
									free(hastimeout);
									return SWORDFISH_FAILED;
								}
							}
						}
					}
				}
				else
				{
					//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
					return SWORDFISH_RESPONSEQUEUEFULL;
				}*/
		}
		else if (valuetype == SWORDFISH_CAMERA_TIME)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_GET_SYS_TIME_COMMAND, 0, (const uint8_t *)NULL, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_time_info *tmpparam = (s_swordfish_time_info *)calloc(1, sizeof(s_swordfish_time_info));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_GET_SYS_TIME_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("swordfish_get camera time timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							//	swordfish_debug_printf("recv result cam param:fx:%f,baseline:%f\n", tmpparam->left_sensor_focus[0], tmpparam->left2right_extern_param[9]);
							memcpy(value, tmpparam, sizeof(s_swordfish_time_info));
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							return SWORDFISH_OK;
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}


		else if (valuetype == SWORDFISH_RGB_ELSE_PARAM)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_GET_RGB_ELSE_PARAM, 0, (const uint8_t *)NULL, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_rgb_else_param *tmpparam = (s_swordfish_rgb_else_param *)calloc(1, sizeof(s_swordfish_rgb_else_param));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_GET_RGB_ELSE_PARAM_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("swordfish_get rgb else param timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							//	swordfish_debug_printf("recv result cam param:fx:%f,baseline:%f\n", tmpparam->left_sensor_focus[0], tmpparam->left2right_extern_param[9]);
							memcpy(value, tmpparam, sizeof(s_swordfish_rgb_else_param));
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							return SWORDFISH_OK;
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}

		else if (valuetype == SWORDFISH_HIGH_PRECISION)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_GET_HIGH_PRECISION_PARAM, 0, (const uint8_t *)NULL, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_high_precision *tmpparam = (s_swordfish_high_precision *)calloc(1, sizeof(s_swordfish_high_precision));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_GET_HIGH_PRECISION_PARAM_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("swordfish_get high precision timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							//	swordfish_debug_printf("recv result cam param:fx:%f,baseline:%f\n", tmpparam->left_sensor_focus[0], tmpparam->left2right_extern_param[9]);
							memcpy(value, tmpparam, sizeof(s_swordfish_high_precision));
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							return SWORDFISH_OK;
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_GOODFEATURE_PARAM)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_GET_GOOD_FEATURE_PARAM, 0, (const uint8_t *)NULL, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_good_feature *tmpparam = (s_swordfish_good_feature *)calloc(1, sizeof(s_swordfish_good_feature));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_GET_GOOD_FEATURE_PARAM_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("swordfish_getgoodfeatureparam timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							//	swordfish_debug_printf("recv result cam param:fx:%f,baseline:%f\n", tmpparam->left_sensor_focus[0], tmpparam->left2right_extern_param[9]);
							memcpy(value, tmpparam, sizeof(s_swordfish_good_feature));
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							return SWORDFISH_OK;
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_LK_PARAM)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_GET_LK_PARAM, 0, (const uint8_t *)NULL, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_lk *tmpparam = (s_swordfish_lk *)calloc(1, sizeof(s_swordfish_lk));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_GET_LK_PARAM_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("swordfish_getlkparam timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							//	swordfish_debug_printf("recv result cam param:fx:%f,baseline:%f\n", tmpparam->left_sensor_focus[0], tmpparam->left2right_extern_param[9]);
							memcpy(value, tmpparam, sizeof(s_swordfish_lk));
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							return SWORDFISH_OK;
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_DOWNLOAD_FILE)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_DOWNLOAD_DATA, SWORDFISH_COMMAND_USB_DOWNLOAD_USER_FILE, sizeof(s_swordfish_download_data), (const uint8_t *)value, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_download_result *tmpparam = (s_swordfish_download_result *)calloc(1, sizeof(s_swordfish_download_result) + MAXDOWNLOADFILESIZE);
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				p->responsesubtype = (SWORDFISH_COMMAND_SUB_TYPE)SWORDFISH_COMMAND_USB_DOWNLOAD_USER_FILE_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("download timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							if (tmpparam->result == 0)
							{
								s_swordfish_download_bidata *tmpdata = (s_swordfish_download_bidata *)value;
								swordfish_debug_printf("recv result file size:%d\n", *responselen - sizeof(int));
								tmpdata->len = *responselen - sizeof(int);
								memcpy(tmpdata->data, tmpparam->data, *responselen - sizeof(int));
								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							else
							{
								swordfish_debug_printf("recv download file not valid\n");

								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_FAILED;
							}
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_IMU_CONFIG)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_GET_IMU_CONFIGURATION_COMMAND, 0, (const uint8_t *)NULL, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_imu_info *tmpparam = (s_swordfish_imu_info *)calloc(1, sizeof(s_swordfish_imu_info));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_GET_IMU_CONFIGURATION_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("swordfish_get imuconfig timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							//	if (tmpparam->cam_param_valid)
							{
								*(s_swordfish_imu_info *)value = *tmpparam;

								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							/*	else
								{
									swordfish_debug_printf("recv result cam param not valid:fx:%f,baseline:%f\n", tmpparam->left_sensor_focus[0], tmpparam->left2right_extern_param[9]);
									memcpy(value, tmpparam, sizeof(s_swordfish_cam_param));
									SEM_DESTROY(tmpsem);
									free(tmpsem);
									free(tmpparam);
free(responselen);
									free(hastimeout);
									return SWORDFISH_FAILED;
								}*/
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_IMU_INTERNALREF)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_GET_IMU_INTERNAL_REFRRENCE, 0, (const uint8_t *)NULL, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_imu_internal_reference *tmpparam = (s_swordfish_imu_internal_reference *)calloc(1, sizeof(s_swordfish_imu_internal_reference));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_GET_IMU_INTERNAL_REFRRENCE_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("swordfish_get imu internalref timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							//	if (tmpparam->cam_param_valid)
							{
								*(s_swordfish_imu_internal_reference *)value = *tmpparam;

								SEM_DESTROY(tmpsem);
								free(tmpsem);
								free(tmpparam);
								free(responselen);
								free(hastimeout);
								return SWORDFISH_OK;
							}
							/*	else
								{
									swordfish_debug_printf("recv result cam param not valid:fx:%f,baseline:%f\n", tmpparam->left_sensor_focus[0], tmpparam->left2right_extern_param[9]);
									memcpy(value, tmpparam, sizeof(s_swordfish_cam_param));
									SEM_DESTROY(tmpsem);
									free(tmpsem);
									free(tmpparam);
free(responselen);
									free(hastimeout);
									return SWORDFISH_FAILED;
								}*/
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_CAMERA_IP)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_GET_IP_ADDRESS_PARAM, 0, (const uint8_t *)NULL, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				char *tmpparam = (char *)calloc(1, 24);
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_GET_IP_ADDRESS_PARAM_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("get ip timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv ip:%s\n", (char *)tmpparam);
							strcpy((char *)value, (char *)tmpparam);
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							return SWORDFISH_OK;
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_REPEAT_TEXTURE_FILTER)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_GET_REPEATED_TEXTURE_PARAM, 0, (const uint8_t *)NULL, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_repeated_texture_filter *tmpparam = (s_swordfish_repeated_texture_filter *)calloc(1, sizeof(s_swordfish_repeated_texture_filter));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_GET_REPEATED_TEXTURE_PARAM_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("get repeat texture filter timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv result repeat texture filter:enable:%d\n", tmpparam->enable);
							memcpy(value, tmpparam, sizeof(s_swordfish_repeated_texture_filter));
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							return SWORDFISH_OK;
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}

		else if (valuetype == SWORDFISH_EDGE_FILTER)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_GET_EDGE_FILTER_PARAM, 0, (const uint8_t *)NULL, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_edge_filter_cmd *tmpparam = (s_swordfish_edge_filter_cmd *)calloc(1, sizeof(s_swordfish_edge_filter_cmd));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_GET_EDGE_FILTER_PARAM_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("get edge filter timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv result edge filter:enable:%u\n", tmpparam->enable);
							memcpy(value, tmpparam, sizeof(s_swordfish_edge_filter_cmd));
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							return SWORDFISH_OK;
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_BAD_FILTER)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_GET_BAD_FILTER_PARAM, 0, (const uint8_t *)NULL, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_bad_filter *tmpparam = (s_swordfish_bad_filter *)calloc(1, sizeof(s_swordfish_bad_filter));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_GET_BAD_FILTER_PARAM_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("get bad filter timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv result bad filter:enable:%u\n", tmpparam->enable);
							memcpy(value, tmpparam, sizeof(s_swordfish_bad_filter));
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							return SWORDFISH_OK;
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_LIGHT_FILTER)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_GET_LIGHT_FILTER_PARAM, 0, (const uint8_t *)NULL, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_light_filter *tmpparam = (s_swordfish_light_filter *)calloc(1, sizeof(s_swordfish_light_filter));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_GET_LIGHT_FILTER_PARAM_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("get light filter timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv result light filter:enable:%u,sigma:%f\n", tmpparam->enable, tmpparam->sigma);
							memcpy(value, tmpparam, sizeof(s_swordfish_light_filter));
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							return SWORDFISH_OK;
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_CONFIDENCE)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_GET_CONFIDENCE_PARAM, 0, (const uint8_t *)NULL, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_confidence *tmpparam = (s_swordfish_confidence *)calloc(1, sizeof(s_swordfish_confidence));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_GET_CONFIDENCE_PARAM_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("get confidence timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv result confidence:enable:%u,sigma:%f\n", tmpparam->enable, tmpparam->sigma);
							memcpy(value, tmpparam, sizeof(s_swordfish_confidence));
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							return SWORDFISH_OK;
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_DEPTH_CONFIG)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_GET_DEPTH_CONFIGURATION_COMMAND, 0, (const uint8_t *)NULL, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_depth_config *tmpparam = (s_swordfish_depth_config *)calloc(1, sizeof(s_swordfish_depth_config));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_GET_DEPTH_CONFIGURATION_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("get depth mode timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv result depth mode:%d\n", tmpparam->app_depth_mode);
							memcpy(value, tmpparam, sizeof(s_swordfish_depth_config));
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							return SWORDFISH_OK;
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_RUN_MODE)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_GET_RUN_CONFIGURATION_COMMAND, 0, (const uint8_t *)NULL, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_run_config *tmpparam = (s_swordfish_run_config *)calloc(1, sizeof(s_swordfish_run_config));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_GET_RUN_CONFIGURATION_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("get run mode timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv result run mode:%d\n", tmpparam->app_run_mode);
							memcpy(value, tmpparam, sizeof(s_swordfish_run_config));
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							return SWORDFISH_OK;
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_IR_EXPOSURE)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_GET_IR_IMAGE_SENSOR_EXPOSURE_COMMAND, 0, (const uint8_t *)NULL, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_ir_sensor_exposure_infor *tmpparam = (s_swordfish_ir_sensor_exposure_infor *)calloc(1, sizeof(s_swordfish_ir_sensor_exposure_infor));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_GET_IR_IMAGE_SENSOR_EXPOSURE_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("get ir exposure timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv result ir exposure mode:%d\n", tmpparam->exposure_mode);
							memcpy(value, tmpparam, sizeof(s_swordfish_ir_sensor_exposure_infor));
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							return SWORDFISH_OK;
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_RGB_EXPOSURE)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_GET_RGB_IMAGE_SENSOR_EXPOSURE_COMMAND, 0, (const uint8_t *)NULL, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				s_swordfish_rgb_sensor_exposure_infor *tmpparam = (s_swordfish_rgb_sensor_exposure_infor *)calloc(1, sizeof(s_swordfish_rgb_sensor_exposure_infor));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_GET_RGB_IMAGE_SENSOR_EXPOSURE_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("get rgb exposure timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv result rgb exposure mode:%d\n", tmpparam->exposure_mode);
							memcpy(value, tmpparam, sizeof(s_swordfish_rgb_sensor_exposure_infor));
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							return SWORDFISH_OK;
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_PROJECTOR_STATUS)
		{
			if (0 <= send_one_packet(inst, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_GET_PROJECTOR_CURRENT_COMMAND, 0, (const uint8_t *)NULL, timeout))
				swordfish_debug_printf("send ok!!!\n");
			else
			{
				return SWORDFISH_NETFAILED;
			}

			sendcmd_t *p = 0;
			p = (sendcmd_t *)SOLO_Write(inst->responsequeue);
			if (p)
			{
				BOOL *hastimeout = (BOOL *)malloc(sizeof(BOOL));
				*hastimeout = FALSE;
				SEM_T *tmpsem = (SEM_T *)calloc(1, sizeof(SEM_T));
				SEM_INIT(tmpsem, 0, 0);

				memset(p, 0, sizeof(sendcmd_t));

				int *tmpparam = (int *)calloc(1, sizeof(int));
				// p->cmdsubtype = SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND;
				int *responselen = (int *)calloc(1, sizeof(int));
				p->responselen = responselen;
				p->responsesubtype = SWORDFISH_COMMAND_GET_PROJECTOR_CURRENT_RETURN;
				p->responsedata = tmpparam;
				p->hastimeout = hastimeout;
				p->waitsem = tmpsem;
				// p->data = NULL;
				// p->len = 0;
				MYTIMEVAL tmpstarttm;
				gettimeofday(&tmpstarttm, NULL);
				p->starttm = tmpstarttm;
				p->timeout = timeout;
				SOLO_Write_Over(inst->responsequeue);
				//	int countmilliseoncds = 0;
				for (;;)
				{
					int retvalue = SEM_TRYWAIT(tmpsem);
					if (retvalue == -1)
					{
						swordfish_debug_printf("still no answer!\n");
#ifdef _WINDOWS
						Sleep(500);
#else
						usleep(500 * 1000);
#endif
					}
					else
					{
						if (*hastimeout)
						{ // timeout
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							swordfish_debug_printf("swordfish_get projector timeout.....\n");
							return SWORDFISH_TIMEOUT;
						}
						else
						{
							swordfish_debug_printf("recv result projector:%d\n", *tmpparam);
							memcpy(value, tmpparam, sizeof(int));
							SEM_DESTROY(tmpsem);
							free(tmpsem);
							free(tmpparam);
							free(responselen);
							free(hastimeout);
							return SWORDFISH_OK;
						}
					}
				}
			}
			else
			{
				//	swordfish_debug_printf("send queue has no empty node to use to send!!!\n");
				return SWORDFISH_RESPONSEQUEUEFULL;
			}
		}
		else if (valuetype == SWORDFISH_CAMERA_INFO)
		{
			MYTIMEVAL startval;
			gettimeofday(&startval, NULL);
			while (1)
			{
				MYTIMEVAL tmpval;
				gettimeofday(&tmpval, NULL);
				if (((tmpval.tv_sec - startval.tv_sec) * 1000 + (tmpval.tv_usec - startval.tv_usec) / 1000) >= timeout)
				{
					return SWORDFISH_TIMEOUT;
				}

				if (inst->camera_info == NULL)
				{
#ifdef _WINDOWS
					Sleep(500);
#else
					usleep(500 * 1000);
#endif
				}
				else
				{
					*(s_swordfish_device_info *)value = *inst->camera_info;
					return SWORDFISH_OK;
				}
			}
		}
		else
		{
			return SWORDFISH_NOTSUPPORTED;
		}
	}
	else
	{
		swordfish_debug_printf("not connected,just return SWORDFISH_NOTCONNECT\n");
		return SWORDFISH_NOTCONNECT;
	}
}

/*
void swordfish_setmorphology(SWORDFISH_DEVICE_HANDLE handle, s_swordfish_morphology *morphology)
{
	if (swordfish_hasconnect(handle))
	{
		send_one_packet(handle, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_MORPGOLOGY_PARAM, sizeof(s_swordfish_morphology), (const uint8_t *)morphology, 2000);
	}
}


void swordfish_setimuoffset(SWORDFISH_DEVICE_HANDLE handle, s_swordfish_imu_offset *offset)
{
	if (swordfish_hasconnect(handle))
	{
		send_one_packet(handle, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_SET_IMU_OFFSET_DATA_PARAM, sizeof(s_swordfish_imu_offset), (const uint8_t *)offset, 2000);
	}
}
void swordfish_getimuoffset(SWORDFISH_DEVICE_HANDLE handle)
{
	if (swordfish_hasconnect(handle))
	{
		send_one_packet(handle, SWORDFISH_COMMAND_DATA, SWORDFISH_COMMAND_GET_IMU_OFFSET_DATA_PARAM, 0, (const uint8_t *)NULL, 2000);
	}
}*/
#ifdef USEIMU
#ifndef USEYAMLCPP
s_swordfish_imu_internal_reference *swordfish_getimuinternalref(SWORDFISH_DEVICE_HANDLE handle, s_swordfish_imu_internal_reference *imuinternalref)
{
	if (!swordfish_hasconnect(handle))
	{
		return NULL;
	}
	// 1.download txt file
	s_swordfish_download_bidata *pdata1 = (s_swordfish_download_bidata *)malloc(sizeof(s_swordfish_download_bidata) + MAXDOWNLOADFILESIZE);
	strcpy(pdata1->downloaddata.file_path, "/nextvpu/example/media/feynman/imu/imu_internal_param.txt");

	if (SWORDFISH_OK == swordfish_get(handle, SWORDFISH_DOWNLOAD_FILE, pdata1, 2000)) // timeout 2000milliseconds
	{
		// parse imu_internal_param.txt to get s_swordfish_imu_internal_reference
		char *buffer = (char *)calloc(1, pdata1->len + 1);
		memcpy(buffer, pdata1->data, pdata1->len);

		char *p = NULL;
		p = strtok(buffer, "\r\n: ");
		int count = 0;
		while (p != NULL)
		{
			char *tmpp = strdup(p);
			p = strtok(NULL, "\r\n: ");
			if (0 == strcmp(tmpp, "Accel_m[0][0]"))
			{
				//  printf("%s:%s\n", tmpp, p);
				imuinternalref->accel_m[0] = (float)atof(p);
				count++;
			}
			else if (0 == strcmp(tmpp, "Accel_m[0][1]"))
			{
				//   printf("%s:%s\n", tmpp, p);
				imuinternalref->accel_m[1] = (float)atof(p);
				count++;
			}
			else if (0 == strcmp(tmpp, "Accel_m[0][2]"))
			{
				//    printf("%s:%s\n", tmpp, p);
				imuinternalref->accel_m[2] = (float)atof(p);
				count++;
			}
			else if (0 == strcmp(tmpp, "Accel_m[1][0]"))
			{
				//    printf("%s:%s\n", tmpp, p);

				imuinternalref->accel_m[3] = (float)atof(p);
				count++;
			}
			else if (0 == strcmp(tmpp, "Accel_m[1][1]"))
			{
				//   printf("%s:%s\n", tmpp, p);

				imuinternalref->accel_m[4] = (float)atof(p);
				count++;
			}
			else if (0 == strcmp(tmpp, "Accel_m[1][2]"))
			{
				//    printf("%s:%s\n", tmpp, p);

				imuinternalref->accel_m[5] = (float)atof(p);
				count++;
			}
			else if (0 == strcmp(tmpp, "Accel_m[2][0]"))
			{
				//   printf("%s:%s\n", tmpp, p);

				imuinternalref->accel_m[6] = (float)atof(p);
				count++;
			}
			else if (0 == strcmp(tmpp, "Accel_m[2][1]"))
			{
				//    printf("%s:%s\n", tmpp, p);

				imuinternalref->accel_m[7] = (float)atof(p);
				count++;
			}
			else if (0 == strcmp(tmpp, "Accel_m[2][2]"))
			{
				//   printf("%s:%s\n", tmpp, p);

				imuinternalref->accel_m[8] = (float)atof(p);
				count++;
			}
			else if (0 == strcmp(tmpp, "Accel_b[0]"))
			{
				//   printf("%s:%s\n", tmpp, p);

				imuinternalref->accel_b[0] = (float)atof(p);
				count++;
			}
			else if (0 == strcmp(tmpp, "Accel_b[1]"))
			{
				//   printf("%s:%s\n", tmpp, p);

				imuinternalref->accel_b[1] = (float)atof(p);
				count++;
			}
			else if (0 == strcmp(tmpp, "Accel_b[2]"))
			{
				//    printf("%s:%s\n", tmpp, p);

				imuinternalref->accel_b[2] = (float)atof(p);
				count++;
			}
			else if (0 == strcmp(tmpp, "Gyro_b[0]"))
			{
				//     printf("%s:%s\n", tmpp, p);

				imuinternalref->gyro_b[0] = (float)atof(p);
				count++;
			}
			else if (0 == strcmp(tmpp, "Gyro_b[1]"))
			{
				//    printf("%s:%s\n", tmpp, p);

				imuinternalref->gyro_b[1] = (float)atof(p);
				count++;
			}
			else if (0 == strcmp(tmpp, "Gyro_b[2]"))
			{
				//     printf("%s:%s\n", tmpp, p);

				imuinternalref->gyro_b[2] = (float)atof(p);
				count++;
			}

			free(tmpp);
		}
		free(buffer);
		free(pdata1);
		if (count == 15)
		{
			return imuinternalref;
		}
		else
		{

			//	printf("get imuinternalref failed!!!\n");
			return NULL;
		}
	}
	else
	{
		free(pdata1);
		//	printf("get imuinternalref timeout!!!\n");
		return NULL;
	}
}
static void parseimuconfig(const char *buffer, int bufferlen, float *values)
{
	yaml_parser_t parser;
	yaml_token_t token;

	if (!yaml_parser_initialize(&parser))
		fputs("Failed to initialize parser!\n", stderr);
	yaml_parser_set_input_string(&parser, (const unsigned char *)buffer, bufferlen);

	int startindex = -1;
	int state = 0;
	do
	{
		char *tk;

		yaml_parser_scan(&parser, &token);
		switch (token.type)
		{
		case YAML_KEY_TOKEN:
			state = 0;
			break;
		case YAML_VALUE_TOKEN:
			state = 1;
			break;
		case YAML_SCALAR_TOKEN:
			tk = (char *)token.data.scalar.value;
			if (state == 0)
			{
				//	printf("got key token:%s\n", tk);

				if (!strcmp(tk, "accelerometer_noise_density"))
				{
					startindex = 0;
				}
				else if (!strcmp(tk, "accelerometer_random_walk"))
				{

					startindex = 1;
				}
				else if (!strcmp(tk, "gyroscope_noise_density"))
				{
					startindex = 2;
				}
				else if (!strcmp(tk, "gyroscope_random_walk"))
				{
					startindex = 3;
				}
				else
				{
					startindex = -1;
				}
			}
			else
			{
				//	printf("got value token:%s\n", tk);
				if (startindex >= 0 && startindex <= 3)
				{
					float floatvalue = atof(tk);
					//		printf("got float:%f\n", floatvalue);
					values[startindex] = floatvalue;
					startindex++;
				}
			}
			break;
		default:
			break;
		}
		if (token.type != YAML_STREAM_END_TOKEN)
			yaml_token_delete(&token);
	} while (token.type != YAML_STREAM_END_TOKEN);
	yaml_token_delete(&token);

	yaml_parser_delete(&parser);
}
static float parsecamimushift(const char *buffer, int bufflen)
{
	float ret = 0;
	yaml_parser_t parser;
	yaml_token_t token;

	if (!yaml_parser_initialize(&parser))
		fputs("Failed to initialize parser!\n", stderr);
	yaml_parser_set_input_string(&parser, (const unsigned char *)buffer, bufflen);

	int startindex = -1;
	int state = 0;
	do
	{
		char *tk;

		yaml_parser_scan(&parser, &token);
		switch (token.type)
		{
		case YAML_KEY_TOKEN:
			state = 0;
			break;
		case YAML_VALUE_TOKEN:
			state = 1;
			break;
		case YAML_SCALAR_TOKEN:
			tk = (char *)token.data.scalar.value;
			if (state == 0)
			{
				//	printf("got key token:%s\n", tk);
				if (!strcmp(tk, "timeshift_cam_imu"))
				{
					startindex = 0;
				}
			}
			else
			{
				//	printf("got value token:%s\n", tk);
				if (startindex >= 0 && startindex <= 15)
				{
					float floatvalue = atof(tk);
					//	printf("got float:%f\n", floatvalue);
					ret = floatvalue;
					break;
				}
			}
			break;
		default:
			break;
		}
		if (token.type != YAML_STREAM_END_TOKEN)
			yaml_token_delete(&token);
	} while (token.type != YAML_STREAM_END_TOKEN);
	yaml_token_delete(&token);

	yaml_parser_delete(&parser);

	return ret;
}
static void parsecamimuconfig(const char *buffer, int bufflen, float *values)
{
	yaml_parser_t parser;
	yaml_token_t token;

	if (!yaml_parser_initialize(&parser))
		fputs("Failed to initialize parser!\n", stderr);
	yaml_parser_set_input_string(&parser, (const unsigned char *)buffer, bufflen);

	int startindex = -1;
	int state = 0;
	do
	{
		char *tk;

		yaml_parser_scan(&parser, &token);
		switch (token.type)
		{
		case YAML_KEY_TOKEN:
			state = 0;
			break;
		case YAML_VALUE_TOKEN:
			state = 1;
			break;
		case YAML_SCALAR_TOKEN:
			tk = (char *)token.data.scalar.value;
			if (state == 0)
			{
				//	printf("got key token:%s\n", tk);
				if (!strcmp(tk, "T_cam_imu"))
				{
					startindex = 0;
				}
			}
			else
			{
				//	printf("got value token:%s\n", tk);
				if (startindex >= 0 && startindex <= 15)
				{
					float floatvalue = atof(tk);
					//	printf("got float:%f\n", floatvalue);
					values[startindex] = floatvalue;
					startindex++;
				}
			}
			break;
		default:
			break;
		}
		if (token.type != YAML_STREAM_END_TOKEN)
			yaml_token_delete(&token);
	} while (token.type != YAML_STREAM_END_TOKEN);
	yaml_token_delete(&token);

	yaml_parser_delete(&parser);
}
s_swordfish_imu_external_reference *swordfish_getimuexternalref(SWORDFISH_DEVICE_HANDLE handle, s_swordfish_imu_external_reference *imuexternalref)
{
	if (!swordfish_hasconnect(handle))
	{
		return NULL;
	}
	// 1.download two yaml file
	s_swordfish_download_bidata *pdata1 = (s_swordfish_download_bidata *)malloc(sizeof(s_swordfish_download_bidata) + MAXDOWNLOADFILESIZE);
	strcpy(pdata1->downloaddata.file_path, "/nextvpu/example/media/feynman/wide-angle/param/camchain-imucam.yaml");

	if (SWORDFISH_OK == swordfish_get(handle, SWORDFISH_DOWNLOAD_FILE, pdata1, 2000)) // timeout 2000milliseconds
	{
		//	printf("download camchain-imucam.yaml ok!\n");

		//	printf("got camimu:%s\n", pdata1->data);
		// 2.parse yaml to get imu external ref
		s_swordfish_download_bidata *pdata2 = (s_swordfish_download_bidata *)malloc(sizeof(s_swordfish_download_bidata) + MAXDOWNLOADFILESIZE);
		strcpy(pdata2->downloaddata.file_path, "/nextvpu/example/media/feynman/wide-angle/param/imu.yaml");

		if (SWORDFISH_OK == swordfish_get(handle, SWORDFISH_DOWNLOAD_FILE, pdata2, 2000)) // timeout 2000milliseconds
		{
			//	printf("download imu.yaml ok!\n");
			//	printf("got imu:%s\n", pdata2->data);
			float camimus[16];
			parsecamimuconfig(pdata1->data, pdata1->len, &camimus[0]);

			float imus[4];
			parseimuconfig(pdata2->data, pdata2->len, &imus[0]);

			for (int row = 0; row < 4; row++)
				for (int col = 0; col < 4; col++)
				{
					imuexternalref->t_cam_imu[row * 4 + col] = camimus[row * 4 + col];
				}
			// printf("will parse acc and gyro!\n");
			imuexternalref->acc_noise_density = imus[0];  // imuconfig["imu0"]["accelerometer_noise_density"].as<float>();
			imuexternalref->acc_random_walk = imus[1];	  // imuconfig["imu0"]["accelerometer_random_walk"].as<float>();
			imuexternalref->gyro_noise_density = imus[2]; // imuconfig["imu0"]["gyroscope_noise_density"].as<float>();
			imuexternalref->gyro_random_walk = imus[3];	  // imuconfig["imu0"]["gyroscope_random_walk"].as<float>();
			imuexternalref->timeshift_cam_imu = parsecamimushift(pdata1->data, pdata1->len);
			; // imuconfig["imu0"]["gyroscope_random_walk"].as<float>();

			// printf("got imuexternalref:\n");
			free(pdata1);
			free(pdata2);
			return imuexternalref;
		}
		else
		{
			free(pdata1);
			free(pdata2);
			swordfish_debug_printf("get imuexternalref timeout!!!\n");
			return NULL;
		}
	}
	else
	{
		free(pdata1);
		swordfish_debug_printf("get imuexternalref timeout!!!\n");
		return NULL;
	}
}
#endif
#endif
/*
s_swordfish_sensor_distortion_ratio swordfish_getrgbdistortion(SWORDFISH_DEVICE_HANDLE handle)
{
	s_swordfish_sensor_distortion_ratio tmpratio;
	tmpratio.k1 = 0.0;
	tmpratio.k2 = 0.0;
	tmpratio.k3 = 0.0;
	tmpratio.p1 = 0.0;
	tmpratio.p2 = 0.0;
	return tmpratio;
}
s_swordfish_sensor_distortion_ratio swordfish_getleftirdistortion(SWORDFISH_DEVICE_HANDLE handle)
{
	s_swordfish_sensor_distortion_ratio tmpratio;
	tmpratio.k1 = 0.0;
	tmpratio.k2 = 0.0;
	tmpratio.k3 = 0.0;
	tmpratio.p1 = 0.0;
	tmpratio.p2 = 0.0;
	return tmpratio;
}
s_swordfish_sensor_distortion_ratio swordfish_getrightirdistortion(SWORDFISH_DEVICE_HANDLE handle)
{
	s_swordfish_sensor_distortion_ratio tmpratio;
	tmpratio.k1 = 0.0;
	tmpratio.k2 = 0.0;
	tmpratio.k3 = 0.0;
	tmpratio.p1 = 0.0;
	tmpratio.p2 = 0.0;
	return tmpratio;
}
void swordfish_downloadfile(SWORDFISH_DEVICE_HANDLE handle, const char *remotefilepath)
{
	if (swordfish_hasconnect(handle))
	{
		s_swordfish_download_data tmpdata;
		strcpy(tmpdata.file_path, remotefilepath);
		send_one_packet(handle, SWORDFISH_DOWNLOAD_DATA, SWORDFISH_COMMAND_USB_DOWNLOAD_USER_FILE, sizeof(s_swordfish_download_data), (const uint8_t *)&tmpdata, 2000);
	}
}*/
const char *swordfish_getsdkversion()
{
	return SDKVERSION(MAINVER, SUBVER1, SUBVER2);
}
/*
void swordfish_setcapture(IMGTYPE imgtype, BOOL willcapture) {
	if (willcapture) {
		uint8_t current_mode_ = CURRENT_MODE_CAPTURE_AFTER_DSP;
		send_one_packet(CMD_TYPE_CONTROL, CONTROL_SET_CURRENT_MODE, 1, (const uint8_t*)&current_mode_);
	}
	else {
		uint8_t current_mode_ = CURRENT_MODE_NULL;
		send_one_packet(CMD_TYPE_CONTROL, CONTROL_SET_CURRENT_MODE, 1, (const uint8_t*)&current_mode_);
	}
}*/
void swordfish_zerorect(uint16_t *depthdata, int width, int height, int fromcolindex, int tocolindex, int fromrowindex, int torowindex)
{
	if (fromcolindex < 0 || fromcolindex >= width ||
		tocolindex < 0 || tocolindex >= width ||
		fromrowindex < 0 || fromrowindex >= height ||
		torowindex < 0 || torowindex >= height ||
		fromcolindex > tocolindex || fromrowindex > torowindex)
	{
		return;
	}
	for (int i = fromrowindex; i <= torowindex; i++)
	{
		uint16_t *linestart = depthdata + i * width + fromcolindex;
		memset((uint8_t *)linestart, 0, (tocolindex - fromcolindex + 1) * 2);
	}
}
SWORDFISH_RESULT
swordfish_sendbuffer (SWORDFISH_DEVICE_HANDLE handle, uint8_t * data,
		      int datalen, int pktsize)
{
  struct timeval tv;
  gettimeofday (&tv, NULL);

  int total_blk = (unsigned int) (datalen + pktsize - 1) / pktsize;
  printf ("total pkts:%d\n", total_blk);
  for (int i = 0; i < total_blk; i++) {
    int send_len = (i == (total_blk - 1) ? (datalen % pktsize) : pktsize);

    int indexflag = (i == (total_blk - 1) ? 0 : (i + 1));
    if (0 ==
	send_one_packet (handle, 0, indexflag, send_len, data + i * pktsize,
			 2000)) {
      printf ("send %d pkt ok!\n", i + 1);
    } else {
      printf ("send %d pkt fail!\n", i + 1);
      return SWORDFISH_FAILED;
    }
  }

  struct timeval lasttv;
  gettimeofday (&lasttv, NULL);
  printf ("send frame cost:%ld ms\n",
	  (lasttv.tv_sec - tv.tv_sec) * 1000 + (lasttv.tv_usec -
						tv.tv_usec) / 1000);
  return SWORDFISH_OK;
}
void swordfish_globaltimeenable(SWORDFISH_DEVICE_HANDLE handle, int enable)
{
	SWORDFISH_INSTANCE *inst = (SWORDFISH_INSTANCE *)handle;
	if (inst != NULL)
	{
		inst->global_time_enabled = enable;
	}
}
