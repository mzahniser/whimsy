import {Data} from "./data.js";
import {Interaction} from "./interaction.js";
import {Point} from "./point.js";
import {Sprite} from "./sprite.js";

/** A single "room" that the avatar can explore. The terrain of the room is formed entirely
 * out of sprites, possibly with interaction objects placed among them as well.
 * @typedef {Object} Room
 */
export class Room {
  /** Construct a room from the given data file.
   * @param {Data} data
   */
  constructor(data) {
    if (!data) return;
    
    this._name = data.value();
    while (data.next()) {
      while (data.tag() === "interaction") {
        this._interactions.push(new Interaction(data));
      }
      if (!data.size()) {
        break;
      }
      if (data.tag() === "background") {
        this._background = `rgb(${+data.arg(1)},${+data.arg(2)},${+data.arg(3)})`;
      }
      else {
        this._sprites.push({
          sprite: Sprite.get(+data.arg(0)),
          center: data.point(1),
          name: data.value(2),
        });
      }
    }
  }

  /** Make a copy of this room, which can be modified without changing the original.
   * @returns {Room} A copy of this room.
   */
  clone() {
    const result = new Room();
    // These attributes can be shared, because they are not changed once the
    // object has been initialized.
    result._name = this._name;
    result._background = this._background;
    // In these arrays, it's okay if the objects in the array are shared, but
    // the array itself needs to be copied.
    result._sprites = this._sprites.slice();
    result._interactions = this._interactions.slice();
    return result;
  }

  /** Add the given sprite to this room at the given coordinates.
   * @param {Sprite} sprite
   * @param {Point} center
   * @param {?string} name
   * @returns {number} The index of the newly added sprite.
   */
  addSprite(sprite, center, name = null) {
    const entry = {sprite, center, name};
    const index = this._search(entry);
    this._sprites.splice(index, 0, entry);
    return index;
  }

  /** Add an interaction to this room.
   * @param {Interaction} interaction
   */
  addInteraction(interaction) {
    this._interactions.push(interaction);
  }

  /** Remove the sprite at the given index in the list of sprites. 
   * @param {number} index
   */
  removeSprite(index) {
    this._sprites.splice(index, 1);
  }

  /** Remove all sprites and interactions with the given name.
   * @param {string} name
   */
  remove(name) {
    for (let i = 0; i < this._sprites.length; ) {
      if (this._sprites[i].name === name) this._sprites.splice(i, 1);
      else i++;
    }
    for (let i = 0; i < this._interactions.length; ) {
      if (this._interactions[i].name() === name) this._interactions.splice(i, 1);
      else i++;
    }
  }

  /** Draw the room, with the given room coordinates at the top left corner of the view. 
   * @param {CanvasRenderingContext2D} context The HTML canvas's 2d drawing context.
   * @param {Point} corner The room coordinates to place in the top left.
   */
  draw(context, corner) {
    // First, erase the canvas.
    context.fillStyle = this._background;
    context.fillRect(0, 0, context.canvas.width, context.canvas.height);
    for (const element of this._sprites) {
      element.sprite.draw(context, element.center.sub(corner));
    }
    for (const it of this._interactions) {
      const icon = it.icon();
      if (icon) icon.draw(context, it.iconCenter().sub(corner));
    }
  }

  /** Access the array of sprites in this room. Do not modify this array directly.
   * @returns  {{sprite: Sprite, center: Point, name: string}[]} The array of sprites.
   */
  sprites() {
    return this._sprites;
  }

  /** Access the array of interactions in this room. Do not modify this array directly.
   * @returns {Interaction[]} The array of interactions.
   */
  interactions() {
    return this._interactions;
  }

  _search(entry) {
    // Find the first element in the array that does not sort less than the
    // given entry. Track the possible insertion range, inclusive.
    let start = 0;
    let end = this._sprites.length;

    while (start < end) {
      const mid = (start + end) >> 1;
      const here = this._sprites[mid];
      const less = (entry.sprite.layer() < here.sprite.layer() || (
          !entry.sprite.layer() && !here.sprite.layer() && entry.center.y < here.center.y));
      // If less, I could insert here or anywhere to the left. Otherwise, I
      // could not insert here, but I might insert to the right of here.
      if (less) end = mid;

      else start = mid + 1;
    }
    return start;
  }

  /** @type {string} */
  _name;
  /** @type {string} */
  _background = "rgb(64,64,64)";
  /** @type {{sprite: Sprite, center: Point, name: string}} */
  _sprites = [];
  /** @type {Interaction[]} */
  _interactions = [];
}
