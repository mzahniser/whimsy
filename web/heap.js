/** A binary heap, for use as a priority queue.
 * @typedef {Object} Heap
 */
export class Heap {
  /** Construct a heap with the given sorting condition: anywhere in the heap, 
   * sort(parent, child) should be true.
   * @param {function(any, any) => boolean} sort
   */
  constructor(sort) {
    this._sort = sort;
  }

  /** Check if the heap is empty.
   * @returns {boolean} True if the heap is empty.
   */
  empty() {
    return !this._elements.length;
  }

  /** Insert an element into the heap.
   * @param {any} element
   */
  insert(element) {
    let index = this._elements.length;
    while (index) {
      const parent = (index - 1) >> 1;
      // Bail out if this is a valid place to insert the element.
      if (this._sort(this._elements[parent], element)) break;
      // Rotate the elements upward in a chain.
      this._elements[index] = this._elements[parent];
      index = parent;
    }
    this._elements[index] = element;
  }

  /** Remove the top element from the heap.
   * @returns {any} The element A for which sort(A, B) is true for every other element B.
   */
  pop() {
    const top = this._elements[0];
    // Pull the last element off the array, and find the right place to insert it.
    const sinker = this._elements.pop();
    const length = this._elements.length;
    // Bail out if that was the last item in the queue.
    if (!length) return top;

    // Find the right place to insert the "sinker" element.
    let index = 0;
    for (let child = 1; child < length; child = child * 2 + 1) {
      // There are actually two children. Check if the other is smaller.
      child += (child + 1 < length && this._sort(this._elements[child + 1], this._elements[child]));
      // Now, check if the smaller child is smaller than the "sinker."
      if (this._sort(sinker, this._elements[child])) break;
      // Rotate the elements in a chain.
      this._elements[index] = this._elements[child];
      index = child;
    }
    this._elements[index] = sinker;

    // Return the previous top element.
    return top;
  }

  /** @type {any[]} */
  _elements = [];
  /** @type {function(any, any) => boolean} */
  _sort;
}
