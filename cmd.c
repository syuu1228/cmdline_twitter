#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <signal.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <histedit.h>

#include <sys/ioctl.h>
#include <sys/poll.h>
#include <termios.h>

#define CLI_LUA "/cli/cli.lua"
#define CLI_LUA_PATH "/usr/share/lua/5.2/?.lua;/cli/lib/?.lua;/cli/lua/share/?.lua"
#define CLI_LUA_CPATH "/usr/lib/lua/5.2/?.so;/cli/lua/lib/?.so"
#define CLI_COMMANDS_PATH "/cli/commands"

#define PROMPT_MAXLEN 128
static char sprompt[PROMPT_MAXLEN];

/* Console size handling */
static int con_height = 24;
static int con_width = 80;

/* Report console size back to Lua */
static int cli_lua_console_dim(lua_State *);

static  lua_State *L;
lua_State *cli_luaL_newstate();
void       cli_luaL_renewstate(lua_State**);
char      *cli_prompt(EditLine*);

/* Command execution interrupt */
static int  cli_interrupted = 0;
static int  cli_lua_interrupted(lua_State *);

/* Context */
#define CTXC 5
static char* ctxv[CTXC];
static struct {
  char* name; char* env; struct option lopt;
} ctx[CTXC] = {
  {"api", "OSV_API", {"api", required_argument, 0, 'a'}},
  {"ssl_key", "OSV_SSL_KEY", {"key", required_argument, 0, 'k'}},
  {"ssl_cert", "OSV_SSL_CERT", {"cert", required_argument, 0, 'c'}},
  {"ssl_cacert", "OSV_SSL_CACERT", {"cacert", required_argument, 0, 'C'}},
  {"ssl_verify", "OSV_SSL_VERIFY", {"verify", required_argument, 0, 'V'}}
};

void cmd_init(void) {
  int i;

  /* Context from environment variables */
  for (i=0; i<CTXC; i++) {
    ctxv[i] = getenv(ctx[i].env);
  }

  /* Lua state */
  L = cli_luaL_newstate();
}

int cmd_run(const char *line) {
  /* If lua failed to load previously, retry */
  if (L == NULL) {
    cli_luaL_renewstate(&L);
  }

  /* Pass the line, as is, to cli() */
  lua_getglobal(L, "cli_command");
  lua_pushstring(L, line);

  /* Reset "interrupted" state */
  cli_interrupted = 0;
  int error = lua_pcall(L, 1, 0, 0);
  if (error) {
    fprintf(stderr, "%s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
  }

  return 0;
}

void cli_luaL_renewstate(lua_State **L) {
  fprintf(stderr, "\nRestarting shell\n");
  if (*L != NULL) {
    lua_close(*L);
  }

  *L = cli_luaL_newstate();
}

void cli_lua_settable(lua_State *L, char *table, char *key, const char *value) {
  lua_getglobal(L, table);
  lua_pushstring(L, key);
  lua_pushstring(L, value);
  lua_settable(L, -3);
  lua_pop(L, 1);
}

lua_State *cli_luaL_newstate() {
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);

  cli_lua_settable(L, "package", "path", CLI_LUA_PATH);
  cli_lua_settable(L, "package", "cpath", CLI_LUA_CPATH);

  int error = luaL_loadfile(L, CLI_LUA) || lua_pcall(L, 0, 0, 0);
  if (error) {
    fprintf(stderr, "Failed to load shell: %s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
    lua_close(L);

    return NULL;
  }

  cli_lua_settable(L, "context", "commands_path", CLI_COMMANDS_PATH);
  for (int i=0; i<CTXC; i++) {
    if (ctxv[i]) {
      cli_lua_settable(L, "context", ctx[i].name, ctxv[i]);
    }
  }

  /* Bind some functions into Lua */
  lua_pushcfunction(L, cli_lua_console_dim);
  lua_setglobal(L, "cli_console_dim");
  lua_pushcfunction(L, cli_lua_interrupted);
  lua_setglobal(L, "cli_interrupted");

  return L;
}

char *cli_prompt(EditLine *e) {
  /* Get the shell prompt from Lua */
  lua_getglobal(L, "prompt");
  int error = lua_pcall(L, 0, 1, 0);
  if (error) {
    fprintf(stderr, "%s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
  } else {
    if (!lua_isnil(L, -1)) {
      const char *lprompt = lua_tostring(L, -1);
      int len = strlen(lprompt);
      snprintf(sprompt, len < PROMPT_MAXLEN ? len+1 : PROMPT_MAXLEN, "%s", lprompt);
      lua_pop(L, 1);
      return sprompt;
    }
  }

  /* Default to an empty prompt in case of an error */
  return (char *)"# ";
}

/* Returns console size to Lua */
static int cli_lua_console_dim(lua_State *L) {
  lua_pushnumber(L, con_height);
  lua_pushnumber(L, con_width);
  return 2;
}

/* Interrupt a running command */
static int  cli_lua_interrupted(lua_State *L) {
  lua_pushboolean(L, cli_interrupted);
  return 1;
}

