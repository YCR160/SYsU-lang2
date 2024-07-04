
#include "LoopUnrollOptimization.hpp"

using namespace llvm;

void LoopUnrollOptimization::unrollLoop(Loop *LP)
{
    // 从 header 块中找到 icmp 指令
    int start = 114514, end = 114514, step = 114514;
    for (Instruction &inst : *LP->getHeader())
    {
        if (ICmpInst *icmp = dyn_cast<ICmpInst>(&inst))
        {
            if (ConstantInt *CI = dyn_cast<ConstantInt>(icmp->getOperand(1)))
            {
                end = CI->getZExtValue();
            }
            if (PHINode *phi = dyn_cast<PHINode>(icmp->getOperand(0)))
            {
                if (ConstantInt *CI = dyn_cast<ConstantInt>(phi->getIncomingValue(0)))
                {
                    start = CI->getZExtValue();
                }
                if (Instruction *op = dyn_cast<Instruction>(phi->getIncomingValue(1)))
                {
                    if (BinaryOperator *binOp = dyn_cast<BinaryOperator>(op))
                    {
                        if (binOp->getOpcode() == Instruction::Add || binOp->getOpcode() == Instruction::Sub)
                        {
                            if (ConstantInt *CI = dyn_cast<ConstantInt>(binOp->getOperand(1)))
                            {
                                step = CI->getZExtValue();
                            }
                        }
                    }
                }
            }
            if (start != 114514 && end != 114514 && step != 114514 && end < 80)
            {
                BasicBlock *newBB = BasicBlock::Create(icmp->getContext(), "loop.unroll",
                                                       icmp->getParent()->getParent(), LP->getLoopLatch());
                IRBuilder<> builder(newBB);
                std::map<Value *, Value *> paramMap;
                for (int i = start; i < end; i += step)
                {
                    // header
                    for (Instruction &inst : *LP->getHeader())
                    {
                        if (BranchInst *branch = dyn_cast<BranchInst>(&inst))
                        {
                            continue;
                        }
                        if (PHINode *phi = dyn_cast<PHINode>(&inst))
                        {
                            if (paramMap.find(&inst) != paramMap.end())
                            {
                                paramMap[&inst] = paramMap[phi->getIncomingValue(1)];
                            }
                            else
                            {
                                paramMap[&inst] = phi->getIncomingValue(0);
                            }
                            continue;
                        }
                        Instruction *newInst = inst.clone();
                        for (unsigned i = 0; i < newInst->getNumOperands(); i++)
                        {
                            if (paramMap.find(newInst->getOperand(i)) != paramMap.end())
                            {
                                newInst->setOperand(i, paramMap[newInst->getOperand(i)]);
                            }
                        }
                        builder.Insert(newInst);
                        paramMap[&inst] = newInst;
                    }
                    // latch
                    for (Instruction &inst : *LP->getLoopLatch())
                    {
                        if (BranchInst *branch = dyn_cast<BranchInst>(&inst))
                        {
                            continue;
                        }
                        if (PHINode *phi = dyn_cast<PHINode>(&inst))
                        {
                            if (paramMap.find(&inst) != paramMap.end())
                            {
                                paramMap[&inst] = paramMap[phi->getIncomingValue(1)];
                            }
                            else
                            {
                                paramMap[&inst] = phi->getIncomingValue(0);
                            }
                            continue;
                        }
                        Instruction *newInst = inst.clone();
                        for (unsigned i = 0; i < newInst->getNumOperands(); i++)
                        {
                            if (paramMap.find(newInst->getOperand(i)) != paramMap.end())
                            {
                                newInst->setOperand(i, paramMap[newInst->getOperand(i)]);
                            }
                        }
                        builder.Insert(newInst);
                        paramMap[&inst] = newInst;
                    }
                }
                // header
                for (Instruction &inst : *LP->getHeader())
                {
                    if (BranchInst *branch = dyn_cast<BranchInst>(&inst))
                    {
                        continue;
                    }
                    if (PHINode *phi = dyn_cast<PHINode>(&inst))
                    {
                        if (paramMap.find(&inst) != paramMap.end())
                        {
                            paramMap[&inst] = paramMap[phi->getIncomingValue(1)];
                            continue;
                        }
                        else
                        {
                            paramMap[&inst] = phi->getIncomingValue(0);
                            continue;
                        }
                    }
                    Instruction *newInst = inst.clone();
                    for (unsigned i = 0; i < newInst->getNumOperands(); i++)
                    {
                        if (paramMap.find(newInst->getOperand(i)) != paramMap.end())
                        {
                            newInst->setOperand(i, paramMap[newInst->getOperand(i)]);
                        }
                    }
                    builder.Insert(newInst);
                    paramMap[&inst] = newInst;
                }
                // 跳转替换
                builder.CreateBr(LP->getExitBlock());
                for (Instruction &inst : *LP->getLoopPreheader())
                {
                    if (BranchInst *branch = dyn_cast<BranchInst>(&inst))
                    {
                        for (unsigned i = 0; i < branch->getNumSuccessors(); i++)
                        {
                            if (branch->getSuccessor(i) == LP->getHeader())
                            {
                                branch->setSuccessor(i, newBB);
                            }
                        }
                    }
                }
                // 后续变量使用替换
                for (auto &pair : paramMap)
                {
                    pair.first->replaceAllUsesWith(pair.second);
                }
            }
        }
    }

    for (Loop *subLoop : LP->getSubLoops())
    {
        unrollLoop(subLoop);
    }
}

PreservedAnalyses LoopUnrollOptimization::run(Module &mod, ModuleAnalysisManager &mam)
{
    int unrollTimes = 0;

    // 在FunctionAnalysisManager注册LoopAnalysis
    FunctionAnalysisManager fam;
    PassBuilder pb;
    fam.registerPass([&] { return LoopAnalysis(); });
    pb.registerFunctionAnalyses(fam);

    for (Function &func : mod)
    {
        if (func.isDeclaration())
        {
            continue;
        }

        LoopInfo &LI = fam.getResult<LoopAnalysis>(func);
        for (Loop *LP : LI)
        {
            unrollLoop(LP);
        }
    }

    mOut << "LoopUnrollOptimization running...\n\rUnroll times: " << unrollTimes << "\n\r";
    return PreservedAnalyses::all();
}
