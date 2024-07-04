lexer grammar SYsULexer;

Alignas: 'alignas';
Alignof: 'alignof';
And: 'and';
And_eq: 'and_eq';
Asm: 'asm';
Atomic_cancel: 'atomic_cancel';
Atomic_commit: 'atomic_commit';
Atomic_noexcept: 'atomic_noexcept';
Auto: 'auto';
Bitand: 'bitand';
Bitor: 'bitor';
Bool: 'bool';
Break: 'break';
Case: 'case';
Catch: 'catch';
Char: 'char';
Char8_t: 'char8_t';
Char16_t: 'char16_t';
Char32_t: 'char32_t';
Class: 'class';
Compl: 'compl';
Concept: 'concept';
Const: 'const';
Consteval: 'consteval';
Constexpr: 'constexpr';
Constinit: 'constinit';
Const_cast: 'const_cast';
Continue: 'continue';
Co_await: 'co_await';
Co_return: 'co_return';
Co_yield: 'co_yield';
Decltype: 'decltype';
Default: 'default';
Delete: 'delete';
Do: 'do';
Double: 'double';
Dynamic_cast: 'dynamic_cast';
Else: 'else';
Enum: 'enum';
Explicit: 'explicit';
Export: 'export';
Extern: 'extern';
False: 'false';
Float: 'float';
For: 'for';
Friend: 'friend';
Goto: 'goto';
If: 'if';
Inline: 'inline';
Int: 'int';
Long: 'long';
Mutable: 'mutable';
Namespace: 'namespace';
New: 'new';
Noexcept: 'noexcept';
Not: 'not';
Not_eq: 'not_eq';
Nullptr: 'nullptr';
Operator: 'operator';
Or: 'or';
Or_eq: 'or_eq';
Private: 'private';
Protected: 'protected';
Public: 'public';
Reflexpr: 'reflexpr';
Register: 'register';
Reinterpret_cast: 'reinterpret_cast';
Requires: 'requires';
Return: 'return';
Short: 'short';
Signed: 'signed';
Sizeof: 'sizeof';
Static: 'static';
Static_assert: 'static_assert';
Static_cast: 'static_cast';
Struct: 'struct';
Switch: 'switch';
Synchronized: 'synchronized';
Template: 'template';
This: 'this';
Thread_local: 'thread_local';
Throw: 'throw';
True: 'true';
Try: 'try';
Typedef: 'typedef';
Typeid: 'typeid';
Typename: 'typename';
Union: 'union';
Unsigned: 'unsigned';
Using: 'using';
Virtual: 'virtual';
Void: 'void';
Volatile: 'volatile';
Wchar_t: 'wchar_t';
While: 'while';
Xor: 'xor';
Xor_eq: 'xor_eq';

L_brace: '{';
R_brace: '}';
L_square: '[';
R_square: ']';
L_paren: '(';
R_paren: ')';
Semi: ';';
Colon: ':';
Ellipsis: '...';
Question: '?';
Coloncolon: '::';
Dot: '.';

Plus: '+';
Minus: '-';
Star: '*';
Slash: '/';
Percent: '%';
Caret: '^';
Amp: '&';
Pipe: '|';
Tilde: '~';
Exclaim: '!';
Equal: '=';
Less: '<';
Greater: '>';
Plusequal: '+=';
Minusequal: '-=';
Starequal: '*=';
Slashequal: '/=';
Percentequal: '%=';
Caretequal: '^=';
Ampequal: '&=';
Pipeequal: '|=';
Ltlt: '<<';
Gtgt: '>>';
Ltltequal: '<<=';
Gtgtequal: '>>=';
Equalequal: '==';
Exclaimequal: '!=';
Lessequal: '<=';
Greaterequal: '>=';
Spaceship: '<=>';
Ampamp: '&&';
Pipepipe: '||';
Plusplus: '++';
Minusminus: '--';
Comma: ',';
Arrowstar: '->*';
Arrow: '->';

Identifier
    :   IdentifierNondigit
        (   IdentifierNondigit
        |   Digit
        )*
    ;

fragment
IdentifierNondigit
    :   Nondigit
    ;

fragment
Nondigit
    :   [a-zA-Z_]
    ;

fragment
Digit
    :   [0-9]
    ;

Constant
    :   IntegerConstant
    ;

fragment
IntegerConstant
    :   DecimalConstant
    |   OctalConstant
    |   HexadecimalConstant
    ;

fragment
DecimalConstant
    :   NonzeroDigit Digit*
    ;

fragment
OctalConstant
    :   '0' OctalDigit*
    ;


fragment
NonzeroDigit
    :   [1-9]
    ;

fragment
OctalDigit
    :   [0-7]
    ;

fragment
HexadecimalConstant
    :   HexadecimalPrefix HexadecimalDigit+
    ;

fragment
HexadecimalPrefix
    :   '0' [xX]
    ;

fragment
HexadecimalDigit
    :   [0-9a-fA-F]
    ;


// 预处理信息处理，可以从预处理信息中获得文件名以及行号
// 预处理信息前面的数组即行号
LineAfterPreprocessing
    :   '#' Whitespace* ~[\r\n]*
    ;

Whitespace
    :   [ \t]+
    ;

// 换行符号，可以利用这个信息来更新行号
Newline
    :   (   '\r' '\n'?
        |   '\n'
        )
    ;

