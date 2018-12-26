%{
#include <cstring>
#include <string>
#include <math.h>

#include "driver.hpp"
#include "parser.hpp"
%}

%option noyywrap nounput noinput batch debug

DIGIT [0-9]
NAME [a-zA-Z][a-zA-Z0-9_]*

%{
  // Code run each time a pattern is matched.
  # define YY_USER_ACTION  loc.columns (yyleng);
%}

%x MULTILINE_COMMENT

%%

%{
  // A handy shortcut to the location held by the driver.
  yy::location& loc = drv.m_location;
  // Code run each time yylex is called.
  loc.step();
%}

[ \t]+ loc.step();
\n+ {
  loc.lines(yyleng);
  loc.step();
  return yy::parser::make_EOL(loc);
}

"Scene:" return yy::parser::make_SCENE_HEADER(loc);
"Assets:" return yy::parser::make_ASSETS_HEADER(loc);
"Entities:" return yy::parser::make_ENTITIES_HEADER(loc);
"{" return yy::parser::make_OPEN_CURLY(loc);
"}" return yy::parser::make_CLOSE_CURLY(loc);
"[" return yy::parser::make_OPEN_SQUARE(loc);
"]" return yy::parser::make_CLOSE_SQUARE(loc);
"#" return yy::parser::make_POUND(loc);

[+-]?{DIGIT}+ return yy::parser::make_INTEGER(atoi(yytext), loc);

[+-]?{DIGIT}*"."{DIGIT}+ return yy::parser::make_FLOAT(atof(yytext), loc);

\"[^\"]*\" {
  return yy::parser::make_STRING(std::string(&yytext[1], strlen(yytext)-2), loc);
}

{NAME} {
  return yy::parser::make_NAME(std::string(yytext), loc);
}

"//".* {}

"/*"            { BEGIN(MULTILINE_COMMENT); }
<MULTILINE_COMMENT>"*/" { BEGIN(INITIAL); }
<MULTILINE_COMMENT>\n   {}
<MULTILINE_COMMENT>.    {}

. {
  throw yy::parser::syntax_error(loc, "invalid character: " + std::string(yytext));
}

<<EOF>> return yy::parser::make_END(loc);

%%

namespace scene {
void Driver::scanBegin() {
  yy_flex_debug = m_traceScanning;
  yyin = m_file;
}

void Driver::scanEnd() {
  if (m_text.length() != 0) {
    fclose(yyin);
  }
}
}
