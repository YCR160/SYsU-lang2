#include "CommonSubexpressionElimination.hpp"

using namespace llvm;

PreservedAnalyses CommonSubexpressionElimination::run(Module &mod, ModuleAnalysisManager &mam)
{
    int cseTimes = 0;

    for (auto &func : mod)
    {
        for (auto &bb : func)
        {
            std::map<std::pair<Instruction::BinaryOps, std::pair<Value *, Value *>>, Value *> seenExprs;
            std::map<std::pair<Value *, std::vector<Value *>>, GetElementPtrInst *> seenGEPs;
            for (auto &inst : bb)
            {
                if (auto binOp = dyn_cast<BinaryOperator>(&inst))
                {
                    // 创建一个键，表示这个表达式
                    Value *lhs = binOp->getOperand(0);
                    Value *rhs = binOp->getOperand(1);
                    auto key = std::make_pair(binOp->getOpcode(), std::make_pair(lhs, rhs));
                    auto it = seenExprs.find(key);
                    if (it != seenExprs.end())
                    {
                        binOp->replaceAllUsesWith(it->second);
                        ++cseTimes;
                    }
                    else
                    {
                        seenExprs[key] = binOp;
                    }
                }
                else if (auto *gepInst = dyn_cast<GetElementPtrInst>(&inst))
                {
                    // 创建一个键，表示这个GEP表达式
                    std::vector<Value *> operands(gepInst->op_begin(), gepInst->op_end());
                    auto key = std::make_pair(gepInst->getPointerOperand(), operands);
                    auto it = seenGEPs.find(key);
                    if (it != seenGEPs.end())
                    {
                        gepInst->replaceAllUsesWith(it->second);
                        ++cseTimes;
                    }
                    else
                    {
                        seenGEPs[key] = gepInst;
                    }
                }
            }
        }
    }
    mOut << "CommonSubexpressionElimination running...\n\rTo eliminate " << cseTimes << " instructions\n\r";
    return PreservedAnalyses::all();
}
