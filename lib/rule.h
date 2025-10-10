#ifndef RULE_H
#define RULE_H

#include "date.h"
#include "intdef.h"

enum
{
  MATCHER_TYPE_ALL = 0,
  MATCHER_TYPE_SIMPLE,
  MATCHER_TYPE_RANGE,
  MATCHER_TYPE_MULTI
};

struct matcher_simple
{
  int value;
};

struct matcher_range
{
  int from;
  int to;
};

struct matcher_multi
{
  int count;
  struct matcher *array;
};

struct matcher
{
  int type;
  union
  {
    struct matcher_simple simple;
    struct matcher_range range;
    struct matcher_multi multi;
  } data;
};

struct rule
{
  struct matcher year;
  struct matcher month;
  struct matcher day;
  struct matcher week_day;
};

int rule_compile(const unsigned char *input, usize input_count, struct rule **ret_rule);

void rule_free(struct rule *rule);

int rule_matches(struct rule *rule, struct date *date);

#endif /* !RULE_H */
