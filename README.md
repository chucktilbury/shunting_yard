# Shunting Yard

C implementation of the shunting yard expression parser/evaluator as a command-line calculator. The algorithm works by converting an infix expression into a postfix format so that it can be solved using a stack based solver. It's name comes from the way that a train switching yard is arranged. A segment of a train is directed to a side track to allow it to connect to another part of the train based upon the order in which the cars are to be delivered. So, the operators in an expression are re-arranged so that they appear in a queue in the order in which they will be used.

## Operation

This is a command line utility. Expressions can the typed in manually or shell redirection can be used. There are also commands that are accepted by the command line to perform operations that are not directly related to solving an expression. For a list of these commands type ```.h```, ```.help```, or ```?```. This command also provides some simple examples. To quit the command line, simply type ```q```.

This calculator uses named variables as well as literal floating point numbers. Variables must be assigned a value to be used. Variables are retained as long as the program is running, but the actual formulas that are typed in are stored as strings in the command line history.

## Shunting Yard Algorithm.

### The algorithm requires the following data structures
- A stack for operations
- A queue for the output
- A way to deliver the tokens that make up the expression.

### The steps that implement the algorithm
- For all the input tokens:
  - Read the next token
  - If token is an operator (x)
    - While there is an operator (y) at the top of the operators stack and either (x) is left-associative and its precedence is less or equal to that of (y), or (x) is right-associative and its precedence is less than (y)
      - Pop (y) from the stack
      - Add (y) output buffer
    - Push (x) on the stack
  - Else if token is left parenthesis, then push it on the stack
  - Else if token is a right parenthesis
    - Until the top token (from the stack) is left parenthesis, pop from the stack to the output buffer
    - Also pop the left parenthesis but don’t include it in the output buffer
  - Else add token to output buffer
- Pop any remaining operator tokens from the stack to the output

## Solver Algorithm.
The solver is stack based as a Reverse Polish Notation (RPN) or Infix notation.

### Required data structures
- The expression to solve is in a list or array.
- The expression is solved using a stack.

### The algorithm
- While there are items in the list
  - If the current item is a number
    - Push it on the stack.
  - Else if the current item is a binary operator
    - Pop two items
    - Apply the operator
    - Push result on the stack
  - Else if the current item is a unary operator
    - Pop one item
    - Apply the operator
    - Push the result

When the solver is finished, there should be exactly one item on the stack. If there is not, then that means that there was a syntax problem in the expression.

## References
- https://brilliant.org/wiki/shunting-yard-algorithm/
- https://en.wikipedia.org/wiki/Shunting_yard_algorithm
- https://www.geeksforgeeks.org/expression-evaluation/

