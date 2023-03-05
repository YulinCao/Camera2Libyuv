#ifndef __SWORDFISH__
#define __SWORDFISH__
#ifdef __cplusplus
extern "C"
{
#endif

#define MAINVER 3
#define SUBVER1 5
#define SUBVER2 9

#define STR(s) #s
#define SDKVERSION(a, b, c) "v" STR(a) "." STR(b) "." STR(c)

#define TRUE 1
#define FALSE 0
#ifdef _WINDOWS
	typedef unsigned short uint16_t;
	typedef unsigned char uint8_t;
	typedef unsigned int uint32_t;
	typedef int int32_t;
	typedef short int16_t;
	typedef unsigned long long uint64_t;
#else
#include <stdint.h>
#endif
	typedef signed short NVP_S16;
	typedef unsigned long long NVP_U64;
	typedef float NVP_FLOAT;

	typedef int BOOL;

	typedef void *SWORDFISH_DEVICE_HANDLE;
#ifdef _WINDOWS
#define REAL_USB_PACKET_MAX_SIZE (64 * 1024)
#else
#define REAL_USB_PACKET_MAX_SIZE (256 * 1024)
#endif
#define USB_PACKET_MAX_SIZE (3 * 1024 * 1024 + 1024)
#define MAXDOWNLOADFILESIZE (512 * 1024)
#define ONE_PACKET_DATA_MAX_SIZE (USB_PACKET_MAX_SIZE - 16)
#define PREVIEW_FRAME_DATA_MAX_SIZE (ONE_PACKET_DATA_MAX_SIZE - sizeof(FrameHeader))

	/** SWORDFISH_STATUS */
	typedef enum
	{
		INITIAL = 0,   /**< open procedure status initial:0 */
		CONNECTED,	   /**< open procedure status connected:1 */
		POSTCONNECTED, /**< open procedure status post connected:2 */
		CONFIGURED,	   /**< open procedure status configured:3 */
		STARTED,	   /**< open procedure status started:4 */
		STOPPED,	   /**< open procedure status stopped:5 */
	} SWORDFISH_STATUS;

	/** SWORDFISH_STREAM_TYPE_E */
	typedef enum
	{
		SWORDFISH_STREAM_DEPTH,			/**< depth stream:0 */
		SWORDFISH_STREAM_DEPTH_LEFTIR,	/**< left ir stream in depth mode:1 */
		SWORDFISH_STREAM_DEPTH_RIGHTIR, /**< right ir stream in depth mode:2 */
		SWORDFISH_STREAM_RGB,			/**< rgb stream:3 */
		SWORDFISH_STREAM_IMU,			/**< imu stream:4 */
		SWORDFISH_STREAM_GOODFEATURE,	/**< good feature stream:5 */
		SWORDFISH_STREAM_OTHER,	/**< good feature stream:6 */
		SWORDFISH_STREAM_GROUPIMAGES,			/**< depth stream:7 */
	} SWORDFISH_STREAM_TYPE_E;

	/** SWORDFISH_FPS_E */
	typedef enum
	{
		FPS_15, /**< stream fps=15:0 */
		FPS_30, /**< stream fps=30:1 */
		FPS_UNKNOWN,
	} SWORDFISH_FPS_E;

	/** SWORDFISH_RESULT */

	typedef enum
	{
		SWORDFISH_OK,					 /**< result ok:0 */
		SWORDFISH_NOTCONNECT,			 /**< result not connected:1 */
		SWORDFISH_RESPONSEQUEUEFULL,	 /**< result response queue full:2 */
		SWORDFISH_TIMEOUT,				 /**< result timeout:3 */
		SWORDFISH_NETFAILED,			 /**< result net failed:4 */
		SWORDFISH_UNKNOWN,				 /**< result unknown:5 */
		SWORDFISH_NOTSUPPORTED,			 /**< result not supported:6 */
		SWORDFISH_FAILED,				 /**< result failed:7 */
		SWORDFISH_NOTREADY,				 /**< result not ready:8 */
		SWORDFISH_NOTREACHPOSTCONNECTED, /**< result not reach post connected:9 */
	} SWORDFISH_RESULT;

	/** SWORDFISH_VALUE_TYPE */
	typedef enum
	{
		SWORDFISH_CAMERA_PARAM,			  /**< swordfish_get/set camera param:0 */
		SWORDFISH_PROJECTOR_STATUS,		  /**< swordfish_get/set projector status:1 */
		SWORDFISH_CAMERA_INFO,			  /**< swordfish_get/set camera info:2*/
		SWORDFISH_TIME_SYNC,			  /**< swordfish_set time sync:3*/
		SWORDFISH_IR_EXPOSURE,			  /**< swordfish_get/set ir exposure:4*/
		SWORDFISH_RGB_EXPOSURE,			  /**< swordfish_get/set rgb exposure:5*/
		SWORDFISH_RUN_MODE,				  /**< swordfish_get/set run mode:6*/
		SWORDFISH_DEPTH_CONFIG,			  /**< swordfish_get/set depth config:7*/
		SWORDFISH_CONFIDENCE,			  /**< swordfish_get/set confidence:8*/
		SWORDFISH_LIGHT_FILTER,			  /**< swordfish_get/set light filter:9*/
		SWORDFISH_BAD_FILTER,			  /**< swordfish_get/set bad filter:10*/
		SWORDFISH_EDGE_FILTER,			  /**< swordfish_get/set edge filter:11*/
		SWORDFISH_SWITCH_VI_VPSS,		  /**< swordfish_set switch vi/vpss:12*/
		SWORDFISH_SWITCH_DEPTH_DISPARITY, /**< swordfish_set switch depth/disparity:13*/
		SWORDFISH_CAMERA_IP,			  /**< swordfish_get/set camera ip:14*/
		SWORDFISH_PIPELINE,				  /**< swordfish_set pipeline:15*/
		SWORDFISH_SAVE_CONFIG,			  /**< swordfish_set save config:16*/
		SWORDFISH_TRIGGER_SRC,			  /**< swordfish_get/set trigger src:17*/
		SWORDFISH_TRIGGER_SOFT,			  /**< swordfish_set software trigger:18*/
		SWORDFISH_IMU_CONFIG,			  /**< swordfish_get/set imu config:19*/
		SWORDFISH_IMU_INTERNALREF,		  /**< current not used,swordfish_get/set imu internal ref:20*/
		SWORDFISH_IMU_EXTERNALREF,		  /**< current not used,swordfish_get/set imu external ref:21*/
		SWORDFISH_DOWNLOAD_FILE,		  /**< swordfish_get download file:22*/
		SWORDFISH_GOODFEATURE_PARAM,	  /**< swordfish_get/set good feature param:23*/
		SWORDFISH_LK_PARAM,				  /**< swordfish_get/set lk param:24*/
		SWORDFISH_HIGH_PRECISION,		  /**< swordfish_get/set high precision:25*/
		SWORDFISH_REPEAT_TEXTURE_FILTER,  /**< swordfish_get/set repeat texture filter:26*/
		SWORDFISH_RGB_ELSE_PARAM,		  /**< swordfish_get/set rgb else param:27*/
		SWORDFISH_CAMERA_TIME,		  /**< swordfish_get camera time param:28*/
	} SWORDFISH_VALUE_TYPE;

	/** SWORDFISH_INTERFACE_TYPE */
	typedef enum
	{
		SWORDFISH_NETWORK_INTERFACE, /**< network interface type of camera:0 */
		SWORDFISH_USB_INTERFACE,	 /**< usb interface type of camera:1 */
	} SWORDFISH_INTERFACE_TYPE;

	/** SWORDFISH_PRIMARY_TYPE */
	typedef enum
	{
		SWORDFISH_IMAGE_DATA,		/**< image data:0  */
		SWORDFISH_COMMAND_DATA,		/**< command data :1  */
		SWORDFISH_IMU_DATA,			/**< imu data :2  */
		SWORDFISH_DEVICE_DATA,		/**< device data :3  */
		SWORDFISH_CALIBRATION_DATA, /**< calibration data :4  */
		SWORDFISH_TEST_DATA,		/**< test data :5  */
		SWORDFISH_UPGRADE_DATA,		/**< upgrade data :6  */
		SWORDFISH_LOG_DATA,			/**< log data :7 */
		SWORDFISH_CNN_DATA,			/**< cnn data :8 */
		SWORDFISH_USER_DATA,		/**< user define data :9 */
		SWORDFISH_CV_DATA,			/**< cv data like good feature :10 */
		SWORDFISH_DOWNLOAD_DATA,	/**< download data :11 */
		SWORDFISH_UPDATE_TFTP,	/**< send this to reboot to uboot :12 */
	} SWORDFISH_PRIMARY_TYPE;

	/** SWORDFISH_IMAGE_FORMAT_TYPE */
	typedef enum
	{
		SWORDFISH_IMAGE_FORMAT_RAW10, /**< image format raw10 0  */
		SWORDFISH_IMAGE_FORMAT_YUV2,  /**< image format yuv2 1  */
		SWORDFISH_IMAGE_FORMAT_JPEG,  /**< image format jpeg 2  */
		SWORDFISH_IMAGE_FORMAT_MJPG,  /**< image format mjpg 3  */
		SWORDFISH_IMAGE_FORMAT_INT16, /**< image format int16 4  */
	} SWORDFISH_IMAGE_FORMAT_TYPE;

	/** SWORDFISH_IMAGE_SUB_TYPE */
	typedef enum
	{
		SWORDFISH_IR_IMAGE_LEFT_VI = 0x00,	/**< ir sensor left, YUV,uncalibrated 0  */
		SWORDFISH_IR_IMAGE_RIGHT_VI,		/**< ir sensor right, uncalibrated 1  */
		SWORDFISH_IR_IMAGE_LEFT_VPSS,		/**< ir sensor left, calibrated 2  */
		SWORDFISH_IR_IMAGE_RIGHT_VPSS,		/**< ir sensor right, YUV，calibrated 3  */
		SWORDFISH_IR_IAMGE_DUAL,			/**< ir sensor dual,not used 4  */
		SWORDFISH_RGB_IMAGE_SINGLE,			/**< rgb sensor single, NV12 5 */
		SWORDFISH_RGB_IMAGE_YUV_422_PACKED, /**< rgb sensor, YUV 422 packed 6 */
		SWORDFISH_RGB_IMAGE_RESERVED1,		/**< rgb sensor right, not used 7 */
		SWORDFISH_RGB_IMAGE_RESERVED2,		/**< rgb sensorr dual, not used 8 */
		SWORDFISH_DISPARITYIMAGE,			/**< disparity data，16bit RAW 9  */
		SWORDFISH_DEPTH_IMAGE,				/**< depth data,16bit RAW 10  */
		SWORDFISH_DEPTH_IMAGE_LEFT_RAW,		/**< left ir image data in depth computing,8bit RAW 11  */
		SWORDFISH_DEPTH_IMAGE_RIGHT_RAW,	/**< right ir image data in depth computing,8bit RAW 12  */
		SWORDFISH_DEPTH_PALETTES_YUV,		/**< palettes yuv data, yuv:13 */
		SWORDFISH_POINTCLOUD,				/**< point cloud data:14 */
	} SWORDFISH_IMAGE_SUB_TYPE;

	/** SWORDFISH_IMU_DATASUB_TYPE */
	typedef enum
	{
		SWORDFISH_IMU_DATA_ACC = 0x00, /**< imu data of accel 0  */
		SWORDFISH_IMU_DATA_GYO,		   /**< imu data of gyro 1  */
		SWORDFISH_IMU_DATA_MANG,	   /**< imu data of magnet 2  */
		SWORDFISH_IMU_DATA_ALL,		   /**< imu data of all 3  */
		SWORDFISH_IMU_DATA_RES		   /**< imu data of res 4  */
	} SWORDFISH_IMU_DATASUB_TYPE;

	/** SWORDFISH_COMMAND_SUB_TYPE */
	typedef enum
	{
		SWORDFISH_COMMAND_GET_DEVICE_ID_COMMAND = 0x00,			/**< get device id 0  */
		SWORDFISH_COMMAND_GET_DEVICE_ID_RETURN,					/**< return get device id 1  */
		SWORDFISH_COMMAND_GET_DEVICE_SN_COMMAND = 0x02,			/**< get device sn 2  */
		SWORDFISH_COMMAND_GET_DEVICE_SN_RETURN,					/**< return get device sn 3  */
		SWORDFISH_COMMAND_GET_PROJECTOR_CURRENT_COMMAND = 0x10, /**< get projector current status 16  */
		SWORDFISH_COMMAND_GET_PROJECTOR_CURRENT_RETURN,			/**< return get projector current status 17  */
		SWORDFISH_COMMAND_SET_PROJECTOR_CURRENT_COMMAND = 0x12, /**< set projector current status 18  */
		SWORDFISH_COMMAND_SET_PROJECTOR_CURRENT_RETURN,			/**< return set projector current status 19  */

		SWORDFISH_COMMAND_SET_TRIGGER_SOURCE_COMMNAD = 0X14, /**< set trigger src 20  */
		SWORDFISH_COMMAND_SET_TRIGGER_SOURCE_COMMNAD_RETURN, /**< set trigger src return 21  */
		SWORDFISH_COMMAND_GET_TRIGGER_SOURCE_COMMNAD = 0X16, /**< get trigger src 22  */
		SWORDFISH_COMMAND_GET_TRIGGER_SOURCE_COMMNAD_RETURN, /**< get trigger src return 23  */

		SWORDFISH_COMMAND_SET_SOFTWARE_TRIGGER_COMMNAD = 0X18, /**< software trigger 24  */
		SWORDFISH_COMMAND_SET_SOFTWARE_TRIGGER_COMMNAD_RETURN, /**< software trigger return 25  */
		SWORDFISH_COMMAND_GET_SOFTWARE_TRIGGER_COMMNAD = 0X1A, /**< get software trigger 26  */
		SWORDFISH_COMMAND_GET_SOFTWARE_TRIGGER_COMMNAD_RETURN, /**< get software trigger return 27  */

		SWORDFISH_COMMAND_SET_HARDWARE_TRIGGER_COMMNAD = 0X1C, /**< hardware trigger return 28  */
		SWORDFISH_COMMAND_SET_HARDWARE_TRIGGER_COMMNAD_RETURN, /**< hardware trigger return 29  */
		SWORDFISH_COMMAND_GET_HARDWARE_TRIGGER_COMMNAD = 0X1E, /**< get hardware trigger 30  */
		SWORDFISH_COMMAND_GET_HARDWARE_TRIGGER_COMMNAD_RETURN, /**< get hardware trigger return 31  */

		SWORDFISH_COMMAND_GET_IR_IMAGE_SENSOR_RESOLUTION_FPS_COMMAND = 0x20,  /**< get ir sensor resolution/fps  32 */
		SWORDFISH_COMMAND_GET_IR_IMAGE_SENSOR_RESOLUTION_FPS_RETURN,		  /**<return get ir sensor resolution and fps 33  */
		SWORDFISH_COMMAND_SET_IR_IMAGE_SENSOR_RESOLUTION_FPS_COMMAND = 0x22,  /**< set ir sensor resolution/fps 34  */
		SWORDFISH_COMMAND_SET_IR_IMAGE_SENSOR_RESOLUTION_FPS_RETURN,		  /**< return set ir sensor resolution and fps 35  */
		SWORDFISH_COMMAND_GET_IR_IMAGE_SENSOR_EXPOSURE_COMMAND = 0x24,		  /**< get ir image ir sensor exposure 36  */
		SWORDFISH_COMMAND_GET_IR_IMAGE_SENSOR_EXPOSURE_RETURN,				  /**< return get ir image sensor exposure 37  */
		SWORDFISH_COMMAND_SET_IR_IMAGE_SENSOR_EXPOSURE_COMMAND = 0x26,		  /**< set ir image sensor exposure 38  */
		SWORDFISH_COMMAND_SET_IR_IMAGE_SENSOR_EXPOSURE_RETURN,				  /**< return set ir  image senor exposure 39  */
		SWORDFISH_COMMAND_GET_IR_IMAGE_SENSOR_ROI_COMMAND = 0x28,			  /**< get ir image sensor ROI(Region of interest) 40  */
		SWORDFISH_COMMAND_GET_IR_IMAGE_SENSOR_ROI_RETURN,					  /**< return get ir image sensor ROI(Region of interest) 41  */
		SWORDFISH_COMMAND_SET_IR_IMAGE_SENSOR_ROI_COMMAND = 0x2A,			  /**< set ir image sensor ROI(Region of interest) 42  */
		SWORDFISH_COMMAND_SET_IR_IMAGE_SENSOR_ROI_RETURN,					  /**< return set ir  image sensor ROI(Region of interest) 43  */
		SWORDFISH_COMMAND_GET_RGB_IMAGE_SENSOR_RESOLUTION_FPS_COMMAND = 0x30, /**< get rgb image sensor resolutoin and fps 48  */
		SWORDFISH_COMMAND_GET_RGB_IMAGE_SENSOR_RESOLUTION_FPS_RETURN,		  /**< return get rgb image sensor resolution and fps 49  */
		SWORDFISH_COMMAND_SET_RGB_IMAGE_SENSOR_RESOLUTION_FPS_COMMAND = 0x32, /**< set rgb image sensor resolution  and  fps 50  */
		SWORDFISH_COMMAND_SET_RGB_IMAGE_SENSOR_RESOLUTION_FPS_RETURN,		  /**< return set rgb image sensor resolution and fps 51  */
		SWORDFISH_COMMAND_GET_RGB_IMAGE_SENSOR_EXPOSURE_COMMAND = 0x34,		  /**< get rgb image sensor exposure 52  */
		SWORDFISH_COMMAND_GET_RGB_IMAGE_SENSOR_EXPOSURE_RETURN,				  /**< return rgb image sensor exposure 53  */
		SWORDFISH_COMMAND_SET_RGB_IMAGE_SENSOR_EXPOSURE_COMMAND = 0x36,		  /**< set rbg image sensor exposure 54  */
		SWORDFISH_COMMAND_SET_RGB_IMAGE_SENSOR_EXPOSURE_RETURN,				  /**< return set rgb image sensor exposure 55  */
		SWORDFISH_COMMAND_GET_RGB_IMAGE_SENSOR_AWB_COMMAND = 0x38,			  /**< get rgb image sensor awb 56  */
		SWORDFISH_COMMAND_GET_RGB_IAMGE_SENSOR_AWB_RETURN,					  /**< return get rgb image sensor awb 57  */
		SWORDFISH_COMMAND_SET_RGB_IMAGE_SENSOR_AWB_COMMAND = 0x3A,			  /**< set rgb image sensor awb 58  */
		SWORDFISH_COMMAND_SET_RGB_IMAGE_SENSOR_AWB_RETURN,					  /**< return rgb image sensor awb 59  */
		SWORDFISH_COMMAND_GET_SYS_TIME_COMMAND = 0x3E,
   		SWORDFISH_COMMAND_GET_SYS_TIME_RETURN,
		SWORDFISH_COMMAND_GET_IMU_CONFIGURATION_COMMAND = 0x90,	  /**< get imu configuration 144  */
		SWORDFISH_COMMAND_GET_IMU_CONFIGURATION_RETURN,			  /**< return get imu configuration 145  */
		SWORDFISH_COMMAND_SET_IMU_CONFIGURATION_COMMAND = 0x92,	  /**< set imu configuration 146  */
		SWORDFISH_COMMAND_SET_IMU_CONFIGURATION_RETURN,			  /**< set imu configuration 147  */
		SWORDFISH_COMMAND_GET_DEPTH_CONFIGURATION_COMMAND = 0x94, /**< get depth config 148  */
		SWORDFISH_COMMAND_GET_DEPTH_CONFIGURATION_RETURN,		  /**< return get depth config 149  */
		SWORDFISH_COMMAND_SET_DEPTH_CONFIGURATION_COMMAND = 0x96, /**< set depth config 150  */
		SWORDFISH_COMMAND_SET_DEPTH_CONFIGURATION_RETURN,		  /**< return set depth config 151  */

		SWORDFISH_COMMAND_GET_RUN_CONFIGURATION_COMMAND = 0x98, /**< get run config 152  */
		SWORDFISH_COMMAND_GET_RUN_CONFIGURATION_RETURN,			/**< return get run config 153  */
		SWORDFISH_COMMAND_SET_RUN_CONFIGURATION_COMMAND = 0x9A, /**< set run config 154  */
		SWORDFISH_COMMAND_SET_RUN_CONFIGURATION_RETURN,			/**< return set run config 155  */

		SWORDFISH_COMMAND_SAVE_CONFIGURATION_COMMAND = 0x9C, /**< save config 156  */
		SWORDFISH_COMMAND_SAVE_CONFIGURATION_RETURN,		 /**< return save config 157  */

		SWORDFISH_COMMAND_SWITCH_TO_FACTORY_COMMAND = 0x9E,			   /**< switch factory mode 158  */
		SWORDFISH_COMMAND_SWITCH_TO_FACTORY_RETURN,					   /**< switch factory mode return 159  */
		SWORDFISH_COMMAND_SET_IR_DATA_SOURCE_COMMAND = 0xA0,		   /**< set ir image data source, 0-vi 1-vpss,after rectify 160  */
		SWORDFISH_COMMAND_SET_IR_DATA_SOURCE_RETURN,				   /**< return set ir image data source, 0-vi 1-vpss,after rectify 161  */
		SWORDFISH_COMMAND_SET_DEPTH_DATA_SOURCE_COMMAND = 0xA2,		   /**< set depth image data source, 0-DISPARITY raw, 1-DEPTH raw 162  */
		SWORDFISH_COMMAND_SET_DEPTH_DATA_SOURCE_RETURN,				   /**< return set depth image data source, 0-DISPARITY raw, 1-DEPTH raws 163  */
		SWORDFISH_COMMAND_USB_IMG_TRANSFER_COMMAND = 0xA4,			   /**< usb image tarnsfer 164  */
		SWORDFISH_COMMAND_USB_IMG_TRANSFER_RETURN,					   /**< return  usb image tarnsfer 165  */
		SWORDFISH_COMMAND_GET_CAM_PARAM_COMMAND = 0xA6,				   /**< get camera param 166  */
		SWORDFISH_COMMAND_GET_CAM_PARAM_RETURN,						   /**< return get camera param 167  */
		SWORDFISH_COMMAND_USB_IMU_TRANSFER_COMMAND = 0xA8,			   /**<imu transfer 168  */
		SWORDFISH_COMMAND_USB_IMU_TRANSFER_RETURN,					   /**< return imu transfer 169  */
		SWORDFISH_COMMAND_USB_PIPELINE_COMMAND = 0xAA,				   /**< start/close pipeline 170  */
		SWORDFISH_COMMAND_USB_PIPELINE_RETURN,						   /**< return start/close pipeline 171  */
		SWORDFISH_COMMAND_USB_IMG_DEPTH_IR_TRANSFER_COMMAND = 0xAC,	   /**< image depth ir transfer 172  */
		SWORDFISH_COMMAND_USB_IMG_DEPTH_IR_TRANSFER_RETURN,			   /**< return image depth ir transfer 173  */
		SWORDFISH_COMMAND_USB_IMG_DEPTH_DEPTH_TRANSFER_COMMAND = 0xAE, /**< image depth depth transfer 174  */
		SWORDFISH_COMMAND_USB_IMG_DEPTH_DEPTH_TRANSFER_RETURN,		   /**< return image depth depth transfer 175  */

		SWORDFISH_COMMAND_USB_IMG_DEPTH_RGB_TRANSFER_COMMAND = 0xB0,  /**< image depth rgb transfer 176  */
		SWORDFISH_COMMAND_USB_IMG_DEPTH_RGB_TRANSFER_RETURN,		  /**< return image depth rgb transfer 177  */
		SWORDFISH_COMMAND_USB_IMG_POINTCLOUD_TRANSFER_COMMAND = 0xB2, /**< image pointcloud transfer 178  */
		SWORDFISH_COMMAND_USB_IMG_POINTCLOUD_TRANSFER_RETURN,		  /**< return image pointcloud transfer 179  */

		SWORDFISH_COMMAND_SET_EDGE_FILTER_PARAM = 0xB4, /**< set edge filter param 180  */
		SWORDFISH_COMMAND_SET_EDGE_FILTER_PARAM_RETURN, /**< set edge filter param return 181  */
		SWORDFISH_COMMAND_GET_CONFIDENCE_PARAM = 0xB6,	/**< get confidence param 182  */
		SWORDFISH_COMMAND_GET_CONFIDENCE_PARAM_RETURN,	/**< get confidence param return 183  */
		SWORDFISH_COMMAND_SET_CONFIDENCE_PARAM = 0xB8,	/**< set confidence param 184  */
		SWORDFISH_COMMAND_SET_CONFIDENCE_PARAM_RETURN,	/**< set confidence param return 185  */
		SWORDFISH_COMMAND_SET_SYS_TIME_COMMAND = 0xBA,	/**< set sys time 186  */
		SWORDFISH_COMMAND_SET_SYS_TIME_COMMAND_RETURN,	/**< set sys time return 187  */

		SWORDFISH_COMMAND_GET_IMU_INTERNAL_REFRRENCE = 0xBC, /**< not used,get imu internal reference 188  */
		SWORDFISH_COMMAND_GET_IMU_INTERNAL_REFRRENCE_RETURN, /**< not used,get imu internal reference return 189  */

		SWORDFISH_COMMAND_GET_IP_ADDRESS_PARAM = 0xC0, /**< get ip address 192  */
		SWORDFISH_COMMAND_GET_IP_ADDRESS_PARAM_RETURN, /**< get ip address return  193*/
		SWORDFISH_COMMAND_SET_IP_ADDRESS_PARAM = 0xC2, /**< set ip address 194*/
		SWORDFISH_COMMAND_SET_IP_ADDRESS_PARAM_RETURN, /**< set ip address return 195  */

		SWORDFISH_COMMAND_SET_FRAME_RATE_PARAM = 0xC4, /**< set fps ratio 196  */
		SWORDFISH_COMMAND_SET_FRAME_RATE_PARAM_RETURN, /**<set fps ratio  return 197  */

		SWORDFISH_COMMAND_GET_EDGE_FILTER_PARAM = 0xC6, /**< get edge filter param 198  */
		SWORDFISH_COMMAND_GET_EDGE_FILTER_PARAM_RETURN, /**< get edge filter param return 199  */

		SWORDFISH_COMMAND_SET_LIGHT_FILTER_PARAM = 0xC8, /**< set light filter param 200  */
		SWORDFISH_COMMAND_SET_LIGHT_FILTER_PARAM_RETURN, /**< set light filter param return 201  */
		SWORDFISH_COMMAND_GET_LIGHT_FILTER_PARAM = 0xCA, /**< get light filter param 202  */
		SWORDFISH_COMMAND_GET_LIGHT_FILTER_PARAM_RETURN, /**< get light filter param return 203  */

		SWORDFISH_COMMAND_SET_GOOD_FEATURE_PARAM = 0xCC, /**< set good feature param 204  */
		SWORDFISH_COMMAND_SET_GOOD_FEATURE_PARAM_RETURN, /**< set good feature param return 205  */

		SWORDFISH_COMMAND_SET_BAD_FILTER_PARAM = 0xCE, /**< set bad filter param 206  */
		SWORDFISH_COMMAND_SET_BAD_FILTER_PARAM_RETURN, /**< set bad filter param return 207  */
		SWORDFISH_COMMAND_GET_BAD_FILTER_PARAM = 0xD0, /**< get bad filter param 208  */
		SWORDFISH_COMMAND_GET_BAD_FILTER_PARAM_RETURN, /**< get bad filter param return 209  */

		SWORDFISH_COMMAND_SET_HIGH_PRECISION_PARAM = 0xD2, /**< set high precision param 210  */
		SWORDFISH_COMMAND_SET_HIGH_PRECISION_PARAM_RETURN, /**< set high precision param return 211  */
		SWORDFISH_COMMAND_GET_HIGH_PRECISION_PARAM = 0xD4, /**< get high precision param 212  */
		SWORDFISH_COMMAND_GET_HIGH_PRECISION_PARAM_RETURN, /**< get high precision param return 213  */

		SWORDFISH_COMMAND_SET_RGB_ELSE_PARAM = 0xD6, /**< set rgb else param 214  */
		SWORDFISH_COMMAND_SET_RGB_ELSE_PARAM_RETURN, /**< set rgb else param return 215  */
		SWORDFISH_COMMAND_GET_RGB_ELSE_PARAM = 0xD8, /**< get rgb else param 216  */
		SWORDFISH_COMMAND_GET_RGB_ELSE_PARAM_RETURN, /**< get rgb else param return 217  */

		SWORDFISH_COMMAND_GET_GOOD_FEATURE_PARAM = 0xDA, /**< get good feature param 218  */
		SWORDFISH_COMMAND_GET_GOOD_FEATURE_PARAM_RETURN, /**< get good feature param return 219  */

		SWORDFISH_COMMAND_SET_LK_PARAM = 0xDC, /**< set lk optical flow param 220  */
		SWORDFISH_COMMAND_SET_LK_PARAM_RETURN, /**< set lk optical flow param return 221  */
		SWORDFISH_COMMAND_GET_LK_PARAM = 0xDE, /**< get lk optical flow param 222  */
		SWORDFISH_COMMAND_GET_LK_PARAM_RETURN, /**< get lk optical flow param return 223  */

		SWORDFISH_COMMAND_SET_REPEATED_TEXTURE_PARAM = 0xE0,
		SWORDFISH_COMMAND_SET_REPEATED_TEXTURE_PARAM_RETURN,
		SWORDFISH_COMMAND_GET_REPEATED_TEXTURE_PARAM = 0xE2,
		SWORDFISH_COMMAND_GET_REPEATED_TEXTURE_PARAM_RETURN,

	} SWORDFISH_COMMAND_SUB_TYPE;

	/** SWORDFISH_UPGRADE_SUB_TYPE */
	typedef enum
	{
		SWORDFISH_COMMAND_USB_UPGRADE_USER_FILE = 0x00,				 /**< upgrade user file 0  */
		SWORDFISH_COMMAND_USB_UPGRADE_USER_FILE_RETURN,				 /**< upgrade user file return data type 1  */
		SWORDFISH_COMMAND_USB_UPGRADE_USER_CONFIG,					 /**< upgrade user config file 2  */
		SWORDFISH_COMMAND_USB_UPGRADE_USER_CONFIG_RETURN,			 /**< upgrade user config  return data type 3  */
		SWORDFISH_COMMAND_USB_UPGRADE_NEXTVPU_SYSTEM,				 /**< upgrade system 4  */
		SWORDFISH_COMMAND_USB_UPGRADE_NEXTVPU_SYSTEM_RETURN,		 /**< upgrade system return data type 5  */
		SWORDFISH_COMMAND_USB_UPGRADE_NEXTVPU_CONFIG,				 /**< upgrade config 6  */
		SWORDFISH_COMMAND_USB_UPGRADE_NEXTVPU_CONFIG_RETURN,		 /**< upgrade config return data type 7  */
		SWORDFISH_COMMAND_USB_UPGRADE_NEXTVPU_DIFFER_PACKAGE,		 /**< upgrade differ file type 8  */
		SWORDFISH_COMMAND_USB_UPGRADE_NEXTVPU_DIFFER_PACKAGE_RETURN, /**< upgrade differ file return data type 9 */
	} SWORDFISH_UPGRADE_SUB_TYPE;

	/** SWORDFISH_SENSOR_RESOLUTION_TYPE */
	typedef enum
	{
		SWORDFISH_RESOLUTION_1280_800, /**< resolution 1280x800 0 */
		SWORDFISH_RESOLUTION_1280_720, /**< resolution 1280x720 1 */
		SWORDFISH_RESOLUTION_640_480,  /**< resolution 640x480 2 */
		SWORDFISH_RESOLUTION_640_400,  /**< resolution 640x400 3 */
		SWORDFISH_RESOLUTION_320_200,  /**< resolution 320x200 4 */
		SWORDFISH_RESOLUTION_640_360,  /**< resolution 640x360 5 */
		SWORDFISH_RESOLUTION_320_240,
		SWORDFISH_RESOLUTION_960_600,
		SWORDFISH_RESOLUTION_UNKNOWN,
	} SWORDFISH_SENSOR_RESOLUTION_TYPE;

	/** SWORDFISH_DEVICE_DATASUB_TYPE */
	typedef enum
	{
		SWORDFISH_DEVICE_DATA_ALL = 0x00, /**< all data data type 0  */
	} SWORDFISH_DEVICE_DATASUB_TYPE;

	/** SWORDFISH_LOG_DATA_SUB_TYPE */
	typedef enum
	{
		SWORDFISH_LOG_ALL = 0x00, /**< log data type 0  */
	} SWORDFISH_LOG_DATA_SUB_TYPE;

	/** SWORDFISH_CNN_DATASUB_TYPE */
	typedef enum
	{
		SWORDFISH_CNN_DATA_ALL = 0x00, /**< cnn data type 0  */
	} SWORDFISH_CNN_DATASUB_TYPE;

	/** SWORDFISH_USER_DATA_SUB_TYPE */
	typedef enum
	{
		SWORDFISH_USER_DATA_TO_PC = 0X00, /**< user defined data from camera to pc 0  */
		SWORDFISH_USER_DATA_TO_BOARD	  /**< user defined data from pc to camera 1  */
	} SWORDFISH_USER_DATA_SUB_TYPE;

	/** SWORDFISH_CV_DATA_SUB_TYPE */
	typedef enum
	{
		SWORDFISH_CV_DATA_GOOD_FEATURE, /**< good feature data type:0  */
	} SWORDFISH_CV_DATA_SUB_TYPE;

	/** SWORDFISH_DOWNLOAD_SUB_TYPE */
	typedef enum
	{
		SWORDFISH_COMMAND_USB_DOWNLOAD_USER_FILE = 0x00, /**< download user file data type:0 */
		SWORDFISH_COMMAND_USB_DOWNLOAD_USER_FILE_RETURN	 /**< download user file return data type:1 */
	} SWORDFISH_DOWNLOAD_SUB_TYPE;

	/** RGB_AWB_MODE */
	typedef enum
	{
		AWB_AUTO = 0x35,	 /**< auto white balance type:0 */
		AWB_MANUAL,			 /**< manual white balance type:1 */
		AWB_DAY_LIGHT,		 /**< daylight type:2 */
		AWB_CLOUDY,			 /**< cloudy type:3 */
		AWB_INCANDESCENT,	 /**< incandescent type:4 */
		AWB_FLOURESCENT,	 /**< flourescent type:5 */
		AWB_TWILIGHT,		 /**< twilight type:6 */
		AWB_SHADE,			 /**< shade type:7 */
		AWB_WARM_FLOURESCENT /**< warm flourescent type:8 */
	} RGB_AWB_MODE;
	
	typedef struct _group_pkt_info_custom_t
	{
		union{
			unsigned char buffer[USB_PACKET_MAX_SIZE*4];
			struct{
				unsigned char depthbuffer[USB_PACKET_MAX_SIZE];
				unsigned char rgbbuffer[USB_PACKET_MAX_SIZE];
				unsigned char leftirbuffer[USB_PACKET_MAX_SIZE];
				unsigned char rightirbuffer[USB_PACKET_MAX_SIZE];
			};	
		};	
		int len;
	} grouppkt_info_custom_t;

	/**@struct SWORDFISH_CONFIG
	 * @brief resolution and fps,fps ratio of rgb and ir and depth streams struct \n
	 * define resolution and fps,fps ratio of rgb and ir and depth streams,real fps=fps/fpsratio
	 */
	typedef struct
	{
		SWORDFISH_SENSOR_RESOLUTION_TYPE irres;	 /**< ir and depth stream's resolution */
		SWORDFISH_SENSOR_RESOLUTION_TYPE rgbres; /**< rgb stream's resolution */
		SWORDFISH_FPS_E fps;					 /**< fps of ir and depth and rgb streams */
		unsigned int fpsratio;				 /**< fps ratio of ir and depth and rgb streams */
	} SWORDFISH_CONFIG;

	/**@struct SWORDFISH_DEVICE_INFO
	 * @brief feynman camera device info \n
	 * define feynman camera device info,include network interface camera and usb interface camera
	 */
	typedef struct _swordfish_device_info
	{
		SWORDFISH_INTERFACE_TYPE type; /**< camera interface type,network or usb */
		union
		{
			char usb_camera_name[32]; /**< usb camera name,only for identify cameras plugged to current computer */
			char net_camera_ip[32];	  /**< network camera name,this is ip of camera */
		};
		struct _swordfish_device_info *next; /**< pointer to next camera device */
	} SWORDFISH_DEVICE_INFO;

	/**@struct s_swordfish_run_config
	 * @brief feynman camera run config \n
	 * define for SWORDFISH_COMMAND_GET_RUN_CONFIGURATION_COMMAND & SWORDFISH_COMMAND_SET_RUN_CONFIGURATION_COMMAND
	 */
	typedef struct
	{
		int app_run_mode; /**< 0-sensor 1-sensor+rectify 2-depth 3-depth+cnn 4-upgrade */
	} s_swordfish_run_config;

	/**@struct s_swordfish_bad_filter
	 * @brief bad filter config struct \n
	 * define post process of depth in camera,bad filter param
	 */
	typedef struct
	{
		int enable; /**< 0-disable bad filter 1-enable bad filter */
		int kernel; /**< 15 or 17 or 19 supported */
	} s_swordfish_bad_filter;

	/**@struct s_swordfish_switch_to_factory
	 * @brief result of switch camera mode to factory struct \n
	 * define result of switch camera mode to factory
	 */
	typedef struct
	{
		int status; /**< 0-success else-failed */
	} s_swordfish_switch_to_factory;

	/**@struct s_swordfish_confidence
	 * @brief confidence filter config struct \n
	 * define post process of depth in camera,confidence filter param
	 */
	typedef struct
	{
		unsigned int enable; /**< 0-disable confidence filter 1-enable confidence filter */
		float sigma;		 /**< confidence sigma param,[0.0-1.0],recommend value: 0.0392 */
	} s_swordfish_confidence;

	/**@struct s_swordfish_save_config
	 * @brief result of save current config in camera struct \n
	 * define result of save current config in camera
	 */
	typedef struct
	{
		int status; /**< 0-success else-failed */
	} s_swordfish_save_config;

	/**@struct SWORDFISH_USBHeaderDataPacket
	 * @brief usb packet struct \n
	 * define usb communication packet struct
	 */
	typedef struct
	{
		uint8_t magic[8];  /**< "NEXT_VPU" magic word */
		uint16_t type;	   /**< primary type */
		uint16_t sub_type; /**< sub type */
		uint32_t checksum; /**< checksum */
		uint32_t len;	   /**< length of data member in bytes*/
		uint8_t data[0];   /**< pure data,mean depends type and sub_type,length depends len */
	} SWORDFISH_USBHeaderDataPacket;

	/**@struct s_swordfish_device_id
	 * @brief feynman device id struct \n
	 * define feynman device id struct,corresponding SWORDFISH_COMMAND_GET_DEVICE_ID_COMMAND
	 */
	typedef struct
	{
		uint32_t device_id; /**< device id of feynman */
	} s_swordfish_device_id;

	/**@struct s_swordfish_device_sn
	 * @brief feynman device sn struct \n
	 * define feynman device sn struct,corresponding SWORDFISH_COMMAND_GET_DEVICE_SN_COMMAND
	 */
	typedef struct
	{
		uint8_t sn[16]; /**< device sn of feynman */
	} s_swordfish_device_sn;

	/**@struct s_swordfish_current_infor
	 * @brief feynman device projector status \n
	 * define feynman device projector status,corresponding SWORDFISH_COMMAND_GET_PROJECTOR_CURRENT_COMMAND/SWORDFISH_COMMAND_SET_PROJECTOR_CURRENT_COMMAND
	 */
	typedef struct
	{
		uint32_t crruent_value; /**< projector status of feynman device,0-shutdown projector >0-open projector and set current value */
	} s_swordfish_current_infor;

	/**@struct s_swordfish_depth_config
	 * @brief feynman device depth config struct \n
	 * define feynman device depth config,corresponding SWORDFISH_COMMAND_GET_DEPTH_CONFIGURATION_COMMAND/SWORDFISH_COMMAND_SET_DEPTH_CONFIGURATION_COMMAND
	 */
	typedef struct
	{
		int app_depth_mode;	   /**< depth mode,value:0-whole map,1-whole map(advance),2-cover removal(advance),3-cover removal(advance) */
		int app_depth_denoise; /**< depth denoise,value:0-denoise disable, 1-denoise enable */
		int app_depth_fusion;  /**< depth fusion,value:0-fusion disable, 1-fusion enable, 3-fusion3 enable */
		int app_depth_zoom;	   /**< depth zoom,value:0-zoom disable,1-zoom enable */
		int app_depth_stitch;  /**< depth stitch,value:0-stitch disable,1-stitch enable */
	} s_swordfish_depth_config;

	/**@struct s_swordfish_sensor_resolution_fps_infor
	 * @brief feynman device sensor resolution and fps information struct \n
	 * define feynman device sensor resolution and fps information,corresponding SWORDFISH_COMMAND_GET_IR_IMAGE_SENSOR_RESOLUTION_FPS_COMMAND/SWORDFISH_COMMAND_SET_IR_IMAGE_SENSOR_RESOLUTION_FPS_COMMAND
	 */
	typedef struct
	{
		uint32_t ir_resolution;	 /**< ir resolution */
		uint32_t fps;			 /**< frames per second */
		uint32_t rgb_resolution; /**< rgb resolution */
	} s_swordfish_sensor_resolution_fps_infor;

	/**@struct s_swordfish_rgb_sensor_exposure_infor
	 * @brief feynman device rgb sensor exposure information struct \n
	 * define feynman device rgb sensor exposure information,corresponding SWORDFISH_COMMAND_GET_RGB_IMAGE_SENSOR_EXPOSURE_COMMAND/SWORDFISH_COMMAND_SET_RGB_IMAGE_SENSOR_EXPOSURE_COMMAND
	 */
	typedef struct
	{
		uint32_t exposure_mode;	   /**< 0-auto exposure 1-manual exposure */
		int32_t exposure_time;	   /**< exposure time in us,>=0 valid,<0 not set exposure time */
		int32_t digital_gain;	   /**< exposure digital gain,>=0 valid,<0 not set exposure digital gain */
		int32_t AE_compensation_id;   //128 一般在 96-160之间
		int32_t u32AE_tail_weight;    //30 范围0-100	
		int32_t max_exposure_time; // 0-不设置该项
		int32_t max_again;		   // 0-不设置该项
	} s_swordfish_rgb_sensor_exposure_infor;

	/**@struct s_swordfish_rgb_awb_infor
	 * @brief feynman device rgb awb information struct \n
	 * define feynman device rgb awb information,corresponding SWORDFISH_COMMAND_GET_RGB_IMAGE_SENSOR_AWB_COMMAND/SWORDFISH_COMMAND_SET_RGB_IMAGE_SENSOR_AWB_COMMAND
	 */
	typedef struct
	{
		RGB_AWB_MODE awb_mode; /**< should be [54~57] */
		uint32_t rgain;		   /**< should be [52~512] */
		uint32_t bgain;		   /**< should be [52~512] */
	} s_swordfish_rgb_awb_infor;

	/**@struct s_swordfish_ir_img_source
	 * @brief feynman device ir image source information struct \n
	 * define feynman device ir image source information
	 */
	typedef struct
	{
		int source; /**< 0-sensor  1-sensor or ir data after rectify */
	} s_swordfish_ir_img_source;

	/**@struct s_swordfish_depth_img_source
	 * @brief feynman device depth image source struct \n
	 * define feynman device depth image source
	 */
	typedef struct
	{
		int source; /**< 0-disparity  1-depth 16bit raw */
	} s_swordfish_depth_img_source;

	/**@struct s_swordfish_imu_transfer
	 * @brief feynman device imu transfer status struct \n
	 * define feynman device  imu transfer status
	 */
	typedef struct
	{
		int status; /**< 0-stop transfer, 1-transfer*/
	} s_swordfish_imu_transfer;

	/**@struct s_swordfish_light_filter
	 * @brief light filter struct \n
	 * define post process in camera,light filter params
	 */
	typedef struct
	{
		unsigned int enable; /**< 0-disable light filter, 1-enable light filter*/
		float sigma;		 /**< recommend 1.0 */
		int pixel_threshold; /**< recommend 200 */
		int erode_size;		 /**< recommend 9*/
		int kernel_H;		 /**< recommend 5*/
		int kernel_W;		 /**< recommend 21*/
	} s_swordfish_light_filter;

	/**@struct s_swordfish_good_feature
	 * @brief good feature param struct \n
	 * define config good feature search function in camera
	 */
	typedef struct
	{
		int enable;			/**< 0-disable good feature, 1-enable good feature*/
		int maxCorners;		/**< recommend 200*/
		float qualityLevel; /**< recommend 0.01 */
		float minDistance;	/**< recommend 30.0 */
	} s_swordfish_good_feature;

	/**@struct s_swordfish_cam_param
	 * @brief feynman device camera param struct \n
	 * define feynman device camera param
	 */
	typedef struct
	{
		int img_width;		   /**< image width */
		int img_height;		   /**< image height */
		int left_right_switch; /**< left right switch */
		int is_new_format;	   /**< 0-old calibration file format  1-new calibration file format */

		char camera_model[16]; /**< camera model */
		int camera_number;	   /**<camera number */

		float left_sensor_focus[2];		  /**< [0] - focal in x, [1] - focal in y */
		float left_sensor_photocenter[2]; /**< [0] - photo center in x  [1]- photo center in y */

		float right_sensor_focus[2];	   /**< [0] - focal in x,, [1] - focal in y */
		float right_sensor_photocenter[2]; /**< [0] -  photo center in x  [1]-photo center in y */

		float rgb_sensor_focus[2];		 /**< [0] - focal in x,, [1] - focal in y */
		float rgb_sensor_photocenter[2]; /**< [0] -  photo center in x  [1]-photo center in y */

		float left2right_extern_param[12]; /**< to new calibration format，baseline is eft2right_extern_param[9]，to old calibration format，baseline is left2right_extern_param[3] */
		float left2rgb_extern_param[12];   /**< left and rgb extern params */
		int cam_param_valid;			   /**< 1-valid,0-not valid */
		int rgb_distortion_valid;		   /**< rgb distortion param valid or not */
		float rgb_distortion[5];		   /**< rgb distortion param:K1K2P1P2K3 */
	} s_swordfish_cam_param;

	/**@struct IMU_CALIBRATION_DATA
	 * @brief feynman device imu data struct \n
	 * define feynman device imu data
	 */
	typedef struct
	{
		NVP_FLOAT s16X; /**< value in x */
		NVP_FLOAT s16Y; /**< value in y */
		NVP_FLOAT s16Z; /**< value in z */
	} IMU_CALIBRATION_DATA;

	/**@struct IC20948_CALIBRATION_DATA_STRUC
	 * @brief feynman device imu data struct \n
	 * define feynman device imu data
	 */
	typedef struct
	{
		IMU_CALIBRATION_DATA stGyroRawData;	 /**< gyro value */
		IMU_CALIBRATION_DATA stAccelRawData; /**< accel value */
		IMU_CALIBRATION_DATA stMagnRawData;	 /**< mag value */
		NVP_FLOAT fTemCaliData;				 /**< temperature value */
		NVP_U64 timestamp;					 /**< timestamp in microseconds */
	} IC20948_CALIBRATION_DATA_STRUC;

	/**@struct s_swordfish_imu_data
	 * @brief feynman device imu data struct \n
	 * define feynman device imu data
	 */
	typedef struct
	{
		uint32_t data_type;							  /**<  bit0:gyro,bit1:accel,bit2:MANG ,bit3:temp.0-disable,1-enable */
		uint32_t data_number;						  /**< imu data packets number */
		IC20948_CALIBRATION_DATA_STRUC imu_data[128]; /**< imu data packets */
	} s_swordfish_imu_data;

	/**@struct s_swordfish_upgrade_data
	 * @brief feynman device upgrade data struct \n
	 * define feynman device upgrade data struct
	 */
	typedef struct
	{
		uint32_t packet_numbers;	  /**< total packets number */
		uint32_t curr_packet_numbers; /**< current packet index */
		uint32_t data_len;			  /**< data length */
		char path[256];				  /**< path in camera,just like "/nextvpu/bin/arm" */
		char name[100];				  /**< file name in camera,just like "feynman" */
		char data[0];				  /**< data */
	} s_swordfish_upgrade_data;

	/**@struct s_swordfish_upgrade_result
	 * @brief feynman device upgrade result struct \n
	 * define feynman device upgrade result struct
	 */
	typedef struct
	{
		int result; /**< -2can not upgrade this file，-1upgrade failed，0-current packet upgrade success，  1-upgrade complete */
	} s_swordfish_upgrade_result;

	/**@struct s_swordfish_device_info
	 * @brief feynman device info struct \n
	 * define feynman device info struct
	 */
	typedef struct
	{
		int sensor_type;						 /**< 0-normal 1-wide-angle */
		float cpu_temperature;					 /**< CPU temperature */
		float projector_temperature[2];			 /**< 0-projector1 data, 1-projector2 data */
		s_swordfish_device_id device_id;			 /**< device id */
		s_swordfish_device_sn device_sn;			 /**< device sn */
		s_swordfish_sensor_resolution_fps_infor fps; /**< resolution and fps of sensors */
		int depth_ir_transfer;					 /**< 0-stop transfer, 1-start transfer */
		int depth_depth_transfer;				 /**<0-stop transfer, 1-start transfer */
		int imu_transfer;						 /**< 0-stop transfer, 1-start transfer */
		int image_freq;							 /**< image freqency */
		s_swordfish_run_config app_run_mode;		 /**< camera run mode */
		char usb_speed[8];						 /**< usb speed */
		char software_version[64];				 /**< camera firmware version */
		char usb_proto_version[16];				 /**< usb proto version */
		char product_type[16];					 /**< product type */
		int sensor_number;						 /**< sensor number */
		int has_network;						 /**< camera has network interface or not,0-not 1-has */
		char ip_addr[16];						 /**< camera ip address */
		char cpu_net_type[64];					 /**< cpu type and net sub board type */
		int trigger_source;						 /**< trigger source type,same to trigger_source in s_swordfish_trigger_source */
		int rgb_rectify_mode;					 /**< same to rgb_rectify_mode in s_swordfish_rgb_else_param */
	} s_swordfish_device_info;

	/**@struct s_swordfish_ir_sensor_exposure_infor
	 * @brief feynman device ir sensor exposure infor struct \n
	 * define feynman device ir sensor exposure infor struct
	 */
	typedef struct
	{
		uint32_t exposure_mode;		   /**< 0-auto exposure,  1-manual exposure */
		int32_t exposure_time[2];	   /**< >=0:exposure time in microseconds,<0:ignore */
		int32_t digital_gain[2];	   /**< >=0:exposure digital gain,<0:ignore */
		int32_t exp_ratio;			   /**< exposure ratio,10~25 */
									   /* 以下仅对自动曝光有效 */
		int32_t AE_compensation_id[2]; /**< auto exposure compensation, recommend:128,96~160 */
		int32_t u32AE_tail_weight[2];  // 30 范围0-100
		int32_t max_exposure_time[2];  // 0-不设置该项
		int32_t max_again[2];		   // 0-不设置该项
	} s_swordfish_ir_sensor_exposure_infor;

	/**@struct s_swordfish_cnn_sub_data
	 * @brief feynman cnn sub data infor struct \n
	 * define feynman cnn sub data infor struct
	 */
	typedef struct
	{
		int label;	 /**< label value of cnn result */
		float score; /**< score value of cnn result */
		float ymin;	 /**< bbox y coordination of left top point */
		float xmin;	 /**< bbox x coordination of left top point */
		float ymax;	 /**< bbox y coordination of right bottom point */
		float xmax;	 /**< bbox x coordination of right bottom point */
	} s_swordfish_cnn_sub_data;

	/**@struct s_swordfish_cnn_data
	 * @brief feynman cnn data infor struct \n
	 * define feynman cnn data infor struct
	 */
	typedef struct
	{
		int frame_id;					/**< frame id of cnn result */
		int groups;						/**< groups sum of cnn result */
		s_swordfish_cnn_sub_data group[10]; /**< groups of cnn result */
	} s_swordfish_cnn_data;

	/**@struct s_swordfish_cv_good_feature
	 * @brief feynman good feature struct \n
	 * define feynman good feature struct，idendify one feature point
	 */
	typedef struct
	{
		int index; /**< index of good feature point,for track good feature point in frames */
		float x;   /**< pos in x */
		float y;   /**< pos in y */
		float z;   /**< pos in z,current ignore */
	} s_swordfish_cv_good_feature;

	/**@struct s_swordfish_cv_data_good_feature
	 * @brief feynman good feature data struct \n
	 * define feynman good feature data struct,identify one frame good features
	 */
	typedef struct
	{
		int frame_id;							   /**< frame id of current frame,same to rgb frame */
		int good_feature_number;				   /**< total good feature number in one frame*/
		s_swordfish_cv_good_feature good_feature[400]; /**< good features */
	} s_swordfish_cv_data_good_feature;

	/**@struct s_swordfish_download_data
	 * @brief feynman download data struct \n
	 * define feynman download data struct
	 */
	typedef struct
	{
		char file_path[256]; /**< remote camera file path */
	} s_swordfish_download_data;

	/**@struct s_swordfish_download_bidata
	 * @brief feynman download bidirectional data struct \n
	 * define feynman download bidirectional data struct
	 */
	typedef struct
	{
		s_swordfish_download_data downloaddata; /**< remote camera file path */
		int len;							/**< receive data len */
		char data[0];						/**< receive data */
	} s_swordfish_download_bidata;

	/**@struct s_swordfish_download_result
	 * @brief feynman download result struct \n
	 * define feynman download result struct
	 */
	typedef struct
	{
		int result;	  /**< -3 memory insufficient -2 file open failed -1 file not exist 0 success */
		char data[0]; /**< receive data */
	} s_swordfish_download_result;

	/**@struct s_swordfish_trigger_source
	 * @brief feynman trigger source struct \n
	 * define feynman trigger source struct
	 */
	typedef struct
	{
		int trigger_source; /**< 0-normal 1-hardware trigger  2-soft trigger */
	} s_swordfish_trigger_source;

	/**@struct s_swordfish_software_trigger
	 * @brief feynman software trigger struct \n
	 * define feynman software trigger struct
	 */
	typedef struct
	{
		int trigger_delay;		  /**< delay microseconds before trigger in camera */
		int trigger_image_number; /**< trigger image numbers,<0:always trigger,working status,=0:stop trigger,>0:trigger for trigger_image_number images */
		char reserved[32];		  /**< reserved */
	} s_swordfish_software_trigger;

	/**@struct s_swordfish_hardware_trigger
	 * @brief feynman hardware trigger struct \n
	 * define feynman hardware trigger struct
	 */
	typedef struct
	{
		int trigger_edge;		  /**< 0-trigger at falling edge,1-trigger at rising edge */
		int debounce_time;		  /**< debound time in microseconds */
		int trigger_delay;		  /**< trigger delay in microseconds */
		int trigger_image_number; /**< trigger image number in single io trigger signal */
		char reserved[32];		  /**< reserved */
	} s_swordfish_hardware_trigger;

	/**@struct s_swordfish_pipeline_cmd
	 * @brief feynman start/stop pipeline infor struct \n
	 * define feynman start/stop pipeline infor struct
	 */
	typedef struct
	{
		int status; /**< for SWORDFISH_COMMAND_USB_PIPELINE_COMMAND,0-do nothing, 1-start pipeline, 2-close pipeline, 3-restart pipeline, 4-restart application,5-system reboot */
	} s_swordfish_pipeline_cmd;

	/**@struct s_swordfish_edge_filter_cmd
	 * @brief feynman edge filter param struct \n
	 * define feynman edge filter param struct,enable or disable and set edge filter params
	 */
	typedef struct
	{
		unsigned int enable;			/**< 1-enable  0-disable */
		unsigned int left_ir_point[4];	/**< [0]-top left x [1]-top left y [2]-bottom right x [3]-bottom right y */
		unsigned int right_ir_point[4]; /**< [0]-top left x [1]-top left y [2]-bottom right x [3]-bottom right y */
		unsigned int rgb_point[4];		/**< [0]-top left x [1]-top left y [2]-bottom right x [3]-bottom right y */
		unsigned int depth_point[4];	/**< [0]-top left x [1]-top left y [2]-bottom right x [3]-bottom right y */
	} s_swordfish_edge_filter_cmd;

	/**@struct s_swordfish_img_transfer
	 * @brief feynman start/stop transfer infor struct \n
	 * define feynman start/stop transfer infor struct
	 */
	typedef struct
	{
		int status; /**< for SWORDFISH_COMMAND_USB_IMG_TRANSFER_COMMAND,0-stop transfer, 1-start transfer */
	} s_swordfish_img_transfer;

	/**@struct s_swordfish_img_depth_ir_transfer
	 * @brief feynman depth and ir start/stop transfer infor struct \n
	 * define feynman depth and ir start/stop transfer infor struct
	 */
	typedef struct
	{
		int status; /**< for SWORDFISH_COMMAND_USB_IMG_DEPTH_IR_TRANSFER_COMMAND,0-stop transfer, 1-start transfer */
	} s_swordfish_img_depth_ir_transfer;

	/**@struct s_swordfish_img_depth_depth_transfer
	 * @brief feynman depth start/stop transfer infor struct \n
	 * define feynman depth start/stop transfer infor struct
	 */
	typedef struct
	{
		int status; /**< for SWORDFISH_COMMAND_USB_IMG_DEPTH_DEPTH_TRANSFER_COMMAND,0-stop transfer, 1-start transfer */
	} s_swordfish_img_depth_depth_transfer;

	/**@struct SWORDFISH_USB_IMAGE_HEADER
	 * @brief feynman image data packet information struct \n
	 * define  feynman image data packet information struct,before real image data
	 */
	typedef struct
	{
		uint32_t group_id;	   /**< group_id of data frame,same group_id data frame captured at the same moment */
		uint32_t width;		   /**< width of image data */
		uint32_t height;	   /**< height of image data */
		uint64_t timestamp;	   /**< timestamp of data */
		uint32_t exposure;	   /**< exposure time in microseconds */
		uint32_t reserved[15]; /**< reserved for later data definition */
	} SWORDFISH_USB_IMAGE_HEADER;

	/**@struct s_swordfish_img_depth_rgb_transfer
	 * @brief feynman rgb start/stop transfer infor struct \n
	 * define feynman rgb start/stop transfer infor struct
	 */
	typedef struct
	{
		int status; /**< 0-stop transfer, 1-start transfer */
	} s_swordfish_img_depth_rgb_transfer;

	/**@struct s_swordfish_img_pointcloud_transfer
	 * @brief feynman point cloud start/stop transfer infor struct \n
	 * define feynman point cloud start/stop transfer infor struct
	 */
	typedef struct
	{
		int status; /**< 0-stop transfer, 1-start transfer */
	} s_swordfish_img_pointcloud_transfer;

	/**@struct s_swordfish_img_pointcloud_transfer
	 * @brief sync time received from camera struct \n
	 * define for FEYNMAN_COMMAND_SET_SYS_TIME_COMMAND_RETURN
	 */
	typedef struct
	{
		NVP_U64 local_time_no_calib; /**< camera local time when received sync time from upper computer,uncalibrated */
		NVP_U64 pc_time;			 /**< recevied sync time from upper computer */
		NVP_U64 local_time_calib;	 /**< camera local time,calibrated */
	} s_swordfish_time_ret;
	
	typedef struct
	{
	   NVP_U64 local_time;  //未经校准的本地时间
	   NVP_U64 local_time_calib;//当前本地时间，经校准
	}s_swordfish_time_info;
	
	/**@struct s_swordfish_imu_info
	 * @brief imu get/set information struct \n
	 * define  for FEYNMAN_COMMAND_SET_IMU_CONFIGURATION_COMMAND
	 */
	typedef struct
	{
		int imu_type;		  /**< 0-IMU20948 1-IMU42607,  only valid when this is from camera to upper computer */
		int imu_data_type;	  /**< bit0 ：0-ACCEL disable，1-ACCEL enable,bit1 ：0-GYRO disable，1-GYRO enable,bit3 ：0-temperature disable，1-temperature enable */
		int imu_fps;		  /**< IMU data acquisition frequency，IMU20948 only support 220Hz，IMU42607 support 200Hz or 400Hz */
		int imu_transfer_gap; /**< 0:transfer with image >0：transfer gap time，unit：10ms */
	} s_swordfish_imu_info;

	/**@struct s_swordfish_imu_internal_reference
	 * @brief imu internal reference struct \n
	 * define imu internal reference struct
	 */
	typedef struct
	{
		float accel_m[9]; /**< imu's accel matrix */
		float accel_b[3]; /**< imu's accel bias */
		float gyro_b[3];  /**< imu's gyro bias */
	} s_swordfish_imu_internal_reference;

	/**@struct s_swordfish_imu_external_reference
	 * @brief imu external reference struct \n
	 * define imu external reference struct
	 */
	typedef struct
	{
		float t_cam_imu[16];	  /**< rgb camera to imu external ref matrix 4x4 */
		float acc_noise_density;  /**< accel noise density */
		float acc_random_walk;	  /**< acc random walk value */
		float gyro_noise_density; /**< gyro noise density */
		float gyro_random_walk;	  /**< gyro random walk value */
		float timeshift_cam_imu;
	} s_swordfish_imu_external_reference;

	/**@struct s_swordfish_lk
	 * @brief LK optical flow param struct \n
	 * define LK optical flow param struct
	 */
	typedef struct
	{
		int enable;			/**< 0-disable,1-enable */
		int win_width;		/**< window width */
		int win_height;		/**< window height */
		int point_limt;		/**< point limit */
		int maxCorners;		/**< max corners */
		float qualityLevel; /**< quality level */
		float minDistance;	/**< minimum distance*/
	} s_swordfish_lk;

	// for FEYNMAN_COMMAND_SET_HIGH_PRECISION_PARAM & FEYNMAN_COMMAND_GET_HIGH_PRECISION_PARAM
	typedef struct
	{
		int enable;
	} s_swordfish_high_precision;

	typedef struct
	{
		int enable;		 // 1:enable repeat texture filter,0:disable repeat texture filter
		int winSize;	 // default 15,curently ignored
		float threshold; // 0~1,default 1,currently ignored
	} s_swordfish_repeated_texture_filter;

	typedef struct
	{
		int rgb_format_on_dsp; // 0-YUV_NV12 1-YUV_422_PACKED
		int rgb_rectify_mode;  // 0-rgb正常校准 1-rgb保持黑边 2-rgb不校准
	} s_swordfish_rgb_else_param;

	typedef void (*FRAMECALLBACK)(void *data, void *userdata);
	typedef void (*EVENTCALLBACK)(void *userdata);
	typedef void (*DEVICECALLBACK)(const char *devicename, void *userdata);

	/**
	 * @brief		initial sdk
	 *
	 * @note	Call it once at begining of application.
	 * @return
	 *	 0 : Success \n
	 *	 -1 : Failed \n
	 * @par Sample
	 * @code
	 *	if(0==swordfish_init()){
	 *	 printf("ok to init sdk!\n");
	 *	}else{
	 *	 printf("fail to init sdk!\n");
	 *	 exit(0);
	 *  };
	 * @endcode
	 */
	int swordfish_init(const char* logfile,int logfilemaxsize);
	void swordfish_debug_printf(const char* fmt,...);
	void swordfish_info_printf(const char* fmt,...);
	void swordfish_warn_printf(const char* fmt,...);
	void swordfish_error_printf(const char* fmt,...);


	/**
	 * @brief		deinitial sdk
	 *
	 * @note	Call it once when application exit.
	 * @return
	 *	none \n
	 * @par Sample
	 * @code
	 *	swordfish_deinit();
	 * @endcode
	 */
	void swordfish_deinit();

	/**
	 * @brief		enum feynman devices
	 * @param[inout]	ptotal : int pointer,will output device numbers enumerated
	 * @param[inout]	ppdevices : pointer of pointer of SWORDFISH_DEVICE_INFO type,will output link list of devices info enumerated
	 * @note	Call it to enum feynman camera device,include usb interface and network interface
	 * @return
	 *	 SWORDFISH_OK: Success \n
	 *	 else : Failed \n
	 * @par Sample
	 * @code
	 *  int total=0;
	 *  SWORDFISH_DEVICE_INFO* pdevices=NULL;
	 *	if(SWORDFISH_OK==swordfish_enum_device(&total,&pdevices)){
	 *	 printf("ok to enumerate feynman cameras!\n");
	 *	}else{
	 *	 printf("fail to enumerate feynman cameras!\n");
	 *  };
	 * @endcode
	 */
	SWORDFISH_RESULT swordfish_enum_device(int *ptotal, SWORDFISH_DEVICE_INFO **ppdevices);

	/**
	 * @brief		get frame data of stream which type is streamtype
	 * @param[in]	handle : SWORDFISH_DEVICE_HANDLE return by swordfish_open
	 * @param[in]	steamtype : stream type,imu/rgb/depth/ir/good feature etc.
	 * @param[inout]	ppframe : pointer of pointer of SWORDFISH_USBHeaderDataPacket type,will output packet data received
	 * @param[in]	timeout : timeout in milliseconds
	 * @note	Call it to get frame data of stream which type is streamtype,block until data is received or timeout
	 * @return
	 *	 SWORDFISH_OK: Success \n
	 *	 else : Failed \n
	 * @par Sample
	 * @code
	 *  SWORDFISH_USBHeaderDataPacket* pframe=NULL;
	 *	if(SWORDFISH_OK==swordfish_get_frame(handle,SWORDFISH_STREAM_DEPTH,&pframe,2000)){
	 *	 printf("ok to get frame data from feynman cameras!\n");
	 *	}else{
	 *	 printf("fail to get frame data from feynman cameras!\n");
	 *  };
	 * @endcode
	 */
	SWORDFISH_RESULT swordfish_get_frame(SWORDFISH_DEVICE_HANDLE handle, SWORDFISH_STREAM_TYPE_E steamtype, SWORDFISH_USBHeaderDataPacket **ppframe, int timeout);

	/**
	 * @brief		open feynman camera and return handle for subsequence call
	 * @param[in]	dev_info : SWORDFISH_DEVICE_INFO got by swordfish_enum_device
	 * @note	Call it to open feynman camera which device info is dev_info
	 * @return
	 *	 not NULL: valid handle \n
	 *	 NULL : invalid handle,fail to open \n
	 * @par Sample
	 * @code
	 *	SWORDFISH_DEVICE_HANDLE handle=swordfish_open(dev_info);
	 *  if(handle!=NULL)
	 *	 printf("ok to open feynman camera!\n");
	 *	}else{
	 *	 printf("fail to open feynman camera!\n");
	 *  };
	 * @endcode
	 */
	SWORDFISH_DEVICE_HANDLE swordfish_open(SWORDFISH_DEVICE_INFO *dev_info);

	/**
	 * @brief		config stream params like resolution and fps and fpsratio etc.
	 * @param[in]	handle : SWORDFISH_DEVICE_HANDLE return by swordfish_open
	 * @param[in]	config : SWORDFISH_CONFIG type pointer,configuration of resolution and fps
	 * @note	Call it to config streams from feynman camera
	 * @return
	 *	 SWORDFISH_OK: Success \n
	 *	 else : Failed \n
	 * @par Sample
	 * @code
	 * SWORDFISH_CONFIG config;
	 * config.irres=SWORDFISH_RESOLUTION_640_400;
	 * config.rgbres=SWORDFISH_RESOLUTION_640_400;
	 * config.fps=FPS_30;
	 * config.fpsratio=1;
	 *	if(SWORDFISH_OK==swordfish_config(handle,&config){
	 *	 printf("ok to config feynman camera!\n");
	 *	}else{
	 *	 printf("fail to config feynman camera!\n");
	 *  };
	 * @endcode
	 */
	SWORDFISH_RESULT swordfish_config(SWORDFISH_DEVICE_HANDLE handle, SWORDFISH_CONFIG *config);

	/**
	 * @brief		start stream which type is streamtype,data will be retrieved when callback is called
	 * @param[in]	handle : SWORDFISH_DEVICE_HANDLE return by swordfish_open
	 * @param[in]	stream_type :  stream type,imu/rgb/depth/ir/good feature etc.
	 * @param[in]	callback :  FRAMECALLBACK type callback function pointer,data will be retrieved in this function
	 * @param[in]	userdata :  userdata pointer will be referenced in callback
	 * @note	Call it to start stream which type is streamtype,data will be retrieved in callback function
	 * @return
	 *	 SWORDFISH_OK: Success \n
	 *	 else : Failed \n
	 * @par Sample
	 * @code
	 * void framecallback(void* data,void* userdata){
	 * 	printf("received frame data which type is SWORDFISH_USBHeaderDataPacket!\n");
	 * }
	 *	if(SWORDFISH_OK==swordfish_start(handle,SWORDFISH_STREAM_DEPTH,framecallback,NULL){
	 *	 printf("ok to start stream of feynman camera!\n");
	 *	}else{
	 *	 printf("fail to start stream of feynman camera!\n");
	 *  };
	 * @endcode
	 */
	SWORDFISH_RESULT swordfish_start(SWORDFISH_DEVICE_HANDLE handle, SWORDFISH_STREAM_TYPE_E stream_type, FRAMECALLBACK callback, void *userdata);

	/**
	 * @brief		stop stream which type is streamtype
	 * @param[in]	handle : SWORDFISH_DEVICE_HANDLE return by swordfish_open
	 * @param[in]	stream_type :  stream type,imu/rgb/depth/ir/good feature etc.
	 * @note	Call it to stop stream which type is streamtype
	 * @return
	 *	 SWORDFISH_OK: Success \n
	 *	 else : Failed \n
	 * @par Sample
	 * @code
	 *	if(SWORDFISH_OK==swordfish_stop(handle,SWORDFISH_STREAM_DEPTH){
	 *	 printf("ok to stop stream of feynman camera!\n");
	 *	}else{
	 *	 printf("fail to stop stream of feynman camera!\n");
	 *  };
	 * @endcode
	 */
	SWORDFISH_RESULT swordfish_stop(SWORDFISH_DEVICE_HANDLE handle, SWORDFISH_STREAM_TYPE_E stream_type);

	/**
	 * @brief		close feynman device
	 * @param[in]	handle : SWORDFISH_DEVICE_HANDLE return by swordfish_open
	 * @note	Call it to close feynman device.
	 * @return
	 *	none \n
	 * @par Sample
	 * @code
	 *
	 *  swordfish_close(handle);
	 *
	 * @endcode
	 */
	void swordfish_close(SWORDFISH_DEVICE_HANDLE handle);

	/**
	 * @brief		get yuv color value by index.
	 * @param[in]	index : 0-255 interger
	 * @param[inout]	py : pointer of unsigned char,corresponding y value(unsigned char) to index will be put
	 * @param[inout]	pu : pointer of unsigned char,corresponding u value(unsigned char) to index will be put
	 * @param[inout]	pv : pointer of unsigned char,corresponding v value(unsigned char) to index will be put
	 * @note	Call it to get yuv color value by index.
	 * @return
	 *	 0 : Success to get yuv value of index \n
	 *	 -1 : Failed to get yuv value of index \n
	 * @par Sample
	 * @code
	 *  unsigned char y,u,v;
	 *  printf("will get yuv value of index 23!\n");
	 *  if(0==swordfish_getyuvfromindex(23,&y,&u,&v)){
	 *       printf("get yuv value of index 23 ok!\n);
	 *  }
	 *
	 * @endcode
	 */
	int swordfish_getyuvfromindex(int index, unsigned char *py, unsigned char *pu, unsigned char *pv);

	typedef void (*UPGRADECALLBACK)(int index, int all);

	/**
	 * @brief		upgrade file in feynman device.
	 * @param[in]	handle : SWORDFISH_DEVICE_HANDLE return by swordfish_open
	 * @param[in]	callback : UPGRADECALLBACK type callback function pointer,called every packet transmitted
	 * @param[in]	userdata : userdata pointer will be referenced in callback
	 * @param[in]	upgrade_type : SWORDFISH_UPGRADE_SUB_TYPE
	 * @param[in]	dst_path : pointer of char,dest file path in device,not include filename itself
	 * @param[in]	dst_filename : pointer of char,dest file name in device
	 * @param[in]	local_filename : pointer of char,local file name,include absolute path or relative path
	 * @note	Call it to upgrade file in feynman device.
	 * @return
	 *	 0 : Success to upgrade file \n
	 *	 -1 : Failed to open file or file not exist \n
	 *	 -2 : memory fault \n
	 *	 -3 : error happened,stop transfer \n
	 * @par Sample
	 * @code
	 *
	 *  printf("will upgrade file of feynman in /nextvpu/bin/arm of device!\n");
	 *  if(0==swordfish_upgrade(handle,callback, userdata, SWORDFISH_COMMAND_USB_UPGRADE_USER_FILE,"/nextvpu/bin/arm","feynman","c:/feynman")){
	 *       printf("upgrade file feynman ok!\n);
	 *  }
	 *
	 * @endcode
	 */
	int swordfish_upgrade(SWORDFISH_DEVICE_HANDLE handle, UPGRADECALLBACK callback, void *userdata, int upgrade_type, const char *dst_path, const char *dst_filename, const char *local_filename);
	int swordfish_upgrade_withcontent(SWORDFISH_DEVICE_HANDLE handle, UPGRADECALLBACK callback, void *userdata, int upgrade_type, const char *dst_path, const char *dst_filename, const char *localcontent);
	/**
	 * @brief		get value which type is valuetype from camera
	 * @param[in]	handle : SWORDFISH_DEVICE_HANDLE return by swordfish_open
	 * @param[in]	valuetype : value type
	 * @param[inout]	value : value pointer to save value
	 * @param[in]	timeout : timeout in milliseconds
	 * @note	Call it to get value from feynman device.
	 * @return
	 *	 SWORDFISH_OK : Success to get value \n
	 *	 else : Failed to get value \n
	 * @par Sample
	 * @code
	 *  int projectorstatus=0;
	 *  if(SWORDFISH_OK==swordfish_get(handle,SWORDFISH_PROJECTOR_STATUS, &projectorstatus,2000)){
	 *       printf("get value from feynman ok!\n);
	 *  }
	 *
	 * @endcode
	 */
	SWORDFISH_RESULT swordfish_get(SWORDFISH_DEVICE_HANDLE handle, SWORDFISH_VALUE_TYPE valuetype, void *value, int timeout);

	/**
	 * @brief		set value which type is valuetype to camera
	 * @param[in]	handle : SWORDFISH_DEVICE_HANDLE return by swordfish_open
	 * @param[in]	valuetype : value type
	 * @param[in]	value : value pointer to set value from
	 * @param[in]	datalen : data length of value
	 * @param[in]	timeout : timeout in milliseconds
	 * @note	Call it to set value to camera
	 * @return
	 *	 SWORDFISH_OK : Success to get value \n
	 *	 else : Failed to get value \n
	 * @par Sample
	 * @code
	 *  int projectorstatus=0;
	 *  if(SWORDFISH_OK==swordfish_set(handle,SWORDFISH_PROJECTOR_STATUS, &projectorstatus,sizeof(int),2000)){
	 *       printf("set value from feynman ok!\n);
	 *  }
	 *
	 * @endcode
	 */
	SWORDFISH_RESULT swordfish_set(SWORDFISH_DEVICE_HANDLE handle, SWORDFISH_VALUE_TYPE valuetype, void *value, int datalen, int timeout);

	/**
	 * @brief		get imu internal ref from camera
	 * @param[in]	handle : SWORDFISH_DEVICE_HANDLE return by swordfish_open
	 * @param[inout]	ref : s_swordfish_imu_internal_reference type pointer to save ref
	 * @note	Call it to get imu internal ref from camera
	 * @return
	 *	 not NULL : Success to get imu internal ref \n
	 *	 NULL : Failed to get imu internal ref \n
	 * @par Sample
	 * @code
	 *  s_swordfish_imu_internal_reference ref;
	 *  if(NULL!=swordfish_getimuinternalref(handle, &ref)){
	 *       printf("get imu internal ref from feynman ok!\n);
	 *  }
	 *
	 * @endcode
	 */
	s_swordfish_imu_internal_reference *swordfish_getimuinternalref(SWORDFISH_DEVICE_HANDLE handle, s_swordfish_imu_internal_reference *ref);

	/**
	 * @brief		get imu external ref from camera
	 * @param[in]	handle : SWORDFISH_DEVICE_HANDLE return by swordfish_open
	 * @param[inout]	ref : s_swordfish_imu_external_reference type pointer to save ref
	 * @note	Call it to get imu external ref from camera
	 * @return
	 *	 not NULL : Success to get imu external ref \n
	 *	 NULL : Failed to get imu external ref \n
	 * @par Sample
	 * @code
	 *  s_swordfish_imu_external_reference ref;
	 *  if(NULL!=swordfish_getimuexternalref(handle, &ref)){
	 *       printf("get imu external ref from feynman ok!\n);
	 *  }
	 *
	 * @endcode
	 */
	s_swordfish_imu_external_reference *swordfish_getimuexternalref(SWORDFISH_DEVICE_HANDLE handle, s_swordfish_imu_external_reference *ref);
	SWORDFISH_RESULT swordfish_tarecalibration(SWORDFISH_DEVICE_HANDLE handle,
									   int width,int height,
									float measure_distance_mm,
									float ground_truth_mm);

	/**
	 * @brief		get sdk version
	 * @note	Call it to get sdk version
	 * @return
	 *	 not NULL : Success to get sdk version \n
	 *	 NULL : Failed to get sdk version \n
	 * @par Sample
	 * @code
	 *  printf("sdk version:%s\n",swordfish_getsdkversion());
	 *
	 * @endcode
	 */
	const char *swordfish_getsdkversion();
	BOOL swordfish_hasconnect(SWORDFISH_DEVICE_HANDLE handle);
	void swordfish_zerorect(uint16_t* depthdata,int width,int height,int fromcolindex,int tocolindex,int fromrowindex,int torowindex);	

	SWORDFISH_RESULT swordfish_senduserdata(SWORDFISH_DEVICE_HANDLE handle,void* data,int size);
	SWORDFISH_RESULT swordfish_get_cam_param_by_resolution(SWORDFISH_DEVICE_HANDLE handle, int width, int height, s_swordfish_cam_param *tmpparam);
	SWORDFISH_RESULT swordfish_send_update_tftp(SWORDFISH_DEVICE_HANDLE handle);
	void swordfish_globaltimeenable(SWORDFISH_DEVICE_HANDLE handle, int enable);
	void swordfish_freedevices(SWORDFISH_DEVICE_INFO *thedevice);
SWORDFISH_RESULT
swordfish_sendbuffer (SWORDFISH_DEVICE_HANDLE handle, uint8_t * data,
		      int datalen, int pktsize);
SWORDFISH_RESULT swordfish_setcallback(SWORDFISH_DEVICE_HANDLE handle, FRAMECALLBACK callback, void *userdata);
#ifdef __cplusplus
}
#endif
#endif
