#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "swordfish.h"


void resultcallback(void *data, void *userdata)
{
    if (userdata != NULL)
    {
       // printf("userdata:0x%lX\n", (long)userdata);
       
    }
    
    SWORDFISH_USBHeaderDataPacket *tmppack = (SWORDFISH_USBHeaderDataPacket *)data;
    printf("recv pkt,type:%d,subtype:%d,datalen:%d\n",tmppack->type,tmppack->sub_type,tmppack->len);
} 
int
main (int argc, const char *argv[])
{
  if (argc != 4) {
    printf ("usage:\n");
    printf ("./test_swordfish filename PKTSIZEKB SENDTIMES\n");
    exit (0);
  }
  const char *filepath = strdup (argv[1]);
  int pktsizekb = atoi (argv[2]);
  int sendtimes = atoi (argv[3]);

  SWORDFISH_DEVICE_HANDLE handle = NULL;
  swordfish_init ("./swordfish.log", 1024 * 1024);

  int total = 0;
  SWORDFISH_DEVICE_INFO *tmpdevice = NULL;
  if (SWORDFISH_OK == swordfish_enum_device (&total, &tmpdevice)) {
    printf ("found swordfish device,sum:%d,first usb_camera_name:%s\n", total,
	    tmpdevice[0].usb_camera_name);
    handle = swordfish_open (tmpdevice);
  } else {
    printf ("there's no swordfish usb camera plugged!\n");
  }

  if (NULL == handle) {
    printf ("fail to connect swordfish device\n");
    exit (0);
  }

  swordfish_setcallback(handle,resultcallback,NULL);

  struct stat sb;
  if (stat (filepath, &sb) == -1) {
    perror ("stat");
    exit (EXIT_FAILURE);
  }

  if ((sb.st_mode & S_IFMT) == S_IFREG) {
    uint8_t *buffer = (uint8_t *) malloc (sb.st_size);

    FILE *fp = fopen (filepath, "rb");
    if (fp) {
      if (sb.st_size != fread (buffer, 1, sb.st_size, fp)) {
	fclose (fp);
	printf ("fail to read file in one call!\n");
	exit (EXIT_FAILURE);
      }
      fclose (fp);
      for (int i = 0; i < sendtimes; i++) {
	if (SWORDFISH_OK ==
	    swordfish_sendbuffer (handle, buffer, sb.st_size,
				  pktsizekb * 1024)) {
	  printf ("ok to send buffer!\n");

	} else {
	  printf ("fail to send buffer!\n");
	}
      }

    } else {
      printf ("fail to open file:%s\n", filepath);
      exit (EXIT_FAILURE);

    }
  } else {
    printf ("not regular file!\n");
    exit (EXIT_FAILURE);
  }

  swordfish_deinit ();
  printf ("we exit...\n");
  return 0;
}
