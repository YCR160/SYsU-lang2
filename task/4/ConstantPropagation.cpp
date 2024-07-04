#include "ConstantPropagation.hpp"
#include "map"

using namespace llvm;

PreservedAnalyses ConstantPropagation::run(Module &mod, ModuleAnalysisManager &mam)
{
    int cpTimes = 0;
    std::map<Value *, ConstantInt *> constantMap;

    // 遍历所有全局变量
    for (auto &globalVar : mod.globals())
    {
        // 检查全局变量是否是常量
        if (auto *constVal = dyn_cast<ConstantInt>(globalVar.getInitializer()))
        {
            constantMap[&globalVar] = constVal;
        }
    }

    // 遍历所有函数，找到全局常量
    for (auto &func : mod)
    {
        for (auto &bb : func)
        {
            for (auto &inst : bb)
            {
                if (auto *instPtr = dyn_cast<Instruction>(&inst))
                {
                    if (auto *constInst = dyn_cast<StoreInst>(instPtr))
                    {
                        auto *ptr = constInst->getPointerOperand();
                        if (auto *constInt = dyn_cast<ConstantInt>(constInst->getValueOperand()))
                        {
                            // 将常量赋值到左值
                            if (!constantMap[ptr])
                            {
                                constantMap[ptr] = constInt;
                            }
                            // 如果左值已经有值，且不等于右值，值在执行过程中被修改
                            else if (constantMap[ptr]->getSExtValue() != 114514 && constantMap[ptr] != constInt)
                            {
                                constantMap[ptr] =
                                    llvm::ConstantInt::get(constInst->getContext(), llvm::APInt(32, 114514));
                            }
                        }
                        else
                        {
                            constantMap[ptr] = llvm::ConstantInt::get(constInst->getContext(), llvm::APInt(32, 114514));
                        }
                    }
                }
            }
        }
    }

    for (auto &func : mod)
    {
        std::map<Value *, ConstantInt *> argMap;
        for (auto *user : func.users())
        {
            if (auto *callInst = dyn_cast<CallInst>(user))
            {
                for (unsigned i = 0; i < callInst->arg_size(); i++)
                {
                    Value *arg = callInst->getArgOperand(i);
                    Value *calleeArg = func.getArg(i);
                    if (auto *constArg = dyn_cast<ConstantInt>(arg))
                    {
                        if (argMap[calleeArg] && argMap[calleeArg] != constArg)
                        {
                            argMap[calleeArg] = ConstantInt::get(arg->getContext(), APInt(32, 114514));
                        }
                        else
                        {
                            argMap[calleeArg] = constArg;
                        }
                    }
                    else
                    {
                        argMap[calleeArg] = ConstantInt::get(calleeArg->getContext(), APInt(32, 114514));
                    }
                }
            }
        }
        for (auto &bb : func)
        {
            for (auto &inst : bb)
            {
                for (unsigned i = 0; i < inst.getNumOperands(); i++)
                {
                    if (argMap[inst.getOperand(i)] && argMap[inst.getOperand(i)]->getSExtValue() != 114514)
                    {
                        inst.setOperand(i, argMap[inst.getOperand(i)]);
                        cpTimes++;
                    }
                }

                // constantMap 传播到 Load 指令
                if (auto *instPtr = dyn_cast<Instruction>(&inst))
                {
                    if (auto *constInst = dyn_cast<LoadInst>(instPtr))
                    {
                        auto *ptr = constInst->getPointerOperand();
                        if (constantMap[ptr] && constantMap[ptr]->getSExtValue() != 114514)
                        {
                            instPtr->replaceAllUsesWith(constantMap[ptr]);
                            cpTimes++;
                        }
                    }
                }
            }
        }
    }

    for (auto &func : mod)
    {
        std::set<Value *> unknownGep;
        std::map<std::pair<Value *, std::vector<int>>, ConstantInt *> gepMap;

        for (auto &globalVar : mod.globals())
        {
            if (auto *constVal = dyn_cast<ConstantDataArray>(globalVar.getInitializer()))
            {
                for (unsigned i = 0; i < constVal->getNumElements(); i++)
                {
                    gepMap[std::make_pair(&globalVar, std::vector<int>{(int)0, (int)i})] =
                        ConstantInt::get(Type::getInt32Ty(mod.getContext()), constVal->getElementAsInteger(i));
                }
            }
        }

        for (auto &bb : func)
        {
            for (auto &inst : bb)
            {
                // 遍历指令的操作数
                if (GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(&inst))
                {
                    if (unknownGep.find(gep->getPointerOperand()) != unknownGep.end())
                    {
                        continue;
                    }
                    std::vector<int> idxs;
                    for (int i = 1; i < gep->getNumOperands(); i++)
                    {
                        if (ConstantInt *ci = dyn_cast<ConstantInt>(gep->getOperand(i)))
                        {
                            idxs.push_back(ci->getSExtValue());
                        }
                        else if (SExtInst *sext = dyn_cast<SExtInst>(gep->getOperand(i)))
                        {
                            if (ConstantInt *ci = dyn_cast<ConstantInt>(sext->getOperand(0)))
                            {
                                idxs.push_back(ci->getValue().getSExtValue());
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                    if (idxs.size() == gep->getNumOperands() - 1)
                    {
                        bool hasStore = false;
                        bool hasUnknown = false;
                        int storeNum = 0;
                        for (const auto &user : gep->users())
                        {
                            if (StoreInst *store = dyn_cast<StoreInst>(user))
                            {
                                if (ConstantInt *ci = dyn_cast<ConstantInt>(store->getValueOperand()))
                                {
                                    if (!hasStore)
                                    {
                                        storeNum = ci->getSExtValue();
                                        hasStore = true;
                                    }
                                    else if (storeNum != ci->getSExtValue())
                                    {
                                        hasStore = true;
                                        hasUnknown = true;
                                        break;
                                    }
                                }
                                else
                                {
                                    hasUnknown = true;
                                    hasStore = true;
                                    break;
                                }
                            }
                        }
                        if (!hasUnknown && hasStore)
                        {
                            gepMap[std::make_pair(gep->getPointerOperand(), idxs)] =
                                ConstantInt::get(gep->getContext(), APInt(32, storeNum));
                        }
                        else if (hasUnknown && hasStore)
                        {
                            gepMap[std::make_pair(gep->getPointerOperand(), idxs)] =
                                ConstantInt::get(gep->getContext(), APInt(32, 114514));
                        }
                    }
                    else
                    {
                        unknownGep.insert(gep->getPointerOperand());
                    }
                }
            }
        }

        for (auto &bb : func)
        {
            for (auto &inst : bb)
            {
                // gep
                // load
                if (GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(&inst))
                {
                    if (unknownGep.find(gep) != unknownGep.end())
                    {
                        continue;
                    }
                    std::vector<int> idxs;
                    for (int i = 1; i < gep->getNumOperands(); i++)
                    {
                        if (ConstantInt *ci = dyn_cast<ConstantInt>(gep->getOperand(i)))
                        {
                            idxs.push_back(ci->getSExtValue());
                        }
                        else if (SExtInst *sext = dyn_cast<SExtInst>(gep->getOperand(i)))
                        {
                            if (ConstantInt *ci = dyn_cast<ConstantInt>(sext->getOperand(0)))
                            {
                                idxs.push_back(ci->getValue().getSExtValue());
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                    if (idxs.size() == gep->getNumOperands() - 1)
                    {
                        if (gepMap[std::make_pair(gep->getPointerOperand(), idxs)] &&
                            gepMap[std::make_pair(gep->getPointerOperand(), idxs)]->getSExtValue() != 114514)
                        {
                            for (const auto &user : gep->users())
                            {
                                if (LoadInst *load = dyn_cast<LoadInst>(user))
                                {
                                    load->replaceAllUsesWith(gepMap[std::make_pair(gep->getPointerOperand(), idxs)]);
                                    cpTimes++;
                                }
                            }
                        }
                    }
                }
                // load gep
                else if (auto *load = dyn_cast<LoadInst>(&inst))
                {
                    if (auto *gep = dyn_cast<ConstantExpr>(load->getPointerOperand()))
                    {
                        if (gep->getOpcode() == Instruction::GetElementPtr)
                        {
                            std::vector<int> idxs;
                            for (int i = 1; i < gep->getNumOperands(); i++)
                            {
                                if (ConstantInt *ci = dyn_cast<ConstantInt>(gep->getOperand(i)))
                                {
                                    idxs.push_back(ci->getSExtValue());
                                }
                                else if (SExtInst *sext = dyn_cast<SExtInst>(gep->getOperand(i)))
                                {
                                    if (ConstantInt *ci = dyn_cast<ConstantInt>(sext->getOperand(0)))
                                    {
                                        idxs.push_back(ci->getValue().getSExtValue());
                                    }
                                }
                                else
                                {
                                    break;
                                }
                            }
                            if (idxs.size() == gep->getNumOperands() - 1)
                            {
                                if (gepMap[std::make_pair(gep->getOperand(0), idxs)] &&
                                    gepMap[std::make_pair(gep->getOperand(0), idxs)]->getSExtValue() != 114514)
                                {
                                    load->replaceAllUsesWith(gepMap[std::make_pair(gep->getOperand(0), idxs)]);
                                    cpTimes++;
                                }
                            }
                        }
                    }
                    else if (auto *globalVar = dyn_cast<GlobalVariable>(load->getPointerOperand()))
                    {
                        // 处理全局变量的情况
                        if (gepMap[std::make_pair(globalVar, std::vector<int>{0, 0})] &&
                            gepMap[std::make_pair(globalVar, std::vector<int>{0, 0})]->getSExtValue() != 114514)
                        {
                            load->replaceAllUsesWith(gepMap[std::make_pair(globalVar, std::vector<int>{0, 0})]);
                            cpTimes++;
                        }
                    }
                }
            }
        }
    }

    mOut << "ConstantPropagation running...\n\rPropagated " << cpTimes << " constants\n\r";
    return PreservedAnalyses::all();
}
