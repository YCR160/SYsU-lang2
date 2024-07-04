#include "EmitIR.hpp"
#include <llvm/Transforms/Utils/ModuleUtils.h>

#define self (*this)

using namespace asg;

EmitIR::EmitIR(Obj::Mgr &mgr, llvm::LLVMContext &ctx, llvm::StringRef mid)
    : mMgr(mgr), mMod(mid, ctx), mCtx(ctx), mIntTy(llvm::Type::getInt32Ty(ctx)),
      mCurIrb(std::make_unique<llvm::IRBuilder<>>(ctx)),
      mCtorTy(llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), false))
{
}

llvm::Module &EmitIR::operator()(asg::TranslationUnit *tu)
{
    for (auto &&i : tu->decls)
        self(i);
    return mMod;
}

//==============================================================================
// 类型
//==============================================================================

llvm::Type *EmitIR::operator()(const Type *type)
{
    if (type->texp == nullptr)
    {
        switch (type->spec)
        {
        case Type::Spec::kInt:
            return llvm::Type::getInt32Ty(mCtx);
        case Type::Spec::kVoid:
            return llvm::Type::getVoidTy(mCtx);
        case Type::Spec::kChar:
            return llvm::Type::getInt8Ty(mCtx);
        case Type::Spec::kLong:
            return llvm::Type::getInt64Ty(mCtx);
        case Type::Spec::kLongLong:
            return llvm::Type::getInt64Ty(mCtx);
        default:
            ABORT();
        }
    }

    Type subt;
    subt.spec = type->spec;
    subt.qual = type->qual;
    subt.texp = type->texp->sub;

    // 对指针类型、数组类型和函数类型的处理

    if (auto p = type->texp->dcst<PointerType>())
    {
        auto subty = self(&subt);
        return subty->getPointerTo();
    }

    if (auto p = type->texp->dcst<ArrayType>())
    {
        auto subty = self(&subt);
        return llvm::ArrayType::get(subty, p->len);
    }

    if (auto p = type->texp->dcst<FunctionType>())
    {
        std::vector<llvm::Type *> pty;
        // TODO: 在此添加对函数参数类型的处理
        return llvm::FunctionType::get(self(&subt), std::move(pty), false);
    }

    ABORT();
}

//==============================================================================
// 表达式
//==============================================================================

llvm::Value *EmitIR::operator()(Expr *obj)
{
    if (auto p = obj->dcst<IntegerLiteral>())
        return self(p);

    if (auto p = obj->dcst<StringLiteral>())
        return self(p);

    if (auto p = obj->dcst<DeclRefExpr>())
        return self(p);

    if (auto p = obj->dcst<ParenExpr>())
        return self(p);

    if (auto p = obj->dcst<UnaryExpr>())
        return self(p);

    if (auto p = obj->dcst<BinaryExpr>())
        return self(p);

    if (auto p = obj->dcst<CallExpr>())
        return self(p);

    if (auto p = obj->dcst<InitListExpr>())
        return self(p);

    if (auto p = obj->dcst<ImplicitInitExpr>())
        return self(p);

    if (auto p = obj->dcst<ImplicitCastExpr>())
        return self(p);

    ABORT();
}

llvm::Constant *EmitIR::operator()(IntegerLiteral *obj)
{
    return llvm::ConstantInt::get(self(obj->type), obj->val);
}

llvm::Constant *EmitIR::operator()(StringLiteral *obj)
{
    return llvm::ConstantDataArray::getString(mCtx, obj->val);
}

llvm::Value *EmitIR::operator()(DeclRefExpr *obj)
{
    // 在LLVM IR层面，左值体现为返回指向值的指针
    // 在ImplicitCastExpr::kLValueToRValue中发射load指令从而变成右值
    return reinterpret_cast<llvm::Value *>(obj->decl->any);
}

llvm::Value *EmitIR::operator()(ParenExpr *obj)
{
    return self(obj->sub);
}

llvm::Value *EmitIR::operator()(UnaryExpr *obj)
{
    auto sub = self(obj->sub);
    auto &irb = *mCurIrb;

    switch (obj->op)
    {
    case UnaryExpr::kPos:
        // 对于正操作，直接返回子表达式的值
        return sub;
    case UnaryExpr::kNeg:
        // 对于负操作，创建一个取负的指令
        return irb.CreateNeg(sub, "neg");
    case UnaryExpr::kNot:
        // 对于非操作，如果为零则返回1，否则返回0
        return irb.CreateICmpEQ(sub, llvm::ConstantInt::get(sub->getType(), 0));
    default:
        throw std::runtime_error("Invalid unary operation");
    }
}

llvm::Value *EmitIR::operator()(BinaryExpr *obj)
{
    llvm::Value *lftVal, *rhtVal;

    lftVal = self(obj->lft);
    if (obj->op != BinaryExpr::kAnd && obj->op != BinaryExpr::kOr)
        rhtVal = self(obj->rht);

    auto &irb = *mCurIrb;

    switch (obj->op)
    {
    case BinaryExpr::kMul:
        return irb.CreateMul(lftVal, rhtVal);
    case BinaryExpr::kDiv:
        return irb.CreateSDiv(lftVal, rhtVal);
    case BinaryExpr::kMod:
        return irb.CreateSRem(lftVal, rhtVal);
    case BinaryExpr::kAdd:
        return irb.CreateAdd(lftVal, rhtVal);
    case BinaryExpr::kSub:
        return irb.CreateSub(lftVal, rhtVal);
    case BinaryExpr::kGt:
        return irb.CreateICmpSGT(lftVal, rhtVal);
    case BinaryExpr::kLt:
        return irb.CreateICmpSLT(lftVal, rhtVal);
    case BinaryExpr::kGe:
        return irb.CreateICmpSGE(lftVal, rhtVal);
    case BinaryExpr::kLe:
        return irb.CreateICmpSLE(lftVal, rhtVal);
    case BinaryExpr::kEq:
        return irb.CreateICmpEQ(lftVal, rhtVal);
    case BinaryExpr::kNe:
        return irb.CreateICmpNE(lftVal, rhtVal);
    case BinaryExpr::kAnd:
    {
        // 将左操作数转换为 i1 类型
        lftVal = mCurIrb->CreateICmpNE(lftVal, llvm::ConstantInt::get(lftVal->getType(), 0));

        // 创建基本块
        auto lhsBB = llvm::BasicBlock::Create(mCtx, "land.lhs", mCurFunc);
        auto rhsBB = llvm::BasicBlock::Create(mCtx, "land.rhs", mCurFunc);
        auto endBB = llvm::BasicBlock::Create(mCtx, "land.end", mCurFunc);

        // 在 lhsBB 中处理左操作数
        mCurIrb->CreateCondBr(lftVal, rhsBB, endBB);
        mCurIrb = std::make_unique<llvm::IRBuilder<>>(lhsBB);
        mCurIrb->CreateBr(endBB);

        // 在 rhsBB 中处理右操作数
        mCurIrb = std::make_unique<llvm::IRBuilder<>>(rhsBB);
        rhtVal = self(obj->rht);
        rhtVal = mCurIrb->CreateICmpNE(rhtVal, llvm::ConstantInt::get(rhtVal->getType(), 0));
        mCurIrb->CreateBr(endBB);

        // 在 endBB 中创建 phi 节点
        mCurIrb = std::make_unique<llvm::IRBuilder<>>(endBB);
        llvm::PHINode *phi = mCurIrb->CreatePHI(mCurIrb->getInt1Ty(), 2);
        phi->addIncoming(mCurIrb->getInt1(0), lhsBB);
        phi->addIncoming(rhtVal, rhsBB);

        return phi;
    }
    case BinaryExpr::kOr:
    {
        // 将左操作数转换为 i1 类型
        lftVal = mCurIrb->CreateICmpNE(lftVal, llvm::ConstantInt::get(lftVal->getType(), 0));

        // 创建基本块
        auto lhsBB = llvm::BasicBlock::Create(mCtx, "lor.lhs", mCurFunc);
        auto rhsBB = llvm::BasicBlock::Create(mCtx, "lor.rhs", mCurFunc);
        auto endBB = llvm::BasicBlock::Create(mCtx, "lor.end", mCurFunc);

        // 在 lhsBB 中处理左操作数
        mCurIrb->CreateCondBr(lftVal, endBB, rhsBB);
        mCurIrb = std::make_unique<llvm::IRBuilder<>>(lhsBB);
        mCurIrb->CreateBr(endBB);

        // 在 rhsBB 中处理右操作数
        mCurIrb = std::make_unique<llvm::IRBuilder<>>(rhsBB);
        rhtVal = self(obj->rht);
        rhtVal = mCurIrb->CreateICmpNE(rhtVal, llvm::ConstantInt::get(rhtVal->getType(), 0));
        mCurIrb->CreateBr(endBB);

        // 在 endBB 中创建 phi 节点
        mCurIrb = std::make_unique<llvm::IRBuilder<>>(endBB);
        llvm::PHINode *phi = mCurIrb->CreatePHI(mCurIrb->getInt1Ty(), 2);
        phi->addIncoming(mCurIrb->getInt1(1), lhsBB);
        phi->addIncoming(rhtVal, rhsBB);

        return phi;
    }
    case BinaryExpr::kAssign:
        return irb.CreateStore(rhtVal, lftVal);
    case BinaryExpr::kComma:
        return rhtVal;
    case BinaryExpr::kIndex: {
        auto ty = self(obj->lft->type);
        auto idx = irb.CreateSExt(rhtVal, llvm::Type::getInt64Ty(mCtx));
        std::vector<llvm::Value *> idxList{
            idx
        };
        return irb.CreateInBoundsGEP(ty, lftVal, idxList);
    }

    default:
        ABORT();
    }
}

llvm::Value *EmitIR::operator()(CallExpr *obj)
{
    ImplicitCastExpr* implicitCastExpr = dynamic_cast<ImplicitCastExpr*>(obj->head);
    if (implicitCastExpr == nullptr) {
        throw std::runtime_error("Invalid function call");
    }

    DeclRefExpr* declRefExpr = dynamic_cast<DeclRefExpr*>(implicitCastExpr->sub);
    if (declRefExpr == nullptr) {
        throw std::runtime_error("Invalid function call");
    }

    // 获取函数引用
    llvm::Function *func = mMod.getFunction(declRefExpr->decl->name);
    if (func == nullptr) {
        throw std::runtime_error("Function not found");
    }

    // 创建参数列表
    std::vector<llvm::Value *> args;
    for (auto &&arg : obj->args)
        args.push_back(this->operator()(arg));

    // 创建函数调用
    return mCurIrb->CreateCall(func, args);
}

llvm::Value *EmitIR::operator()(InitListExpr *obj)
{
    auto ty = self(obj->type);
    std::vector<llvm::Constant*> initListValues;

    for (auto& initValue : obj->list)
    {
        llvm::Value* value = self(initValue);
        if (llvm::Constant* constant = llvm::dyn_cast<llvm::Constant>(value)) { // IntegerLiteral
            initListValues.push_back(constant);
        } else { // ImplicitCastExpr
            llvm::Value* loadValue = nullptr;
            if (llvm::LoadInst* loadInst = llvm::dyn_cast<llvm::LoadInst>(value)) {
                loadValue = loadInst->getPointerOperand();
            } else {
                loadValue = value;
            }
        }
    }

    return llvm::ConstantArray::get(llvm::cast<llvm::ArrayType>(ty), llvm::ArrayRef<llvm::Constant*>(initListValues));
}

llvm::Value *EmitIR::operator()(ImplicitInitExpr *obj)
{
    auto ty = self(obj->type);
    if (!llvm::isa<llvm::ArrayType>(ty)) {
        ABORT();
    }
    auto arrType = llvm::cast<llvm::ArrayType>(ty);
    auto arr = llvm::ConstantArray::get(arrType, llvm::ArrayRef<llvm::Constant *>({}));
    auto globalVar = new llvm::GlobalVariable(mMod, arr->getType(), true, llvm::GlobalVariable::ExternalLinkage, arr);
    return globalVar;
}

llvm::Value *EmitIR::operator()(ImplicitCastExpr *obj)
{
    auto sub = self(obj->sub);

    auto &irb = *mCurIrb;
    switch (obj->kind)
    {
    case ImplicitCastExpr::kLValueToRValue: {
        // 检查子节点是否为 ArraySubscriptExpr 类型
        BinaryExpr *be = reinterpret_cast<asg::BinaryExpr *>(obj->sub);
        if (be->op == asg::BinaryExpr::kIndex) {
            ImplicitCastExpr *ice = reinterpret_cast<asg::ImplicitCastExpr *>(be->lft);
            // 获取数组的引用和索引
            llvm::ArrayType *arrayType = llvm::ArrayType::get(llvm::Type::getInt32Ty(mCtx), ice->sub->type->texp->dcst<ArrayType>()->len);
            auto indexVal = reinterpret_cast<asg::IntegerLiteral *>(be->rht)->val;

            // 创建索引
            std::vector<llvm::Value *> indexList = {
                mCurIrb->getInt64(0),
                mCurIrb->getInt64(indexVal)
            };

            // 创建数组指针
            llvm::Value *arrayPtr = mCurIrb->CreateInBoundsGEP(arrayType, self(ice->sub), indexList);

            // 创建加载指令
            auto loadVal = irb.CreateLoad(llvm::Type::getInt32Ty(mCtx), arrayPtr);
            return loadVal;
        } else {
            auto ty = self(obj->sub->type);
            auto loadVal = irb.CreateLoad(ty, sub);
            return loadVal;
        }
    }
    case ImplicitCastExpr::kIntegralCast: {
        auto ty = self(obj->type);
        return irb.CreateIntCast(sub, ty, true);
    }
    case ImplicitCastExpr::kArrayToPointerDecay: {
        return sub;
        // 获取数组类型 
        llvm::ArrayType *arrayType = llvm::ArrayType::get(llvm::Type::getInt64Ty(mCtx), obj->sub->type->texp->dcst<ArrayType>()->len);

        // 创建索引
        std::vector<llvm::Value *> indexList;

        // 创建索引值
        // auto tmp = obj->sub->type->texp->dcst<ArrayType>();
        // llvm::Value* value = self(obj->sub->type->texp->dcst<ArrayType>()->dcst<IntegerLiteral>());
        // llvm::Constant* constant = llvm::dyn_cast<llvm::Constant>(value);
        // llvm::Value *index = mCurIrb->getInt64(constant->getUniqueInteger().getLimitedValue());
        llvm::Value *index = mCurIrb->getInt64(1); 
        indexList.push_back(index);

        // 创建数组指针
        llvm::Value *arrayPtr = mCurIrb->CreateInBoundsGEP(arrayType, sub, indexList);

        return arrayPtr;
    }
    case ImplicitCastExpr::kFunctionToPointerDecay: {
        return sub;
    }
    case ImplicitCastExpr::kNoOp: {
        return sub;
    }

    default:
        ABORT();
    }
}

//==============================================================================
// 语句
//==============================================================================

void EmitIR::operator()(Stmt *obj)
{
    if (auto p = obj->dcst<NullStmt>())
        return self(p);

    if (auto p = obj->dcst<DeclStmt>())
        return self(p);

    if (auto p = obj->dcst<ExprStmt>())
        return self(p);

    if (auto p = obj->dcst<CompoundStmt>())
        return self(p);

    if (auto p = obj->dcst<IfStmt>())
        return self(p);

    if (auto p = obj->dcst<WhileStmt>())
        return self(p);

    if (auto p = obj->dcst<DoStmt>())
        return self(p);

    if (auto p = obj->dcst<BreakStmt>())
        return self(p);

    if (auto p = obj->dcst<ContinueStmt>())
        return self(p);

    if (auto p = obj->dcst<ReturnStmt>())
        return self(p);

    ABORT();
}

void EmitIR::operator()(NullStmt *obj)
{
}

void EmitIR::operator()(DeclStmt *obj)
{
    for (auto &&decl : obj->decls)
    {
        auto ty = llvm::Type::getInt32Ty(mCtx); // 直接使用 LLVM 的 int32 类型
        llvm::AllocaInst *a = mCurIrb->CreateAlloca(ty, nullptr, decl->name);
        mCurIrb->CreateStore(llvm::ConstantInt::get(ty, 0), a);

        decl->any = a;

        auto var = decl->dcst<VarDecl>();
        if (var->init != nullptr)
            trans_init(a, var->init);
    }
}

void EmitIR::operator()(ExprStmt *obj)
{
    self(obj->expr);
}

void EmitIR::operator()(CompoundStmt *obj)
{
    // TODO: 可以在此添加对符号重名的处理
    for (auto &&stmt : obj->subs)
        self(stmt);
}

void EmitIR::operator()(IfStmt *obj)
{
    auto condVal = self(obj->cond);
    auto func = mCurFunc;

    auto thenBb = llvm::BasicBlock::Create(mCtx, "if.then", func);
    auto elseBb = llvm::BasicBlock::Create(mCtx, "if.else", func);
    auto endBb = llvm::BasicBlock::Create(mCtx, "if.end", func);

    mCurIrb->CreateCondBr(condVal, thenBb, elseBb);

    mCurIrb = std::make_unique<llvm::IRBuilder<>>(thenBb);
    self(obj->then);
    mCurIrb->CreateBr(endBb);

    mCurIrb = std::make_unique<llvm::IRBuilder<>>(elseBb);
    if (obj->else_)
        self(obj->else_);
    mCurIrb->CreateBr(endBb);

    mCurIrb = std::make_unique<llvm::IRBuilder<>>(endBb);
}

void EmitIR::operator()(WhileStmt *obj)
{
    auto func = mCurFunc;

    auto condBb = llvm::BasicBlock::Create(mCtx, "while.cond", func);
    auto bodyBb = llvm::BasicBlock::Create(mCtx, "while.body", func);
    auto endBb = llvm::BasicBlock::Create(mCtx, "while.end", func);

    mLoopCondBbs[obj] = condBb;
    mLoopEndBbs[obj] = endBb;

    mCurIrb->CreateBr(condBb);

    mCurIrb = std::make_unique<llvm::IRBuilder<>>(condBb);
    auto condVal = self(obj->cond);
    mCurIrb->CreateCondBr(condVal, bodyBb, endBb);

    mCurIrb = std::make_unique<llvm::IRBuilder<>>(bodyBb);
    self(obj->body);
    mCurIrb->CreateBr(condBb);

    mCurIrb = std::make_unique<llvm::IRBuilder<>>(endBb);

    mLoopCondBbs.erase(obj);
    mLoopEndBbs.erase(obj);
}

void EmitIR::operator()(DoStmt *obj)
{
    auto func = mCurFunc;

    auto bodyBb = llvm::BasicBlock::Create(mCtx, "do.body", func);
    auto condBb = llvm::BasicBlock::Create(mCtx, "do.cond", func);
    auto endBb = llvm::BasicBlock::Create(mCtx, "do.end", func);

    mLoopCondBbs[obj] = condBb;
    mLoopEndBbs[obj] = endBb;

    mCurIrb->CreateBr(bodyBb);

    mCurIrb = std::make_unique<llvm::IRBuilder<>>(bodyBb);
    self(obj->body);
    mCurIrb->CreateBr(condBb);

    mCurIrb = std::make_unique<llvm::IRBuilder<>>(condBb);
    auto condVal = self(obj->cond);
    mCurIrb->CreateCondBr(condVal, bodyBb, endBb);

    mCurIrb = std::make_unique<llvm::IRBuilder<>>(endBb);

    mLoopCondBbs.erase(obj);
    mLoopEndBbs.erase(obj);
}

void EmitIR::operator()(BreakStmt *obj)
{
    if (!obj->loop)
        throw std::runtime_error("Invalid break statement");

    // 获取循环的结束基本块
    auto endBb = mLoopEndBbs[obj->loop];

    mCurIrb->CreateBr(endBb);

    auto exitBb = llvm::BasicBlock::Create(mCtx, "break_exit", mCurFunc);
    mCurIrb = std::make_unique<llvm::IRBuilder<>>(exitBb);
}

void EmitIR::operator()(ContinueStmt *obj)
{
    if (!obj->loop)
        throw std::runtime_error("Invalid continue statement");

    // 获取循环的条件检查基本块
    auto condBb = mLoopCondBbs[obj->loop];

    mCurIrb->CreateBr(condBb);

    auto exitBb = llvm::BasicBlock::Create(mCtx, "continue_exit", mCurFunc);
    mCurIrb = std::make_unique<llvm::IRBuilder<>>(exitBb);
}

void EmitIR::operator()(ReturnStmt *obj)
{
    llvm::Value *retVal;
    if (!obj->expr)
        retVal = nullptr;
    else
        retVal = self(obj->expr);

    mCurIrb->CreateRet(retVal);

    auto exitBb = llvm::BasicBlock::Create(mCtx, "return_exit", mCurFunc);
    mCurIrb = std::make_unique<llvm::IRBuilder<>>(exitBb);
}

//==============================================================================
// 声明
//==============================================================================

void EmitIR::operator()(Decl *obj)
{
    if (auto p = obj->dcst<VarDecl>())
        return self(p);

    if (auto p = obj->dcst<FunctionDecl>())
        return self(p);

    ABORT();
}

void EmitIR::trans_init(llvm::Value *val, Expr *obj)
{
    auto &irb = *mCurIrb;

    if (auto p = obj->dcst<IntegerLiteral>())
    {
        auto initVal = llvm::ConstantInt::get(self(p->type), p->val);
        irb.CreateStore(initVal, val);
        return;
    }

    if (auto p = obj->dcst<StringLiteral>())
    {
        auto initVal = self(p);
        irb.CreateStore(initVal, val);
        return;
    }

    if (auto p = obj->dcst<DeclRefExpr>())
    {
        auto initVal = self(p);
        irb.CreateStore(initVal, val);
        return;
    }

    if (auto p = obj->dcst<ParenExpr>())
    {
        auto initVal = self(p);
        irb.CreateStore(initVal, val);
        return;
    }

    if (auto p = obj->dcst<UnaryExpr>())
    {
        auto initVal = self(p);
        irb.CreateStore(initVal, val);
        return;
    }

    if (auto p = obj->dcst<BinaryExpr>())
    {
        auto initVal = self(p);
        irb.CreateStore(initVal, val);
        return;
    }

    if (auto p = obj->dcst<CallExpr>())
    {
        auto initVal = self(p);
        irb.CreateStore(initVal, val);
        return;
    }

    if (auto p = obj->dcst<InitListExpr>())
    {
        auto initVal = self(p);
        irb.CreateStore(initVal, val);
        return;
    }

    if (auto p = obj->dcst<ImplicitInitExpr>())
    {
        auto initVal = self(p);
        irb.CreateStore(initVal, val);
        return;
    }

    if (auto p = obj->dcst<ImplicitCastExpr>())
    {
        auto initVal = self(p);
        irb.CreateStore(initVal, val);
        return;
    }

    ABORT();
}

void EmitIR::operator()(VarDecl *obj)
{
    llvm::Type* intType = llvm::Type::getInt32Ty(mCtx); // 使用 LLVM 的 int32 类型
    llvm::Constant* initVal = llvm::ConstantInt::get(intType, 0); // 默认初始化为 0

    // 创建全局变量
    llvm::GlobalVariable* gvar = new llvm::GlobalVariable(mMod, intType, false, llvm::GlobalVariable::ExternalLinkage, initVal, obj->name);
    obj->any = gvar;

    if (obj->init == nullptr)
        return;

    // 创建构造函数用于初始化
    llvm::Function* ctor = llvm::Function::Create(mCtorTy, llvm::GlobalVariable::PrivateLinkage, "ctor_" + obj->name, mMod);
    llvm::appendToGlobalCtors(mMod, ctor, 0);

    llvm::BasicBlock* entryBb = llvm::BasicBlock::Create(mCtx, "entry", ctor);
    mCurIrb = std::make_unique<llvm::IRBuilder<>>(entryBb);
    trans_init(gvar, obj->init);
    mCurIrb->CreateRet(nullptr);
}

void EmitIR::operator()(FunctionDecl *obj)
{
    // 检查 obj->type 是否为 FunctionType
    auto funcType = llvm::dyn_cast<llvm::FunctionType>(self(obj->type));
    if (funcType == nullptr)
        throw std::runtime_error("Invalid function type");

    // 获取参数类型列表
    std::vector<llvm::Type *> paramTypes;
    for (auto &&param : obj->params)
        paramTypes.push_back(self(param->type));

    // 创建函数类型
    auto llvmFuncType = llvm::FunctionType::get(funcType->getReturnType(), paramTypes, false);

    // 创建函数
    auto func = llvm::Function::Create(llvmFuncType, llvm::GlobalVariable::ExternalLinkage, obj->name, mMod);

    obj->any = func;

    if (obj->body == nullptr)
        return;

    // 创建函数入口基本块
    auto entryBb = llvm::BasicBlock::Create(mCtx, "entry", func);
    mCurIrb = std::make_unique<llvm::IRBuilder<>>(entryBb);
    auto &entryIrb = *mCurIrb;

    // 为函数参数设置名字，并创建局部变量
    auto argIt = func->arg_begin();
    for (auto &&param : obj->params)
    {
        argIt->setName(param->name);
        auto paramAlloca = entryIrb.CreateAlloca(argIt->getType(), 0, param->name + ".addr");
        entryIrb.CreateStore(argIt, paramAlloca);
        param->any = paramAlloca;
        ++argIt;
    }

    // 翻译函数体
    mCurFunc = func;
    self(obj->body);
    auto &exitIrb = *mCurIrb;

    // 创建返回语句
    if (funcType->getReturnType()->isVoidTy())
        exitIrb.CreateRetVoid();
    else
        exitIrb.CreateUnreachable();
}
