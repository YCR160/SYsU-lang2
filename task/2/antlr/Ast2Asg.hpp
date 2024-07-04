#pragma once

#include "SYsUParser.h"
#include "asg.hpp"

namespace asg
{

using ast = SYsUParser;

class Ast2Asg
{
  public:
    Obj::Mgr &mMgr;

    Ast2Asg(Obj::Mgr &mgr) : mMgr(mgr)
    {
    }

    TranslationUnit *operator()(ast::TranslationUnitContext *ctx);

    //============================================================================
    // 类型
    //============================================================================

    using SpecQual = std::pair<Type::Spec, Type::Qual>;

    SpecQual operator()(ast::DeclarationSpecifiersContext *ctx);

    SpecQual operator()(ast::DeclarationSpecifiers2Context *ctx);

    std::pair<TypeExpr *, std::string> operator()(ast::DeclaratorContext *ctx, TypeExpr *sub);

    std::pair<TypeExpr *, std::string> operator()(ast::AbstractDeclaratorContext *ctx, TypeExpr *sub);

    std::pair<TypeExpr *, std::string> operator()(ast::DirectDeclaratorContext *ctx, TypeExpr *sub);

    std::pair<TypeExpr *, std::string> operator()(ast::DirectAbstractDeclaratorContext *ctx, TypeExpr *sub);

    std::pair<TypeExpr *, std::string> operator()(ast::TypeQualifierListContext *ctx, TypeExpr *sub);

    std::pair<TypeExpr *, std::string> operator()(ast::ParameterTypeListContext *ctx, TypeExpr *sub);

    std::pair<TypeExpr *, std::string> operator()(ast::ParameterListContext *ctx, TypeExpr *sub);

    std::pair<TypeExpr *, std::string> operator()(ast::IdentifierListContext *ctx, TypeExpr *sub);

    std::pair<TypeExpr *, std::string> operator()(ast::ParameterDeclarationContext *ctx, TypeExpr *sub);

    //============================================================================
    // 表达式
    //============================================================================

    Expr *operator()(ast::ExpressionContext *ctx);

    Expr *operator()(ast::AssignmentExpressionContext *ctx);

    Expr *operator()(ast::MultiplicativeExpressionContext *ctx);

    Expr *operator()(ast::AdditiveExpressionContext *ctx);

    Expr *operator()(ast::RelationalExpressionContext *ctx);

    Expr *operator()(ast::EqualityExpressionContext *ctx);

    Expr *operator()(ast::LogicalAndExpressionContext *ctx);

    Expr *operator()(ast::LogicalOrExpressionContext *ctx);

    Expr *operator()(ast::CastExpressionContext *ctx);

    Expr *operator()(ast::UnaryExpressionContext *ctx);

    Expr *operator()(ast::PostfixExpressionContext *ctx);

    Expr *operator()(ast::ArgumentExpressionListContext *ctx);

    Expr *operator()(ast::PrimaryExpressionContext *ctx);

    Expr *operator()(ast::InitializerContext *ctx);

    //============================================================================
    // 语句
    //============================================================================

    Stmt *operator()(ast::StatementContext *ctx);

    CompoundStmt *operator()(ast::CompoundStatementContext *ctx);

    Stmt *operator()(ast::ExpressionStatementContext *ctx);

    Stmt *operator()(ast::SelectionStatementContext *ctx);

    Stmt *operator()(ast::IterationStatementContext *ctx);

    Stmt *operator()(ast::ForConditionContext *ctx);

    std::vector<Decl *> operator()(ast::ForDeclarationContext *ctx);

    Expr *operator()(ast::ForExpressionContext *ctx);

    Stmt *operator()(ast::JumpStatementContext *ctx);

    //============================================================================
    // 声明
    //============================================================================

    std::vector<Decl *> operator()(ast::DeclarationContext *ctx);

    FunctionDecl *operator()(ast::FunctionDefinitionContext *ctx);

    Decl *operator()(ast::InitDeclaratorContext *ctx, SpecQual sq);

  private:
    struct Symtbl;
    Symtbl *mSymtbl{nullptr};

    FunctionDecl *mCurrentFunc{nullptr};

    template <typename T, typename... Args> T *make(Args... args)
    {
        return mMgr.make<T>(args...);
    }
};

} // namespace asg
