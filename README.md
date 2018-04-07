# lua
Lua Ru 5.3.4 Русификация Lua, русские переменные, имена функций и операторы

https://тхаб.рф/wiki/Учебник_по_LuaRu - Учебник в процессе перовода!! по LuaRu

http://plana.mybb.ru/viewtopic.php?id=576 Обсуждение LuaRu 

исправленный файлы lctype.h и llex.c

== Русские синонимы англоязычных ключевых слов ==
(в скобках не реализованные варианты)

and - и ( И ?)

break - стоп (выйти ?)

do -  делать (начало ?)

else - иначе

elseif - иначе_если

end - конец

false - ложь (+ нет? НЕТ) 

for - для

function - функция

goto - перейти_к 

if - если 

in - в 

local - локальная

nil - пусто (нуль ?) 

not - не (НЕ ?)

or - или (ИЛИ ?)

repeat - повторить

return - вернуть (вернутся ?)

then - тогда

true - истина (да? ДА?)

until - пока-не 

while - пока

--- Из llex.c---
* ORDER RESERVED */
static const char *const luaX_tokens [] = {
    "and", "break", "do", "else", "elseif",
    "end", "false", "for", "function", "goto", "if",
    "in", "local", "nil", "not", "or", "repeat",
    "return", "then", "true", "until", "while",
    "//", "..", "...", "==", ">=", "<=", "~=",
    "<<", ">>", "::", "<eof>",
    "<number>", "<integer>", "<name>", "<string>"
};
static const char *const luaX_tokens_cyr [] = {
      "и", "выход", "делать", "иначе", "иначе_если",
    "конец", "ложь", "для", "функция", "перейти_к", "если",
    "в", "локальная", "пусто", "не", "или", "повторить",
    "вернуть", "тогда", "истина", "пока_не", "пока"
};

==Как скомпилировать исходники==
