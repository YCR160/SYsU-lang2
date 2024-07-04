#include "lex.hpp"
#include <iostream>

void
print_token();

namespace lex {

static const char* kTokenNames[] = { "identifier",
                                     "numeric_constant",
                                     "string_literal",
                                     "alignas",
                                     "alignof",
                                     "and",
                                     "and_eq",
                                     "asm",
                                     "atomic_cancel",
                                     "atomic_commit",
                                     "atomic_noexcept",
                                     "auto",
                                     "bitand",
                                     "bitor",
                                     "bool",
                                     "break",
                                     "case",
                                     "catch",
                                     "char",
                                     "char8_t",
                                     "char16_t",
                                     "char32_t",
                                     "class",
                                     "compl",
                                     "concept",
                                     "const",
                                     "consteval",
                                     "constexpr",
                                     "constinit",
                                     "const_cast",
                                     "continue",
                                     "co_await",
                                     "co_return",
                                     "co_yield",
                                     "decltype",
                                     "default",
                                     "delete",
                                     "do",
                                     "double",
                                     "dynamic_cast",
                                     "else",
                                     "enum",
                                     "explicit",
                                     "export",
                                     "extern",
                                     "false",
                                     "float",
                                     "for",
                                     "friend",
                                     "goto",
                                     "if",
                                     "inline",
                                     "int",
                                     "long",
                                     "mutable",
                                     "namespace",
                                     "new",
                                     "noexcept",
                                     "not",
                                     "not_eq",
                                     "nullptr",
                                     "operator",
                                     "or",
                                     "or_eq",
                                     "private",
                                     "protected",
                                     "public",
                                     "reflexpr",
                                     "register",
                                     "reinterpret_cast",
                                     "requires",
                                     "return",
                                     "short",
                                     "signed",
                                     "sizeof",
                                     "static",
                                     "static_assert",
                                     "static_cast",
                                     "struct",
                                     "switch",
                                     "synchronized",
                                     "template",
                                     "this",
                                     "thread_local",
                                     "throw",
                                     "true",
                                     "try",
                                     "typedef",
                                     "typeid",
                                     "typename",
                                     "union",
                                     "unsigned",
                                     "using",
                                     "virtual",
                                     "void",
                                     "volatile",
                                     "wchar_t",
                                     "while",
                                     "xor",
                                     "xor_eq",
                                     "l_brace",
                                     "r_brace",
                                     "l_square",
                                     "r_square",
                                     "l_paren",
                                     "r_paren",
                                     "semi",
                                     "colon",
                                     "ellipsis",
                                     "question",
                                     "coloncolon",
                                     "dot",
                                     "plus",
                                     "minus",
                                     "star",
                                     "slash",
                                     "percent",
                                     "caret",
                                     "amp",
                                     "pipe",
                                     "tilde",
                                     "exclaim",
                                     "equal",
                                     "less",
                                     "greater",
                                     "plusequal",
                                     "minusequal",
                                     "starequal",
                                     "slashequal",
                                     "percentequal",
                                     "caretequal",
                                     "ampequal",
                                     "pipeequal",
                                     "ltlt",
                                     "gtgt",
                                     "ltltequal",
                                     "gtgtequal",
                                     "equalequal",
                                     "exclaimequal",
                                     "lessequal",
                                     "greaterequal",
                                     "spaceship",
                                     "ampamp",
                                     "pipepipe",
                                     "plusplus",
                                     "minusminus",
                                     "comma",
                                     "arrowstar",
                                     "arrow" };

const char*
id2str(Id id)
{
  static char sCharBuf[2] = { 0, 0 };
  if (id == Id::YYEOF) {
    return "eof";
  } else if (id < Id::IDENTIFIER) {
    sCharBuf[0] = char(id);
    return sCharBuf;
  }
  return kTokenNames[int(id) - int(Id::IDENTIFIER)];
}

G g;

int
come(int tokenId, const char* yytext, int yyleng, int yylineno)
{
  auto dump_mode = 0;
  g.mId = Id(tokenId);
  g.mText = { yytext, std::size_t(yyleng) };
  if (g.mId == Id::YYFILE) {
    std::string s = { yytext, std::size_t(yyleng) };
    {
      auto first = s.find_first_of(' ');
      auto second = s.find_first_of(' ', first + 1);
      g.mLine = std::stoul(s.substr(first + 1, second - first - 1)) - 1;
    }

    {
      auto first = s.find_first_of('\"');
      auto second = s.find_first_of('\"', first + 1);
      g.mFile = s.substr(first + 1, second - first - 1);
    }
    if (dump_mode) {
      print_token();
    }
    return tokenId;
  } else if (g.mId == Id::YYSPACE) {
    g.mLeadingSpace = true;
    if (dump_mode) {
      print_token();
    }
    return tokenId;
  } else if (g.mId == Id::YYLINE) {
    g.mLine += 1;
    g.mColumn = 1;
    g.mStartOfLine = true;
    if (dump_mode) {
      print_token();
    }
    return tokenId;
  }

  g.mColumn -= yyleng;
  print_token();
  g.mColumn += yyleng;
  g.mStartOfLine = false;
  g.mLeadingSpace = false;

  return tokenId;
}

} // namespace lex
