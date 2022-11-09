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
    auto DomainOP0 = getOrExtract(InMem, BinOp->getOperand(0));
    auto DomainOP1 = getOrExtract(InMem, BinOp->getOperand(1));
    switch (BinOp->getOpcode())
    {
    case Instruction::Add:
      return Domain::add(DomainOP0, DomainOP1);
    case Instruction::Sub:
      return Domain::sub(DomainOP0, DomainOP1);
    case Instruction::Mul:
      return Domain::mul(DomainOP0, DomainOP1);
    case Instruction::SDiv:
    case Instruction::UDiv:
      return Domain::div(DomainOP0, DomainOP1);
    default:
      errs() << "Not handled instrunction: " << *BinOp << "\n";
      assert(false);
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
    return getOrExtract(InMem, Cast->getOperand(0));
  }

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
    auto *DomainOP0 = getOrExtract(InMem, Cmp->getOperand(0));
    auto *DomainOP1 = getOrExtract(InMem, Cmp->getOperand(1));

    if (!Cmp->isEquality())
      return new Domain(Domain::MaybeZero);

    if (DomainOP0->Value == Domain::Uninit || DomainOP1->Value == Domain::Uninit)
      return new Domain(Domain::Uninit);
    if (DomainOP0->Value == Domain::Zero && DomainOP1->Value == Domain::Zero &&
            Cmp->isTrueWhenEqual() ||
        DomainOP0->Value == Domain::NonZero && DomainOP1->Value == Domain::Zero &&
            Cmp->isFalseWhenEqual() ||
        DomainOP1->Value == Domain::NonZero && DomainOP0->Value == Domain::Zero &&
            Cmp->isFalseWhenEqual())
      return new Domain(Domain::NonZero);
    if (DomainOP0->Value == Domain::Zero && DomainOP1->Value == Domain::Zero &&
            Cmp->isFalseWhenEqual() ||
        DomainOP0->Value == Domain::NonZero && DomainOP1->Value == Domain::Zero &&
            Cmp->isTrueWhenEqual() ||
        DomainOP1->Value == Domain::NonZero && DomainOP0->Value == Domain::Zero &&
            Cmp->isTrueWhenEqual())
      return new Domain(Domain::Zero);

    return new Domain(Domain::MaybeZero);
  }

  void DivZeroAnalysis::transfer(Instruction *Inst, const Memory *In, Memory &NOut, PointerAnalysis *PA, SetVector<Value *> PointerSet)
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
      /**
       * TODO: Store instruction can either add new variables or overwrite existing variables into memory maps.
       * To update the memory map, we rely on the points-to graph constructed in PointerAnalysis.
       *
       * To build the abstract memory map, you need to ensure all pointer references are in-sync, and
       * will converge upon a precise abstract value. To achieve this, implement the following workflow:
       *
       * Iterate through the provided PointerSet:
       *   - If there is a may-alias (i.e., `alias()` returns true) between two variables:
       *     + Get the abstract values of each variable.
       *     + Join the abstract values using Domain::join().
       *     + Update the memory map for the current assignment with the joined abstract value.
       *     + Update the memory map for all may-alias assignments with the joined abstract value.
       * */

      // use alias function in PointerAnalysis.cpp to see if there is a possible may-alias
      if (Store->getOperand(0)->getType()->isIntegerTy())
      {
        auto domain1 = getOrExtract(In, Store->getOperand(0));

        SetVector<Value *>::iterator itr;

        // iterate through the pointer set
        for (itr = PointerSet.begin(); itr != PointerSet.end(); ++itr)
        {
          // use the alias function to compare current pointer to the pointer of getOperand(1)
          auto pointer1 = variable(Store->getOperand(1));
          // *** not sure how to get the current iteration of PointerSet
          auto pointer2 = variable(*itr);
          // if it is an alias, then we use join function to join (assuming it's not the same pointer instruction)
          if (PA->alias(pointer1, pointer2))
          {
            if (pointer1 != pointer2)
            {
              domain1 = Domain::join(domain1, getOrExtract(In, *itr));
            }
            // ** not sure how to access and change domain value
          }
        }

        // interate through pointer set again to assign group domain to all of the aliases
        for (itr = PointerSet.begin(); itr != PointerSet.end(); ++itr)
        {

          auto pointer1 = variable(Store->getOperand(1));
          auto pointer2 = variable(*itr);

          if (PA->alias(pointer1, pointer2))
          {
            NOut[variable(*itr)] = domain1;
          }
        }
      }

      // get domain for value operand and for the rest of the aliases
      // get the group operand and assign it for the value operand and all of the other aliases
      // (make sure pointer operand only gets counted once for group operand)

      // if yes,
      // get Domain values of both variables and join them using Domain::join() functaion
      // update NOUT to be the Domain value returned from join
    }
    else if (auto Load = dyn_cast<LoadInst>(Inst))
    {
      /**
       * TODO: Rely on the existing variables defined within the `In` memory to
       * know what abstract domain should be for the new variable
       * introduced by a load instruction.
       *
       * If the memory map already contains the variable, propagate the existing
       * abstract value to NOut.
       * Otherwise, initialize the memory map for it.
       *
       * Hint: You may use getPointerOperand().
       */

      // check operand(0) for integer type
      if (Load->getType()->isIntegerTy())
      {
        // get domain value for pointer operand and join it with value of the first operand
        auto domain1 = getOrExtract(In, getPointerOperand(Load));

        NOut[variable(Load)] = domain1;
      }
    }
    else if (auto Branch = dyn_cast<BranchInst>(Inst))
    {
      // Analysis is flow-insensitive, so do nothing here.
    }
    else if (auto Call = dyn_cast<CallInst>(Inst))
    {
      /**
       * TODO: Populate the NOut with an appropriate abstract domain.
       *
       * You only need to consider calls with int return type.
       */

      // check for integer return type for the call
      if (Call->getType()->isIntegerTy())
      {
        // assign the domain to MaybeZero since we don't know what the domain will be since we're just getting a function
        NOut[variable(Call)] = new Domain(Domain::MaybeZero);
      }
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