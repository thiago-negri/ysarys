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

#include "rule_lua.h"
#include "date.h"
#include "lua.h"
#include <lauxlib.h>
#include <stdlib.h>

int
rule_alloc(struct rule **ret_rule)
{
	struct rule *rule = NULL;
	int r = 0;

	rule = malloc(sizeof *rule);
	if (rule == NULL)
	{
		r = RULE_EOOM;
		goto _done;
	}
	rule->lua_state = NULL;
	rule->rule_count = 0;

	rule->lua_state = luaL_newstate();
	if (rule->lua_state == NULL)
	{
		r = RULE_EOOM;
		goto _done;
	}

	/* Create a new array to hold all our rules, leave it at the stack. */
	lua_newtable(rule->lua_state);

	*ret_rule = rule;
	r = RULE_OK;
_done:
	if (r != RULE_OK && rule != NULL)
	{
		if (rule->lua_state != NULL)
			lua_close(rule->lua_state);
		free(rule);
	}
	return r;
}

int
rule_add_file(struct rule *rule, const char *lua_source_path,
              const char **reterr_lua_error)
{
	int r = 0;

	/* Load the Lua source. */
	if (luaL_dofile(rule->lua_state, lua_source_path) != LUA_OK)
	{
		if (reterr_lua_error != NULL)
			*reterr_lua_error = lua_tostring(rule->lua_state, -1);
		r = RULE_ELUA;
		goto _done;
	}

	/* Assign the return value to the global rules array at index. */
	lua_seti(rule->lua_state, -2, rule->rule_count);

	rule->rule_count += 1;

	r = RULE_OK;
_done:
	return r;
}

int
rule_add_string(struct rule *rule, const char *lua_source,
                const char **reterr_lua_error)
{
	int r = 0;

	/* Load the Lua source. */
	if (luaL_dostring(rule->lua_state, lua_source) != LUA_OK)
	{
		if (reterr_lua_error != NULL)
			*reterr_lua_error = lua_tostring(rule->lua_state, -1);
		r = RULE_ELUA;
		goto _done;
	}

	/* Assign the return value to the global rules array at index. */
	lua_seti(rule->lua_state, -2, rule->rule_count);

	rule->rule_count += 1;

	r = RULE_OK;
_done:
	return r;
}

int
rule_run(struct rule *rule, struct weekdate *date, size_t *reterr_index,
         const char **reterr_lua_error)
{
	const char *title = NULL;
	const char *tag_csv = NULL;
	size_t i = 0;
	int trigger_result = 0;
	int r = 0;
	int lua_top = 0;

	lua_top = lua_gettop(rule->lua_state);

	lua_createtable(rule->lua_state, 0, 4);
	/* s: G, date. */

	lua_pushnumber(rule->lua_state, date->year);
	lua_setfield(rule->lua_state, -2, "year");
	lua_pushnumber(rule->lua_state, date->month);
	lua_setfield(rule->lua_state, -2, "month");
	lua_pushnumber(rule->lua_state, date->day);
	lua_setfield(rule->lua_state, -2, "day");
	lua_pushnumber(rule->lua_state, date->week_day);
	lua_setfield(rule->lua_state, -2, "week_day");
	lua_pushnumber(rule->lua_state,
	               date_month_last_day(date->year, date->month));
	lua_setfield(rule->lua_state, -2, "last_day_of_month");

	for (i = 0; i < rule->rule_count; i++)
	{
		lua_geti(rule->lua_state, -2, i);
		/* s: G, date, G[i]. */

		lua_getfield(rule->lua_state, -1, "trigger");
		/* s: G, date, G[i], G[i].trigger. */

		if (!lua_isfunction(rule->lua_state, -1))
		{
			if (reterr_index != NULL)
				*reterr_index = i;
			if (reterr_lua_error != NULL)
				*reterr_lua_error =
				    "'trigger' is not a function";
			r = RULE_ELUA;
			goto _done;
		}

		lua_pushvalue(rule->lua_state, -3);
		/* s: G, date, G[i], G[i].trigger, date. */

		if (lua_pcall(rule->lua_state, 1, 1, 0) != LUA_OK)
		{
			if (reterr_index != NULL)
				*reterr_index = i;
			if (reterr_lua_error != NULL)
				*reterr_lua_error =
				    lua_tostring(rule->lua_state, -1);
			r = RULE_ELUA;
			goto _done;
		}
		/* s: G, date, G[i], result. */

		if (!lua_isboolean(rule->lua_state, -1))
		{
			if (reterr_index != NULL)
				*reterr_index = i;
			if (reterr_lua_error != NULL)
				*reterr_lua_error =
				    "'trigger' must return a boolean";
			r = RULE_ELUA;
			goto _done;
		}

		trigger_result = lua_toboolean(rule->lua_state, -1);

		lua_remove(rule->lua_state, -1);
		/* s: G, date, G[i]. */

		if (!trigger_result)
		{
			lua_remove(rule->lua_state, -1);
			/* s: G, date. */

			continue;
		}

		lua_getfield(rule->lua_state, -1, "title");
		/* s: G, date, G[i], G[i].title. */

		if (lua_isfunction(rule->lua_state, -1))
		{
			lua_pushvalue(rule->lua_state, -3);
			/* s: G, date, G[i], G[i].title, date. */

			if (lua_pcall(rule->lua_state, 1, 1, 0) != LUA_OK)
			{
				if (reterr_index != NULL)
					*reterr_index = i;
				if (reterr_lua_error != NULL)
					*reterr_lua_error =
					    lua_tostring(rule->lua_state, -1);
				r = RULE_ELUA;
				goto _done;
			}
			/* s: G, date, G[i], result. */

			if (!lua_isstring(rule->lua_state, -1))
			{
				if (reterr_index != NULL)
					*reterr_index = i;
				if (reterr_lua_error != NULL)
					*reterr_lua_error =
					    "'title' function must return a "
					    "string";
				r = RULE_ELUA;
				goto _done;
			}

			title = lua_tostring(rule->lua_state, -1);

			lua_remove(rule->lua_state, -1);
			/* s: G, date, G[i]. */
		}
		else if (lua_isstring(rule->lua_state, -1))
		{
			title = lua_tostring(rule->lua_state, -1);

			lua_remove(rule->lua_state, -1);
			/* s: G, date, G[i]. */
		}
		else
		{
			if (reterr_index != NULL)
				*reterr_index = i;
			if (reterr_lua_error != NULL)
				*reterr_lua_error = "'title' must be either a "
				                    "function or a string";
			r = RULE_ELUA;
			goto _done;
		}

		lua_getfield(rule->lua_state, -1, "tag_csv");
		/* s: G, date, G[i], G[i].tag_csv. */

		if (!lua_isstring(rule->lua_state, -1))
		{
			if (reterr_index != NULL)
				*reterr_index = i;
			if (reterr_lua_error != NULL)
				*reterr_lua_error =
				    "'tag_csv' must be a string";
			r = RULE_ELUA;
			goto _done;
		}

		tag_csv = lua_tostring(rule->lua_state, -1);

		lua_pop(rule->lua_state, 2);
		/* s: G, date. */

		date_fprintf(stdout, (struct date *)date);
		fprintf(stdout, "\t%s\t%s\n", title, tag_csv);
	}

	r = RULE_OK;
_done:
	lua_settop(rule->lua_state, lua_top);
	return r;
}

void
rule_lua_free(struct rule *rule)
{
	if (rule->lua_state != NULL)
		lua_close(rule->lua_state);
	free(rule);
}
