#include "Strategy.h"

/*******************************************************
 * NOTE: You are free to edit this file as you see fit *
 *******************************************************/

/**
 * Strategy to explore various paths of execution.
 *
 * TODO: Implement your search strategy.
 *
 * @param OldVec Vector of Z3 expressions.
 */
void searchStrategy(z3::expr_vector &OldVec)
{
    // first strategy: take last element and negate it
    auto last = OldVec.back();
    OldVec.pop_back();
    OldVec.push_back(!last);
}
