import {Data} from "./data.js";
import {Rect} from "./rect.js";
import {Polygon} from "./polygon.js";
import {Point} from "./point.js";

/** A sprite, i.e. a (possibly animated) image to be drawn on the screen.
 * @typedef {Object} Sprite
 */
export class Sprite {
  /** Set a callback for when all the sprite sheets have been loaded. 
   * @param {function() => void} callback
   */
  static setLoadCallback(callback) {
    Sprite._loadCallback = callback;
  }

  /** Set the index to be assigned to the next sprite that is added.
   * @param {Data} data The data file to read the index from.
   */
  static setIndex(data) {
    Sprite._nextIndex = +data.value();
    data.next();
  }

  /** Begin loading the given sprite sheet, and use it for the next sprites that are added.
   * @param {Data} data The data file to read the sheet definition from.
   */
  static loadSheet(data) {
    let path = data.value();
    while (data.next() && data.size()) {
      if (data.tag() == "web") {
        path = data.value();
      }
    }
    const image = new Image();
    image.onload = Sprite._loaded;
    image.src = path;
    Sprite._sheets.push(image);
  }

  /** Add a new sprite definition.
   * @param {Data} data The data file to read the definition from.
   */
  static loadSprite(data) {
    Sprite._sprites[Sprite._nextIndex] = new Sprite(data);
    ++Sprite._nextIndex;
  }

  /** Get the sprite with the given index.
   * @param {number} index
   * @returns {Sprite}
   */
  static get(index) {
    return Sprite._sprites[index];
  }

  static _loaded() {
    ++Sprite._loadedCount;
    if (Sprite._loadedCount == Sprite._sheets.length) {
      document.getElementById("progress-box").style.display = "none";
      Sprite._loadCallback();
    }
    else {
      const percent = 100 * Sprite._loadedCount / Sprite._sheets.length;
      document.getElementById("progress-bar").style.width = `${percent}%`;
    }
  }

  /** Construct a sprite from the given data.
   * @param {Data} data A data file, currently at a "sprite" tag.
   */
  constructor(data) {
    this._sheet = Sprite._sheets[Sprite._sheets.length - 1];
    let center;
    while (data.next() && data.size()) {
      if (data.tag() == "bounds") {
        const tl = data.point(1);
        const br = data.point(2);
        const bounds = new Rect(tl.x, tl.y, br.x, br.y);
        if (!this._sources.length) {
          // If this is the first bounding rect, set the offset as well.
          this._offset = new Point(-0.5 * bounds.width, -0.5 * bounds.height);
          center = new Point(-0.5 * (tl.x + br.x), -0.5 * (tl.y + br.y));
        }
        this._sources.push(bounds);
      }
      else if (data.tag() == "baseline") {
        center.y = -data.value();
        this._offset.y = this._sources[0].top + center.y;
      }
      else if (data.tag() == "layer") {
        this._layer = +data.value();
      }
      else if (data.tag() == "mask" && center) {
        const points = [];
        for (let i = 1; i < data.size(); i++) {
          const vertex = data.point(i).add(center);
          points.push(vertex.x, vertex.y);
        }
        this._mask.push(points);
      }
    }
    // Always round the offset to a whole number of pixels.
    this._offset = this._offset.round();
  }

  /** Get this sprite's draw layer index. Layers are drawn in order from lowest to highest index,
   * with all sprites on layer 0 additionally sorted by their y coordinates.
   * @returns {number} The index of the sprite's draw layer.
   */
  layer() {
    return this._layer;
  }

  /** Get the sprite's collision mask, if any.
   * @returns {Polygon} An array of collision masks.
   */
  mask() {
    return this._mask;
  }

  /** Draw this sprite, with its center at the given point.
   * @param {CanvasRenderingContext2D} context The HTML canvas's 2d drawing context.
   * @param {Point} point The location of the sprite.
   */
  draw(context, point) {
    if (this._sources && this._sheet) {
      const source = this._sources[Sprite._frame % this._sources.length];
      context.drawImage(this._sheet, source.left, source.top, source.width, source.height,
          point.x + this._offset.x, point.y + this._offset.y, source.width, source.height);
    }
  }

  /** Offset where the sprite's top left corner is drawn. 
   * @returns {number} The (x, y) offset of the top left corner from the sprite's "center."
   */
  cornerOffset() {
    return this._offset;
  }

  /** Width of the sprite, assuming all animation frames have the same size as the first.
   * @returns {number} The width, in pixels.
   */
  width() {
    return (this._sources.length ? this._sources[0].width : 0);
  }

  /** Height of the sprite, assuming all animation frames have the same size as the first.
   * @returns {number} The height, in pixels.
   */
  height() {
    return (this._sources.length ? this._sources[0].height : 0);
  }

  /** Get a string suitable for use in a CSS "background" property, that describes the
   * source of this sprite and its offset within the source.
   * @returns {string} The background property.
   */
  source() {
    return `url(${this._sheet.src}) ${-this._sources[0].left}px ${-this._sources[0].top}px`;
  }

  /** @type {number} */
  static _nextIndex = 1;
  /** @type {Sprite[]} */
  static _sprites = [];
  /** @type {HTMLImageElement[]} */
  static _sheets = [];
  /** @type {number} */
  static _frame = 0;
  /** @type {number} */
  static _loadedCount = 0;
  /** @type {function() => void} */
  static _loadCallback;

  /** @type {HTMLImageElement} */
  _sheet;
  /** @type {Rect[]} */
  _sources = [];
  /** @type {Point} */
  _offset;
  /** @type {number} */
  _layer = 0;
  /** @type {Polygon} */
  _mask = new Polygon();
}
