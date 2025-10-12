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

#include "rule.h"
#include "intdef.h"
#include "scan.h"
#include <stdlib.h>

static int
matcher_compile(const unsigned char *input, usize input_count,
                struct matcher *ret_matcher)
{
	struct matcher *matcher_array = NULL;
	int multi_index = 0;
	int multi_count = 1;
	isize range_index = -1;
	usize matcher_start = 0;
	usize i = 0;
	int r = 0;

	if (input_count == 1 && input[0] == '*')
	{
		ret_matcher->type = MATCHER_TYPE_ALL;
		return RULE_OK;
	}

	for (i = 0; i < input_count; i++)
	{
		switch (input[i])
		{
			case ',':
				multi_count++;
				break;
			case '.':
				range_index = i;
				break;
		}
	}

	if (multi_count > 1)
	{
		ret_matcher->type = MATCHER_TYPE_MULTI;
		ret_matcher->data.multi.count = multi_count;
	}
	else if (range_index >= 0)
		ret_matcher->type = MATCHER_TYPE_RANGE;
	else
		ret_matcher->type = MATCHER_TYPE_SIMPLE;

	switch (ret_matcher->type)
	{
		case MATCHER_TYPE_SIMPLE:
			r = scan_int(input, input_count,
			             &ret_matcher->data.simple.value);
			if (r != RULE_OK)
				goto _done;
			break;

		case MATCHER_TYPE_RANGE:
			r = scan_int(input, range_index,
			             &ret_matcher->data.range.from);
			if (r != RULE_OK)
				goto _done;
			r = scan_int(&input[range_index + 1],
			             input_count - range_index - 1,
			             &ret_matcher->data.range.to);
			if (r != RULE_OK)
				goto _done;
			break;

		case MATCHER_TYPE_MULTI:
			matcher_array =
			    malloc(sizeof(struct matcher) * multi_count);
			if (matcher_array == NULL)
			{
				r = RULE_EOOM;
				goto _done;
			}
			for (i = 0; i < input_count; i++)
			{
				switch (input[i])
				{
					case ',':
						r = matcher_compile(
						    &input[matcher_start],
						    i - matcher_start,
						    &matcher_array
						        [multi_index]);
						if (r != RULE_OK)
							goto _done;
						matcher_start = i + 1;
						multi_index++;
						break;
				}
			}
			r = matcher_compile(&input[matcher_start],
			                    i - matcher_start,
			                    &matcher_array[multi_index]);
			if (r != RULE_OK)
				goto _done;
			ret_matcher->data.multi.array = matcher_array;
			matcher_array = NULL;
			break;
	}

	r = RULE_OK;
_done:
	if (r != RULE_OK && matcher_array != NULL)
		free(matcher_array);
	return r;
}

/* non-0 = match */
static int
matcher_matches(struct matcher *matcher, int value)
{
	int i = 0;

	switch (matcher->type)
	{
		case MATCHER_TYPE_ALL:
			return 1;

		case MATCHER_TYPE_SIMPLE:
			return matcher->data.simple.value == value;

		case MATCHER_TYPE_RANGE:
			return matcher->data.range.from <= value &&
			       matcher->data.range.to >= value;

		case MATCHER_TYPE_MULTI:
			for (i = 0; i < matcher->data.multi.count; i++)
				if (matcher_matches(
				        &matcher->data.multi.array[i], value))
					return 1;

			return 0;
	}

	return 0;
}

int
rule_compile(const unsigned char *input, usize input_count,
             struct rule **ret_rule)
{
	struct matcher *current_matcher = NULL;
	usize matcher_start = 0;
	usize i = 0;
	int r = 0;

	*ret_rule = malloc(sizeof(struct rule));
	if (*ret_rule == NULL)
	{
		r = RULE_EOOM;
		goto _done;
	}

	(*ret_rule)->year.type = MATCHER_TYPE_ALL;
	(*ret_rule)->month.type = MATCHER_TYPE_ALL;
	(*ret_rule)->day.type = MATCHER_TYPE_ALL;
	(*ret_rule)->week_day.type = MATCHER_TYPE_ALL;

	for (i = 0; i < input_count; i++)
	{
		switch (input[i])
		{
			case ' ':
			case 'y':
			case 'm':
			case 'd':
			case 'w':
				if (current_matcher != NULL)
				{
					r = matcher_compile(
					    &input[matcher_start],
					    i - matcher_start, current_matcher);
					if (r != RULE_OK)
						goto _done;
				}

				switch (input[i])
				{
					case 'y':
						current_matcher =
						    &(*ret_rule)->year;
						matcher_start = i + 1;
						break;

					case 'm':
						current_matcher =
						    &(*ret_rule)->month;
						matcher_start = i + 1;
						break;

					case 'd':
						current_matcher =
						    &(*ret_rule)->day;
						matcher_start = i + 1;
						break;

					case 'w':
						current_matcher =
						    &(*ret_rule)->week_day;
						matcher_start = i + 1;
						break;

					default:
						current_matcher = NULL;
						break;
				}
				break;
		}
	}

	if (current_matcher != NULL)
	{
		r = matcher_compile(&input[matcher_start], i - matcher_start,
		                    current_matcher);
		if (r != RULE_OK)
			goto _done;
	}

	r = RULE_OK;
_done:
	if (r != RULE_OK && *ret_rule != NULL)
	{
		free(*ret_rule);
		*ret_rule = NULL;
	}
	return r;
}

void
rule_free(struct rule *rule)
{
	if (rule->year.type == MATCHER_TYPE_MULTI)
		free(rule->year.data.multi.array);
	if (rule->month.type == MATCHER_TYPE_MULTI)
		free(rule->month.data.multi.array);
	if (rule->day.type == MATCHER_TYPE_MULTI)
		free(rule->day.data.multi.array);
	if (rule->week_day.type == MATCHER_TYPE_MULTI)
		free(rule->week_day.data.multi.array);
	free(rule);
}

/* non-0 = match */
int
rule_matches(struct rule *rule, struct weekdate *date)
{
	if (!matcher_matches(&rule->year, date->year))
		return 0;
	if (!matcher_matches(&rule->month, date->month))
		return 0;
	if (!matcher_matches(&rule->day, date->day) &&
	    !matcher_matches(&rule->day,
	                     date_negative_day((struct date *)date)))
		return 0;
	if (!matcher_matches(&rule->week_day, date->week_day))
		return 0;
	return 1;
}
