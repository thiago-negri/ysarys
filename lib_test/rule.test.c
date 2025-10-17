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

#include "../lib/rule.h"
#include <stdio.h>
#include <stdlib.h>

const char *current_group;
struct weekdate date_value = WEEKDATE_ZERO;

void
fail(const char *message, int expected, int actual)
{
	const char *format = current_group
	                         ? "\nFAIL: %s (expected: %d, actual: %d)\n"
	                         : "FAIL: %s (expected: %d, actual: %d)\n";
	fprintf(stderr, format, message, expected, actual);
	exit(EXIT_FAILURE);
}

void
assert_equal(const char *message, int expected, int actual)
{
	if (expected != actual)
		fail(message, expected, actual);
}

void
test_group(const char *group)
{
	if (current_group)
		fprintf(stderr, " OK\n");
	fprintf(stderr, "> %s", group);
	current_group = group;
}

void
test_done(void)
{
	if (current_group)
		fprintf(stderr, " OK\n");
	current_group = NULL;
}

struct weekdate *
date(int year, int month, int day, int week_day)
{
	date_value.year = year;
	date_value.month = month;
	date_value.day = day;
	date_value.week_day = week_day;
	return &date_value;
}

int
main(void)
{
	struct rule *rule = NULL;

	test_group("rule: <empty>");
	assert_equal("compile", 0, rule_compile("*", 1, &rule));
	assert_equal("year", MATCHER_TYPE_ALL, rule->year.type);
	assert_equal("month", MATCHER_TYPE_ALL, rule->month.type);
	assert_equal("day", MATCHER_TYPE_ALL, rule->day.type);
	assert_equal("week_day", MATCHER_TYPE_ALL, rule->week_day.type);
	rule_free(rule);

	test_group("rule: y* m1 d2,3 w-3.-1");
	assert_equal("compile", 0,
	             rule_compile("y* m1 d2,3 w-3.-1", 17, &rule));
	assert_equal("year", MATCHER_TYPE_ALL, rule->year.type);
	assert_equal("month", MATCHER_TYPE_SIMPLE, rule->month.type);
	assert_equal("month.value", 1, rule->month.data.simple.value);
	assert_equal("day", MATCHER_TYPE_MULTI, rule->day.type);
	assert_equal("day.count", 2, rule->day.data.multi.count);
	assert_equal("day.0", MATCHER_TYPE_SIMPLE,
	             rule->day.data.multi.array[0].type);
	assert_equal("day.0.value", 2,
	             rule->day.data.multi.array[0].data.simple.value);
	assert_equal("day.1", MATCHER_TYPE_SIMPLE,
	             rule->day.data.multi.array[1].type);
	assert_equal("day.1.value", 3,
	             rule->day.data.multi.array[1].data.simple.value);
	assert_equal("week_day", MATCHER_TYPE_RANGE, rule->week_day.type);
	assert_equal("week_day.from", -3, rule->week_day.data.range.from);
	assert_equal("week_day.to", -1, rule->week_day.data.range.to);
	rule_free(rule);

	test_group("rule: d1");
	assert_equal("compile", 0, rule_compile("d1", 2, &rule));
	assert_equal("jan 1st", 1,
	             rule_matches(rule, date(2024, 1, 1, WEEK_DAY_MONDAY)));
	assert_equal("jan 2nd", 0,
	             rule_matches(rule, date(2024, 1, 2, WEEK_DAY_THURSDAY)));
	assert_equal("feb 1st", 1,
	             rule_matches(rule, date(2024, 2, 1, WEEK_DAY_FRIDAY)));
	assert_equal("mar 1st", 1,
	             rule_matches(rule, date(2024, 3, 1, WEEK_DAY_MONDAY)));
	assert_equal("apr 1st", 1,
	             rule_matches(rule, date(2024, 4, 1, WEEK_DAY_WEDNESDAY)));
	assert_equal("may 1st", 1,
	             rule_matches(rule, date(2024, 5, 1, WEEK_DAY_SATURDAY)));
	assert_equal("jun 1st", 1,
	             rule_matches(rule, date(2024, 6, 1, WEEK_DAY_MONDAY)));
	assert_equal("jul 1st", 1,
	             rule_matches(rule, date(2024, 7, 1, WEEK_DAY_THURSDAY)));
	assert_equal("aug 1st", 1,
	             rule_matches(rule, date(2024, 8, 1, WEEK_DAY_SUNDAY)));
	assert_equal("sep 1st", 1,
	             rule_matches(rule, date(2024, 9, 1, WEEK_DAY_TUESDAY)));
	assert_equal("oct 1st", 1,
	             rule_matches(rule, date(2024, 10, 1, WEEK_DAY_TUESDAY)));
	assert_equal("nov 1st", 1,
	             rule_matches(rule, date(2024, 11, 1, WEEK_DAY_FRIDAY)));
	assert_equal("dec 1st", 1,
	             rule_matches(rule, date(2024, 12, 1, WEEK_DAY_SUNDAY)));
	rule_free(rule);

	test_group("rule: d-7.-1 w3");
	assert_equal("compile", 0, rule_compile("d-7.-1 w3", 9, &rule));
	assert_equal("jan 30th mon", 0,
	             rule_matches(rule, date(2024, 1, 30, WEEK_DAY_MONDAY)));
	assert_equal("jan 30th tue", 1,
	             rule_matches(rule, date(2024, 1, 30, WEEK_DAY_TUESDAY)));
	assert_equal("jan 30th wed", 0,
	             rule_matches(rule, date(2024, 1, 30, WEEK_DAY_WEDNESDAY)));
	assert_equal("feb 27th", 1,
	             rule_matches(rule, date(2024, 2, 27, WEEK_DAY_TUESDAY)));
	assert_equal("jan 23rd", 0,
	             rule_matches(rule, date(2024, 1, 23, WEEK_DAY_TUESDAY)));
	assert_equal("jan 29th", 0,
	             rule_matches(rule, date(2024, 1, 29, WEEK_DAY_MONDAY)));
	assert_equal("jan 31st", 0,
	             rule_matches(rule, date(2024, 1, 31, WEEK_DAY_WEDNESDAY)));
	assert_equal("feb 20th", 0,
	             rule_matches(rule, date(2024, 1, 20, WEEK_DAY_TUESDAY)));
	rule_free(rule);

	test_done();
	return 0;
}
