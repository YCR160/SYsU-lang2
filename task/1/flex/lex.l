%{
#include "lex.hpp"
/* 所有代码全部抽离出来，放到 lex.hpp 和 lex.cpp 里 */

using namespace lex;

#define ADDCOL() g.mColumn += yyleng;
#define COME(id) return come(id, yytext, yyleng, yylineno)
%}

%option 8bit warn noyywrap yylineno

D     [0-9]
L     [a-zA-Z_]
H     [0-9a-fA-F_]
IS    ((u|U)|(u|U)?(l|L|ll|LL)|(l|L|ll|LL)(u|U))

%%

"alignas"       { ADDCOL(); COME(ALIGNAS); }
"alignof"       { ADDCOL(); COME(ALIGNOF); }
"and"           { ADDCOL(); COME(AND); }
"and_eq"        { ADDCOL(); COME(AND_EQ); }
"asm"           { ADDCOL(); COME(ASM); }
"atomic_cancel" { ADDCOL(); COME(ATOMIC_CANCEL); }
"atomic_commit" { ADDCOL(); COME(ATOMIC_COMMIT); }
"atomic_noexcept"   { ADDCOL(); COME(ATOMIC_NOEXCEPT); }
"auto"          { ADDCOL(); COME(AUTO); }
"bitand"        { ADDCOL(); COME(BITAND); }
"bitor"         { ADDCOL(); COME(BITOR); }
"bool"          { ADDCOL(); COME(BOOL); }
"break"         { ADDCOL(); COME(BREAK); }
"case"          { ADDCOL(); COME(CASE); }
"catch"         { ADDCOL(); COME(CATCH); }
"char"          { ADDCOL(); COME(CHAR); }
"char8_t"       { ADDCOL(); COME(CHAR8_T); }
"char16_t"      { ADDCOL(); COME(CHAR16_T); }
"char32_t"      { ADDCOL(); COME(CHAR32_T); }
"class"         { ADDCOL(); COME(CLASS); }
"compl"         { ADDCOL(); COME(COMPL); }
"concept"       { ADDCOL(); COME(CONCEPT); }
"const"         { ADDCOL(); COME(CONST); }
"consteval"     { ADDCOL(); COME(CONSTEVAL); }
"constexpr"     { ADDCOL(); COME(CONSTEXPR); }
"constinit"     { ADDCOL(); COME(CONSTINIT); }
"const_cast"    { ADDCOL(); COME(CONST_CAST); }
"continue"      { ADDCOL(); COME(CONTINUE); }
"co_await"      { ADDCOL(); COME(CO_AWAIT); }
"co_return"     { ADDCOL(); COME(CO_RETURN); }
"co_yield"      { ADDCOL(); COME(CO_YIELD); }
"decltype"      { ADDCOL(); COME(DECLTYPE); }
"default"       { ADDCOL(); COME(DEFAULT); }
"delete"        { ADDCOL(); COME(DELETE); }
"do"            { ADDCOL(); COME(DO); }
"double"        { ADDCOL(); COME(DOUBLE); }
"dynamic_cast"  { ADDCOL(); COME(DYNAMIC_CAST); }
"else"          { ADDCOL(); COME(ELSE); }
"enum"          { ADDCOL(); COME(ENUM); }
"explicit"      { ADDCOL(); COME(EXPLICIT); }
"export"        { ADDCOL(); COME(EXPORT); }
"extern"        { ADDCOL(); COME(EXTERN); }
"false"         { ADDCOL(); COME(FALSE); }
"float"         { ADDCOL(); COME(FLOAT); }
"for"           { ADDCOL(); COME(FOR); }
"friend"        { ADDCOL(); COME(FRIEND); }
"goto"          { ADDCOL(); COME(GOTO); }
"if"            { ADDCOL(); COME(IF); }
"inline"        { ADDCOL(); COME(INLINE); }
"int"           { ADDCOL(); COME(INT); }
"long"          { ADDCOL(); COME(LONG); }
"mutable"       { ADDCOL(); COME(MUTABLE); }
"namespace"     { ADDCOL(); COME(NAMESPACE); }
"new"           { ADDCOL(); COME(NEW); }
"noexcept"      { ADDCOL(); COME(NOEXCEPT); }
"not"           { ADDCOL(); COME(NOT); }
"not_eq"        { ADDCOL(); COME(NOT_EQ); }
"nullptr"       { ADDCOL(); COME(NULLPTR); }
"operator"      { ADDCOL(); COME(OPERATOR); }
"or"            { ADDCOL(); COME(OR); }
"or_eq"         { ADDCOL(); COME(OR_EQ); }
"private"       { ADDCOL(); COME(PRIVATE); }
"protected"     { ADDCOL(); COME(PROTECTED); }
"public"        { ADDCOL(); COME(PUBLIC); }
"reflexpr"      { ADDCOL(); COME(REFLEXPR); }
"register"      { ADDCOL(); COME(REGISTER); }
"reinterpret_cast"  { ADDCOL(); COME(REINTERPRET_CAST); }
"requires"      { ADDCOL(); COME(REQUIRES); }
"return"        { ADDCOL(); COME(RETURN); }
"short"         { ADDCOL(); COME(SHORT); }
"signed"        { ADDCOL(); COME(SIGNED); }
"sizeof"        { ADDCOL(); COME(SIZEOF); }
"static"        { ADDCOL(); COME(STATIC); }
"static_assert" { ADDCOL(); COME(STATIC_ASSERT); }
"static_cast"   { ADDCOL(); COME(STATIC_CAST); }
"struct"        { ADDCOL(); COME(STRUCT); }
"switch"        { ADDCOL(); COME(SWITCH); }
"synchronized"  { ADDCOL(); COME(SYNCHRONIZED); }
"template"      { ADDCOL(); COME(TEMPLATE); }
"this"          { ADDCOL(); COME(THIS); }
"thread_local"  { ADDCOL(); COME(THREAD_LOCAL); }
"throw"         { ADDCOL(); COME(THROW); }
"true"          { ADDCOL(); COME(TRUE); }
"try"           { ADDCOL(); COME(TRY); }
"typedef"       { ADDCOL(); COME(TYPEDEF); }
"typeid"        { ADDCOL(); COME(TYPEID); }
"typename"      { ADDCOL(); COME(TYPENAME); }
"union"         { ADDCOL(); COME(UNION); }
"unsigned"      { ADDCOL(); COME(UNSIGNED); }
"using"         { ADDCOL(); COME(USING); }
"virtual"       { ADDCOL(); COME(VIRTUAL); }
"void"          { ADDCOL(); COME(VOID); }
"volatile"      { ADDCOL(); COME(VOLATILE); }
"wchar_t"       { ADDCOL(); COME(WCHAR_T); }
"while"         { ADDCOL(); COME(WHILE); }
"xor"           { ADDCOL(); COME(XOR); }
"xor_eq"        { ADDCOL(); COME(XOR_EQ); }

"{"         { ADDCOL(); COME(L_BRACE); }
"}"         { ADDCOL(); COME(R_BRACE); }
"["         { ADDCOL(); COME(L_SQUARE); }
"]"         { ADDCOL(); COME(R_SQUARE); }
"("         { ADDCOL(); COME(L_PAREN); }
")"         { ADDCOL(); COME(R_PAREN); }
";"         { ADDCOL(); COME(SEMI); }
":"         { ADDCOL(); COME(COLON); }
"..."       { ADDCOL(); COME(ELLIPSIS); }
"?"         { ADDCOL(); COME(QUESTION); }
"::"        { ADDCOL(); COME(COLONCOLON); }
"."         { ADDCOL(); COME(DOT); }

"+"         { ADDCOL(); COME(PLUS); }
"-"         { ADDCOL(); COME(MINUS); }
"*"         { ADDCOL(); COME(STAR); }
"/"         { ADDCOL(); COME(SLASH); }
"%"         { ADDCOL(); COME(PERCENT); }
"^"         { ADDCOL(); COME(CARET); }
"&"         { ADDCOL(); COME(AMP); }
"|"         { ADDCOL(); COME(PIPE); }
"~"         { ADDCOL(); COME(TILDE); }
"!"         { ADDCOL(); COME(EXCLAIM); }
"="         { ADDCOL(); COME(EQUAL); }
"<"         { ADDCOL(); COME(LESS); }
">"         { ADDCOL(); COME(GREATER); }
"+="        { ADDCOL(); COME(PLUSEQUAL); }
"-="        { ADDCOL(); COME(MINUSEQUAL); }
"*="        { ADDCOL(); COME(STAREQUAL); }
"/="        { ADDCOL(); COME(SLASHEQUAL); }
"%="        { ADDCOL(); COME(PERCENTEQUAL); }
"^="        { ADDCOL(); COME(CARETEQUAL); }
"&="        { ADDCOL(); COME(AMPEQUAL); }
"|="        { ADDCOL(); COME(PIPEEQUAL); }
"<<"        { ADDCOL(); COME(LTLT); }
">>"        { ADDCOL(); COME(GTGT); }
"<<="       { ADDCOL(); COME(LTLTEQUAL); }
">>="       { ADDCOL(); COME(GTGTEQUAL); }
"=="        { ADDCOL(); COME(EQUALEQUAL); }
"!="        { ADDCOL(); COME(EXCLAIMEQUAL); }
"<="        { ADDCOL(); COME(LESSEQUAL); }
">="        { ADDCOL(); COME(GREATEREQUAL); }
"<=>"       { ADDCOL(); COME(SPACESHIP); }
"&&"        { ADDCOL(); COME(AMPAMP); }
"||"        { ADDCOL(); COME(PIPEPIPE); }
"++"        { ADDCOL(); COME(PLUSPLUS); }
"--"        { ADDCOL(); COME(MINUSMINUS); }
","         { ADDCOL(); COME(COMMA); }
"->*"       { ADDCOL(); COME(ARROWSTAR); }
"->"        { ADDCOL(); COME(ARROW); }

{L}({L}|{D})*         { ADDCOL(); COME(IDENTIFIER); }

L?\"(\\.|[^\\"\n])*\" { ADDCOL(); COME(STRING_LITERAL); }

0[0-7]*{IS}?          { ADDCOL(); COME(CONSTANT); }
[1-9]{D}*{IS}?        { ADDCOL(); COME(CONSTANT); }
0x{H}*{IS}?           { ADDCOL(); COME(CONSTANT); }

^#[^\n]*              { COME(YYFILE); } /* 预处理信息处理，可以从预处理信息中获得文件名以及行号 */

[ ]                   { ADDCOL(); COME(YYSPACE); }
[\t\v\n\f]            { COME(YYLINE); } /* 需要处理行号和列号信息 */

<<EOF>>     { ADDCOL(); COME(YYEOF); }

%%

/* about symbols avaliable (yytext, yyleng etc.) in the context of Flex:
 * https://ftp.gnu.org/old-gnu/Manuals/flex-2.5.4/html_node/flex_14.html
 * https://ftp.gnu.org/old-gnu/Manuals/flex-2.5.4/html_node/flex_15.html
*/
