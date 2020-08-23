/** An (x, y) point or vector.
 * @typedef {Object} Point
 * @property {number} x
 * @property {number} y
 */
export class Point {
  /** Construct a point. Once created, points should generally be treated as immutable to 
   * avoid issues where modifying x or y inadvertently modifies other references to the point.
   * @param {number=} x
   * @param {number=} y
   */
  constructor(x = 0, y = 0) {
    this.x = x;
    this.y = y;
  }

  /** Add this point to another point vector.
   * @param {Point} point
   * @returns {Point} this.x + point.x, this.y + point.y
   */
  add(point) {
    return new Point(this.x + point.x, this.y + point.y);
  }

  /** Subtract a point vector from this point.
   * @param {Point} point
   * @returns {Point} this.x - point.x, this.y - point.y
   */
  sub(point) {
    return new Point(this.x - point.x, this.y - point.y);
  }

  /** Multiply this point vector by a scalar.
   * @param {number} scalar
   * @returns {Point} this.x * scalar, this.y * scalar
   */
  mul(scalar) {
    return new Point(this.x * scalar, this.y * scalar);
  }

  /** Divide this point vector by a scalar.
   * @param {number} scalar
   * @returns {Point} this.x / scalar, this.y / scalar
   */
  div(scalar) {
    return new Point(this.x / scalar, this.y / scalar);
  }

  /** Calculate the dot product of two point vectors.
   * @param {Point} point
   * @returns {number} this.x * point.x + this.y * point.y
   */
  dot(point) {
    return this.x * point.x + this.y * point.y;
  }

  /** Calculate the cross product of two point vectors.
   * @param {Point} point
   * @returns {number} this.x * point.y - this.y * point.x
   */
  cross(point) {
    return this.x * point.y - this.y * point.x;
  }

  /** Calculate the length of this point vector.
   * @returns {number} The length of this vector.
   */
  length() {
    return Math.hypot(this.x, this.y);
  }

  /** Calculate the distance between this point and the given point.
   * @param {Point} point
   * @returns {number} The distance to the given point.
   */
  distance(point) {
    return Math.hypot(this.x - point.x, this.y - point.y);
  }

  /** Round this point's coordinate to integer values.
   * @returns {Point} A new point, with integer coordinates.
   */
  round() {
    return new Point(Math.round(this.x), Math.round(this.y));
  }

  // Be sure not to modify the values of these properties directly if it is
  // possible that there are other references to this point.
  x = 0;
  y = 0;
}
