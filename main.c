#include <stdio.h>
#include <string.h>

#include <serial.h>
#include <command.h>
#include <pattern.h>
#include <crc16.h>

/* local constants */
#define RET_OK                      (0)
#define RET_ERR_USAGE               (1)
#define RET_ERR_GET_FIRMWAREVERSION (2)
#define RET_ERR_DISPLAY_TEXT        (3)
#define RET_ERR_STORE_TEXT          (4)
#define RET_ERR_SET_TEXTSPEED       (5)
#define RET_ERR_DISPLAY_PATTERN     (6)
#define RET_ERR_STORE_PATTERN       (7)
#define RET_ERR_SET_NORMALMODE      (8)
#define RET_ERR_SET_TEXTMODE        (9)
#define RET_ERR_SET_PATTERNMODE     (10)
#define RET_ERR_EXE_FACTORYRESET    (11)

#define CMD_NOMATCH (0)

/* local types */
typedef int (*CMD_FCT)(SERHDL hdl, int myargc, char **myargv);

typedef struct {
  char   *cmd_name;       /* name as typed on the command line */
  int     cmd_nargs;      /* no of arguments this command needs */
  CMD_FCT cmd_fct;        /* pointer to command function */
  int     cmd_rc;         /* process failed exit code for this command */
} CMD;


static int find_command(int nargs, char *command);
static void print_usage(void);


/* local variables */
static CMD cmd_table[] =
{
/*  cmd_name,          nargs, command fct,       rc */
  { "",                0,   NULL,                RET_ERR_USAGE },/* no match */
  { "firmwareversion", 0,   get_firmwareversion, RET_ERR_GET_FIRMWAREVERSION },
  { "displaytext",     1,   display_text,        RET_ERR_DISPLAY_TEXT },
  { "storetext",       1,   store_text,          RET_ERR_STORE_TEXT },
  { "settextspeed",    1,   set_textspeed,       RET_ERR_SET_TEXTSPEED },
  { "displaypattern",  1,   display_pattern,     RET_ERR_DISPLAY_PATTERN },
  { "storepattern",    1,   store_pattern,       RET_ERR_STORE_PATTERN },
  { "setnormalmode",   0,   set_normalmode,      RET_ERR_SET_NORMALMODE },
  { "settextmode",     0,   set_textmode,        RET_ERR_SET_TEXTMODE },
  { "setpatternmode",  0,   set_patternmode,     RET_ERR_SET_PATTERNMODE },
  { "factoryreset",    0,   exe_factoryreset,    RET_ERR_EXE_FACTORYRESET },
};


/* code section */
int main(int argc, char **argv)
{
  int rc;
  int cmd;
  SERHDL hdl;

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

  if ((rc = open_serial(argv[1], &hdl)) != RET_SERIAL_OK)
  {
    fprintf(stderr, "open of device %s has failed.\n", argv[1]);
    goto EXIT;
  }
 
  rc = cmd_table[cmd].cmd_fct(hdl, argc - 3, &argv[3]);
  if (rc != RET_OK)
  {
    rc = cmd_table[cmd].cmd_rc;
  }

  close_serial(hdl);

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
  fprintf(stderr, "       mmm8x8 <serial device> settextspeed "
                  "<speed: 0-255>\n");
  fprintf(stderr, "       mmm8x8 <serial device> displaypattern <inputfile>\n");
  fprintf(stderr, "       mmm8x8 <serial device> storepattern <inputfile>\n");
  fprintf(stderr, "       mmm8x8 <serial device> setnormalmode\n");
  fprintf(stderr, "       mmm8x8 <serial device> settextmode\n");
  fprintf(stderr, "       mmm8x8 <serial device> setpatternmode\n");
  fprintf(stderr, "       mmm8x8 <serial device> factoryreset\n");
}
