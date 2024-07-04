#pragma once

#include <cstring>
#include <string>
#include <string_view>

namespace lex {

enum Id
{
  YYEMPTY = -2,
  YYEOF = 0,     /* "end of file"  */
  YYerror = 256, /* error  */
  YYUNDEF = 257, /* "invalid token"  */
  YYFILE = 114,
  YYSPACE = 514,
  YYLINE = 1919,
  IDENTIFIER,
  CONSTANT,
  STRING_LITERAL,
  ALIGNAS,
  ALIGNOF,
  AND,
  AND_EQ,
  ASM,
  ATOMIC_CANCEL,
  ATOMIC_COMMIT,
  ATOMIC_NOEXCEPT,
  AUTO,
  BITAND,
  BITOR,
  BOOL,
  BREAK,
  CASE,
  CATCH,
  CHAR,
  CHAR8_T,
  CHAR16_T,
  CHAR32_T,
  CLASS,
  COMPL,
  CONCEPT,
  CONST,
  CONSTEVAL,
  CONSTEXPR,
  CONSTINIT,
  CONST_CAST,
  CONTINUE,
  CO_AWAIT,
  CO_RETURN,
  CO_YIELD,
  DECLTYPE,
  DEFAULT,
  DELETE,
  DO,
  DOUBLE,
  DYNAMIC_CAST,
  ELSE,
  ENUM,
  EXPLICIT,
  EXPORT,
  EXTERN,
  FALSE,
  FLOAT,
  FOR,
  FRIEND,
  GOTO,
  IF,
  INLINE,
  INT,
  LONG,
  MUTABLE,
  NAMESPACE,
  NEW,
  NOEXCEPT,
  NOT,
  NOT_EQ,
  NULLPTR,
  OPERATOR,
  OR,
  OR_EQ,
  PRIVATE,
  PROTECTED,
  PUBLIC,
  REFLEXPR,
  REGISTER,
  REINTERPRET_CAST,
  REQUIRES,
  RETURN,
  SHORT,
  SIGNED,
  SIZEOF,
  STATIC,
  STATIC_ASSERT,
  STATIC_CAST,
  STRUCT,
  SWITCH,
  SYNCHRONIZED,
  TEMPLATE,
  THIS,
  THREAD_LOCAL,
  THROW,
  TRUE,
  TRY,
  TYPEDEF,
  TYPEID,
  TYPENAME,
  UNION,
  UNSIGNED,
  USING,
  VIRTUAL,
  VOID,
  VOLATILE,
  WCHAR_T,
  WHILE,
  XOR,
  XOR_EQ,
  L_BRACE,
  R_BRACE,
  L_SQUARE,
  R_SQUARE,
  L_PAREN,
  R_PAREN,
  SEMI,
  COLON,
  ELLIPSIS,
  QUESTION,
  COLONCOLON,
  DOT,
  PLUS,
  MINUS,
  STAR,
  SLASH,
  PERCENT,
  CARET,
  AMP,
  PIPE,
  TILDE,
  EXCLAIM,
  EQUAL,
  LESS,
  GREATER,
  PLUSEQUAL,
  MINUSEQUAL,
  STAREQUAL,
  SLASHEQUAL,
  PERCENTEQUAL,
  CARETEQUAL,
  AMPEQUAL,
  PIPEEQUAL,
  LTLT,
  GTGT,
  LTLTEQUAL,
  GTGTEQUAL,
  EQUALEQUAL,
  EXCLAIMEQUAL,
  LESSEQUAL,
  GREATEREQUAL,
  SPACESHIP,
  AMPAMP,
  PIPEPIPE,
  PLUSPLUS,
  MINUSMINUS,
  COMMA,
  ARROWSTAR,
  ARROW
};

const char*
id2str(Id id);

struct G
{
  Id mId{ YYEOF };              // 词号
  std::string_view mText;       // 对应文本
  std::string mFile;            // 文件路径
  int mLine{ 1 }, mColumn{ 1 }; // 行号、列号
  bool mStartOfLine{ true };    // 是否是行首
  bool mLeadingSpace{ false };  // 是否有前导空格
};

extern G g;

int
come(int tokenId, const char* yytext, int yyleng, int yylineno);

} // namespace lex
