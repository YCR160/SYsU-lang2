parser grammar SYsUParser;

options {
  tokenVocab=SYsULexer;
}

// Expressions
primaryExpression
    :   Identifier
    |   Constant
    |   LeftParen expression RightParen
    ;

postfixExpression
    :   primaryExpression (
        LeftBracket expression RightBracket
        | LeftParen argumentExpressionList? RightParen
    )*
    ;

argumentExpressionList
    : assignmentExpression (Comma assignmentExpression)*
    ;

unaryExpression
    :   postfixExpression
    |   unaryOperator castExpression
    ;

unaryOperator
    :   Plus | Minus | Exclaim
    ;

castExpression
    :   unaryExpression
    ;

multiplicativeExpression
    :   castExpression ((Star|Div|Mod) castExpression)*
    ;

additiveExpression
    :   multiplicativeExpression ((Plus|Minus) multiplicativeExpression)*
    ;

relationalExpression
    :   additiveExpression ((Less|LessEqual|Greater|GreaterEqual) additiveExpression)*
    ;

equalityExpression
    :   relationalExpression ((EqualEqual|ExclaimEqual) relationalExpression)*
    ;

logicalAndExpression
    :   equalityExpression (AmpAmp equalityExpression)*
    ;

logicalOrExpression
    :   logicalAndExpression (PipePipe logicalAndExpression)*
    ;

assignmentExpression
    :   logicalOrExpression
    |   unaryExpression Equal assignmentExpression
    ;

expression
    :   assignmentExpression (Comma assignmentExpression)*
    ;

// Declarations
declaration
    :   declarationSpecifiers initDeclaratorList? Semi
    ;

declarationSpecifiers
    :   declarationSpecifier+
    ;

declarationSpecifiers2
    :   declarationSpecifier+
    ;

declarationSpecifier
    :   typeSpecifier
    |   typeQualifier
    ;

initDeclaratorList
    :   initDeclarator (Comma initDeclarator)*
    ;

initDeclarator
    :   declarator (Equal initializer)?
    ;

typeSpecifier
    :   Void
    |   Char
    |   Int
    |   Long
    |   LongLong
    ;

typeQualifier
    :   Const
    ;

declarator
    :   directDeclarator
    ;

directDeclarator
    :   Identifier
    |   LeftParen declarator RightParen
    |   directDeclarator LeftBracket typeQualifierList? assignmentExpression? RightBracket
    |   directDeclarator LeftParen parameterTypeList RightParen
    |   directDeclarator LeftParen identifierList? RightParen
    ;

typeQualifierList
    :   typeQualifier+
    ;

parameterTypeList
    :   parameterList (Comma Ellipsis)?
    ;

parameterList
    :   parameterDeclaration (Comma parameterDeclaration)*
    ;

parameterDeclaration
    :   declarationSpecifiers declarator
    |   declarationSpecifiers2 abstractDeclarator?
    ;

identifierList
    :   Identifier (Comma Identifier)*
    ;

abstractDeclarator
    :   directAbstractDeclarator
    ;

directAbstractDeclarator
    :   LeftParen abstractDeclarator RightParen
    |   LeftBracket typeQualifierList? assignmentExpression? RightBracket
    |   LeftParen parameterTypeList? RightParen
    |   directAbstractDeclarator LeftBracket typeQualifierList? assignmentExpression? RightBracket
    |   directAbstractDeclarator LeftParen parameterTypeList? RightParen
    ;

initializer
    :   assignmentExpression
    |   LeftBrace initializerList? Comma? RightBrace
    ;

initializerList
    // :   designation? initializer (Comma designation? initializer)*
    :   initializer (Comma initializer)*
    ;

// Statements
statement
    :   compoundStatement
    |   expressionStatement
    |   selectionStatement
    |   iterationStatement
    |   jumpStatement
    ;

compoundStatement
    :   LeftBrace blockItemList? RightBrace
    ;

blockItemList
    :   blockItem+
    ;

blockItem
    :   statement
    |   declaration
    ;

expressionStatement
    :   expression? Semi
    ;

selectionStatement
    :   If LeftParen expression RightParen statement (Else statement)?
    ;

iterationStatement
    :   While LeftParen expression RightParen statement
    |   For LeftParen forCondition RightParen statement
    ;

forCondition
    :   (forDeclaration | expression?) Semi forExpression? Semi forExpression?
    ;

forDeclaration
    :   declarationSpecifiers initDeclaratorList?
    ;

forExpression
    :   assignmentExpression (Comma assignmentExpression)*
    ;

jumpStatement
    :   (   Continue
        |   Break
        |   Return expression?
    )   Semi
    ;

// Compilation unit
compilationUnit
    :   translationUnit? EOF
    ;

translationUnit
    :   externalDeclaration+
    ;

externalDeclaration
    :   functionDefinition
    |   declaration
    |   Semi
    ;

functionDefinition
    :   declarationSpecifiers directDeclarator LeftParen funcParamDeclaration? RightParen (compoundStatement | Semi)
    ;

funcParamDeclaration
    :   declarationSpecifiers initDeclarator (Comma declarationSpecifiers initDeclarator)*
    ;