#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <crc16.h>

#define RET_OK      (0)
#define RET_USAGE   (1)
#define RET_OPEN    (2)
#define RET_SETATTR (3)

static int get_firmwareversion(int fd);
static int display_text(int fd, char *text);
static int store_text(int fd, char *text);
static int send_command(int fd, char command, int nparam,
                        unsigned char *params);
static int receive_response(int fd, int rsplen, unsigned char *response);
static int open_serial(char *serialport, int *fd);
static int close_serial(int fd);
static void usage(void);


int main(int argc, char **argv)
{
  int rc;
  int fd;

  if (argc < 2)
  {
    usage();
    rc = RET_USAGE; 
    goto EXIT;
  }

#if 1
  if ((rc = open_serial(argv[1], &fd)) != RET_OK)
  {
    fprintf(stderr, "open of device %s has failed.\n", argv[1]);
    goto EXIT;
  }
#endif

  if (strcmp(argv[2], "firmwareversion") == 0) 
  {
    rc = get_firmwareversion(fd);
  } 
  else if (strcmp(argv[2], "displaytext") == 0)
  {
    rc = display_text(fd, argv[3]);
  }
  else if (strcmp(argv[2], "storetext") == 0)
  {
    rc = store_text(fd, argv[3]);
  }


  rc = RET_OK;

EXIT:
#if 1
  close_serial(fd);
#endif
  return rc;
}


static int get_firmwareversion(int fd)
{
  int rc;
  #define CMD_FIRMWARE_RSP_LEN (12)
  unsigned char response[CMD_FIRMWARE_RSP_LEN];

  rc = send_command(fd, 'v', 0, NULL);
  if (rc != RET_OK) 
  {
    fprintf(stderr, "send_command with command %c has failed.\n", 'v');
    goto EXIT;
  }

  rc = receive_response(fd, CMD_FIRMWARE_RSP_LEN, response);
  if (rc != RET_OK) 
  {
    fprintf(stderr, "receive_response with command %c has failed.\n", 'v');
    goto EXIT;
  }
  
  printf("Firmware version: 0x%x%x%x%x%x%x\n", response[4], response[5],
         response[6], response[7], response[8], response[9]); 

EXIT:
  return rc;
}


static int display_text(int fd, char *text)
{
  int rc;
  #define CMD_DISPLAY_TEXT_RSP_LEN (6)
  unsigned char response[CMD_FIRMWARE_RSP_LEN];
  int textlen;

  textlen = strlen(text);
  rc = send_command(fd, 'E', textlen + 1, text);

  rc = receive_response(fd, CMD_DISPLAY_TEXT_RSP_LEN, response);
  if (rc != RET_OK) 
  {
    goto EXIT;
  }

EXIT:
  return rc;
}


static int store_text(int fd, char *text)
{
  int rc;
  #define CMD_STORE_TEXT_RSP_LEN (6)
  unsigned char response[CMD_FIRMWARE_RSP_LEN];
  int textlen;

  textlen = strlen(text);
  rc = send_command(fd, 'J', textlen + 1, text);

  rc = receive_response(fd, CMD_STORE_TEXT_RSP_LEN, response);
  if (rc != RET_OK) 
  {
    goto EXIT;
  }

EXIT:
  return rc;
}
static int send_command(int fd, char command, int nparam, unsigned char *params)
{
  int rc;
  unsigned char checksum[2];
  unsigned char frame_start;
  unsigned char length_high;
  unsigned char length_low;
  unsigned short crc16;
  int i;

  /* write start frame character */
  frame_start = 0x02;
  rc = write(fd, &frame_start, 1);
  if (rc != 1)
  {
    fprintf(stderr, "write failed, errno = %d\n", errno);
    goto EXIT;
  }
  crc16 = calc_crc16(INITIAL_VALUE, frame_start);

  /* write two byte length, command + params */
  length_high = 0;
  rc = write(fd, &length_high, 1);
  if (rc != 1)
  {
    fprintf(stderr, "write failed, errno = %d\n", errno);
    goto EXIT;
  }
  crc16 = calc_crc16(crc16, length_high);
  
  length_low = 1 + nparam;
  rc = write(fd, &length_low, 1);
  if (rc != 1)
  {
    fprintf(stderr, "write failed, errno = %d\n", errno);
    goto EXIT;
  }
  crc16 = calc_crc16(crc16, length_low);

  /* write command */
  rc = write(fd, &command, 1);
  if (rc != 1)
  {
    fprintf(stderr, "write failed, errno = %d\n", errno);
    goto EXIT;
  }
  crc16 = calc_crc16(crc16, command);

  /* write params */
  if (nparam) 
  {
    for (i = 0; i < nparam; i++)
    {
      rc = write(fd, &params[i], 1);
      if (rc != 1)
      {
        fprintf(stderr, "write failed, errno = %d\n", errno);
        goto EXIT;
      }
      crc16 = calc_crc16(crc16, *(params + i));
    }
  }

  /* write checksum CRC16 */
  checksum[0] = (crc16 >> 8) & 0xff;
  checksum[1] = crc16 & 0xff;
  rc = write(fd, checksum, 2); 
  if (rc != 2)
  {
    fprintf(stderr, "write failed, errno = %d\n", errno);
    goto EXIT;
  }

  rc = RET_OK;

EXIT:
  return rc;
}


static int receive_response(int fd, int rsplen, unsigned char *response)
{
  int rc;
  unsigned char *pos;
  int nread;

  pos = response;
  nread = rsplen;
  do 
  {
    do
    {
      rc = read(fd, pos, nread);
      if (rc != -1) 
      {
        pos = pos + rc;
        nread -= rc;
      }
    }
    while ((rc != -1) && (nread > 0));
  }
  while ((rc == -1) && (errno == EAGAIN));
  if (rc == -1)
  {
    fprintf(stderr, "read failed, rc = %d, errno = %d\n", rc, errno);
    goto EXIT;
  }
  else
  {
    int i;
    
    printf("rsp: ");
    for (i = 0; i < rsplen; i++)
    {
      printf("%02X ", *(response + i));
    }
    printf("\n");
  }

  rc = RET_OK;

EXIT:
  return rc;
}


static int open_serial(char *serialport, int *fd)
{
  int rc;
  struct termios options;

  if ((*fd = open(serialport, O_RDWR | O_NOCTTY | O_NDELAY)) == -1)
  {
    rc = RET_OPEN;
    goto EXIT;
  }

  /* now set 38400,8,N,1 */
  /* get the current settings of the serial port */
  tcgetattr(*fd, &options);

  /* set the read and write speed to 38400 BAUD */
  cfsetispeed(&options, B38400);
  cfsetospeed(&options, B38400);

  /* disable the parity bit */
  options.c_cflag &= ~PARENB;

  /* only one stop bit */
  options.c_cflag &= ~CSTOPB;

  /* 8-bits */
  options.c_cflag &= ~CSIZE;
  options.c_cflag |= CS8;

  /* enable the receiver and set local mode */
  options.c_cflag |= (CLOCAL | CREAD);

  /* choosing raw input */
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

  /* choosing raw output */
  options.c_oflag &= ~OPOST;
 
  /* now set the settings */
  if(tcsetattr(*fd, TCSANOW, &options) == -1)
  {
    close(*fd);
    rc = RET_SETATTR;
    goto EXIT;
  }

  rc = RET_OK;

EXIT:
  return rc;
}


static int close_serial(int fd)
{
  close(fd);
}


static void usage(void)
{
  fprintf(stderr, "Usage: mmm8x8 <serial device> firmwareversion\n");
  fprintf(stderr, "       mmm8x8 <serial device> displaytext <text>\n");
  fprintf(stderr, "       mmm8x8 <serial device> storetext <text>\n");
}
