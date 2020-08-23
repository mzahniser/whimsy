/** A static class for tracking variables defined and modified by game events.
 * @typedef {Object} Variables
 */
export class Variables {
  /** Evaluate an expression. (Note: it is assumed that this is the contents of
   * an "if", so it cannot assign new values to any variables.)
   * @param {string} line The expression to evaluate.
   * @returns {number} The value of the expression.
   */
	static eval(line) {
    let i = 0;
    let length = line.length;
    
    // Output stack.
    const values = [];
    // Operator stack.
    const ops = [];
    
    let wasOp = true;
    while (i != length) {
      // Find the next token. Begin by skipping whitespace.
      if (isSpace(line[i])) {
        i++;
        continue;
      }
      
      if (line[i] == '(') {
        ops.push(PAREN_OP);
        // The first thing inside the parentheses will either be a value or
        // a unary operator, never a binary operator.
        wasOp = true;
        i++;
      }
      else if (line[i] == ')') {
        // Apply all operators up until the most recent open parentheses.
        while (true) {
          if (!ops.length) {
            console.log("Mismatched parentheses: " + line);
            return;
          }
          if (ops[ops.length - 1] == PAREN_OP)
            break;
          apply(values, ops);
        }
        // Discard the parenthesis operator.
        ops.pop();
        // An ending parenthesis causes the expression inside to be
        // evaluated, so it counts as a value, not an operator.
        wasOp = false;
        i++;
      }
      else if (isOpChar(line[i])) {
        // If the previous token was an operator, this token can only be a
        // unary operator.
        const op = findOp(line, i, wasOp ? UNARY_OPS : BINARY_OPS);
        if (!op) {
          console.log("Invalid expression: " + line);
          return;
        }
        // Apply any operators in the stack whose precedence is greater than
        // this one. If this operator is binary, also apply operators that
        // have the same precedence as it.
        while (ops.length) {
          const back = ops[ops.length - 1];
          if (back.precedence < op.precedence) break;
          if (back.precedence == op.precedence && op.args == 1) break;
          apply(values, ops);
        }
        // Add this operator to the stack.
        ops.push(op);
        wasOp = true;
        // Skip to the end of the operator.
        i += op.token.length;
      }
      else {
        // Find the end of this value by scanning forward to the next op
        // char, remembering the last non-whitespace character encountered.
        // The current char is not a space, so the token is not empty.
        const first = i;
        let last = i + 1;
        for ( ; i != length && !isOpChar(line[i]); i++) {
          if (!isSpace(line[i])) last = i + 1;
        }
        // Add the value of this token to the stack.
        values.push(Variables._value(line.slice(first, last)));
        // Remember that the previous token was not an operator.
        wasOp = false;
        // The iterator can remain where it is, because it already points to
        // the beginning of the next token we want to process.
      }
    }
    while (ops.length)
      apply(values, ops);
    return values.pop();  
  }

  /** Evaluate a "set" command, changing the value of a single variable. If no
   * assignment operator is found, an implied " = 1" is appended.
   * @param {string} line The line, which must be a single token or an assignment.
   */
	static set(line) {
    // The line will be retrieved via Data.value(), so it is guaranteed not to
    // have any whitespace at the beginning or end. But, it may be empty.
    if (!line.length) return;

    // Find the first value and the first operator.
    let i = 0;
    const length = line.length;
    const first = 0;
    let last = 0;
    for ( ; i != length && !isOpChar(line[i]); i++) {
      if (!isSpace(line[i])) last = i + 1;
    }
    if (first == last) {
      console.log("Set expression begins with operator: " + line);
      return;
    }
    // Get the token before the operator (if any), with spaces trimmed.
    const token = line.slice(first, last);
    // If no operator was found, this whole expression is just setting a
    // variable's value to "true," i.e. 1.
    if (i == length) Variables._variables[token] = 1;
    else {
      // Figure out what operator we're applying, and apply it.
      const op = findOp(line, i, ASSIGN_OPS);
      if (!op) {
        console.log("Missing assignment operator: " + line);
        return;
      }
      const value = Variables.eval(line.slice(i + op.token.length));
      Variables._variables[token] = op.fun(Variables._variables[token], value);
    }
  }

	/** Clear all variable definitions. */
  static clear() {
    Variables._variables = {};
  }

  static _value(token) {
    const number = Number(token);
    if (!isNaN(number)) return number;
    if (token in Variables._variables) return Variables._variables[token];
    return 0;
  }
  
  /** @type {Object.<string, number>} */
  static _variables = {};
}


class Op {
  constructor(token, precedence, args, fun) {
    this.token = token;
    this.precedence = precedence;
    this.args = args;
    this.fun = fun;
  }
}
// Operators may only contain these characters, and variable names may not
// use these characters.
const OP_CHARS = "()!*/%+-<=>&|^?";
// Operators, listed in greedy parsing order. That is, if parsing a group of
// operators you can go with the first match found here. Binary operators
// should be matched any time the previous token was a variable or value:
const BINARY_OPS = [
  new Op("**", 8, 2, (a, b) => Math.pow(a, b)),
  new Op("<=", 4, 2, (a, b) => a <= b),
  new Op(">=", 4, 2, (a, b) => a >= b),
  new Op("==", 3, 2, (a, b) => a == b),
  new Op("!=", 3, 2, (a, b) => a != b),
  new Op("&&", 2, 2, (a, b) => a && b),
  new Op("||", 1, 2, (a, b) => a || b),
  new Op( "*", 6, 2, (a, b) => a * b),
  new Op( "/", 6, 2, (a, b) => a / b),
  new Op( "%", 6, 2, (a, b) => a % b),
  new Op( "+", 5, 2, (a, b) => a + b),
  new Op( "-", 5, 2, (a, b) => a - b),
  new Op( "<", 4, 2, (a, b) => a < b),
  new Op( ">", 4, 2, (a, b) => a > b),
];
// Unary operators should be matched whenever the previous token was not a
// variable or value. (This includes the very beginning of the expression.)
const UNARY_OPS = [
  new Op( "!", 7, 1, (a, b) => !b),
  new Op( "-", 7, 1, (a, b) => -b),
];
// Object representing an open parenthesis. Mark it with an empty function.
// Its precedence is 0, i.e. it stays in the list until a right paren (which
// conceptually has precedence 9) completes it.
const PAREN_OP = new Op("(", 0, 1, null);
// Assignment operators. The variable's current value will be passed in a.
const ASSIGN_OPS = [
  new Op("+=", 0, 2, (a, b) => a + b),
  new Op("-=", 0, 2, (a, b) => a - b),
  new Op("*=", 0, 2, (a, b) => a * b),
  new Op("/=", 0, 2, (a, b) => a / b),
  new Op("%=", 0, 2, (a, b) => a % b),
  new Op( "=", 0, 2, (a, b) => b),
];

// Pop the top operator of the given operator stack and apply it to the top
// value(s) of the given value stack.
function apply(values, ops) {
  const op = ops.pop();
  const b = values.pop();
  const a = (op.args == 2 ? values.pop() : 0);
  values.push(op.fun(a, b));
}

// Check if the given character is reserved for operators.
function isOpChar(char) {
  return OP_CHARS.includes(char);
}

// Check if the given character is whitespace.
function isSpace(char) {
  return " \t\r".includes(char);
}

// Find the first operator in the given set that matches the given string.
function findOp(line, i, ops) {
  for (const op of ops) {
    if (line[i] === op.token[0]) {
      if (op.token.length === 1) return op;
      if (line.length > i + 1 && line[i + 1] === op.token[1]) return op;
    }
  }
  return null;
}
