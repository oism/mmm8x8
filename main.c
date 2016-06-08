#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <serial.h>
#include <crc16.h>

/* local constants */
#define RET_OK          (0)
#define RET_ERR_USAGE   (1)
#define RET_ERR_OPEN    (2)
#define RET_ERR_SETATTR (3)

#define CMD_NOMATCH (0)

/* local types */
typedef int (*CMD_FCT)(int fd, int myargc, char **myargv);

typedef struct {
  char   *cmd_name;       /* name as typed on the command line */
  int     cmd_nargs;      /* no of arguments this command needs */
  CMD_FCT cmd_fct;        /* pointer to command function */
} CMD;


/* local functions */
static int get_firmwareversion(int fd, int myargc, char **myargv);
static int display_text(int fd, int myargc, char **myargv);
static int store_text(int fd, int myargc, char **myargv);
static int set_textspeed(int fd, int myargc, char **myargv);

static int send_command(int fd, char command, int nparam,
                        unsigned char *params);
static int receive_response(int fd, int rsplen, unsigned char *response);

static int find_command(int nargs, char *command);
static void print_usage(void);


/* local variables */
static CMD cmd_table[] =
{
/*  cmd_name,          nargs, pointer to command fct */
  { "",                0,     NULL }, /* CMD_NOMATCH */
  { "firmwareversion", 0,     get_firmwareversion },
  { "displaytext",     1,     display_text },
  { "storetext",       1,     store_text },
  { "settextspeed",    1,     set_textspeed },
};


/* code section */
int main(int argc, char **argv)
{
  int rc;
  int cmd;
  int fd;

  if (argc < 3)
  {
    print_usage();
    rc = RET_ERR_USAGE; 
    goto EXIT;
  }

  cmd = find_command(argc - 3, argv[2]);
  if (cmd == CMD_NOMATCH)
  {
    print_usage();
    rc = RET_ERR_USAGE;
    goto EXIT;
  }

  if ((rc = open_serial(argv[1], &fd)) != RET_SERIAL_OK)
  {
    fprintf(stderr, "open of device %s has failed.\n", argv[1]);
    goto EXIT;
  }
 
  rc = cmd_table[cmd].cmd_fct(fd, argc - 3, &argv[3]);

  close_serial(fd);

EXIT:
  return rc;
}


static int get_firmwareversion(int fd, int myargc, char **myargv)
{
  int rc;
  #define CMD_FIRMWARE_RSP_LEN (12)
  unsigned char response[CMD_FIRMWARE_RSP_LEN];

printf("argc = %d\n", myargc);
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


static int display_text(int fd, int myargc, char **myargv)
{
  int rc;
  #define CMD_DISPLAY_TEXT_RSP_LEN (6)
  unsigned char response[CMD_FIRMWARE_RSP_LEN];
  int textlen;

  textlen = strlen(myargv[0]);
  rc = send_command(fd, 'E', textlen, myargv[0]);

  rc = receive_response(fd, CMD_DISPLAY_TEXT_RSP_LEN, response);
  if (rc != RET_OK) 
  {
    goto EXIT;
  }

EXIT:
  return rc;
}


static int store_text(int fd, int myargc, char **myargv)
{
  int rc;
  #define CMD_STORE_TEXT_RSP_LEN (6)
  unsigned char response[CMD_STORE_TEXT_RSP_LEN];
  int textlen;

  textlen = strlen(myargv[0]);
  rc = send_command(fd, 'J', textlen, myargv[0]);

  rc = receive_response(fd, CMD_STORE_TEXT_RSP_LEN, response);
  if (rc != RET_OK) 
  {
    goto EXIT;
  }

EXIT:
  return rc;
}


static int set_textspeed(int fd, int myargc, char **myargv)
{
  int rc;
  #define CMD_SET_TEXTSPEED_RSP_LEN (6)
  unsigned char response[CMD_SET_TEXTSPEED_RSP_LEN];
  unsigned char speed[1];
  
  speed[0] = atoi(myargv[0]);
  rc = send_command(fd, 'F', 1, speed);

  rc = receive_response(fd, CMD_SET_TEXTSPEED_RSP_LEN, response);
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
  unsigned char escape;

  escape = 0x10;

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

  /* escape STX */
  if (length_low == 2)
  {
    rc = write(fd, &escape, 1);
    crc16 = calc_crc16(crc16, escape);
    length_low = 0x82;
  }

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


static int find_command(int nargs, char *command)
{
  int i;

  for (i = 0; i < (sizeof(cmd_table) / sizeof(CMD)); i++)
  {
    if (strcmp(cmd_table[i].cmd_name, command) == 0)
    {
      if (cmd_table[i].cmd_nargs == nargs)
      {
        return (i);
      }
      else
      {
        return (CMD_NOMATCH);
      }
    }
  }

  return (CMD_NOMATCH);
}


static void print_usage(void)
{
  fprintf(stderr, "Usage: mmm8x8 <serial device> firmwareversion\n");
  fprintf(stderr, "       mmm8x8 <serial device> displaytext <text>\n");
  fprintf(stderr, "       mmm8x8 <serial device> storetext <text>\n");
  fprintf(stderr, "       mmm8x8 <serial device> settextspeed <speed: 0-255>\n");
}
