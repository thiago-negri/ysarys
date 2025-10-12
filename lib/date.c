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

#include "date.h"
#include <stddef.h>

#define SECS_PER_MINUTE            (60)
#define SECS_PER_DAY               (24 * 60 * 60)
#define BRAZIL_TIMEZONE_IN_MINUTES (-180)

/* https://howardhinnant.github.io/date_algorithms.html#civil_from_days */
void
weekdate_from_time(time_t time, struct weekdate *ret_date)
{
	time_t time_tz, z, era, doe, yoe, y, doy, mp, d, m, year, month, day,
	    week_day;

	time_tz = time + (BRAZIL_TIMEZONE_IN_MINUTES * SECS_PER_MINUTE);

	z = time_tz / SECS_PER_DAY + 719468;
	era = ((z >= 0) ? z : z - 146096) / 146097;
	doe = (z - era * 146097);
	yoe = (doe - (doe / 1460) + (doe / 36524) - (doe / 146096)) / 365;
	y = yoe + era * 400;
	doy = doe - (365 * yoe + (yoe / 4) - (yoe / 100));
	mp = (5 * doy + 2) / 153;
	d = doy - (153 * mp + 2) / 5 + 1;
	m = (mp < 10) ? mp + 3 : mp - 9;

	year = (m <= 2) ? y + 1 : y;
	month = m;
	day = d;
	week_day = (z + 3) % 7;

	ret_date->year = year;
	ret_date->month = month;
	ret_date->day = day;
	ret_date->week_day = week_day;
}

/* https://howardhinnant.github.io/date_algorithms.html#days_from_civil */
time_t
date_to_time(struct date *date)
{
	time_t y, m, d, era, yoe, doy, doe;

	y = date->year;
	m = date->month;
	d = date->day;
	if (m <= 2)
		y -= 1;
	era = ((y >= 0) ? y : y - 399) / 400;
	yoe = y - era * 400;
	if (yoe < 0)
		yoe *= -1;
	doy = ((153 * ((m > 2) ? m - 3 : m + 9) + 2) / 5) + d - 1;
	doe = yoe * 365 + (yoe / 4) - (yoe / 100) + doy;
	return SECS_PER_DAY * (era * 146097 + doe - 719468);
}

void
weekdate_add_days(struct weekdate *date, int days, struct weekdate *ret_date)
{
	int new_day = 0;
	int new_month = 0;
	int new_year = 0;
	int last_day_of_month = 0;
	int new_week_day = 0;

	new_day = date->day + days;
	new_week_day = new_day % 7;
	new_month = date->month;
	new_year = date->year;
	last_day_of_month = date_month_last_day(new_year, new_month);
	while (new_day > last_day_of_month)
	{
		new_day -= last_day_of_month;
		new_month = (new_month % 12) + 1;
		if (new_month == MONTH_JANUARY)
			new_year += 1;

		last_day_of_month = date_month_last_day(new_year, new_month);
	}

	ret_date->year = new_year;
	ret_date->month = new_month;
	ret_date->day = new_day;
	ret_date->week_day = new_week_day;
}

int
date_negative_day(struct date *date)
{
	return date->day - date_month_last_day(date->year, date->month) - 1;
}

int
date_compare(struct date *a, struct date *b)
{
	if (a->year < b->year)
		return -1;
	if (a->year > b->year)
		return 1;

	if (a->month < b->month)
		return -1;
	if (a->month > b->month)
		return 1;

	if (a->day < b->day)
		return -1;
	if (a->day > b->day)
		return 1;

	return 0;
}

const char *
weekdate_week_day_string(int week_day)
{
	switch (week_day)
	{
		case WEEK_DAY_SUNDAY:
			return "Monday";

		case WEEK_DAY_MONDAY:
			return "Tuesday";

		case WEEK_DAY_TUESDAY:
			return "Wednesday";

		case WEEK_DAY_WEDNESDAY:
			return "Thursday";

		case WEEK_DAY_THURSDAY:
			return "Friday";

		case WEEK_DAY_FRIDAY:
			return "Saturday";

		case WEEK_DAY_SATURDAY:
			return "Sunday";
	}

	return NULL;
}

int
date_month_last_day(int year, int month)
{
	switch (month)
	{
		case MONTH_JANUARY:
		case MONTH_MARCH:
		case MONTH_MAY:
		case MONTH_JULY:
		case MONTH_AUGUST:
		case MONTH_OCTOBER:
		case MONTH_DECEMBER:
			return 31;

		case MONTH_APRIL:
		case MONTH_JUNE:
		case MONTH_SEPTEMBER:
		case MONTH_NOVEMBER:
			return 30;

		case MONTH_FEBRUARY:
			return (year % 4 == 0 && year % 100 != 0) ? 29 : 28;
	}

	return -1;
}

void
weekdate_fprintf(FILE *fd, struct weekdate *date)
{
	fprintf(fd, "%d-", date->year);
	if (date->month < 10)
		fprintf(fd, "0%d-", date->month);
	else
		fprintf(fd, "%d-", date->month);
	if (date->day < 10)
		fprintf(fd, "0%d", date->day);
	else
		fprintf(fd, "%d", date->day);
}
