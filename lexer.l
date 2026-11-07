%{
#include "ast.hpp"
#include "parser.tab.hpp"
#include <cstdlib>
#include <cstring>
%}

%option noyywrap
%option yylineno

%%

"//".*                  { }
[ \t\r\n]+              { }

"int"                   { return T_INT; }
"if"                    { return T_IF; }
"else"                  { return T_ELSE; }
"while"                 { return T_WHILE; }
"print"                 { return T_PRINT; }

"=="                    { return T_EQ; }
"!="                    { return T_NEQ; }
"<="                    { return T_LE; }
">="                    { return T_GE; }
"<"                     { return T_LT; }
">"                     { return T_GT; }

"+"                     { return '+'; }
"-"                     { return '-'; }
"*"                     { return '*'; }
"/"                     { return '/'; }
"="                     { return '='; }
";"                     { return ';'; }
"("                     { return '('; }
")"                     { return ')'; }
"{"                     { return '{'; }
"}"                     { return '}'; }

[0-9]+                  {
                            yylval.num = std::atoi(yytext);
                            return T_NUMBER;
                        }

[a-zA-Z_][a-zA-Z0-9_]*  {
                            yylval.str = strdup(yytext);
                            return T_ID;
                        }

.                       {
                            std::cerr << "[Лексер] Невідомий символ на рядку "
                                      << yylineno << ": " << yytext << "\n";
                        }

%%