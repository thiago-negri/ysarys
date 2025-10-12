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

#ifndef DATE_H
#define DATE_H

#include <stdio.h> /* IWYU pragma: keep ... FILE* */
#include <time.h>  /* IWYU pragma: keep ... time_t */

#define SECS_PER_MINUTE            (60)
#define SECS_PER_DAY               (24 * 60 * 60)
#define BRAZIL_TIMEZONE_IN_MINUTES (-180)

enum
{
	MONTH_JANUARY = 1,
	MONTH_FEBRUARY,
	MONTH_MARCH,
	MONTH_APRIL,
	MONTH_MAY,
	MONTH_JUNE,
	MONTH_JULY,
	MONTH_AUGUST,
	MONTH_SEPTEMBER,
	MONTH_OCTOBER,
	MONTH_NOVEMBER,
	MONTH_DECEMBER
};

enum
{
	WEEK_DAY_SUNDAY = 1,
	WEEK_DAY_MONDAY,
	WEEK_DAY_TUESDAY,
	WEEK_DAY_WEDNESDAY,
	WEEK_DAY_THURSDAY,
	WEEK_DAY_FRIDAY,
	WEEK_DAY_SATURDAY
};

struct date
{
	int year;
	int month;
	int day;
};

struct weekdate
{
	int year;
	int month;
	int day;
	int week_day;
};

#define DATE_ZERO     { 0, 0, 0 }
#define WEEKDATE_ZERO { 0, 0, 0, 0 }

void weekdate_from_time(time_t time, struct weekdate *ret_date);
void weekdate_fprintf(FILE *fd, struct weekdate *date);
const char *weekdate_week_day_string(int week_day);
void weekdate_add_days(struct weekdate *date, int days,
                       struct weekdate *ret_date);

time_t date_to_time(struct date *date);
int date_negative_day(struct date *date);
int date_compare(struct date *a, struct date *b);
int date_month_last_day(int year, int month);

#endif /* !DATE_H */
