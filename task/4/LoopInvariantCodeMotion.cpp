#include "LoopInvariantCodeMotion.hpp"

using namespace llvm;

void findNoNestedLoops(Loop *loop, std::vector<Loop *> &noNestedLoops)
{
    if (loop->getSubLoops().empty())
    {
        bool hasCall = false;
        for (auto *bb : loop->blocks())
        {
            for (auto &inst : *bb)
            {
                if (auto *callInst = dyn_cast<CallInst>(&inst))
                {
                    hasCall = true;
                    break;
                }
            }
        }
        if (!hasCall)
        {
            noNestedLoops.push_back(loop);
        }
    }
    else
    {
        for (auto *subLoop : loop->getSubLoops())
        {
            findNoNestedLoops(subLoop, noNestedLoops);
        }
    }
}

PreservedAnalyses LoopInvariantCodeMotion::run(llvm::Module &mod, llvm::ModuleAnalysisManager &mam)
{
    int licmTimes = 0;

    // 在FunctionAnalysisManager注册LoopAnalysis
    FunctionAnalysisManager fam;
    PassBuilder pb;
    fam.registerPass([&] { return LoopAnalysis(); });
    pb.registerFunctionAnalyses(fam);
    std::vector<Loop *> noNestedLoops;

    for (auto &func : mod)
    {
        if (func.isDeclaration())
            continue;

        auto &LI = fam.getResult<LoopAnalysis>(func);

        for (auto *loop : LI)
        {
            findNoNestedLoops(loop, noNestedLoops);
        }
    }

    for (auto *loop : noNestedLoops)
    {
        // 随循环改变的是 phi 节点的结果
        std::vector<Instruction *> phiInsts;
        std::vector<Instruction *> loopChangingInsts;

        for (auto *bb : loop->blocks())
        {
            for (auto &inst : *bb)
            {
                if (auto *phiInst = dyn_cast<PHINode>(&inst))
                {
                    phiInsts.push_back(phiInst);
                }
            }
        }
        while (!phiInsts.empty())
        {
            auto *phiInst = phiInsts.back();
            phiInsts.pop_back();
            if (std::find(loopChangingInsts.begin(), loopChangingInsts.end(), phiInst) == loopChangingInsts.end())
            {
                loopChangingInsts.push_back(phiInst);
            }
            for (auto *user : phiInst->users())
            {
                if (auto *inst = dyn_cast<Instruction>(user))
                {
                    if (std::find(phiInsts.begin(), phiInsts.end(), inst) == phiInsts.end() &&
                        std::find(loopChangingInsts.begin(), loopChangingInsts.end(), inst) == loopChangingInsts.end())
                    {
                        phiInsts.push_back(inst);
                        // inst->print(mOut);
                        // mOut << "\n\r";
                    }
                }
            }
        }

        // 提到循环之外的指令
        std::vector<Instruction *> toMove;
        for (auto *bb : loop->blocks())
        {
            for (auto &inst : *bb)
            {
                if (auto *brInst = dyn_cast<BranchInst>(&inst))
                {
                    continue;
                }
                if (std::find(loopChangingInsts.begin(), loopChangingInsts.end(), &inst) == loopChangingInsts.end())
                {
                    toMove.push_back(&inst);
                }
            }
        }

            for (auto *inst : toMove)
            {
                inst->moveBefore(loop->getLoopPreheader()->getTerminator());
                ++licmTimes;
            }
        // // 倒序移动
        // for (auto it = toMove.rbegin(); it != toMove.rend(); ++it)
        // {
        //     (*it)->print(mOut);
        //     mOut << "\n\r";
        //     (*it)->moveBefore(loop->getLoopPreheader()->getTerminator());
        //     ++licmTimes;
        // }
    }

    mOut << "LoopInvariantCodeMotion running...\n\rTo move " << licmTimes << " instructions out of the loop\n\r";
    return PreservedAnalyses::all();
}
