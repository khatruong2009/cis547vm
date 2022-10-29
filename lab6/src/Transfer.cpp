#include "DivZeroAnalysis.h"
#include "Utils.h"

namespace dataflow
{

  /**
   * @brief Is the given instruction a user input?
   *
   * @param Inst The instruction to check.
   * @return true If it is a user input, false otherwise.
   */
  bool isInput(Instruction *Inst)
  {
    if (auto Call = dyn_cast<CallInst>(Inst))
    {
      if (auto Fun = Call->getCalledFunction())
      {
        return (Fun->getName().equals("getchar") ||
                Fun->getName().equals("fgetc"));
      }
    }
    return false;
  }

  /**
   * Evaluate a PHINode to get its Domain.
   *
   * @param Phi PHINode to evaluate
   * @param InMem InMemory of Phi
   * @return Domain of Phi
   */
  Domain *eval(PHINode *Phi, const Memory *InMem)
  {
    if (auto ConstantVal = Phi->hasConstantValue())
    {
      return new Domain(extractFromValue(ConstantVal));
    }

    Domain *Joined = new Domain(Domain::Uninit);

    for (unsigned int i = 0; i < Phi->getNumIncomingValues(); i++)
    {
      auto Dom = getOrExtract(InMem, Phi->getIncomingValue(i));
      Joined = Domain::join(Joined, Dom);
    }
    return Joined;
  }

  /**
   * @brief Evaluate the +, -, * and / BinaryOperator instructions
   * using the Domain of its operands and return the Domain of the result.
   *
   * @param BinOp BinaryOperator to evaluate
   * @param InMem InMemory of BinOp
   * @return Domain of BinOp
   */
  Domain *eval(BinaryOperator *BinOp, const Memory *InMem)
  {
    /**
     * TODO: Write your code here that evaluates +, -, * and /
     * based on the Domains of the operands.
     */

    // return the type of Domain that the result is based on BinOp and memory
    if (BinOp->getOpcode() == Instruction::Add)
    {
      return Domain::add(getOrExtract(InMem, BinOp->getOperand(0)), getOrExtract(InMem, BinOp->getOperand(1)));
    }
    else if (BinOp->getOpcode() == Instruction::Sub)
    {
      return Domain::sub(getOrExtract(InMem, BinOp->getOperand(0)), getOrExtract(InMem, BinOp->getOperand(1)));
    }
    else if (BinOp->getOpcode() == Instruction::Mul)
    {
      return Domain::mul(getOrExtract(InMem, BinOp->getOperand(0)), getOrExtract(InMem, BinOp->getOperand(1)));
    }
    else if (BinOp->getOpcode() == Instruction::SDiv || BinOp->getOpcode() == Instruction::UDiv)
    {
      return Domain::div(getOrExtract(InMem, BinOp->getOperand(0)), getOrExtract(InMem, BinOp->getOperand(1)));
    }
    else
    {
      // return Domain::MaybeZero;
      return new Domain(Domain(Domain::MaybeZero));
    }
  }

  /**
   * @brief Evaluate Cast instructions.
   *
   * @param Cast Cast instruction to evaluate
   * @param InMem InMemory of Instruction
   * @return Domain of Cast
   */
  Domain *eval(CastInst *Cast, const Memory *InMem)
  {
    /**
     * TODO: Write your code here to evaluate Cast instruction.
     */

    auto domain = getOrExtract(InMem, Cast->getOperand(0));
    return domain;
  }

  // do we just write what would happen if we try to cast an int into something else for example?

  /**
   * @brief Evaluate the ==, !=, <, <=, >=, and > Comparision operators using
   * the Domain of its operands to compute the Domain of the result.
   *
   * @param Cmp Comparision instruction to evaluate
   * @param InMem InMemory of Cmp
   * @return Domain of Cmp
   */
  Domain *eval(CmpInst *Cmp, const Memory *InMem)
  {
    /**
     * TODO: Write your code here that evaluates:
     * ==, !=, <, <=, >=, and > based on the Domains of the operands.
     *
     * NOTE: There is a lot of scope for refining this, but you can just return
     * MaybeZero for comparisons other than equality.
     */

    // is equality
    //  is false when equal
    //  is true when equal

    if (Cmp->getPredicate() == CmpInst::ICMP_EQ)
    {
      auto operand1 = getOrExtract(InMem, Cmp->getOperand(0));
      auto operand2 = getOrExtract(InMem, Cmp->getOperand(1));

      if (Cmp->isTrueWhenEqual())
      {
        return new Domain(Domain::NonZero);
      }
      else if (Cmp->isFalseWhenEqual())
      {
        return new Domain(Domain::Zero);
      }
    }
    else if (Cmp->getPredicate() == CmpInst::ICMP_NE)
    {
      auto operand1 = getOrExtract(InMem, Cmp->getOperand(0));
      auto operand2 = getOrExtract(InMem, Cmp->getOperand(1));
      if (Cmp->isFalseWhenEqual())
      {
        return new Domain(Domain::NonZero);
      }
      else if (Cmp->isTrueWhenEqual())
      {
        return new Domain(Domain::Zero);
      }
    }
    return new Domain(Domain::MaybeZero);
  }

  void DivZeroAnalysis::transfer(Instruction *Inst, const Memory *In, Memory &NOut)
  {
    if (isInput(Inst))
    {
      // The instruction is a user controlled input, it can have any value.
      NOut[variable(Inst)] = new Domain(Domain::MaybeZero);
    }
    else if (auto Phi = dyn_cast<PHINode>(Inst))
    {
      // Evaluate PHI node
      NOut[variable(Phi)] = eval(Phi, In);
    }
    else if (auto BinOp = dyn_cast<BinaryOperator>(Inst))
    {
      // Evaluate BinaryOperator
      NOut[variable(BinOp)] = eval(BinOp, In);
    }
    else if (auto Cast = dyn_cast<CastInst>(Inst))
    {
      // Evaluate Cast instruction
      NOut[variable(Cast)] = eval(Cast, In);
    }
    else if (auto Cmp = dyn_cast<CmpInst>(Inst))
    {
      // Evaluate Comparision instruction
      NOut[variable(Cmp)] = eval(Cmp, In);
    }
    else if (auto Alloca = dyn_cast<AllocaInst>(Inst))
    {
      // Used for the next lab, do nothing here.
    }
    else if (auto Store = dyn_cast<StoreInst>(Inst))
    {
      // Used for the next lab, do nothing here.
    }
    else if (auto Load = dyn_cast<LoadInst>(Inst))
    {
      // Used for the next lab, do nothing here.
    }
    else if (auto Branch = dyn_cast<BranchInst>(Inst))
    {
      // Analysis is flow-insensitive, so do nothing here.
    }
    else if (auto Call = dyn_cast<CallInst>(Inst))
    {
      // Analysis is intra-procedural, so do nothing here.
    }
    else if (auto Return = dyn_cast<ReturnInst>(Inst))
    {
      // Analysis is intra-procedural, so do nothing here.
    }
    else
    {
      errs() << "Unhandled instruction: " << *Inst << "\n";
    }
  }

} // namespace dataflow