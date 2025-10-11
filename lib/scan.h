#ifndef SCAN_H
#define SCAN_H

enum
{
	SCAN_OK,
	SCAN_EINVAL
};

int scan_int(const unsigned char *input, int count, int *ret_value);

#endif /* SCAN_H */
