/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2024 Silverlan */

#include "pragma/lua/converters/global_string_converter.hpp"
#include <luabind/object.hpp>

void luabind::default_converter<pragma::GString>::to_lua(lua_State *L, pragma::GString const &x)
{
	std::string str = x;
	luabind::object {L, str}.push(L);
}

void luabind::default_converter<pragma::GString>::to_lua(lua_State *L, pragma::GString *x)
{
	if(!x)
		lua_pushnil(L);
	else {
		std::string str = *x;
		to_lua(L, str);
	}
}
