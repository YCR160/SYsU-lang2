#include "FunctionInlining.hpp"

using namespace llvm;

PreservedAnalyses FunctionInlining::run(Module &mod, ModuleAnalysisManager &mam)
{
    int inlineTimes = 0;
    std::map<Function *, std::vector<Instruction *>> smallFuncs;
    std::vector<CallInst *> calls;

    // 遍历所有函数，找到足够小的函数
    for (auto &func : mod)
    {
        if (func.isDeclaration())
            continue;
        std::vector<Instruction *> insts;
        bool hasCall = false;

        for (auto &bb : func)
        {
            for (auto &inst : bb)
            {
                if (auto *callInst = dyn_cast<CallInst>(&inst))
                {
                    hasCall = true;
                    break;
                }
                if (auto *brInst = dyn_cast<BranchInst>(&inst))
                {
                    if (brInst->isConditional())
                    {
                        hasCall = true;
                        break;
                    }
                }
                insts.push_back(&inst);
            }
        }

        // 函数足够小且没有函数调用
        if (insts.size() < 1145141919810 && !hasCall)
        {
            smallFuncs[&func] = insts;
        }
    }

    // 遍历所有函数，找到调用小函数指令
    for (auto &func : mod)
    {
        if (func.isDeclaration())
            continue;
        for (auto &bb : func)
        {
            for (auto &inst : bb)
            {
                if (auto *callInst = dyn_cast<CallInst>(&inst))
                {
                    Function *callee = callInst->getCalledFunction();
                    if (callee && smallFuncs.find(callee) != smallFuncs.end())
                    {
                        calls.push_back(callInst);
                    }
                }
            }
        }
    }

    // 替换函数调用指令
    for (auto *callInst : calls)
    {
        Function *callee = callInst->getCalledFunction();
        llvm::IRBuilder<> builder(callInst->getNextNode());
        std::map<Value *, Value *> paramMap;

        for (unsigned i = 0; i < callInst->arg_size(); ++i)
        {
            paramMap[callee->getArg(i)] = callInst->getArgOperand(i);
        }

        Value *retVal = nullptr;
        bool encounteredError = false;

        // 替换函数调用指令
        for (auto *inst : smallFuncs[callee])
        {
            if (auto *retInst = dyn_cast<ReturnInst>(inst))
            {
                if (!callee->getReturnType()->isVoidTy())
                {
                    retVal = paramMap.find(retInst->getReturnValue()) != paramMap.end()
                                 ? paramMap[retInst->getReturnValue()]
                                 : retInst->getReturnValue();
                }
                continue;
            }

            Value *operand0 = (paramMap.find(inst->getOperand(0)) != paramMap.end()) ? paramMap[inst->getOperand(0)]
                                                                                     : inst->getOperand(0);
            Value *operand1 = inst->getNumOperands() > 1 ? (paramMap.find(inst->getOperand(1)) != paramMap.end())
                                                               ? paramMap[inst->getOperand(1)]
                                                               : inst->getOperand(1)
                                                         : nullptr;

            switch (inst->getOpcode())
            {
            case Instruction::Add:
                paramMap[inst] = builder.CreateAdd(operand0, operand1, "add");
                break;
            case Instruction::Sub:
                paramMap[inst] = builder.CreateSub(operand0, operand1, "sub");
                break;
            case Instruction::Mul:
                paramMap[inst] = builder.CreateMul(operand0, operand1, "mul");
                break;
            case Instruction::SDiv:
                paramMap[inst] = builder.CreateSDiv(operand0, operand1, "sdiv");
                break;
            case Instruction::UDiv:
                paramMap[inst] = builder.CreateUDiv(operand0, operand1, "udiv");
                break;
            case Instruction::SRem:
                paramMap[inst] = builder.CreateSRem(operand0, operand1, "srem");
                break;
            case Instruction::Shl:
                paramMap[inst] = builder.CreateShl(operand0, operand1, "shl");
                break;
            case Instruction::LShr:
                paramMap[inst] = builder.CreateLShr(operand0, operand1, "lshr");
                break;
            case Instruction::AShr:
                paramMap[inst] = builder.CreateAShr(operand0, operand1, "ashr");
                break;
            case Instruction::And:
                paramMap[inst] = builder.CreateAnd(operand0, operand1, "and");
                break;
            case Instruction::Or:
                paramMap[inst] = builder.CreateOr(operand0, operand1, "or");
                break;
            case Instruction::Xor:
                paramMap[inst] = builder.CreateXor(operand0, operand1, "xor");
                break;
            case Instruction::Alloca:
                paramMap[inst] = builder.CreateAlloca(inst->getType(), nullptr, "alloca");
                break;
            case Instruction::Load:
                paramMap[inst] = builder.CreateLoad(Type::getInt32Ty(operand0->getContext()), operand0);
                break;
            case Instruction::Store:
                paramMap[inst] = builder.CreateStore(operand0, operand1);
                break;
            default:
                encounteredError = true;
                break;
            }
        }

        if (!encounteredError)
        {
            if (retVal)
            {
                callInst->replaceAllUsesWith(retVal);
            }
            callInst->eraseFromParent();
            ++inlineTimes;
        }
    }

    mOut << "FunctionInlining running...\n\rInline " << inlineTimes << " functions\n\r";
    return PreservedAnalyses::all();
}
