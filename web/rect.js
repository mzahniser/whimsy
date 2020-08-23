/** A rectangle, defined by its top left corner and its size.
 * @typedef {Object} Rect
 * @property {number} left The left hand edge of the rectangle.
 * @property {number} top The top edge of the rectangle.
 * @property {number} width The width of the rectangle.
 * @property {number} height The height of the rectangle.
 */
export class Rect {
  /** Construct a rectangle.
   * @param {number} left The left hand edge of the rectangle.
   * @param {number} top The top edge of the rectangle.
   * @param {number} right The right edge of the rectangle.
   * @param {number} bottom The bottom edge of the rectangle.
   */
  constructor(left, top, right, bottom) {
    this.left = +left;
    this.top = +top;
    this.width = +right - this.left;
    this.height = +bottom - this.top;
  }
  left = 0;
  top = 0;
  width = 0;
  height = 0;
}
