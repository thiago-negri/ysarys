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

#ifndef RULE_H
#define RULE_H

#include "date.h"
#include "intdef.h"

enum
{
	RULE_OK = 0,
	RULE_EINVALNUM,
	RULE_EOOM
};

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

int rule_compile(const char *input, usize input_count, struct rule **ret_rule);

void rule_free(struct rule *rule);

int rule_matches(struct rule *rule, struct weekdate *date);

#endif /* !RULE_H */
