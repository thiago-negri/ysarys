/* ISC License
 *
 * Copyright (c) 2025 Thiago Negri
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "scan.h"
#include <assert.h>

int
scan_int(const unsigned char *input, int count, int *ret_value)
{
	int result = 0;
	int sign = 1;
	int i = 0;

	/* A 32 bit number can't have more than 10 digits. */
	if (count <= 0 || count > 10)
		return SCAN_EINVAL;

	if (input[0] == '-')
	{
		sign = -1;
		i = 1;
	}

	for (; i < count; i++)
	{
		if (input[i] < '0' || input[i] > '9')
			return SCAN_EINVAL;
		result *= 10;
		result += input[i] - '0';
	}

	result *= sign;
	*ret_value = result;
	return SCAN_OK;
}

int
scan_date(const unsigned char *input, int count, struct date *ret_value)
{
	int year = 0;
	int month = 0;
	int day = 0;
	int r = 0;

	/* Input must be in form 'YYYY-MM-DD'. */
	if (count != 10 || input[4] != '-' || input[7] != '-')
		return SCAN_EINVAL;

	r = scan_int(input, 4, &year);
	if (r != SCAN_OK)
		goto _done;
	if (year < 0)
	{
		r = SCAN_EINVAL;
		goto _done;
	}

	r = scan_int(&input[5], 2, &month);
	if (r != SCAN_OK)
		goto _done;
	if (month < 0)
	{
		r = SCAN_EINVAL;
		goto _done;
	}

	r = scan_int(&input[8], 2, &day);
	if (r != SCAN_OK)
		goto _done;
	if (day < 0)
	{
		r = SCAN_EINVAL;
		goto _done;
	}

	ret_value->year = year;
	ret_value->month = month;
	ret_value->day = day;
	r = SCAN_OK;
_done:
	return r;
}
