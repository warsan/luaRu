/*
** $Id: lbaselib.c,v 1.322 2018/02/27 18:47:32 roberto Exp roberto $
** Basic library
** См. Уведомление об авторских правах в lua.h
*/

#define lbaselib_c
#define LUA_LIB

#include "lprefix.h"


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"


static int luaB_print (lua_State *L) {
  int n = lua_gettop(L);  /* количество аргументов */
  int i;
  lua_getglobal(L, "tostring");
  for (i=1; i<=n; i++) {
    const char *s;
    size_t l;
    lua_pushvalue(L, -1);  /* вызываемая функция */
    lua_pushvalue(L, i);   /* значение для печати */
    lua_call(L, 1, 1);
    s = lua_tolstring(L, -1, &l);  /* получить результат */
    if (s == NULL)
      return luaL_error(L, "'tostring' должен возвращать строку в 'print'");
    if (i>1) lua_writestring("\t", 1);
    lua_writestring(s, l);
    lua_pop(L, 1);  /* поп-результат */
  }
  lua_writeline();
  return 0;
}


#define SPACECHARS	" \f\n\r\t\v"

static const char *b_str2int (const char *s, int base, lua_Integer *pn) {
  lua_Unsigned n = 0;
  int neg = 0;
  s += strspn(s, SPACECHARS);  /* пропустить начальные пробелы */
  if (*s == '-') { s++; neg = 1; }  /* знак рукоятки */
  else if (*s == '+') s++;
  if (!isalnum((unsigned char)*s))  /* нет цифры? */
    return NULL;
  do {
    int digit = (isdigit((unsigned char)*s)) ? *s - '0'
                   : (toupper((unsigned char)*s) - 'A') + 10;
    if (digit >= base) return NULL;  /* недопустимая цифра */
    n = n * base + digit;
    s++;
  } while (isalnum((unsigned char)*s));
  s += strspn(s, SPACECHARS);  /* пропустить трейлинг-пространства */
  *pn = (lua_Integer)((neg) ? (0u - n) : n);
  return s;
}


static int luaB_tonumber (lua_State *L) {
  if (lua_isnoneornil(L, 2)) {  /* стандартное преобразование? */
    luaL_checkany(L, 1);
    if (lua_type(L, 1) == LUA_TNUMBER) {  /* уже номер? */
      lua_settop(L, 1);  /* да; верни это */
      return 1;
    }
    else {
      size_t l;
      const char *s = lua_tolstring(L, 1, &l);
      if (s != NULL && lua_stringtonumber(L, s) == l + 1)
        return 1;  /* успешное преобразование в число */
      /* иначе не число */
    }
  }
  else {
    size_t l;
    const char *s;
    lua_Integer n = 0;  /* во избежание предупреждений */
    lua_Integer base = luaL_checkinteger(L, 2);
    luaL_checktype(L, 1, LUA_TSTRING);  /* нет чисел в виде строк */
    s = lua_tolstring(L, 1, &l);
    luaL_argcheck(L, 2 <= base && base <= 36, 2, "база вне диапазона");
    if (b_str2int(s, (int)base, &n) == s + l) {
      lua_pushinteger(L, n);
      return 1;
    }  /* иначе не число */
  }  /* иначе не число */
  lua_pushnil(L);  /* не число */
  return 1;
}


static int luaB_error (lua_State *L) {
  int level = (int)luaL_optinteger(L, 2, 1);
  lua_settop(L, 1);
  if (lua_type(L, 1) == LUA_TSTRING && level > 0) {
    luaL_where(L, level);   /* добавить дополнительную информацию */
    lua_pushvalue(L, 1);
    lua_concat(L, 2);
  }
  return lua_error(L);
}


static int luaB_getmetatable (lua_State *L) {
  luaL_checkany(L, 1);
  if (!lua_getmetatable(L, 1)) {
    lua_pushnil(L);
    return 1;  /* нет метаданных */
  }
  luaL_getmetafield(L, 1, "__metatable");
  return 1;  /* возвращает либо __metatable field (if present) или metatable */
}


static int luaB_setmetatable (lua_State *L) {
  int t = lua_type(L, 2);
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_argcheck(L, t == LUA_TNIL || t == LUA_TTABLE, 2,
                    "нуль или ожидаются таблицы ");
  if (luaL_getmetafield(L, 1, "__metatable") != LUA_TNIL)
    return luaL_error(L, "не может изменить защищенную метатаблицу");
  lua_settop(L, 2);
  lua_setmetatable(L, 1);
  return 1;
}


static int luaB_rawequal (lua_State *L) {
  luaL_checkany(L, 1);
  luaL_checkany(L, 2);
  lua_pushboolean(L, lua_rawequal(L, 1, 2));
  return 1;
}


static int luaB_rawlen (lua_State *L) {
  int t = lua_type(L, 1);
  luaL_argcheck(L, t == LUA_TTABLE || t == LUA_TSTRING, 1,
                   "ожидаемая таблица или строка");
  lua_pushinteger(L, lua_rawlen(L, 1));
  return 1;
}


static int luaB_rawget (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_checkany(L, 2);
  lua_settop(L, 2);
  lua_rawget(L, 1);
  return 1;
}

static int luaB_rawset (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_checkany(L, 2);
  luaL_checkany(L, 3);
  lua_settop(L, 3);
  lua_rawset(L, 1);
  return 1;
}


static int pushmode (lua_State *L, int oldmode) {
  lua_pushstring(L, (oldmode == LUA_GCINC) ? "incremental" : "generational");
  return 1;
}


static int luaB_collectgarbage (lua_State *L) {
  static const char *const opts[] = {"stop", "restart", "collect",
    "count", "step", "setpause", "setstepmul",
    "isrunning", "generational", "incremental", NULL};
  static const int optsnum[] = {LUA_GCSTOP, LUA_GCRESTART, LUA_GCCOLLECT,
    LUA_GCCOUNT, LUA_GCSTEP, LUA_GCSETPAUSE, LUA_GCSETSTEPMUL,
    LUA_GCISRUNNING, LUA_GCGEN, LUA_GCINC};
  int o = optsnum[luaL_checkoption(L, 1, "collect", opts)];
  switch (o) {
    case LUA_GCCOUNT: {
      int k = lua_gc(L, o);
      int b = lua_gc(L, LUA_GCCOUNTB);
      lua_pushnumber(L, (lua_Number)k + ((lua_Number)b/1024));
      return 1;
    }
    case LUA_GCSTEP: {
      int step = (int)luaL_optinteger(L, 2, 0);
      int res = lua_gc(L, o, step);
      lua_pushboolean(L, res);
      return 1;
    }
    case LUA_GCSETPAUSE:
    case LUA_GCSETSTEPMUL: {
      int p = (int)luaL_optinteger(L, 2, 0);
      int previous = lua_gc(L, o, p);
      lua_pushinteger(L, previous);
      return 1;
    }
    case LUA_GCISRUNNING: {
      int res = lua_gc(L, o);
      lua_pushboolean(L, res);
      return 1;
    }
    case LUA_GCGEN: {
      int minormul = (int)luaL_optinteger(L, 2, 0);
      int majormul = (int)luaL_optinteger(L, 3, 0);
      return pushmode(L, lua_gc(L, o, minormul, majormul));
    }
    case LUA_GCINC: {
      int pause = (int)luaL_optinteger(L, 2, 0);
      int stepmul = (int)luaL_optinteger(L, 3, 0);
      int stepsize = (int)luaL_optinteger(L, 4, 0);
      return pushmode(L, lua_gc(L, o, pause, stepmul, stepsize));
    }
    default: {
      int res = lua_gc(L, o);
      lua_pushinteger(L, res);
      return 1;
    }
  }
}


static int luaB_type (lua_State *L) {
  int t = lua_type(L, 1);
  luaL_argcheck(L, t != LUA_TNONE, 1, "ожидаемое значение");
  lua_pushstring(L, lua_typename(L, t));
  return 1;
}


static int luaB_next (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  lua_settop(L, 2);  /* создайте   второй аргумент (2nd), если его нет*/
  if (lua_next(L, 1))
    return 2;
  else {
    lua_pushnil(L);
    return 1;
  }
}


static int luaB_pairs (lua_State *L) {
  luaL_checkany(L, 1);
  if (luaL_getmetafield(L, 1, "__pairs") == LUA_TNIL) {  /* нет мета-метода? */
    lua_pushcfunction(L, luaB_next);  /* будет возвращать генератор, */
    lua_pushvalue(L, 1);  /* состояние, */
    lua_pushnil(L);  /* и начальное значение */
  }
  else {
    lua_pushvalue(L, 1);  /* аргумент 'self' в этот мета-метод */
    lua_call(L, 1, 3);  /* получить 3 значения из мета-метода */
  }
  return 3;
}


/*
** Функция траверса для 'ipairs'
*/
static int ipairsaux (lua_State *L) {
  lua_Integer i = luaL_checkinteger(L, 2) + 1;
  lua_pushinteger(L, i);
  return (lua_geti(L, 1, i) == LUA_TNIL) ? 1 : 2;
}


/*
** 'ipairs' функция. Возвращает 'ipairsaux', given "table", 0.
** (Данная «таблица» может не быть таблицей.)
*/
static int luaB_ipairs (lua_State *L) {
  luaL_checkany(L, 1);
  lua_pushcfunction(L, ipairsaux);  /* итерационная функция */
  lua_pushvalue(L, 1);  /* 	состояние */
  lua_pushinteger(L, 0);  /* начальное значение */
  return 3;
}


static int load_aux (lua_State *L, int status, int envidx) {
  if (status == LUA_OK) {
    if (envidx != 0) {  /* 'env' параметр? */
      lua_pushvalue(L, envidx);  /* среда для загруженной функции */
      if (!lua_setupvalue(L, -2, 1))  /* установить его в качестве первого значения */
        lua_pop(L, 1);  /* удалить 'env', если не используется предыдущим вызовом */
    }
    return 1;
  }
  else {  /* ошибка (сообщение находится поверх стёка) */
    lua_pushnil(L);
    lua_insert(L, -2);  /* поставить перед сообщением об ошибке */
    return 2;  /* return nil plus сообщение об ошибке */
  }
}


static int luaB_loadfile (lua_State *L) {
  const char *fname = luaL_optstring(L, 1, NULL);
  const char *mode = luaL_optstring(L, 2, NULL);
  int env = (!lua_isnone(L, 3) ? 3 : 0);  /* 'env' index или 0, если не 'env' */
  int status = luaL_loadfilex(L, fname, mode);
  return load_aux(L, status, env);
}


/*
** {======================================================
** Функция общего чтения
** =======================================================
*/


/*
** зарезервированный слот, прежде всего аргументы, чтобы сохранить копию возвращенного
** чтобы избежать его сбора при анализе. 'load' имеет четыре
** необязательных аргумента (chunk, source name, mode, and environment).   
** (фрагмент, имя источника, режим и среда).
*/
#define RESERVEDSLOT	5


/*
** Читатель для общих 'load' function: 'lua_load' 
** используется для внутренних вещей,
** поэтому читатель не может изменить верх стека.
** Вместо этого он сохраняет свою итоговую строку в  
** зарезервированный слот внутри стека.
*/
static const char *generic_reader (lua_State *L, void *ud, size_t *size) {
  (void)(ud);  /* не используется */
  luaL_checkstack(L, 2, "много вложенных функций");
  lua_pushvalue(L, 1);  /* получить функцию */
  lua_call(L, 0, 1);  /* назови это */
  if (lua_isnil(L, -1)) {
    lua_pop(L, 1);  /* поп-результат */
    *size = 0;
    return NULL;
  }
  else if (!lua_isstring(L, -1))
    luaL_error(L, "функция чтения должна отдать строку");
  lua_replace(L, RESERVEDSLOT);  /* функция чтения должна отдать строку */
  return lua_tolstring(L, RESERVEDSLOT, size);
}


static int luaB_load (lua_State *L) {
  int status;
  size_t l;
  const char *s = lua_tolstring(L, 1, &l);
  const char *mode = luaL_optstring(L, 3, "bt");
  int env = (!lua_isnone(L, 4) ? 4 : 0);  /* 'env' index или 0, если не 'env' */
  if (s != NULL) {  /* загрузить строку? */
    const char *chunkname = luaL_optstring(L, 2, s);
    status = luaL_loadbufferx(L, s, l, chunkname, mode);
  }
  else {  /* загрузка из функции чтения */
    const char *chunkname = luaL_optstring(L, 2, "=(load)");
    luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_settop(L, RESERVEDSLOT);  /* создать резервный слот */
    status = lua_load(L, generic_reader, NULL, chunkname, mode);
  }
  return load_aux(L, status, env);
}

/* }====================================================== */


static int dofilecont (lua_State *L, int d1, lua_KContext d2) {
  (void)d1;  (void)d2;  /* только для соответствия прототипу 'lua_Kfunction' */
  return lua_gettop(L) - 1;
}


static int luaB_dofile (lua_State *L) {
  const char *fname = luaL_optstring(L, 1, NULL);
  lua_settop(L, 1);
  if (luaL_loadfile(L, fname) != LUA_OK)
    return lua_error(L);
  lua_callk(L, 0, LUA_MULTRET, 0, dofilecont);
  return dofilecont(L, 0, 0);
}


static int luaB_assert (lua_State *L) {
  if (lua_toboolean(L, 1))  /* условие верно? */
    return lua_gettop(L);  /* вернуть все аргументы */
  else {  /* error */
    luaL_checkany(L, 1);  /* должно быть условие */
    lua_remove(L, 1);  /* убери это */
    lua_pushliteral(L, "не утвердилось!");  /* сообщение по умолчанию */
    lua_settop(L, 1);  /* оставить только сообщение (по умолчанию, если нет другого) */
    return luaB_error(L);  /* ошибка вызова */
  }
}


static int luaB_select (lua_State *L) {
  int n = lua_gettop(L);
  if (lua_type(L, 1) == LUA_TSTRING && *lua_tostring(L, 1) == '#') {
    lua_pushinteger(L, n-1);
    return 1;
  }
  else {
    lua_Integer i = luaL_checkinteger(L, 1);
    if (i < 0) i = n + i;
    else if (i > n) i = n;
    luaL_argcheck(L, 1 <= i, 1, "индекс вне диапазона");
    return n - (int)i;
  }
}


/*
** Функция продолжения для «pcall» и «xpcall». 
** Обе функции уже выдвинули «истину» перед выполнением вызова, 
** поэтому в случае успеха 'finishpcall' 
** нужно только вернуть все в стеке 
** минус 'extra' значение 
** (где «extra» - это точное количество предметов, 
** которые должны быть проигнорированы).
*/
static int finishpcall (lua_State *L, int status, lua_KContext extra) {
  if (status != LUA_OK && status != LUA_YIELD) {  /* ошибка? */
    lua_pushboolean(L, 0);  /* первый результат (ложный) */
    lua_pushvalue(L, -2);  /* сообщение об ошибке */
    return 2;  /* return false, msg */
  }
  else
    return lua_gettop(L) - (int)extra;  /* вернуть все результаты */
}


static int luaB_pcall (lua_State *L) {
  int status;
  luaL_checkany(L, 1);
  lua_pushboolean(L, 1);  /* первый безошибочный результат */
  lua_insert(L, 1);  /* положить его на место */
  status = lua_pcallk(L, lua_gettop(L) - 2, LUA_MULTRET, 0, 0, finishpcall);
  return finishpcall(L, status, 0);
}


/*
** Защищенный вызов с обработкой ошибок. 
** После 'lua_rotate' stack будет иметь  
** <f, err, true, f, [args ...]>; 
** поэтому функция проходит 
** 2 до 'finishpcall', чтобы пропустить 
** 2 первых значения при возврате результатов.
** стек будет <f, err, true, f, [args...]>; 
** поэтому функция проходит 461 ** 2 до 'finishpcall', 
** чтобы пропустить 2 первых значения при возврате результатов.
*/
static int luaB_xpcall (lua_State *L) {
  int status;
  int n = lua_gettop(L);
  luaL_checktype(L, 2, LUA_TFUNCTION);  /* функция проверки ошибок */
  lua_pushboolean(L, 1);  /* первый результат */
  lua_pushvalue(L, 1);  /* function */
  lua_rotate(L, 3, 2);  /* переместить их ниже аргументов функции */
  status = lua_pcallk(L, n - 2, LUA_MULTRET, 2, 2, finishpcall);
  return finishpcall(L, status, 2);
}


static int luaB_tostring (lua_State *L) {
  luaL_checkany(L, 1);
  luaL_tolstring(L, 1, NULL);
  return 1;
}


static const luaL_Reg base_funcs[] = {
  {"assert", luaB_assert},
  {"collectgarbage", luaB_collectgarbage},
  {"dofile", luaB_dofile},
  {"error", luaB_error},
  {"getmetatable", luaB_getmetatable},
  {"ipairs", luaB_ipairs},
  {"loadfile", luaB_loadfile},
  {"load", luaB_load},
  {"next", luaB_next},
  {"pairs", luaB_pairs},
  {"pcall", luaB_pcall},
  {"print", luaB_print},
  {"rawequal", luaB_rawequal},
  {"rawlen", luaB_rawlen},
  {"rawget", luaB_rawget},
  {"rawset", luaB_rawset},
  {"select", luaB_select},
  {"setmetatable", luaB_setmetatable},
  {"tonumber", luaB_tonumber},
  {"tostring", luaB_tostring},
  {"type", luaB_type},
  {"xpcall", luaB_xpcall},
  /* placeholders */
  {LUA_GNAME, NULL},
  {"_VERSION", NULL},
  /* добавить русские синонимы */
  // {"assert", luaB_assert},
  // {"collectgarbage", luaB_collectgarbage},
  // {"dofile", luaB_dofile},
  // {"error", luaB_error},
  // {"getmetatable", luaB_getmetatable},
  // {"ipairs", luaB_ipairs},
  {"çàãðóçèòü_ôàéë", luaB_loadfile},
  {"çàãðóçèòü", luaB_load},
  {"äàëåå", luaB_next},
  // {"pairs", luaB_pairs},
  // {"pcall", luaB_pcall},
  {"ïå÷àòü", luaB_print},
  // {"rawequal", luaB_rawequal},
  // {"rawlen", luaB_rawlen},
  // {"rawget", luaB_rawget},
  // {"rawset", luaB_rawset},
  // {"select", luaB_select},
  // {"setmetatable", luaB_setmetatable},
  // {"tonumber", luaB_tonumber},
  // {"tostring", luaB_tostring},
  {"òèï", luaB_type},
  // {"xpcall", luaB_xpcall},
  // /* placeholders */
  // {LUA_GNAME, NULL},
  {"_ÂÅÐÑÈß", NULL},
  {NULL, NULL}
};


LUAMOD_API int luaopen_base (lua_State *L) {
  /* open lib into global table */
  lua_pushglobaltable(L);
  luaL_setfuncs(L, base_funcs, 0);
  /* установить глобальную _G */
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, LUA_GNAME);
  /* установить глобальную _VERSION */
  lua_pushliteral(L, LUA_VERSION);
  lua_setfield(L, -2, "_VERSION");
  return 1;
}

