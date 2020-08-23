import {Point} from "./point.js";

/** Structured data in whitespace-delimited text format.
 * @typedef {Object} Data
 */
export class Data {
  /** Initialize a Data object from a string or an array of strings. If a single string
   * is given, it will be split into lines wherever a newline ('\n') is found.
   * @constructor
   * @param {string | string[]} text
   */
  constructor(text) {
    this._lines = (text instanceof Array ? text : text.split("\n"));
    this._tokenize();
  }

  /** Advance to the next line of the data file.
   * @returns {boolean} False if the end of the data was reached; otherwise, true.
   */
  next() {
    do {
      ++this._lineIndex;
      if (this._lineIndex >= this._lines.length) return false;

      this._tokenize();
      // Skip comment lines.
    } while(this._tokens.length && this._lines[this._lineIndex][this._tokens[0]] == '#');
    return true;
  }

  /** Get the number of whitespace-delimited tokens in the current line of the data.
   * @returns {number}
   */
  size() {
    return this._tokens.length / 2;
  }

  /** Get the first whitespace-delimited token in this line.
   * @returns {string} The token, or an empty string if this is a blank line.
   */
  tag() {
    return this.arg(0);
  }

  /** Get the given whitespace-delimited token.
   * @param {number} index
   * @returns {string} The token, as a string, with no leading or trailing whitespace.
   */
  arg(index) {
    if (!this.isValid() || this._badIndex(index)) return "";
    
    const start = this._tokens[2 * index];
    const end = this._tokens[2 * index + 1];
    return this._lines[this._lineIndex].slice(start, end);
  }

  /** Get the whole line beginning at the given token index, as a string.
   * @param {number} [index = 1]
   * @returns {string} Te requested portion of the line, with no leading or trailing whitespace.
   */
  value(index = 1) {
    if (this._badIndex(index)) {
      return "";
    }
    const start = this._tokens[2 * index];
    const end = this._tokens[this._tokens.length - 1];
    return this._lines[this._lineIndex].slice(start, end);
  }

  /** Get the given argument, parsed as an "x,y" point.
   * @param {number} index
   * @returns {Point} A point, or (0, 0) if the argument is not in "x,y" form.
   */
  point(index) {
    const coords = this.arg(index).split(',');
    if (coords.length != 2) {
      return new Point();
    }
    return new Point(+coords[0], +coords[1]);
  }

  /** Get the current line, including any leading or trailing whitespace.
   * @returns {string} The current line of the data, in its original format.
   */
  line() {
    return this._lines[this._lineIndex];
  }

  /** Get the indentation of this line, i.e. the number of leading whitespace characters.
   * @returns {number} The number of leading whitespace characters, or 0 if this is a blank line.
   */
  indent() {
    return this._tokens.length ? this._tokens[0] : 0;
  }

  /** Check if this object has reached the end of the data.
   * @returns {boolean} False if the end fo the data has been reached; otherwise, true.
   */
  isValid() {
    return (this._lineIndex < this._lines.length);
  }

  _badIndex(index) {
    return (index < 0 || index * 2 >= this._tokens.length)
  }

  _tokenize() {
    this._tokens = [];
    let wasWhite = true;
    const line = this._lines[this._lineIndex];
    for (let i = 0; i < line.length; ++i) {
      const c = line[i];
      // Handle text files with Windows line endings, which may have a '\r' at the end
      // of the line before the '\n'.
      const isWhite = (c === ' ' || c === '\t' || c === '\r');
      if (isWhite != wasWhite) {
        this._tokens.push(i);
        wasWhite = isWhite;
      }
    }
    if (!wasWhite) this._tokens.push(line.length);
  }

  /** @type {string[]} */
  _lines = [];
  /** @type {number} */
  _lineIndex = 0;
  /** @type {number[]} */
  _tokens = [];
}
