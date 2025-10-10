#include "date.h"
#include <stddef.h>

#define SECS_PER_MINUTE            (60)
#define SECS_PER_DAY               (24 * 60 * 60)
#define BRAZIL_TIMEZONE_IN_MINUTES (-180)

void
date_from_time(time_t time, struct date *ret_date)
{
  long time_tz = time + (BRAZIL_TIMEZONE_IN_MINUTES * SECS_PER_MINUTE);

  /* https://howardhinnant.github.io/date_algorithms.html#civil_from_days */
  long z = time_tz / SECS_PER_DAY + 719468;
  long era = ((z >= 0) ? z : z - 146096) / 146097;
  long doe = (z - era * 146097);
  long yoe = (doe - (doe / 1460) + (doe / 36524) - (doe / 146096)) / 365;
  long y = yoe + era * 400;
  long doy = doe - (365 * yoe + (yoe / 4) - (yoe / 100));
  long mp = (5 * doy + 2) / 153;
  long d = doy - (153 * mp + 2) / 5 + 1;
  long m = (mp < 10) ? mp + 3 : mp - 9;

  int year = (m <= 2) ? y + 1 : y;
  int month = m;
  int day = d;
  int week_day = (z + 3) % 7;

  ret_date->year = year;
  ret_date->month = month;
  ret_date->day = day;
  ret_date->week_day = week_day;
}

void
date_add_days(struct date *date, int days, struct date *ret_date)
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
  last_day_of_month = month_last_day(new_year, new_month);
  while (new_day > last_day_of_month)
  {
    new_day -= last_day_of_month;
    new_month = (new_month % 12) + 1;
    if (new_month == MONTH_JANUARY)
      new_year += 1;

    last_day_of_month = month_last_day(new_year, new_month);
  }

  ret_date->year = new_year;
  ret_date->month = new_month;
  ret_date->day = new_day;
  ret_date->week_day = new_week_day;
}

int
date_negative_day(struct date *date)
{
  return date->day - month_last_day(date->year, date->month) - 1;
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
week_day_string(int week_day)
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
month_last_day(int year, int month)
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
