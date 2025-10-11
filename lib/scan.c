#include "scan.h"
#include <assert.h>

int
scan_int(const unsigned char *input, int input_count, int *ret_value)
{
  int result = 0;
  int sign = 1;
  int i = 0;

  /* A 32 bit number can't have more than 10 digits. */
  assert(input_count > 0 && input_count < 11);

  if (input[0] == '-')
  {
    sign = -1;
    i = 1;
  }

  for (; i < input_count; i++)
  {
    if (input[i] < '0' || input[i] > '9') return SCAN_EINVAL;
    result *= 10;
    result += input[i] - '0';
  }

  result *= sign;
  *ret_value = result;
  return SCAN_OK;
}
