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

#ifndef RULE_LUA_H
#define RULE_LUA_H

#include "date.h"
#include <lua.h>

enum
{
	RULE_OK,
	RULE_EOOM,
	RULE_ELUA
};

struct rule
{
	lua_State *lua_state;
	size_t rule_count;
};

int rule_alloc(struct rule **ret_rule);

int rule_add(struct rule *rule, const char *lua_source,
             const char **reterr_lua_error);

int rule_run(struct rule *rule, struct weekdate *date, size_t *reterr_index,
             const char **reterr_lua_error);

void rule_lua_free(struct rule *rule);

#endif /* !RULE_LUA_H */
