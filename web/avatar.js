import {Data} from "./data.js";
import {Point} from "./point.js";
import {Room} from "./room.js";
import {Sprite} from "./sprite.js";

/** The avatar is the character that the player controls.
 * @typedef {Object} Avatar
 */
export class Avatar {
  /** Load the game's definition of the avatar's properties.
   * @param {Data} data
   */
  static load(data) {
    while (data.next() && data.size()) {
      if (data.tag() === "sprite" && data.size() >= 2) {
        Avatar._facings.push({
          sprite: Sprite.get(+data.arg(1)),
          vector: Avatar._vector(+data.arg(2)),
        });
      }
      else if (data.tag() === "speed" && data.size() >= 2) {
        Avatar._speed = +data.arg(1);
      }
    }
  }

  /** Get a reference to the room the avatar is in.
   * @returns {?Room} The current room, or null if the avatar has not been placed.
   */
  room() {
    return this._room;
  }

  /** Get the avatar's current position within the room.
   * @returns {Point} The avatar's (x, y) coordinates in room space.
   */
  position() {
    return this._position;
  }

  /** Get the sprite to be used to draw the avatar given the direction they are facing.
   * @returns {Sprite} The avatar's sprite.
   */
  sprite() {
    return this._sprite;
  }

  /** Get the avatar's speed in world units per frame.
   * @returns {number} Speed in "world units," i.e. low resolution pixels.
   */
  speed() {
    return Avatar._speed;
  }

  /** Move the avatar to the given position and, optionally, to a different room.
   * @param {Point} position
   * @param {Room=} room
   */
  enter(position, room = undefined) {
    if (!this._sprite) {
      this.face(180);
    }
    if (room) {
      this._room = room;
    }
    this._position = position;
  }

  /** Move the avatar to the given position, and update its facing based on the movement direction.
   * @param {Point} position
   */
  move(position) {
    const vector = position.sub(this._position);
    this._position = position;
    this._sprite = Avatar._facing(vector);
  }

  /** Set the avatar's sprite to the one closest to the given facing angle, in "compass" degrees.
   * @param degrees
   */
  face(degrees) {
    this._sprite = Avatar._facing(Avatar._vector(degrees));
  }

  static _facing(vector) {
    let bestDot = -Infinity;
    let best;
    for (const facing of Avatar._facings) {
      const dot = vector.dot(facing.vector);
      if (dot > bestDot) {
        bestDot = dot;
        best = facing.sprite;
      }
    }
    return best;
  }

  static _vector(degrees) {
    const radians = degrees * Math.PI / 180;
    return new Point(Math.sin(radians), -Math.cos(radians));
  }

  /** @type {{sprite: Sprite, vector: Point}} */
  static _facings = [];
  /** @type {number} */
  static _speed = 50;

  // Allocate the position object immediately, so we can call Point.set() on it
  // instead of copying references to points that are handed in.
  /** @type {?Room} */
  _room = null;
  /** @type {Point} */
  _position = new Point();
  /** @type {?Sprite} */
  _sprite = null;
}
