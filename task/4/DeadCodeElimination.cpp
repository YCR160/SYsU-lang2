#include "DeadCodeElimination.hpp"

using namespace llvm;

PreservedAnalyses DeadCodeElimination::run(Module &mod, ModuleAnalysisManager &mam)
{
    int deadCodeEliminationTimes = 0;
    int lastDeadCodeEliminationTimes = -1;

    while (lastDeadCodeEliminationTimes != deadCodeEliminationTimes)
    {
        lastDeadCodeEliminationTimes = deadCodeEliminationTimes;
        for (auto &func : mod)
        {
            for (auto &bb : func)
            {
                std::vector<Instruction *> instToErase;
                for (auto &inst : bb)
                {
                    if (inst.use_empty() && !isa<StoreInst>(inst) && !isa<CallInst>(inst) && !isa<ReturnInst>(inst) &&
                        !isa<BranchInst>(inst) && !isa<SwitchInst>(inst))
                    {
                        instToErase.push_back(&inst);
                        deadCodeEliminationTimes++;
                    }
                }
                for (auto &i : instToErase)
                    i->eraseFromParent();
            }
        }

        // 如果全局变量只被 store，将其删除
        std::vector<GlobalVariable *> globalsToErase;
        std::vector<Instruction *> instToErase;
        for (auto &global : mod.globals())
        {
            bool onlyStored = true;
            for (const auto &user : global.users())
                if (!isa<StoreInst>(user))
                {
                    onlyStored = false;
                    break;
                }
            if (onlyStored)
            {
                globalsToErase.push_back(&global);
                for (const auto &user : global.users())
                    instToErase.push_back(cast<Instruction>(user));
                deadCodeEliminationTimes++;
            }
        }
        for (auto &g : globalsToErase)
            g->eraseFromParent();
        for (auto &i : instToErase)
            i->eraseFromParent();

        // 指针为常量的GEP指令替换
        // 基址到GEP指令的映射
        std::map<Value *, std::vector<GetElementPtrInst *>> pointerToGEPs;
        for (auto &func : mod)
        {
            for (auto &bb : func)
            {
                for (auto &inst : bb)
                {
                    if (auto *gep = dyn_cast<GetElementPtrInst>(&inst))
                    {
                        pointerToGEPs[gep->getPointerOperand()].push_back(gep);
                    }
                }
            }
        }

        for (auto &pair : pointerToGEPs)
        {
            // 相同基址的所有GEP指令
            auto &geps = pair.second;
            bool allConstant = true;
            for (auto *gep : geps)
            {
                for (auto &idx : gep->indices())
                {
                    if (!isa<ConstantInt>(idx))
                    {
                        allConstant = false;
                        break;
                    }
                }
                if (!allConstant)
                    break;
            }

            // 这个数组的所有索引都是常量
            if (allConstant)
            {
                for (auto &func : mod)
                {
                    for (auto &bb : func)
                    {
                        for (auto &inst : bb)
                        {
                            if (auto *gep = dyn_cast<GetElementPtrInst>(&inst))
                            {
                                if (gep->getPointerOperand() == pair.first)
                                {
                                    int offset = gep->getPointerOperandIndex();
                                    // 遍历pair.second中的GEP指令
                                    for (auto *gep2 : geps)
                                    {
                                        if (gep == gep2)
                                            continue;
                                        // gep的使用者有且只有一个store指令
                                        StoreInst *gepStore = nullptr;
                                        int gepStoreCount = 0;
                                        for (auto *user : gep->users())
                                        {
                                            if (auto *store = dyn_cast<StoreInst>(user))
                                            {
                                                gepStore = store;
                                                gepStoreCount++;
                                            }
                                        }
                                        // gep2的使用者有且只有一个load指令
                                        LoadInst *gep2Load = nullptr;
                                        int gep2LoadCount = 0;
                                        for (auto *user : gep2->users())
                                        {
                                            if (auto *load = dyn_cast<LoadInst>(user))
                                            {
                                                gep2Load = load;
                                                gep2LoadCount++;
                                            }
                                        }
                                        // 将gep2之后load的使用替换为gep之后store的值
                                        if (gepStore && gepStoreCount == 1 && gep2Load && gep2LoadCount == 1)
                                        {
                                            gep2Load->replaceAllUsesWith(gepStore->getValueOperand());
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // 函数中只被 store 的数组删除
        // 函数参数和调用不优化
        std::vector<Value *> arraysToErase;
        std::vector<Value *> arraysToKeep;
        for (auto &func : mod)
        {
            // 函数参数不动
            for (auto &arg : func.args())
            {
                arraysToKeep.push_back(&arg);
            }
            for (auto &bb : func)
            {
                for (auto &inst : bb)
                {
                    if (auto *gep = dyn_cast<GetElementPtrInst>(&inst))
                    {
                        // 如果在 arraysToKeep 中，不再处理
                        if (std::find(arraysToKeep.begin(), arraysToKeep.end(), gep) != arraysToKeep.end())
                            continue;
                        // 如果不在 arraysToErase 中，加入
                        if (std::find(arraysToErase.begin(), arraysToErase.end(), gep) == arraysToErase.end())
                        {
                            if (auto *array = dyn_cast<AllocaInst>(gep->getPointerOperand()))
                            {
                                arraysToErase.push_back(gep->getPointerOperand());
                            }
                        }
                        // 查找 gep 的使用者
                        for (auto *user : gep->users())
                        {
                            if (!isa<StoreInst>(user))
                            {
                                arraysToKeep.push_back(gep->getPointerOperand());
                                break;
                            }
                        }
                    }
                    else if (auto *call = dyn_cast<CallInst>(&inst))
                    {
                        for (auto &arg : call->args())
                        {
                            arraysToKeep.push_back(arg);
                        }
                    }
                }
            }
        }
        for (auto &array : arraysToErase)
        {
            if (std::find(arraysToKeep.begin(), arraysToKeep.end(), array) == arraysToKeep.end())
            {
                for (auto &func : mod)
                {
                    for (auto &bb : func)
                    {
                        std::vector<Instruction *> instToErase;
                        for (auto &inst : bb)
                        {
                            if (auto *gep = dyn_cast<GetElementPtrInst>(&inst))
                            {
                                if (gep->getPointerOperand() == array)
                                {
                                    instToErase.push_back(gep);
                                    for (auto *user : gep->users())
                                    {
                                        instToErase.push_back(cast<Instruction>(user));
                                        deadCodeEliminationTimes++;
                                    }
                                    deadCodeEliminationTimes++;
                                }
                            }
                        }
                        for (auto &i : instToErase)
                            i->eraseFromParent();
                    }
                }
            }
        }

        std::vector<Function *> funcsToErase;
        for (auto &func : mod)
        {
            if (func.getName() != "main" && func.use_empty())
            {
                funcsToErase.push_back(&func);
                deadCodeEliminationTimes++;
            }
        }
        for (auto &f : funcsToErase)
            f->eraseFromParent();

        // 函数内未被使用的参数删除
        for (auto &func : mod)
        {
            if (func.isDeclaration())
                continue;
            std::vector<int> argsToErase;
            for (auto &arg : func.args())
            {
                if (arg.use_empty())
                {
                    argsToErase.push_back(arg.getArgNo());
                }
            }
            for (auto *user : func.users())
            {
                if (auto *callInst = dyn_cast<CallInst>(user))
                {
                    for (auto i : argsToErase)
                    {
                        callInst->setArgOperand(i, UndefValue::get(func.getFunctionType()->getParamType(i)));
                    }
                }
            }
        }

        for (auto &func : mod)
        {
            for (auto &bb : func)
            {
                // 只有一个跳转指令的基本块
                if (auto *br = dyn_cast<BranchInst>(&bb.front()))
                {
                    auto *pred = bb.getSinglePredecessor();
                    auto *succ = bb.getSingleSuccessor();
                    if (pred && succ)
                    {
                        // 检查后继是否存在 phi 指令
                        bool hasPhi = false;
                        for (auto &inst : *succ)
                        {
                            if (isa<PHINode>(&inst))
                            {
                                hasPhi = true;
                                break;
                            }
                        }

                        // 后继不存在 phi 指令才进行优化
                        if (!hasPhi)
                        {
                            // 将前驱基本块的跳转目标改为当前基本块的跳转目标
                            auto *predBr = cast<BranchInst>(pred->getTerminator());
                            for (int i = 0; i < predBr->getNumSuccessors(); ++i)
                            {
                                if (predBr->getSuccessor(i) == &bb)
                                {
                                    predBr->setSuccessor(i, succ);
                                    deadCodeEliminationTimes++;
                                }
                            }
                        }
                    }
                }
            }
        }

        // 删除不是入口块，且没有前驱块的基本块
        std::vector<BasicBlock *> bbsToErase;
        for (auto &func : mod)
        {
            for (auto &bb : func)
            {
                if (bb.getName() == "entry")
                    continue;
                if (pred_begin(&bb) == pred_end(&bb))
                {
                    bbsToErase.push_back(&bb);
                    deadCodeEliminationTimes++;
                }
            }
        }
        for (auto &bb : bbsToErase)
            bb->eraseFromParent();
    }

    mOut << "DeadCodeElimination running...\n\rTo eliminate " << deadCodeEliminationTimes << " instructions\n\r";
    return PreservedAnalyses::all();
}
