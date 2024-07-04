#include "SYsULexer.h" // 确保这里的头文件名与您生成的词法分析器匹配
#include <fstream>
#include <iostream>
#include <unordered_map>

// 映射定义，将ANTLR的tokenTypeName映射到clang的格式
std::unordered_map<std::string, std::string> tokenTypeMapping = {
  { "Int", "int" },
  { "Identifier", "identifier" },
  { "LeftParen", "l_paren" },
  { "RightParen", "r_paren" },
  { "RightBrace", "r_brace" },
  { "LeftBrace", "l_brace" },
  { "LeftBracket", "l_square" },
  { "RightBracket", "r_square" },
  { "Constant", "numeric_constant" },
  { "EOF", "eof" },

  { "LineAfterPreprocessing", "file" },
  { "Whitespace", "space" },
  { "Newline", "line" },

  { "Alignas", "alignas" },
  { "Alignof", "alignof" },
  { "And", "and" },
  { "And_eq", "and_eq" },
  { "Asm", "asm" },
  { "Atomic_cancel", "atomic_cancel" },
  { "Atomic_commit", "atomic_commit" },
  { "Atomic_noexcept", "atomic_noexcept" },
  { "Auto", "auto" },
  { "Bitand", "bitand" },
  { "Bitor", "bitor" },
  { "Bool", "bool" },
  { "Break", "break" },
  { "Case", "case" },
  { "Catch", "catch" },
  { "Char", "char" },
  { "Char8_t", "char8_t" },
  { "Char16_t", "char16_t" },
  { "Char32_t", "char32_t" },
  { "Class", "class" },
  { "Compl", "compl" },
  { "Concept", "concept" },
  { "Const", "const" },
  { "Consteval", "consteval" },
  { "Constexpr", "constexpr" },
  { "Constinit", "constinit" },
  { "Const_cast", "const_cast" },
  { "Continue", "continue" },
  { "Co_await", "co_await" },
  { "Co_return", "co_return" },
  { "Co_yield", "co_yield" },
  { "Decltype", "decltype" },
  { "Default", "default" },
  { "Delete", "delete" },
  { "Do", "do" },
  { "Double", "double" },
  { "Dynamic_cast", "dynamic_cast" },
  { "Else", "else" },
  { "Enum", "enum" },
  { "Explicit", "explicit" },
  { "Export", "export" },
  { "Extern", "extern" },
  { "False", "false" },
  { "Float", "float" },
  { "For", "for" },
  { "Friend", "friend" },
  { "Goto", "goto" },
  { "If", "if" },
  { "Inline", "inline" },
  { "Int", "int" },
  { "Long", "long" },
  { "Mutable", "mutable" },
  { "Namespace", "namespace" },
  { "New", "new" },
  { "Noexcept", "noexcept" },
  { "Not", "not" },
  { "Not_eq", "not_eq" },
  { "Nullptr", "nullptr" },
  { "Operator", "operator" },
  { "Or", "or" },
  { "Or_eq", "or_eq" },
  { "Private", "private" },
  { "Protected", "protected" },
  { "Public", "public" },
  { "Reflexpr", "reflexpr" },
  { "Register", "register" },
  { "Reinterpret_cast", "reinterpret_cast" },
  { "Requires", "requires" },
  { "Return", "return" },
  { "Short", "short" },
  { "Signed", "signed" },
  { "Sizeof", "sizeof" },
  { "Static", "static" },
  { "Static_assert", "static_assert" },
  { "Static_cast", "static_cast" },
  { "Struct", "struct" },
  { "Switch", "switch" },
  { "Synchronized", "synchronized" },
  { "Template", "template" },
  { "This", "this" },
  { "Thread_local", "thread_local" },
  { "Throw", "throw" },
  { "True", "true" },
  { "Try", "try" },
  { "Typedef", "typedef" },
  { "Typeid", "typeid" },
  { "Typename", "typename" },
  { "Union", "union" },
  { "Unsigned", "unsigned" },
  { "Using", "using" },
  { "Virtual", "virtual" },
  { "Void", "void" },
  { "Volatile", "volatile" },
  { "Wchar_t", "wchar_t" },
  { "While", "while" },
  { "Xor", "xor" },
  { "Xor_eq", "xor_eq" },

  { "L_brace", "l_brace" },
  { "R_brace", "r_brace" },
  { "L_square", "l_square" },
  { "R_square", "r_square" },
  { "L_paren", "l_paren" },
  { "R_paren", "r_paren" },
  { "Semi", "semi" },
  { "Colon", "colon" },
  { "Ellipsis", "ellipsis" },
  { "Question", "question" },
  { "Coloncolon", "coloncolon" },
  { "Dot", "dot" },

  { "Plus", "plus" },
  { "Minus", "minus" },
  { "Star", "star" },
  { "Slash", "slash" },
  { "Percent", "percent" },
  { "Caret", "caret" },
  { "Amp", "amp" },
  { "Pipe", "pipe" },
  { "Tilde", "tilde" },
  { "Exclaim", "exclaim" },
  { "Equal", "equal" },
  { "Less", "less" },
  { "Greater", "greater" },
  { "Plusequal", "plusequal" },
  { "Minusequal", "minusequal" },
  { "Starequal", "starequal" },
  { "Slashequal", "slashequal" },
  { "Percentequal", "percentequal" },
  { "Caretequal", "caretequal" },
  { "Ampequal", "ampequal" },
  { "Pipeequal", "pipeequal" },
  { "Ltlt", "ltlt" },
  { "Gtgt", "gtgt" },
  { "Ltltequal", "ltltequal" },
  { "Gtgtequal", "gtgtequal" },
  { "Equalequal", "equalequal" },
  { "Exclaimequal", "exclaimequal" },
  { "Lessequal", "lessequal" },
  { "Greaterequal", "greaterequal" },
  { "Spaceship", "spaceship" },
  { "Ampamp", "ampamp" },
  { "Pipepipe", "pipepipe" },
  { "Plusplus", "plusplus" },
  { "Minusminus", "minusminus" },
  { "Comma", "comma" },
  { "Arrowstar", "arrowstar" },
  { "Arrow", "arrow" }

  // 在这里继续添加其他映射
};

int line = 0;
std::string file = "";
bool startOfLine = false;
bool leadingSpace = false;

void
print_token(const antlr4::Token* token,
            const antlr4::CommonTokenStream& tokens,
            std::ofstream& outFile,
            const antlr4::Lexer& lexer)
{
  auto& vocabulary = lexer.getVocabulary();

  auto tokenTypeName =
    std::string(vocabulary.getSymbolicName(token->getType()));

  if (tokenTypeName.empty())
    tokenTypeName = "<UNKNOWN>"; // 处理可能的空字符串情况

  if (tokenTypeMapping.find(tokenTypeName) != tokenTypeMapping.end()) {
    tokenTypeName = tokenTypeMapping[tokenTypeName];
  }
  
  if (tokenTypeName == "file") {
    std::string s = token->getText();
    {
      auto first = s.find_first_of(' ');
      auto second = s.find_first_of(' ', first + 1);
      line = std::stoul(s.substr(first + 1, second - first - 1)) - 1;
    }

    {
      auto first = s.find_first_of('\"');
      auto second = s.find_first_of('\"', first + 1);
      file = s.substr(first + 1, second - first - 1);
    }
    return;
  }

  if (tokenTypeName == "space") {
    leadingSpace = true;
    return;
  } else if (tokenTypeName == "line") {
    startOfLine = true;
    line ++;
    return;
  }

  std::string locInfo = " Loc=<" + file + ":" +
                        std::to_string(line) + ":" +
                        std::to_string(token->getCharPositionInLine() + 1) +
                        ">";

  if (token->getText() != "<EOF>")
    outFile << tokenTypeName << " '" << token->getText() << "'";
  else
    outFile << tokenTypeName << " '"
            << "'";
  if (startOfLine)
    outFile << "\t [StartOfLine]";
  if (leadingSpace)
    outFile << " [LeadingSpace]";
  outFile << locInfo << std::endl;

  startOfLine = false;
  leadingSpace = false;
}

int
main(int argc, char* argv[])
{
  if (argc != 3) {
    std::cout << "Usage: " << argv[0] << " <input> <output>\n";
    return -1;
  }

  std::ifstream inFile(argv[1]);
  if (!inFile) {
    std::cout << "Error: unable to open input file: " << argv[1] << '\n';
    return -2;
  }

  std::ofstream outFile(argv[2]);
  if (!outFile) {
    std::cout << "Error: unable to open output file: " << argv[2] << '\n';
    return -3;
  }

  std::cout << "程序 '" << argv[0] << std::endl;
  std::cout << "输入 '" << argv[1] << std::endl;
  std::cout << "输出 '" << argv[2] << std::endl;

  antlr4::ANTLRInputStream input(inFile);
  SYsULexer lexer(&input);

  antlr4::CommonTokenStream tokens(&lexer);
  tokens.fill();

  for (auto&& token : tokens.getTokens()) {
    print_token(token, tokens, outFile, lexer);
  }
}
