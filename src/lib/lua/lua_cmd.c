/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "lua.h"
#include "lauxlib.h"
#include "shell/cmd.h"

static int print(lua_State *L)
{
	int n=lua_gettop(L);
	int i;
	for (i=1; i<=n; i++) {
		if (i>1)
			printf("\t");
		if (lua_isstring(L,i))
			printf("%s",lua_tostring(L,i));
		else if (lua_isnil(L,i))
			printf("%s","nil");
		else if (lua_isboolean(L,i))
			printf("%s",lua_toboolean(L,i) ? "true" : "false");
		else
			printf("%s:%p",luaL_typename(L,i),lua_topointer(L,i));
	}
	printf("\n");
	return 0;
}

int cmd_lua_func(int argc, char *argv[])
{
	if(argc > 1) {
		lua_State *L=lua_open();
		luaL_openlibs(L);
		lua_register(L,"print",print);
		if (luaL_dostring(L,argv[1])!=0) fprintf(stderr,"%s\n",lua_tostring(L,-1));
		lua_close(L);
	}
	return 0;
}

const cmd_t cmd_lua = {"lua", cmd_lua_func, "lua script parser"};
DECLARE_SHELL_CMD(cmd_lua)

