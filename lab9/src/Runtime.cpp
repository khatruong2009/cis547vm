#include <iostream>

#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"

#include "SymbolicInterpreter.h"

extern SymbolicInterpreter SI;

z3::expr eval(z3::expr &E)
{
  if (E.kind() == Z3_NUMERAL_AST)
  {
    return E;
  }
  else
  {
    MemoryTy Mem = SI.getMemory();
    Address Register(E);
    if (Mem.find(Register) != Mem.end())
    {
      return Mem.at(Register);
    }
    else
    {
      std::cout << "Warning: Cannot find register " << Register << " in memory "
                << std::endl;
      return E;
    }
  }
}

/**
 * @brief Symbolically evaluate Alloca
 *
 * @param R RegisterID
 * @param Ptr Address
 */
extern "C" void __DSE_Alloca__(int R, int *Ptr)
{
  MemoryTy &Mem = SI.getMemory();
  Address Register(R);
  z3::expr SE = SI.getContext().int_val((uintptr_t)Ptr);
  Mem.insert(std::make_pair(Register, SE));
}

/**
 * @brief Symbolically evaluate Store
 *
 * @param X Address
 */
extern "C" void __DSE_Store__(int *X)
{
  // getting symbolic menu
  MemoryTy &Mem = SI.getMemory();
  Address Addr(X);
  // evaluating value on top of the stack and getting an expression back
  z3::expr SE = eval(SI.getStack().top()); // trying to get register ID or constant
  SI.getStack().pop();
  Mem.erase(Addr);
  Mem.insert(std::make_pair(Addr, SE));
}

/**
 * @brief Symbolically evaluate Load
 *
 * TODO: Implement this.
 *
 * @param Y Address of destination
 * @param X Address of Load source
 */
extern "C" void __DSE_Load__(int Y, int *X)
{
  // int y = *x;
  MemoryTy &Mem = SI.getMemory();
  Address Addr(Y);
  Address Addr2(X);
  // iterate through memory and check to make sure Addr2 exists, if not, do nothing
  if (Mem.find(Addr2) != Mem.end())
  {
    // Mem.insert(std::make_pair(Addr, Mem[Addr2]));
    Mem.erase(Addr);
    Mem.insert(std::make_pair(Addr, Mem.at(Addr2)));
  }
}

/**
 * @brief Symbolically evaluate Comparisions
 *
 * TODO: Implement this.
 *
 * @param R Address of Comparision result
 * @param Op Operator Kind
 */
extern "C" void __DSE_ICmp__(int R, int Op)
{
  MemoryTy &Mem = SI.getMemory();
  Address Register(R);

  z3::expr SE2 = eval(SI.getStack().top());
  SI.getStack().pop();
  z3::expr SE = eval(SI.getStack().top());
  SI.getStack().pop();

  if (Op == llvm::CmpInst::ICMP_EQ)
  {
    z3::expr result = (SE == SE2);
    Mem.insert(std::make_pair(Register, result));
  }
  else if (Op == llvm::CmpInst::ICMP_NE)
  {
    z3::expr result = (SE != SE2);
    Mem.insert(std::make_pair(Register, result));
  }
  else if (Op == llvm::CmpInst::ICMP_SGE || Op == llvm::CmpInst::ICMP_UGE)
  {
    z3::expr result = (SE >= SE2);
    Mem.insert(std::make_pair(Register, result));
  }
  else if (Op == llvm::CmpInst::ICMP_SLE || Op == llvm::CmpInst::ICMP_ULE)
  {
    z3::expr result = (SE <= SE2);
    Mem.insert(std::make_pair(Register, result));
  } // still have to finish less than and greater than
  else if (Op == llvm::CmpInst::ICMP_SGT || Op == llvm::CmpInst::ICMP_UGT)
  {
    z3::expr result = (SE > SE2);
    Mem.insert(std::make_pair(Register, result));
  }
  else if (Op == llvm::CmpInst::ICMP_SLT || Op == llvm::CmpInst::ICMP_ULT)
  {
    z3::expr result = (SE < SE2);
    Mem.insert(std::make_pair(Register, result));
  }
}
/**
 * @brief Symbolically evaluate Binary Operation.
 *
 * TODO: Implement this.
 *
 * @param R Address of Binary Operation result.
 * @param Op Operator Kind
 */
extern "C" void __DSE_BinOp__(int R, int Op)
{
  // STILL NEED TO DO
  MemoryTy &Mem = SI.getMemory();
  Address Register(R);

  z3::expr SE2 = eval(SI.getStack().top());
  SI.getStack().pop();
  z3::expr SE = eval(SI.getStack().top());
  SI.getStack().pop();

  if (Op == llvm::BinaryOperator::Add)
  {
    z3::expr result = SE + SE2;
    Mem.insert(std::make_pair(Register, result));
  }
  else if (Op == llvm::BinaryOperator::Sub)
  {
    z3::expr result = SE - SE2;
    Mem.insert(std::make_pair(Register, result));
  }
  else if (Op == llvm::BinaryOperator::Mul)
  {
    z3::expr result = SE * SE2;
    Mem.insert(std::make_pair(Register, result));
  }
  else if (Op == llvm::BinaryOperator::SDiv || Op == llvm::BinaryOperator::UDiv)
  {
    z3::expr result = SE / SE2;
    Mem.insert(std::make_pair(Register, result));
  }
  else
  {
    // error message
  }

  // z3::expr SE = eval(SI.getInputs().find(0));
  // Mem.insert(std::make_pair(Register, SE));
}
