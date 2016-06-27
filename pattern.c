#include <stdio.h>
#include <stdlib.h>

#define PATTERN_SRC 1
#include <pattern.h>
#undef PATTERN_SRC

int open_patternfile(char *path, FILE **handle)
{
  int rc;

  if ((*handle = fopen(path, "r")) == NULL)
  {
    rc = RET_PATTERN_ERR_OPEN;
    goto EXIT;
  }

  rc = RET_PATTERN_OK;

EXIT:
  return rc;
}


int read_patternfile(FILE *handle, unsigned char *linevalue)
{
#define MAX_LINE (1024)
  int rc;
  int i;
  char buf[MAX_LINE];

#define BITS_PER_LINE (8)
#define BIT_SET_CHAR 'x'

  if (fgets(buf, MAX_LINE, handle) == NULL)
  {
    rc = RET_PATTERN_ERR_READ;
    goto EXIT;
  }

  *linevalue = 0;
  for (i = 0; i < BITS_PER_LINE; i++)
  {
    *linevalue = (*linevalue << 1);
    if (*(buf + i) == BIT_SET_CHAR)
    {
      *linevalue |= 1;
    }
  }

  rc = RET_PATTERN_OK;

EXIT:
  return rc;
}


int close_patternfile(FILE *handle)
{
  int rc;

  if (fclose(handle) != 0)
  {
    rc = RET_PATTERN_ERR_CLOSE;
    goto EXIT;
  } 

  rc = RET_PATTERN_OK;

EXIT:
  return rc;
}
