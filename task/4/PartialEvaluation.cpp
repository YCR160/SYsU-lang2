#include "PartialEvaluation.hpp"

using namespace llvm;

PreservedAnalyses PartialEvaluation::run(Module &mod, ModuleAnalysisManager &mam)
{
    int peTimes = 0;

    for (auto &func : mod)
    {
        // 从变量到变量组成的映射
        std::unordered_map<Value *, std::vector<std::pair<Value *, int>>> valueMap;

        // 将函数的参数加入到valueMap中
        for (auto &arg : func.args())
        {
            std::vector<std::pair<Value *, int>> vec;
            vec.push_back(std::make_pair(&arg, 1));
            valueMap[&arg] = vec;
        }

        for (auto &bb : func)
        {
            for (auto &inst : bb)
            {
                if (inst.isBinaryOp())
                {
                    Value *op1 = inst.getOperand(0);
                    Value *op2 = inst.getOperand(1);
                    if (inst.getOpcode() == Instruction::Add)
                    {
                        // 将op1和op2在valueMap中的组成部分相加
                        std::vector<std::pair<Value *, int>> vec;
                        vec.insert(vec.end(), valueMap[op1].begin(), valueMap[op1].end());
                        for (auto &pair : valueMap[op2])
                        {
                            bool found = false;
                            for (auto &vecPair : vec)
                            {
                                if (vecPair.first == pair.first)
                                {
                                    vecPair.second += pair.second;
                                    found = true;
                                    break;
                                }
                            }
                            if (!found)
                            {
                                vec.push_back(pair);
                            }
                        }
                        valueMap[&inst] = vec;
                    }
                    else if (inst.getOpcode() == Instruction::SDiv)
                    {
                        if (ConstantInt *ci = dyn_cast<ConstantInt>(op2))
                        {
                            bool canDivide = true;
                            for (auto &pair : valueMap[op1])
                            {
                                if (pair.second % ci->getSExtValue() != 0)
                                {
                                    canDivide = false;
                                    break;
                                }
                            }
                            if (canDivide && valueMap[op1].size() > 10)
                            {
                                std::vector<std::pair<Value *, int>> vec;
                                for (auto &pair : valueMap[op1])
                                {
                                    vec.push_back(std::make_pair(pair.first, pair.second / ci->getSExtValue()));
                                }
                                valueMap[&inst] = vec;
                                // 创建一堆乘法和加法指令
                                Value *ans = ConstantInt::get(ci->getType(), 0);
                                for (auto &pair : vec)
                                {
                                    Instruction *mulInst = BinaryOperator::Create(
                                        Instruction::Mul, pair.first, ConstantInt::get(ci->getType(), pair.second),
                                        "mul", &inst);
                                    Instruction *addInst =
                                        BinaryOperator::Create(Instruction::Add, ans, mulInst, "add", &inst);
                                    ans = addInst;
                                }
                                inst.replaceAllUsesWith(ans);
                            }
                        }
                    }
                }
            }
        }
    }

    // 在一个基本块中，对一个全局变量的反复读写折叠
    for (auto &func : mod)
    {
        for (auto &bb : func)
        {
            std::unordered_map<Value *, Value *> valueMap;
            for (auto &inst : bb)
            {
                if (StoreInst *store = dyn_cast<StoreInst>(&inst))
                {
                    valueMap[store->getPointerOperand()] = store->getValueOperand();
                }
                else if (LoadInst *load = dyn_cast<LoadInst>(&inst))
                {
                    if (valueMap.find(load->getPointerOperand()) != valueMap.end())
                    {
                        load->replaceAllUsesWith(valueMap[load->getPointerOperand()]);
                    }
                }
            }
        }
    }
    mOut << "PartialEvaluation running...\n\rTo evaluate " << peTimes << " instructions\n\r";
    return PreservedAnalyses::all();
}
