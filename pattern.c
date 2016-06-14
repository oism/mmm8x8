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
  int rc;
  char *line;
  size_t nchar;
  int i;

#define BITS_PER_LINE (8)
#define BIT_SET_CHAR 'x'

  line = NULL;
  if ((rc = getline(&line, &nchar, handle)) == -1)
  {
    rc = RET_PATTERN_ERR_READ;
    goto EXIT;
  }

  *linevalue = 0;
  for (i = 0; i < BITS_PER_LINE; i++)
  {
    *linevalue = (*linevalue << 1);
    if (*(line + i) == BIT_SET_CHAR)
    {
      *linevalue |= 1;
    }
  }

  free(line);
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
