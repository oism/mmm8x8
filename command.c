#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

static int send_command(SERHDL hdl, char command, int nparam,
                        unsigned char *params);
static int receive_response(SERHDL hdl, unsigned char *response, int rsplen);
static int write_serial_with_escape(SERHDL hdl, unsigned char byte,
                                    unsigned short *crc16);
static int write_and_crc_byte(SERHDL hdl, unsigned char byte,
                              unsigned short *crc16);

static int read_one_pattern(FILE *patternfile, unsigned char *pattern);


int get_firmwareversion(SERHDL hdl, int myargc, char **myargv)
{
  int rc;
  #define CMD_GET_FIRMWARE_RSP_LEN (12)
  unsigned char response[CMD_GET_FIRMWARE_RSP_LEN];

  rc = send_command(hdl, 'v', 0, NULL);
  if (rc != RET_COMMAND_OK) 
  {
    fprintf(stderr, "sending command firmwareversion has failed.\n");
    goto EXIT;
  }

  rc = receive_response(hdl, response, CMD_GET_FIRMWARE_RSP_LEN);
  if (rc != RET_COMMAND_OK) 
  {
    fprintf(stderr, "receiving response of command firmwareversion "
                    "has failed.\n");
    goto EXIT;
  }
  
  printf("Firmware version: %d.%d.%d\n", response[4] * 256 + response[5],
         response[6] * 256 + response[7], response[8] * 256 + response[9]); 

EXIT:
  return rc;
}


int display_text(SERHDL hdl, int myargc, char **myargv)
{
  int rc;
  #define CMD_DISPLAY_TEXT_RSP_LEN (6)
  unsigned char response[CMD_DISPLAY_TEXT_RSP_LEN];
  int textlen;

  textlen = strlen(myargv[0]);
  rc = send_command(hdl, 'E', textlen, (unsigned char *) myargv[0]);
  if (rc != RET_COMMAND_OK) 
  {
    fprintf(stderr, "sending command displaytext has failed.\n");
    goto EXIT;
  }

  rc = receive_response(hdl, response, CMD_DISPLAY_TEXT_RSP_LEN);
  if (rc != RET_COMMAND_OK) 
  {
    fprintf(stderr, "receiving response of command displaytext "
                    "has failed.\n");
    goto EXIT;
  }

EXIT:
  return rc;
}


int store_text(SERHDL hdl, int myargc, char **myargv)
{
  int rc;
  #define CMD_STORE_TEXT_RSP_LEN (6)
  unsigned char response[CMD_STORE_TEXT_RSP_LEN];
  int textlen;

  textlen = strlen(myargv[0]);
  rc = send_command(hdl, 'J', textlen, (unsigned char *) myargv[0]);
  if (rc != RET_COMMAND_OK) 
  {
    fprintf(stderr, "sending command storetext has failed.\n");
    goto EXIT;
  }

  rc = receive_response(hdl, response, CMD_STORE_TEXT_RSP_LEN);
  if (rc != RET_COMMAND_OK) 
  {
    fprintf(stderr, "receiving response of command storetext "
                    "has failed.\n");
    goto EXIT;
  }

EXIT:
  return rc;
}


int set_textspeed(SERHDL hdl, int myargc, char **myargv)
{
  int rc;
  #define CMD_SET_TEXTSPEED_RSP_LEN (6)
  unsigned char response[CMD_SET_TEXTSPEED_RSP_LEN];
  unsigned char speed[1];
  
  speed[0] = atoi(myargv[0]);
  rc = send_command(hdl, 'F', 1, speed);
  if (rc != RET_COMMAND_OK) 
  {
    fprintf(stderr, "sending command settextspeed has failed.\n");
    goto EXIT;
  }

  rc = receive_response(hdl, response, CMD_SET_TEXTSPEED_RSP_LEN);
  if (rc != RET_COMMAND_OK) 
  {
    fprintf(stderr, "receiving response of command settextspeed "
                    "has failed.\n");
    goto EXIT;
  }

EXIT:
  return rc;
}


int display_pattern(SERHDL hdl, int myargc, char **myargv)
{
  int rc;
#define CMD_DISPLAY_PATTERN_RSP_LEN (6)
  unsigned char response[CMD_DISPLAY_PATTERN_RSP_LEN];
  FILE *patternfile;
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

  rc = send_command(hdl, 'D', LINES_PER_PATTERN, pattern);
  if (rc != RET_COMMAND_OK) 
  {
    fprintf(stderr, "sending command displaypattern has failed.\n");
    goto CLOSE_EXIT;
  }

  rc = receive_response(hdl, response, CMD_DISPLAY_PATTERN_RSP_LEN);
  if (rc != RET_COMMAND_OK) 
  {
    fprintf(stderr, "receiving response of command displaypattern "
                    "has failed.\n");
    goto CLOSE_EXIT;
  }
  

CLOSE_EXIT:
  close_patternfile(patternfile);

EXIT:
  return rc;
}


int store_pattern(SERHDL hdl, int myargc, char **myargv)
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
  rc = send_command(hdl, 'G', LINES_PER_PATTERN + 1, pattern);
  if (rc != RET_COMMAND_OK) 
  {
    fprintf(stderr, "sending command storepattern has failed.\n");
    goto CLOSE_EXIT;
  }

  rc = receive_response(hdl, response, CMD_STORE_PATTERN_RSP_LEN);
  if (rc != RET_COMMAND_OK) 
  {
    fprintf(stderr, "receiving response of command storepattern "
                    "has failed.\n");
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
    rc = send_command(hdl, 'I', LINES_PER_PATTERN + 1, pattern);
    if (rc != RET_COMMAND_OK) 
    {
      fprintf(stderr, "sending command storepattern has failed.\n");
      goto CLOSE_EXIT;
    }

    rc = receive_response(hdl, response, CMD_STORE_PATTERN_RSP_LEN);
    if (rc != RET_COMMAND_OK) 
    {
      fprintf(stderr, "receiving response of command storepattern "
                      "has failed.\n");
      goto CLOSE_EXIT;
    }
  }

CLOSE_EXIT:
  close_patternfile(patternfile);

EXIT:
  return rc;
}


int set_normalmode(SERHDL hdl, int myargc, char **myargv)
{
  int rc;
  #define CMD_SET_NORMALMODE_RSP_LEN (6)
  unsigned char response[CMD_SET_NORMALMODE_RSP_LEN];

  rc = send_command(hdl, 'A', 0, NULL);
  if (rc != RET_COMMAND_OK) 
  {
    fprintf(stderr, "sending command setnormalmode has failed.\n");
    goto EXIT;
  }

  rc = receive_response(hdl, response, CMD_SET_NORMALMODE_RSP_LEN);
  if (rc != RET_COMMAND_OK) 
  {
    fprintf(stderr, "receiving response of command setnormalmode "
                    "has failed.\n");
    goto EXIT;
  }
  
EXIT:
  return rc;
}


int set_textmode(SERHDL hdl, int myargc, char **myargv)
{
  int rc;
  #define CMD_SET_TEXTMODE_RSP_LEN (6)
  unsigned char response[CMD_SET_TEXTMODE_RSP_LEN];

  rc = send_command(hdl, 'C', 0, NULL);
  if (rc != RET_COMMAND_OK) 
  {
    fprintf(stderr, "sending command settextmode has failed.\n");
    goto EXIT;
  }

  rc = receive_response(hdl, response, CMD_SET_TEXTMODE_RSP_LEN);
  if (rc != RET_COMMAND_OK) 
  {
    fprintf(stderr, "receiving response of command settextmode "
                    "has failed.\n");
    goto EXIT;
  }
  
EXIT:
  return rc;
}


int set_patternmode(SERHDL hdl, int myargc, char **myargv)
{
  int rc;
  #define CMD_SET_PATTERNMODE_RSP_LEN (6)
  unsigned char response[CMD_SET_PATTERNMODE_RSP_LEN];

  rc = send_command(hdl, 'B', 0, NULL);
  if (rc != RET_COMMAND_OK) 
  {
    fprintf(stderr, "sending command setpatternmode has failed.\n");
    goto EXIT;
  }

  rc = receive_response(hdl, response, CMD_SET_PATTERNMODE_RSP_LEN);
  if (rc != RET_COMMAND_OK) 
  {
    fprintf(stderr, "receiving response of command setpatternmode "
                    "has failed.\n");
    goto EXIT;
  }
  
EXIT:
  return rc;
}


int exe_factoryreset(SERHDL hdl, int myargc, char **myargv)
{
  int rc;

  rc = send_command(hdl, 'X', 0, NULL);
  if (rc != RET_COMMAND_OK) 
  {
    fprintf(stderr, "sending command factoryreset has failed.\n");
  }
  
  return rc;
}


static int send_command(SERHDL hdl, char command, int nparam,
                        unsigned char *params)
{
  int rc;
  unsigned char checksum[2];
  unsigned char length_high;
  unsigned char length_low;
  unsigned short crc16;
  int i;

  /* set initial value for crc16 computation */
  crc16 = INITIAL_VALUE;

  /* write start frame character */
  rc = write_and_crc_byte(hdl, STX, &crc16);
  if (rc != 1)
  {
    rc = RET_COMMAND_ERR_WRITE;
    goto EXIT;
  }

  /* write two byte length, command + params */
  length_high = 0;
  rc = write_serial_with_escape(hdl, length_high, &crc16);
  if (rc != 1)
  {
    rc = RET_COMMAND_ERR_WRITE;
    goto EXIT;
  }
  
  length_low = 1 + nparam;
  rc = write_serial_with_escape(hdl, length_low, &crc16);
  if (rc != 1)
  {
    rc = RET_COMMAND_ERR_WRITE;
    goto EXIT;
  }

  /* write command */
  rc = write_serial_with_escape(hdl, command, &crc16);
  if (rc != 1)
  {
    rc = RET_COMMAND_ERR_WRITE;
    goto EXIT;
  }

  /* write params */
  if (nparam) 
  {
    for (i = 0; i < nparam; i++)
    {
      rc = write_serial_with_escape(hdl, params[i], &crc16);
      if (rc != 1)
      {
        rc = RET_COMMAND_ERR_WRITE;
        goto EXIT;
      }
    }
  }

  /* write checksum CRC16 */
  checksum[0] = (crc16 >> 8) & 0xff;
  checksum[1] = crc16 & 0xff;
  rc = write_serial(hdl, checksum, 2); 
  if (rc != 2)
  {
    rc = RET_COMMAND_ERR_WRITE;
    goto EXIT;
  }

  rc = RET_COMMAND_OK;

EXIT:
  return rc;
}


static int write_serial_with_escape(SERHDL hdl, unsigned char byte,
                                    unsigned short *crc16)
{
  int rc;

  switch (byte)
  {
    case STX:
      rc = write_and_crc_byte(hdl, ESC, crc16);
      if (rc != 1) 
      {
        goto EXIT;
      }
      rc = write_and_crc_byte(hdl, STX | FLAG, crc16);
      break;

    case ESC:
      rc = write_and_crc_byte(hdl, ESC, crc16);
      if (rc != 1) 
      {
        goto EXIT;
      }
      rc = write_and_crc_byte(hdl, ESC | FLAG, crc16);
      break;

    default:
      rc = write_and_crc_byte(hdl, byte, crc16);
      break;
  }

EXIT:
  return rc;
}


static int write_and_crc_byte(SERHDL hdl, unsigned char byte,
                              unsigned short *crc16)
{
  int rc;

  rc = write_serial(hdl, &byte, 1); 
  if (rc == 1)
  {
    *crc16 = calc_crc16(*crc16, byte);
  }

  return rc;
}


static int receive_response(SERHDL hdl, unsigned char *response, int rsplen)
{
  int rc;
  
  rc = read_serial(hdl, response, rsplen);
  if ( (rc == -1) || (rc != rsplen) )
  {
    rc = RET_COMMAND_ERR_READ;
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
