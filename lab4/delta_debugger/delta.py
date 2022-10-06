import math

from typing import Tuple

from delta_debugger import run_target

EMPTY_STRING = b""


def delta_debug(target: str, input: bytes) -> bytes:
    """
    Delta-Debugging algorithm

    TODO: Implement your algorithm for delta-debugging.

    Hint: It may be helpful to use an auxilary function that
    takes as input a target, input string, n and
    returns the next input and n to try.

    :param target: target program
    :param input: crashing input to be minimized
    :return: 1-minimal crashing input.
    """

    # Add the base case for empty string
    if (run_target(target, '') == 1):
        return b''

    last_result = input
    length = len(input)

    # ALGORITHM STEP 1
    n = 2

    # LOOP THROUGH ALGORITHM UNTIL N IS GREATER THAN LENGTH
    while n <= length:
        aux_result = aux(target, last_result, n)
        n = aux_result[0]
        last_result = aux_result[1]
        length = len(last_result)

    if (isinstance(last_result, str)):
        last_result = bytes(last_result, encoding='utf-8')

    return last_result


# Aux Function
def aux(target: str, input: bytes, n: int):

    # divide the string into n parts

    string_length = int(math.ceil(len(input) / n))

    leftover = int(len(input) % n)

    parts = []

    for i in range(0, len(input), string_length):
        part = input[i: i + string_length]
        parts.append(part)

    # ALGORITHM STEP 2

    # CHECK DELTAS RIGHT AFTER MAKING THEM AND JUST RETURN THAT DELTA
    for i in range(0, len(parts)):
        if (run_target(target, parts[i]) == 1):
            return (2, parts[i])

    # MAKE NABLAS BY GETTING ALL NABLAS EXCEPT FOR DELTA
    nablas = []
    for i in range(0, len(parts)):
        # get parts and remove the delta
        nabla = parts.copy()
        nabla.pop(i)

        # append all nablas together
        result = ''
        for i in range(0, len(nabla)):
            if (isinstance(nabla[i], str)):
                result += nabla[i]
            else:
                result += nabla[i].decode('utf-8')

        # append nabla to list of nablas
        nablas.append(result)

    # TEST NABLAS TO SEE IF THEY FAIL
    for i in range(0, len(nablas)):
        if (run_target(target, nablas[i]) == 1):
            return (n-1, nablas[i])

    # IF ALL FAILS, RETURN ORIGINAL STRING WITH N*2
    return (n*2, input)
