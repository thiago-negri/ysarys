#ifndef DATE_H
#define DATE_H

#include <stdio.h> /* IWYU pragma: keep ... FILE* */
#include <time.h>  /* IWYU pragma: keep ... time_t */

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
	int week_day;
};

#define DATE_ZERO { 0, 0, 0, 0 }

void date_from_time(time_t time, struct date *ret_date);
time_t date_to_time(struct date *date);

void date_add_days(struct date *date, int days, struct date *ret_date);
int date_negative_day(struct date *date);
int date_compare(struct date *a, struct date *b);
void date_fprintf(FILE *fd, struct date *date);

const char *date_week_day_string(int week_day);

int date_month_last_day(int year, int month);

#endif /* !DATE_H */
