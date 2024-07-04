#include "Ast2Asg.hpp"
#include <unordered_map>

#define self (*this)

namespace asg
{

// 符号表，保存当前作用域的所有声明
struct Ast2Asg::Symtbl : public std::unordered_map<std::string, Decl *>
{
    Ast2Asg &m;
    Symtbl *mPrev;

    Symtbl(Ast2Asg &m) : m(m), mPrev(m.mSymtbl)
    {
        m.mSymtbl = this;
    }

    ~Symtbl()
    {
        m.mSymtbl = mPrev;
    }

    Decl *resolve(const std::string &name);
};

Decl *Ast2Asg::Symtbl::resolve(const std::string &name)
{
    auto iter = find(name);
    if (iter != end())
        return iter->second;
    ASSERT(mPrev != nullptr); // 标识符未定义
    return mPrev->resolve(name);
}

// translationUnit
//     :   externalDeclaration+
//     ;

// externalDeclaration
//     :   functionDefinition
//     |   declaration
//     |   Semi
//     ;

TranslationUnit *Ast2Asg::operator()(ast::TranslationUnitContext *ctx)
{
    auto ret = make<TranslationUnit>();
    if (ctx == nullptr)
        return ret;

    Symtbl localDecls(self);

    for (auto &&i : ctx->externalDeclaration())
    {
        if (auto p = i->declaration())
        {
            auto decls = self(p);
            ret->decls.insert(ret->decls.end(), std::make_move_iterator(decls.begin()),
                              std::make_move_iterator(decls.end()));
        }

        else if (auto p = i->functionDefinition())
        {
            auto funcDecl = self(p);
            ret->decls.push_back(funcDecl);

            // 添加到声明表
            localDecls[funcDecl->name] = funcDecl;
        }
    }

    return ret;
}

//==============================================================================
// 类型
//==============================================================================

// declarationSpecifiers
//     :   declarationSpecifier+
//     ;

// declarationSpecifier
//     :   typeSpecifier
//     |   typeQualifier
//     ;

// typeSpecifier
//     :   Void
//     |   Char
//     |   Int
//     |   Long
//     |   LongLong
//     ;

// typeQualifier
//     :   Const
//     ;

Ast2Asg::SpecQual Ast2Asg::operator()(ast::DeclarationSpecifiersContext *ctx)
{
    SpecQual ret = {Type::Spec::kINVALID, Type::Qual()};

    for (auto &&i : ctx->declarationSpecifier())
    {
        if (auto p = i->typeSpecifier())
        {
            if (p->Void())
                ret.first = Type::Spec::kVoid;
            else if (p->Char())
                ret.first = Type::Spec::kChar;
            else if (p->Int())
                ret.first = Type::Spec::kInt;
            else if (p->Long())
                ret.first = Type::Spec::kLong;
            else if (p->LongLong())
                ret.first = Type::Spec::kLongLong;
        }
        else if (auto p = i->typeQualifier())
        {
            if (p->Const())
                ret.second.const_ = true;
        }
    }
    return ret;
}

// declarationSpecifiers2
//     :   declarationSpecifier+
//     ;

Ast2Asg::SpecQual Ast2Asg::operator()(ast::DeclarationSpecifiers2Context *ctx)
{
    SpecQual ret = {Type::Spec::kINVALID, Type::Qual()};

    for (auto &&i : ctx->declarationSpecifier())
    {
        if (auto p = i->typeSpecifier())
        {
            if (ret.first == Type::Spec::kINVALID)
            {
                if (p->Void())
                    ret.first = Type::Spec::kVoid;
                else if (p->Char())
                    ret.first = Type::Spec::kChar;
                else if (p->Int())
                    ret.first = Type::Spec::kInt;
                else if (p->Long())
                    ret.first = Type::Spec::kLong;
            }
        }
        else if (auto p = i->typeQualifier())
        {
            if (p->Const())
                ret.second.const_ = true;
        }
    }
    return ret;
}

std::pair<TypeExpr *, std::string> Ast2Asg::operator()(ast::DeclaratorContext *ctx, TypeExpr *sub)
{
    return self(ctx->directDeclarator(), sub);
}

// abstractDeclarator
//     :   directAbstractDeclarator
//     ;

std::pair<TypeExpr *, std::string> Ast2Asg::operator()(ast::AbstractDeclaratorContext *ctx, TypeExpr *sub)
{
    return self(ctx->directAbstractDeclarator(), sub);
}

static int eval_arrlen(Expr *expr)
{
    if (auto p = expr->dcst<IntegerLiteral>())
        return p->val;

    if (auto p = expr->dcst<DeclRefExpr>())
    {
        if (p->decl == nullptr)
        {
            ABORT();
        }

        auto var = p->decl->dcst<VarDecl>();
        if (!var || !var->type->qual.const_)
            ABORT(); // 数组长度必须是编译期常量

        switch (var->type->spec)
        {
        case Type::Spec::kInt:
            return var->init->dcst<IntegerLiteral>()->val;

        case Type::Spec::kLong:
            return var->init->dcst<IntegerLiteral>()->val;

        case Type::Spec::kLongLong:
            return var->init->dcst<IntegerLiteral>()->val;

        default:
            ABORT();
        }
    }

    if (auto p = expr->dcst<UnaryExpr>())
    {
        auto sub = eval_arrlen(p->sub);

        switch (p->op)
        {
        case UnaryExpr::kPos:
            return sub;

        case UnaryExpr::kNeg:
            return -sub;

        case UnaryExpr::kNot:
            return !sub;

        default:
            ABORT();
        }
    }

    if (auto p = expr->dcst<BinaryExpr>())
    {
        auto lft = eval_arrlen(p->lft);
        auto rht = eval_arrlen(p->rht);

        switch (p->op)
        {
        case BinaryExpr::kAdd:
            return lft + rht;

        case BinaryExpr::kSub:
            return lft - rht;

        case BinaryExpr::kMul:
            return lft * rht;

        case BinaryExpr::kDiv:
            return lft / rht;

        case BinaryExpr::kMod:
            return lft % rht;

        default:
            ABORT();
        }
    }

    if (auto p = expr->dcst<InitListExpr>())
    {
        if (p->list.empty())
            return 0;
        return eval_arrlen(p->list[0]);
    }

    ABORT();
}

// directDeclarator
//     :   Identifier
//     |   directDeclarator LeftBracket assignmentExpression? RightBracket
//     ;

// std::pair<TypeExpr *, std::string> Ast2Asg::operator()(ast::DirectDeclaratorContext *ctx, TypeExpr *sub)
// {
//     if (auto p = ctx->Identifier())
//         return {sub, p->getText()};

//     if (ctx->LeftBracket())
//     {
//         auto arrayType = make<ArrayType>();
//         arrayType->sub = sub;

//         if (auto p = ctx->assignmentExpression())
//             arrayType->len = eval_arrlen(self(p));
//         else
//             arrayType->len = ArrayType::kUnLen;

//         return self(ctx->directDeclarator(), arrayType);
//     }

//     ABORT();
// }

// directDeclarator
//     :   Identifier
//     |   LeftParen declarator RightParen
//     |   directDeclarator LeftBracket typeQualifierList? assignmentExpression? RightBracket
//     |   directDeclarator LeftParen parameterTypeList RightParen
//     |   directDeclarator LeftParen identifierList? RightParen
//     ;

std::pair<TypeExpr *, std::string> Ast2Asg::operator()(ast::DirectDeclaratorContext *ctx, TypeExpr *sub)
{
    if (auto p = ctx->Identifier())
    {
        return {sub, p->getText()};
    }

    if (ctx->declarator())
    {
        return self(ctx->declarator(), sub);
    }

    if (ctx->LeftBracket())
    {
        auto arrayType = make<ArrayType>();
        arrayType->sub = sub;

        if (ctx->typeQualifierList())
        {
            for (auto &&i : ctx->typeQualifierList()->typeQualifier())
            {
                if (i->Const())
                {
                    auto node = make<ArrayType>();
                    node->sub = arrayType;
                    arrayType = node;
                }
            }
        }

        if (auto p = ctx->assignmentExpression())
            arrayType->len = eval_arrlen(self(p));
        else
            arrayType->len = ArrayType::kUnLen;

        return self(ctx->directDeclarator(), arrayType);
    }

    if (ctx->LeftParen())
    {
        if (ctx->parameterTypeList())
        {
            // auto funcType = make<FunctionType>();
            // funcType->sub = sub;
            // auto paramList = self(ctx->parameterTypeList(), nullptr);
            // funcType->params.push_back(dynamic_cast<const asg::Type *>(paramList.first));
            // auto [texp, name] = self(ctx->directDeclarator(), nullptr);
            // return {funcType, name};
            auto [texp, name] = self(ctx->directDeclarator(), nullptr);
            return {sub, name};
        }
        else if (ctx->identifierList())
        {
            auto [texp, name] = self(ctx->directDeclarator(), nullptr);
            return {sub, name};
        }
        else
        {
            auto [texp, name] = self(ctx->directDeclarator(), nullptr);
            return {sub, name};
        }
    }

    ABORT();
}

// typeQualifierList
//     : typeQualifier+
//     ;

// typeQualifier
//     :   Const
//     ;

std::pair<TypeExpr *, std::string> Ast2Asg::operator()(ast::TypeQualifierListContext *ctx, TypeExpr *sub)
{
    for (auto &&i : ctx->typeQualifier())
    {
        if (i->Const())
        {
            auto node = make<PointerType>();
            node->sub = sub;
            sub = node;
        }
    }

    return {sub, ""};
}

// parameterTypeList
//     :   parameterList (Comma Ellipsis)?
//     ;

std::pair<TypeExpr *, std::string> Ast2Asg::operator()(ast::ParameterTypeListContext *ctx, TypeExpr *sub)
{
    return self(ctx->parameterList(), sub);
}

// //==============================================================================
// // 类型
// //==============================================================================

// identifierList
//     :   Identifier (Comma Identifier)*
//     ;

std::pair<TypeExpr *, std::string> Ast2Asg::operator()(ast::IdentifierListContext *ctx, TypeExpr *sub)
{
    auto list = ctx->Identifier();
    auto ret = sub;

    for (unsigned i = 1; i < list.size(); ++i)
    {
        auto node = make<PointerType>();
        node->sub = ret;
        ret = node;
    }

    return {ret, list[0]->getText()};
}

// parameterList
//     : parameterDeclaration (Comma parameterDeclaration)*
//     ;

std::pair<TypeExpr *, std::string> Ast2Asg::operator()(ast::ParameterListContext *ctx, TypeExpr *sub)
{
    auto list = ctx->parameterDeclaration();
    auto ret = sub;

    for (unsigned i = 1; i < list.size(); ++i)
    {
        auto node = make<PointerType>();
        node->sub = ret;
        ret = node;
    }

    return {ret, list[0]->getText()};
}

// parameterDeclaration
//     : declarationSpecifiers declarator
//     ;

// std::pair<TypeExpr *, std::string> Ast2Asg::operator()(ast::ParameterDeclarationContext *ctx)
// {
//     auto sq = self(ctx->declarationSpecifiers());
//     return self(ctx->declarator(), nullptr);
// }

// parameterDeclaration
//     :   declarationSpecifiers declarator
//     |   declarationSpecifiers2 abstractDeclarator?
//     ;

std::pair<TypeExpr *, std::string> Ast2Asg::operator()(ast::ParameterDeclarationContext *ctx, TypeExpr *sub)
{
    if (auto p = ctx->declarator())
    {
        auto sq = self(ctx->declarationSpecifiers());
        return self(p, nullptr);
    }
    else
    {
        auto sq = self(ctx->declarationSpecifiers2());
        return self(p, nullptr);
    }
}

//==============================================================================
// 表达式
//==============================================================================

Expr *Ast2Asg::operator()(ast::ExpressionContext *ctx)
{
    auto list = ctx->assignmentExpression();
    Expr *ret = self(list[0]);

    for (unsigned i = 1; i < list.size(); ++i)
    {
        auto node = make<BinaryExpr>();
        node->op = node->kComma;
        node->lft = ret;
        node->rht = self(list[i]);
        ret = node;
    }

    return ret;
}

// assignmentExpression
//     :   logicalOrExpression
//     |   unaryExpression Equal assignmentExpression
//     ;

Expr *Ast2Asg::operator()(ast::AssignmentExpressionContext *ctx)
{
    if (auto p = ctx->logicalOrExpression())
        return self(p);

    auto ret = make<BinaryExpr>();
    ret->op = ret->kAssign;
    ret->lft = self(ctx->unaryExpression());
    ret->rht = self(ctx->assignmentExpression());
    return ret;
}

// multiplicativeExpression
//     :   castExpression ((Star|Div|Mod) castExpression)*
//     ;

Expr *Ast2Asg::operator()(ast::MultiplicativeExpressionContext *ctx)
{
    auto children = ctx->children;
    Expr *ret = self(dynamic_cast<ast::CastExpressionContext *>(children[0]));

    for (unsigned i = 1; i < children.size(); ++i)
    {
        auto node = make<BinaryExpr>();

        auto token = dynamic_cast<antlr4::tree::TerminalNode *>(children[i])->getSymbol()->getType();
        switch (token)
        {
        case ast::Star:
            node->op = node->kMul;
            break;

        case ast::Div:
            node->op = node->kDiv;
            break;

        case ast::Mod:
            node->op = node->kMod;
            break;
        }

        node->lft = ret;
        node->rht = self(dynamic_cast<ast::CastExpressionContext *>(children[++i]));
        ret = node;
    }

    return ret;
}

// additiveExpression
//     :   multiplicativeExpression ((Plus|Minus) multiplicativeExpression)*
//     ;

Expr *Ast2Asg::operator()(ast::AdditiveExpressionContext *ctx)
{
    auto children = ctx->children;
    Expr *ret = self(dynamic_cast<ast::MultiplicativeExpressionContext *>(children[0]));

    for (unsigned i = 1; i < children.size(); ++i)
    {
        auto node = make<BinaryExpr>();

        auto token = dynamic_cast<antlr4::tree::TerminalNode *>(children[i])->getSymbol()->getType();
        switch (token)
        {
        case ast::Plus:
            node->op = node->kAdd;
            break;

        case ast::Minus:
            node->op = node->kSub;
            break;
        }

        node->lft = ret;
        node->rht = self(dynamic_cast<ast::MultiplicativeExpressionContext *>(children[++i]));
        ret = node;
    }

    return ret;
}

// relationalExpression
//     :   additiveExpression ((Less|LessEqual|Greater|GreaterEqual) additiveExpression)*
//     ;

Expr *Ast2Asg::operator()(ast::RelationalExpressionContext *ctx)
{
    auto children = ctx->children;
    Expr *ret = self(dynamic_cast<ast::AdditiveExpressionContext *>(children[0]));

    for (unsigned i = 1; i < children.size(); ++i)
    {
        auto node = make<BinaryExpr>();

        auto token = dynamic_cast<antlr4::tree::TerminalNode *>(children[i])->getSymbol()->getType();
        switch (token)
        {
        case ast::Less:
            node->op = node->kLt;
            break;

        case ast::LessEqual:
            node->op = node->kLe;
            break;

        case ast::Greater:
            node->op = node->kGt;
            break;

        case ast::GreaterEqual:
            node->op = node->kGe;
            break;
        }

        node->lft = ret;
        node->rht = self(dynamic_cast<ast::AdditiveExpressionContext *>(children[++i]));
        ret = node;
    }

    return ret;
}

// equalityExpression
//     :   relationalExpression ((EqualEqual|ExclaimEqual) relationalExpression)*
//     ;

Expr *Ast2Asg::operator()(ast::EqualityExpressionContext *ctx)
{
    auto children = ctx->children;
    Expr *ret = self(dynamic_cast<ast::RelationalExpressionContext *>(children[0]));

    for (unsigned i = 1; i < children.size(); ++i)
    {
        auto node = make<BinaryExpr>();

        auto token = dynamic_cast<antlr4::tree::TerminalNode *>(children[i])->getSymbol()->getType();
        switch (token)
        {
        case ast::EqualEqual:
            node->op = node->kEq;
            break;

        case ast::ExclaimEqual:
            node->op = node->kNe;
            break;
        }

        node->lft = ret;
        node->rht = self(dynamic_cast<ast::RelationalExpressionContext *>(children[++i]));
        ret = node;
    }

    return ret;
}

// logicalAndExpression
//     :   equalityExpression (AmpAmp equalityExpression)*
//     ;

Expr *Ast2Asg::operator()(ast::LogicalAndExpressionContext *ctx)
{
    auto children = ctx->children;
    Expr *ret = self(dynamic_cast<ast::EqualityExpressionContext *>(children[0]));

    for (unsigned i = 1; i < children.size(); ++i)
    {
        auto node = make<BinaryExpr>();
        node->op = node->kAnd;
        node->lft = ret;
        node->rht = self(dynamic_cast<ast::EqualityExpressionContext *>(children[++i]));
        ret = node;
    }

    return ret;
}

// logicalOrExpression
//     :   logicalAndExpression (PipePipe logicalAndExpression)*
//     ;

Expr *Ast2Asg::operator()(ast::LogicalOrExpressionContext *ctx)
{
    auto children = ctx->children;
    Expr *ret = self(dynamic_cast<ast::LogicalAndExpressionContext *>(children[0]));

    for (unsigned i = 1; i < children.size(); ++i)
    {
        auto node = make<BinaryExpr>();
        node->op = node->kOr;
        node->lft = ret;
        node->rht = self(dynamic_cast<ast::LogicalAndExpressionContext *>(children[++i]));
        ret = node;
    }

    return ret;
}

// castExpression
//     : unaryExpression
//     ;

Expr *Ast2Asg::operator()(ast::CastExpressionContext *ctx)
{
    return self(ctx->unaryExpression());
}

// unaryExpression
//     :   postfixExpression
//     |   unaryOperator castExpression
//     ;

Expr *Ast2Asg::operator()(ast::UnaryExpressionContext *ctx)
{
    if (auto p = ctx->postfixExpression())
        return self(p);

    auto ret = make<UnaryExpr>();

    switch (dynamic_cast<antlr4::tree::TerminalNode *>(ctx->unaryOperator()->children[0])->getSymbol()->getType())
    {
    case ast::Plus:
        ret->op = ret->kPos;
        break;

    case ast::Minus:
        ret->op = ret->kNeg;
        break;

    case ast::Exclaim:
        ret->op = ret->kNot;
        break;
    }

    ret->sub = self(ctx->castExpression());

    return ret;
}

// postfixExpression
//     :   primaryExpression
//     ;

// Expr *Ast2Asg::operator()(ast::PostfixExpressionContext *ctx)
// {
//     auto children = ctx->children;
//     auto sub = self(dynamic_cast<ast::PrimaryExpressionContext *>(children[0]));
//     return sub;
// }

// postfixExpression
//     :   primaryExpression (
//         LeftBracket expression RightBracket
//         | LeftParen argumentExpressionList? RightParen
//     )*
//     ;

Expr *Ast2Asg::operator()(ast::PostfixExpressionContext *ctx)
{
    auto children = ctx->children;
    auto sub = self(dynamic_cast<ast::PrimaryExpressionContext *>(children[0]));

    for (unsigned i = 1; i < children.size(); ++i)
    {
        if (auto p = dynamic_cast<antlr4::tree::TerminalNode *>(children[i]))
        {
            switch (p->getSymbol()->getType())
            {
            case ast::LeftBracket: {
                auto node = make<BinaryExpr>();
                node->op = node->kIndex;
                node->lft = sub;
                node->rht = self(dynamic_cast<ast::ExpressionContext *>(children[++i]));
                sub = node;
                break;
            }

            case ast::LeftParen: {
                auto node = make<CallExpr>();
                node->head = sub;

                if (auto p = dynamic_cast<ast::ArgumentExpressionListContext *>(children[++i]))
                {
                    for (auto &&j : p->assignmentExpression())
                    {
                        node->args.push_back(self(j));
                    }
                }
                    // node->args.push_back(self(p));

                sub = node;
                break;
            }
            }
        }
    }

    return sub;
}

// argumentExpressionList
//     : assignmentExpression (Semi assignmentExpression)*
//     ;

Expr *Ast2Asg::operator()(ast::ArgumentExpressionListContext *ctx)
{
    auto children = ctx->children;
    auto ret = self(dynamic_cast<ast::AssignmentExpressionContext *>(children[0]));

    for (unsigned i = 1; i < children.size(); ++i)
    {
        auto node = make<BinaryExpr>();
        node->op = node->kComma;
        node->lft = ret;
        node->rht = self(dynamic_cast<ast::AssignmentExpressionContext *>(children[++i]));
        ret = node;
    }

    return ret;
}

// primaryExpression
//     :   Identifier
//     |   Constant
//     |   LeftParen expression RightParen
//     ;

Expr *Ast2Asg::operator()(ast::PrimaryExpressionContext *ctx)
{
    if (auto p = ctx->Identifier())
    {
        auto name = p->getText();
        auto ret = make<DeclRefExpr>();
        ret->decl = mSymtbl->resolve(name);
        return ret;
    }

    if (auto p = ctx->Constant())
    {
        auto text = p->getText();

        auto ret = make<IntegerLiteral>();

        ASSERT(!text.empty());
        if (text[0] != '0')
            ret->val = std::stoll(text);

        else if (text.size() == 1)
            ret->val = 0;

        else if (text[1] == 'x' || text[1] == 'X')
            ret->val = std::stoll(text.substr(2), nullptr, 16);

        else
            ret->val = std::stoll(text.substr(1), nullptr, 8);

        return ret;
    }

    if (auto p = ctx->expression())
    {
        auto ret = make<ParenExpr>(); // 创建 ParenExpr 节点
        ret->sub = self(p);           // 设置 expr 属性为表达式的处理结果
        return ret;
    }

    ABORT();
}

// directAbstractDeclarator
//     :   LeftParen abstractDeclarator RightParen
//     |   LeftBracket typeQualifierList? assignmentExpression? RightBracket
//     |   LeftParen parameterTypeList? RightParen
//     |   directAbstractDeclarator LeftBracket typeQualifierList? assignmentExpression? RightBracket
//     |   directAbstractDeclarator LeftParen parameterTypeList? RightParen
//     ;

std::pair<TypeExpr *, std::string> Ast2Asg::operator()(ast::DirectAbstractDeclaratorContext *ctx, TypeExpr *sub)
{
    if (ctx->abstractDeclarator())
    {
        return self(ctx->abstractDeclarator(), sub);
    }

    if (ctx->LeftBracket())
    {
        auto arrayType = make<ArrayType>();
        arrayType->sub = sub;

        if (ctx->typeQualifierList())
        {
            for (auto &&i : ctx->typeQualifierList()->typeQualifier())
            {
                if (i->Const())
                {
                    auto node = make<ArrayType>();
                    node->sub = arrayType;
                    arrayType = node;
                }
            }
        }

        if (auto p = ctx->assignmentExpression())
            arrayType->len = eval_arrlen(self(p));
        else
            arrayType->len = ArrayType::kUnLen;

        return {arrayType, ""};
    }

    if (ctx->LeftParen() && ctx->RightParen())
    {
        if (ctx->parameterTypeList())
        {
            return self(ctx->parameterTypeList(), sub);
        }
        else
        {
            return {sub, ""};
        }
    }

    ABORT();
}

// initializer
//     :   assignmentExpression
//     |   LeftBrace initializerList? Comma? RightBrace
//     ;

// initializerList
//     // :   designation? initializer (Comma designation? initializer)*
//     :   initializer (Comma initializer)*
//     ;

Expr *Ast2Asg::operator()(ast::InitializerContext *ctx)
{
    if (auto p = ctx->assignmentExpression())
        return self(p);

    auto ret = make<InitListExpr>();

    if (auto p = ctx->initializerList())
    {
        for (auto &&i : p->initializer())
        {
            // 将初始化列表展平
            auto expr = self(i);
            if (auto p = expr->dcst<InitListExpr>())
            {
                for (auto &&sub : p->list)
                    ret->list.push_back(sub);
            }
            else
            {
                ret->list.push_back(expr);
            }
        }
    }

    return ret;
}

//==============================================================================
// 语句
//==============================================================================

// statement
//     :   compoundStatement
//     |   expressionStatement
//     |   selectionStatement
//     |   iterationStatement
//     |   jumpStatement
//     ;

Stmt *Ast2Asg::operator()(ast::StatementContext *ctx)
{
    if (auto p = ctx->compoundStatement())
        return self(p);

    if (auto p = ctx->expressionStatement())
        return self(p);

    if (auto p = ctx->selectionStatement())
        return self(p);

    if (auto p = ctx->iterationStatement())
        return self(p);

    if (auto p = ctx->jumpStatement())
        return self(p);

    ABORT();
}

CompoundStmt *Ast2Asg::operator()(ast::CompoundStatementContext *ctx)
{
    auto ret = make<CompoundStmt>();

    if (auto p = ctx->blockItemList())
    {
        Symtbl localDecls(self);

        for (auto &&i : p->blockItem())
        {
            if (auto q = i->declaration())
            {
                auto sub = make<DeclStmt>();
                sub->decls = self(q);
                ret->subs.push_back(sub);
            }

            else if (auto q = i->statement())
                ret->subs.push_back(self(q));
        }
    }

    return ret;
}

// expressionStatement
//     :   expression? Semi
//     ;

Stmt *Ast2Asg::operator()(ast::ExpressionStatementContext *ctx)
{
    if (auto p = ctx->expression())
    {
        auto ret = make<ExprStmt>();
        ret->expr = self(p);
        return ret;
    }

    return make<NullStmt>();
}

// selectionStatement
//     :   If LeftParen expression RightParen statement (Else statement)?
//     ;

Stmt *Ast2Asg::operator()(ast::SelectionStatementContext *ctx)
{
    auto ret = make<IfStmt>();
    ret->cond = self(ctx->expression());
    ret->then = self(ctx->statement(0));

    if (auto p = ctx->statement(1))
        ret->else_ = self(p);

    return ret;
}

// iterationStatement
//     :   While LeftParen expression RightParen statement
//     |   For LeftParen forCondition RightParen statement
//     ;

Stmt *Ast2Asg::operator()(ast::IterationStatementContext *ctx)
{
    if (ctx->While())
    {
        auto ret = make<WhileStmt>();
        ret->cond = self(ctx->expression());
        ret->body = self(ctx->statement());
        return ret;
    }
    else
    {
        auto ret = make<WhileStmt>();
        ret->cond = self(ctx->forCondition()->expression());
        ret->body = self(ctx->statement());
        return ret;
    }
}

// forCondition
//     :   (forDeclaration | expression?) Semi forExpression? Semi forExpression?
//     ;

Stmt *Ast2Asg::operator()(ast::ForConditionContext *ctx)
{
    auto ret = make<CompoundStmt>();

    if (auto p = ctx->forDeclaration())
    {
        auto sub = make<DeclStmt>();
        sub->decls = self(p);
        ret->subs.push_back(sub);
    }

    else if (auto p = ctx->expression())
    {
        auto sub = make<ExprStmt>();
        sub->expr = self(p);
        ret->subs.push_back(sub);
    }

    if (auto p = ctx->forExpression(0))
    {
        auto sub = make<ExprStmt>();
        sub->expr = self(p);
        ret->subs.push_back(sub);
    }

    return ret;
}

// forDeclaration
//     :   declarationSpecifiers initDeclaratorList?
//     ;

std::vector<Decl *> Ast2Asg::operator()(ast::ForDeclarationContext *ctx)
{
    std::vector<Decl *> ret;

    auto sq = self(ctx->declarationSpecifiers());

    if (auto p = ctx->initDeclaratorList())
    {
        for (auto &&j : p->initDeclarator())
            ret.push_back(self(j, sq));
    }

    return ret;
}

// forExpression
//     :   assignmentExpression (Comma assignmentExpression)*
//     ;

Expr *Ast2Asg::operator()(ast::ForExpressionContext *ctx)
{
    auto children = ctx->children;
    Expr *ret = self(dynamic_cast<ast::AssignmentExpressionContext *>(children[0]));

    for (unsigned i = 1; i < children.size(); ++i)
    {
        auto node = make<BinaryExpr>();
        node->op = node->kComma;
        node->lft = ret;
        node->rht = self(dynamic_cast<ast::AssignmentExpressionContext *>(children[++i]));
        ret = node;
    }

    return ret;
}

// jumpStatement
//     :   (   Continue
//         |   Break
//         |   Return expression?
//     )   Semi
//     ;

Stmt *Ast2Asg::operator()(ast::JumpStatementContext *ctx)
{
    if (ctx->Continue())
        return make<ContinueStmt>();

    if (ctx->Break())
        return make<BreakStmt>();

    if (ctx->Return())
    {
        auto ret = make<ReturnStmt>();
        ret->func = mCurrentFunc;
        if (auto p = ctx->expression())
            ret->expr = self(p);
        return ret;
    }

    ABORT();
}

//==============================================================================
// 声明
//==============================================================================

std::vector<Decl *> Ast2Asg::operator()(ast::DeclarationContext *ctx)
{
    std::vector<Decl *> ret;

    auto specs = self(ctx->declarationSpecifiers());

    if (auto p = ctx->initDeclaratorList())
    {
        for (auto &&j : p->initDeclarator())
            ret.push_back(self(j, specs));
    }

    // 如果 initDeclaratorList 为空则这行声明语句无意义
    return ret;
}

// functionDefinition
//     : declarationSpecifiers directDeclarator LeftParen RightParen compoundStatement
//     ;

// FunctionDecl *Ast2Asg::operator()(ast::FunctionDefinitionContext *ctx)
// {
//     auto ret = make<FunctionDecl>();
//     mCurrentFunc = ret;

//     auto type = make<Type>();
//     ret->type = type;

//     auto sq = self(ctx->declarationSpecifiers());
//     type->spec = sq.first;
//     type->qual = sq.second;

//     auto [texp, name] = self(ctx->directDeclarator(), nullptr);
//     auto funcType = make<FunctionType>();
//     funcType->sub = texp;
//     type->texp = funcType;
//     ret->name = std::move(name);

//     Symtbl localDecls(self);

//     // 函数定义在签名之后就加入符号表，以允许递归调用
//     (*mSymtbl)[ret->name] = ret;

//     ret->body = self(ctx->compoundStatement());

//     return ret;
// }

// functionDefinition
//     :   declarationSpecifiers directDeclarator LeftParen funcParamDeclaration? RightParen (compoundStatement | Semi)
//     ;

// funcParamDeclaration
//     :   declarationSpecifiers initDeclarator (Comma declarationSpecifiers initDeclarator)*
//     ;

FunctionDecl *Ast2Asg::operator()(ast::FunctionDefinitionContext *ctx)
{
    auto ret = make<FunctionDecl>();
    mCurrentFunc = ret;

    auto type = make<Type>();
    ret->type = type;

    auto sq = self(ctx->declarationSpecifiers());
    type->spec = sq.first;
    type->qual = sq.second;

    auto [texp, name] = self(ctx->directDeclarator(), nullptr);
    auto funcType = make<FunctionType>();
    funcType->sub = texp;
    type->texp = funcType;
    ret->name = std::move(name);

    Symtbl localDecls(self);

    // 函数定义在签名之后就加入符号表，以允许递归调用
    (*mSymtbl)[ret->name] = ret;

    // 处理可选的 funcParamDeclaration
    if (auto p = ctx->funcParamDeclaration())
    {
        auto children = p->children;
        for (auto i = 0; i < children.size(); i += 3)
        {
            auto specs = self(dynamic_cast<ast::DeclarationSpecifiersContext *>(children[i]));
            ret->params.push_back(self(dynamic_cast<ast::InitDeclaratorContext *>(children[i + 1]), specs));
        }
        // auto paramList = self(ctx->parameterTypeList(), nullptr);
        // funcType->params.push_back(dynamic_cast<const asg::Type *>(paramList.first));
    }

    if (ctx->compoundStatement())
    {
        ret->body = self(ctx->compoundStatement());
    }

    return ret;
}

Decl *Ast2Asg::operator()(ast::InitDeclaratorContext *ctx, SpecQual sq)
{
    auto [texp, name] = self(ctx->declarator(), nullptr);
    Decl *ret;

    if (auto funcType = texp->dcst<FunctionType>())
    {
        auto fdecl = make<FunctionDecl>();
        auto type = make<Type>();
        fdecl->type = type;

        type->spec = sq.first;
        type->qual = sq.second;
        type->texp = funcType;

        fdecl->name = std::move(name);
        for (auto p : funcType->params)
        {
            auto paramDecl = make<VarDecl>();
            paramDecl->type = p;
            fdecl->params.push_back(paramDecl);
        }

        if (ctx->initializer())
            ABORT();
        fdecl->body = nullptr;

        ret = fdecl;
    }

    else
    {
        auto vdecl = make<VarDecl>();
        auto type = make<Type>();
        vdecl->type = type;

        type->spec = sq.first;
        type->qual = sq.second;
        type->texp = texp;
        vdecl->name = std::move(name);

        if (auto p = ctx->initializer())
            vdecl->init = self(p);
        else
            vdecl->init = nullptr;

        ret = vdecl;
    }

    // 这个实现允许符号重复定义，新定义会取代旧定义
    (*mSymtbl)[ret->name] = ret;
    return ret;
}

} // namespace asg
