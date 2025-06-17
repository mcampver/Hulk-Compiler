%option noyywrap
%option noinput nounput

%{
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <cstring>
#include <string>

#include "../AST/ast.hpp"
#include "../Parser/parser.tab.hpp"

extern int yylineno;

// Necesario para evitar conflictos con strdup (definido en <string.h>)
#ifdef strdup
#undef strdup
#endif
%}

%%

\n                          { yylineno++; }
[ \t\r]+                    { /* skip whitespace */ }
"//".*                      { /* skip single line comments */ }

[0-9]+(\.[0-9]+)?           {
                              yylval.expr = new NumberExpr(atof(yytext), yylineno);
                              return NUMBER;
                           }

\"(([^\\\"]|\\.)*)\"          {
                              std::string raw = yytext;
                              // Eliminar comillas del principio y final
                              std::string content = raw.substr(1, raw.length() - 2);
                              
                              // Procesar secuencias de escape
                              std::string processed = "";
                              for (size_t i = 0; i < content.length(); i++) {
                                  if (content[i] == '\\' && i + 1 < content.length()) {
                                      char next = content[i + 1];
                                      switch (next) {
                                          case 'n': processed += '\n'; break;
                                          case 't': processed += '\t'; break;
                                          case 'r': processed += '\r'; break;
                                          case '\\': processed += '\\'; break;
                                          case '\"': processed += '\"'; break;
                                          default: processed += next; break;
                                      }
                                      i++; // Skip the next character
                                  } else {
                                      processed += content[i];
                                  }
                              }
                                yylval.expr = new StringExpr(processed, yylineno);
                              return STRING;
                           }

[A-Za-z][A-Za-z0-9_]*  {
    if (strcmp(yytext, "if") == 0)       return IF;
    if (strcmp(yytext, "else") == 0)     return ELSE;
    if (strcmp(yytext, "elif") == 0)     return ELIF;
    if (strcmp(yytext, "let") == 0)      return LET;
    if (strcmp(yytext, "in") == 0)       return IN;
    if (strcmp(yytext, "while") == 0)    return WHILE;
    if (strcmp(yytext, "for") == 0)      return FOR;    if (strcmp(yytext, "true") == 0)     { yylval.expr = new BooleanExpr(true, yylineno);  return TRUE; }
    if (strcmp(yytext, "false") == 0)    { yylval.expr = new BooleanExpr(false, yylineno); return FALSE; }
    if (strcmp(yytext, "True") == 0)     { yylval.expr = new BooleanExpr(true, yylineno);  return TRUE; }
    if (strcmp(yytext, "False") == 0)    { yylval.expr = new BooleanExpr(false, yylineno); return FALSE; }
    if (strcmp(yytext,"function") == 0)  return FUNCTION; 
    if (strcmp(yytext, "debug") == 0)    return DEBUG;
    if (strcmp(yytext, "type") == 0)     return TYPE;
    if (strcmp(yytext, "assert") == 0)   return ASSERT;
    if (strcmp(yytext, "new") == 0)      return NEW;
    if (strcmp(yytext, "self") == 0)     return SELF;
    if (strcmp(yytext, "inherits") == 0) return INHERITS;
    if (strcmp(yytext, "base") == 0)     return BASE;

    // Si no es ninguna palabra reservada, entonces es IDENT
    yylval.str = strdup(yytext);
    return IDENT;
}

                    
\,                          { return COMMA; }
"+++"                       { return TRIPLE_PLUS; }
\+                          { return PLUS; }
\-                          { return MINUS; }
\*                          { return MULT; }
\/                          { return DIV; }
\**                         { return POW; }
\^                          { return POW; }
\(                          { return LPAREN; }
\)                          { return RPAREN; }
\;                          { return SEMICOLON; }
\.                          { return DOT; }
"%%"                        { return ENHANCED_MOD; }
\%                          { return MOD; }
"<="                        { return LE; }
">="                        { return GE; }
"=="                        { return EQ; }
\!=                         { return NEQ; }
"<"                         { return LESS_THAN; }
">"                         { return GREATER_THAN; }
"&&"                        { return AND; }  
"&"                         { return AND_SIMPLE; }
"||"                        { return OR; }
"|"                         { return OR_SIMPLE; }
"!"                         { return NOT; }
"="                         { return ASSIGN; }
\{                          { return LBRACE; }
\}                          { return RBRACE; }
":="                        { return ASSIGN_DESTRUCT; }
"=>"                        { return ARROW; }
"@@"                        { return CONCAT_SPACE; }
"@"                         { return CONCAT; }
.                           {
                              fprintf(stderr, "Caracter no reconocido: %s en la línea %d\n", yytext, yylineno);
                           }

%%