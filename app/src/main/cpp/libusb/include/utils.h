#ifndef __FEYNMAN_UTILS__
#define __FEYNMAN_UTILS__
#include <libusb.h>
#include "ring_queue.h"
#ifdef __cplusplus
extern "C"
{
#endif
#ifdef _WINDOWS
#include <windows.h>
#else

#include <semaphore.h>
#endif

#ifdef _WINDOWS
#define log_printf(format, ...)                                                     \
    do                                                                              \
    {                                                                               \
        char tmpstr[512];                                                           \
        SYSTEMTIME sTime;                                                           \
        GetLocalTime(&sTime);                                                       \
        sprintf(tmpstr, "[%02d/%02d %02d:%02d:%02d.%03d", sTime.wMonth, sTime.wDay, \
                sTime.wHour, sTime.wMinute, sTime.wSecond, sTime.wMilliseconds);    \
        printf("%s %s %d] " format, tmpstr, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    } while (0)
#else
#define log_printf(format, ...)                                                               \
    do                                                                                        \
    {                                                                                         \
        char tmpstr[512];                                                                     \
        struct timeval tv;                                                                    \
        gettimeofday(&tv, NULL);                                                              \
        struct tm *sTime = localtime(&tv.tv_sec);                                             \
        sprintf(tmpstr, "[%02d/%02d %02d:%02d:%02d.%03ld", sTime->tm_mon + 1, sTime->tm_mday, \
                sTime->tm_hour, sTime->tm_min, sTime->tm_sec, tv.tv_usec / 1000);             \
        printf("%s %s %d] " format, tmpstr, __FUNCTION__, __LINE__, ##__VA_ARGS__);           \
    } while (0)
#endif
#ifdef _WINDOWS
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
    // typedef usb_dev_handle *USBHANDLE;
    typedef HANDLE THREADHANDLE;
    typedef HANDLE SEM_T;
    typedef HANDLE MUTEXHANDLE;
    typedef struct timeval2 MYTIMEVAL;
#else
typedef int SOCKET;
typedef pthread_t THREADHANDLE;
typedef sem_t SEM_T;
typedef struct timeval MYTIMEVAL;
typedef pthread_mutex_t *MUTEXHANDLE;
#endif

    int SEM_TRYWAIT(SEM_T *sem);
    void SEM_INIT(SEM_T *sem, int pshared, unsigned int value);

    int SEM_TIMEDWAIT(SEM_T *sem, int milliseconds);
    void SEM_DESTROY(SEM_T *sem);
    void SEM_POST(SEM_T *sem);
    void COMMONUSLEEP(int microseconds);

    MUTEXHANDLE CREATEMUTEX();
    void MUTEXLOCK(MUTEXHANDLE handle);
    void MUTEXUNLOCK(MUTEXHANDLE handle);
    void CLOSEMUTEX(MUTEXHANDLE handle);
    /*
    #ifdef _WINDOWS
        // typedef usb_dev_handle *USBHANDLE;
        typedef HANDLE THREADHANDLE;
        typedef HANDLE SEM_T;
        typedef struct timeval2 MYTIMEVAL;
        typedef HANDLE MUTEXHANDLE;
    #else
    typedef int SOCKET;
    typedef pthread_t THREADHANDLE;
    typedef sem_t SEM_T;
    typedef struct timeval MYTIMEVAL;
    typedef pthread_mutex_t *MUTEXHANDLE;
    #endif
    */
#ifdef _WINDOWS
    typedef SOCKET SOCKETHANDLE;
#else
typedef int SOCKETHANDLE;
#endif
    typedef libusb_device_handle *USBHANDLE;
/*
	typedef struct
	{
		int timeout;
		SWORDFISH_COMMAND_SUB_TYPE responsesubtype;
		void *responsedata;
		int *responselen;
		MYTIMEVAL starttm;
#ifdef _WINDOWS
		HANDLE waitsem;
#else
		SEM_T *waitsem;
#endif
		BOOL *hastimeout;
	} sendcmd_t;
*/
typedef struct
{
    int timeout;
    /*	SWORDFISH_COMMAND_SUB_TYPE cmdsubtype;
        int len;
        void *data;
    */
    SWORDFISH_COMMAND_SUB_TYPE responsesubtype;
    void *responsedata;
    int *responselen;
    MYTIMEVAL starttm;
#ifdef _WINDOWS
    HANDLE waitsem;
#else
    SEM_T *waitsem;
#endif
    BOOL *hastimeout;
} sendcmd_t;
/*
	typedef struct _swordfish_list_node
	{
		sendcmd_t data;
		struct _swordfish_list_node *next;
	} SWORDFISH_LIST_NODE;
	*/
typedef struct _swordfish_list_node
{
    sendcmd_t data;
    struct _swordfish_list_node *next;
} SWORDFISH_LIST_NODE;

    typedef struct
    {
        double _x;
        double _y;
    } CSample;
typedef struct _sampledeque
{
    struct _sampledeque *prev;
    struct _sampledeque *next;
    CSample sample;
} sampledeque;

typedef struct{
double _prev_a;
double _prev_b; // Linear regression coeffitions - previously used values.
double _dest_a;
double _dest_b; // Linear regression coeffitions - recently calculated.
double _prev_time; 
double _last_request_time;
sampledeque *_last_values;
CSample _base_sample;// = {0, 0};
double max_device_time;
}TIMESTAMPINFO;

typedef struct
{
    SWORDFISH_STATUS status;
    SWORDFISH_INTERFACE_TYPE interface_type;
    USBHANDLE device_usb_handle;
    SOCKET device_net_socket;
    grouppkt_info_custom_t groupbuffer;

    pthread_mutex_t timestampmutex;
    pthread_mutex_t sendmutex;
    int timestamp_is_ready;
    TIMESTAMPINFO* tminfo;

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
    THREADHANDLE timestampthread;
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
    FRAMECALLBACK resultcallback;
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
} SWORDFISH_INSTANCE;
/*
    typedef struct
    {
        SWORDFISH_STATUS status;
        SWORDFISH_INTERFACE_TYPE interface_type;
        USBHANDLE device_usb_handle;
        SOCKETHANDLE device_net_socket;
        gatherpkt_info_custom_t groupbuffer;

        MUTEXHANDLE timestampmutex;
        MUTEXHANDLE sendmutex;
        int timestamp_is_ready;
        TIMESTAMPINFO* tminfo;

        int thread_running_flag;
        int global_time_enabled;
        int irwidth;
        int irheight;
        int rgbwidth;
        int rgbheight;
        int rgb_rectify_mode;
        EVENTCALLBACK eventcallback;
        void *connectuserdata;
        unsigned char streammask;
        //	Ring_Queue *sendqueue;
        Ring_Queue *responsequeue;
        SWORDFISH_LIST_NODE *responselisthead;
        FRAMECALLBACK framecallback;
        Ring_Queue *gatherpktqueue;
        THREADHANDLE recvthread;
        THREADHANDLE timestampthread;

        THREADHANDLE gatherthreadhandle;
        //	THREADHANDLE sendthread;
        uint8_t *usb_buf;
        uint8_t *tmpbuf;
        SEM_T gathersem;
        SEM_T postconnectsem;
        FRAMECALLBACK gathercallback;
        s_swordfish_device_info *camera_info;
        SEM_T *g_upgradesem;
        s_swordfish_upgrade_result g_upgraderesult;
    } SWORDFISH_INSTANCE;
*/
#ifdef __cplusplus
}
#endif
#endif
