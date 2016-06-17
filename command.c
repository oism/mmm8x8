#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <serial.h>
#include <pattern.h>
#include <crc16.h>

#define COMMAND_SRC 1
#include <command.h>
#undef COMMAND_SRC

#define STX 0x02
#define ESC 0x10
#define FLAG 0x80

#define LINES_PER_PATTERN (8)
#define COLUMNS_PER_PATTERN (8)

static int send_command(int fd, char command, int nparam,
                        unsigned char *params);
static int receive_response(int fd, unsigned char *response, int rsplen);
static int write_serial_with_escape(int fd, unsigned char byte,
                                    unsigned short *crc16);
static write_and_crc_byte(int fd, unsigned char byte, unsigned short *crc16);

static int read_one_pattern(FILE *patternfile, unsigned char *pattern);


int get_firmwareversion(int fd, int myargc, char **myargv)
{
  int rc;
  #define CMD_GET_FIRMWARE_RSP_LEN (12)
  unsigned char response[CMD_GET_FIRMWARE_RSP_LEN];

  rc = send_command(fd, 'v', 0, NULL);
  if (rc != RET_COMMAND_OK) 
  {
    fprintf(stderr, "send_command with command %c has failed.\n", 'v');
    goto EXIT;
  }

  rc = receive_response(fd, response, CMD_GET_FIRMWARE_RSP_LEN);
  if (rc != RET_COMMAND_OK) 
  {
    fprintf(stderr, "receive_response with command %c has failed.\n", 'v');
    goto EXIT;
  }
  
  printf("Firmware version: %x%x.%x%x.%x%x\n", response[4], response[5],
         response[6], response[7], response[8], response[9]); 

EXIT:
  return rc;
}


int set_normalmode(int fd, int myargc, char **myargv)
{
  int rc;
  #define CMD_SET_NORMALMODE_RSP_LEN (6)
  unsigned char response[CMD_SET_NORMALMODE_RSP_LEN];

  rc = send_command(fd, 'A', 0, NULL);
  if (rc != RET_COMMAND_OK) 
  {
    fprintf(stderr, "send_command with command %c has failed.\n", 'v');
    goto EXIT;
  }

  rc = receive_response(fd, response, CMD_SET_NORMALMODE_RSP_LEN);
  if (rc != RET_COMMAND_OK) 
  {
    fprintf(stderr, "receive_response with command %c has failed.\n", 'A');
    goto EXIT;
  }
  
EXIT:
  return rc;
}


int display_text(int fd, int myargc, char **myargv)
{
  int rc;
  #define CMD_DISPLAY_TEXT_RSP_LEN (6)
  unsigned char response[CMD_DISPLAY_TEXT_RSP_LEN];
  int textlen;

  textlen = strlen(myargv[0]);
  rc = send_command(fd, 'E', textlen, myargv[0]);

  rc = receive_response(fd, response, CMD_DISPLAY_TEXT_RSP_LEN);
  if (rc != RET_COMMAND_OK) 
  {
    goto EXIT;
  }

EXIT:
  return rc;
}


int store_text(int fd, int myargc, char **myargv)
{
  int rc;
  #define CMD_STORE_TEXT_RSP_LEN (6)
  unsigned char response[CMD_STORE_TEXT_RSP_LEN];
  int textlen;

  textlen = strlen(myargv[0]);
  rc = send_command(fd, 'J', textlen, myargv[0]);

  rc = receive_response(fd, response, CMD_STORE_TEXT_RSP_LEN);
  if (rc != RET_COMMAND_OK) 
  {
    goto EXIT;
  }

EXIT:
  return rc;
}


int set_textspeed(int fd, int myargc, char **myargv)
{
  int rc;
  #define CMD_SET_TEXTSPEED_RSP_LEN (6)
  unsigned char response[CMD_SET_TEXTSPEED_RSP_LEN];
  unsigned char speed[1];
  
  speed[0] = atoi(myargv[0]);
  rc = send_command(fd, 'F', 1, speed);

  rc = receive_response(fd, response, CMD_SET_TEXTSPEED_RSP_LEN);
  if (rc != RET_COMMAND_OK) 
  {
    goto EXIT;
  }

EXIT:
  return rc;
}


int display_pattern(int fd, int myargc, char **myargv)
{
  int rc;
#define CMD_DISPLAY_PATTERN_RSP_LEN (6)
  unsigned char response[CMD_DISPLAY_PATTERN_RSP_LEN];
  FILE *patternfile;
  int lines;
  int columns;
  unsigned char linepattern;
  unsigned char pattern[LINES_PER_PATTERN];
  
  if ((rc = open_patternfile(myargv[0], &patternfile)) != RET_PATTERN_OK)
  {
    fprintf(stderr, "open of patternfile %s has failed\n", myargv[0]);
    goto EXIT;
  }

  
  if ((rc = read_one_pattern(patternfile, pattern)) != RET_COMMAND_OK)
  {
    fprintf(stderr, "read of patternfile %s has failed\n", myargv[0]);
    goto CLOSE_EXIT;
  }

  rc = send_command(fd, 'D', LINES_PER_PATTERN, pattern);

  rc = receive_response(fd, response, CMD_DISPLAY_PATTERN_RSP_LEN);
  if (rc != RET_COMMAND_OK) 
  {
    goto CLOSE_EXIT;
  }
  

CLOSE_EXIT:
  close_patternfile(patternfile);

EXIT:
  return rc;
}


int store_pattern(int fd, int myargc, char **myargv)
{
  int rc;
#define CMD_STORE_PATTERN_RSP_LEN (6)
  unsigned char response[CMD_STORE_PATTERN_RSP_LEN];
  FILE *patternfile;
  unsigned char dummy;
  unsigned char pattern[LINES_PER_PATTERN + 1];
#define DISPLAY_DURATION (1)
  
  /* open pattern file */
  if ((rc = open_patternfile(myargv[0], &patternfile)) != RET_PATTERN_OK)
  {
    fprintf(stderr, "open of patternfile %s has failed\n", myargv[0]);
    goto EXIT;
  }

  /* read the first pattern */
  if ((rc = read_one_pattern(patternfile, pattern)) != RET_COMMAND_OK)
  {
      fprintf(stderr, "read of patternfile %s has failed\n", myargv[0]);
      goto CLOSE_EXIT;
  }

  /* set duration of display in multiples of 100 ms */
  pattern[LINES_PER_PATTERN] = DISPLAY_DURATION;

  /* write first pattern */
  rc = send_command(fd, 'G', LINES_PER_PATTERN + 1, pattern);

  rc = receive_response(fd, response, CMD_STORE_PATTERN_RSP_LEN);
  if (rc != RET_COMMAND_OK) 
  {
    goto CLOSE_EXIT;
  }
  
  /* read the subsequent patterns */
  while (read_patternfile(patternfile, &dummy) == RET_PATTERN_OK)
  {
    if ((rc = read_one_pattern(patternfile, pattern)) != RET_COMMAND_OK)
    {
      fprintf(stderr, "read of patternfile %s has failed\n", myargv[0]);
      goto CLOSE_EXIT;
    }

    /* set duration of display in multiples of 100 ms */
    pattern[LINES_PER_PATTERN] = DISPLAY_DURATION;
    rc = send_command(fd, 'I', LINES_PER_PATTERN + 1, pattern);

    rc = receive_response(fd, response, CMD_STORE_PATTERN_RSP_LEN);
    if (rc != RET_COMMAND_OK) 
    {
      goto CLOSE_EXIT;
    }
  }


CLOSE_EXIT:
  close_patternfile(patternfile);

EXIT:
  return rc;
}


static int send_command(int fd, char command, int nparam, unsigned char *params)
{
  int rc;
  unsigned char checksum[2];
  unsigned char length_high;
  unsigned char length_low;
  unsigned short crc16;
  int i;
  unsigned char escape;
  struct timespec duration;

  /* set initial value for crc16 computation */
  crc16 = INITIAL_VALUE;

  /* write start frame character */
  rc = write_and_crc_byte(fd, STX, &crc16);
  if (rc != 1)
  {
    fprintf(stderr, "write failed, errno = %d\n", errno);
    goto EXIT;
  }

  /* write two byte length, command + params */
  length_high = 0;
  rc = write_serial_with_escape(fd, length_high, &crc16);
  if (rc != 1)
  {
    fprintf(stderr, "write failed, errno = %d\n", errno);
    goto EXIT;
  }
  
  length_low = 1 + nparam;
  rc = write_serial_with_escape(fd, length_low, &crc16);
  if (rc != 1)
  {
    fprintf(stderr, "write failed, errno = %d\n", errno);
    goto EXIT;
  }

  /* write command */
  rc = write_serial_with_escape(fd, command, &crc16);
  if (rc != 1)
  {
    fprintf(stderr, "write failed, errno = %d\n", errno);
    goto EXIT;
  }

  /* write params */
  if (nparam) 
  {
    for (i = 0; i < nparam; i++)
    {
      rc = write_serial_with_escape(fd, params[i], &crc16);
      if (rc != 1)
      {
        fprintf(stderr, "write failed, errno = %d\n", errno);
        goto EXIT;
      }
    }
  }

  /* write checksum CRC16 */
  checksum[0] = (crc16 >> 8) & 0xff;
  checksum[1] = crc16 & 0xff;
  rc = write_serial(fd, checksum, 2); 
  if (rc != 2)
  {
    fprintf(stderr, "write failed, errno = %d\n", errno);
    goto EXIT;
  }

  duration.tv_sec = 0;
  duration.tv_nsec = 100000000;
  nanosleep(&duration, NULL);

  rc = RET_COMMAND_OK;

EXIT:
  return rc;
}


static int write_serial_with_escape(int fd, unsigned char byte,
                                    unsigned short *crc16)
{
  int rc;

  switch (byte)
  {
    case STX:
      rc = write_and_crc_byte(fd, ESC, crc16);
      if (rc != 1) 
      {
        goto EXIT;
      }
      rc = write_and_crc_byte(fd, STX | FLAG, crc16);
      break;

    case ESC:
      rc = write_and_crc_byte(fd, ESC, crc16);
      if (rc != 1) 
      {
        goto EXIT;
      }
      rc = write_and_crc_byte(fd, ESC | FLAG, crc16);
      break;

    default:
      rc = write_and_crc_byte(fd, byte, crc16);
      break;
  }

EXIT:
  return rc;
}


static write_and_crc_byte(int fd, unsigned char byte, unsigned short *crc16)
{
  int rc;

  rc = write_serial(fd, &byte, 1); 
  if (rc == 1)
  {
    *crc16 = calc_crc16(*crc16, byte);
  }

  return rc;
}


static int receive_response(int fd, unsigned char *response, int rsplen)
{
  int rc;
  
  rc = read_serial(fd, response, rsplen);
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

  rc = RET_COMMAND_OK;

EXIT:
  return rc;
}



static int read_one_pattern(FILE *patternfile, unsigned char *pattern)
{
  int rc;
  int lines;
  int columns;
  unsigned char linepattern;

  for (lines = 0; lines  < LINES_PER_PATTERN; lines++)
  {
    pattern[lines] = 0;
  }

  for (lines = 0; lines < LINES_PER_PATTERN; lines++)
  {
    if ((rc = read_patternfile(patternfile, &linepattern)) != RET_PATTERN_OK)
    {
      rc = RET_COMMAND_ERR_READ;
      goto EXIT;
    }

    for (columns = 0; columns < COLUMNS_PER_PATTERN; columns++)
    {
      if (linepattern & (1 << (COLUMNS_PER_PATTERN - columns - 1))) 
      {
        pattern[columns] = pattern[columns] | (1 << lines);
      }
    }
  }
  
  rc = RET_COMMAND_OK;

EXIT:
  return rc;
}
