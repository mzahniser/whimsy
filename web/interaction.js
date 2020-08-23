import {Avatar} from "./avatar.js";
import {Data} from "./data.js";
import {Sprite} from "./sprite.js";
import {Point} from "./point.js";
import {Room} from "./room.js";

const INACTIVE = -1;
const VISIBLE = 0;
const ACTIVE = 1;
const HOVER = 2;

/** A location in a room where the avatar can trigger some sort of interaction.
 * @typedef {Object} Interaction
 */
export class Interaction {
  /** Read an interaction definition from the given data.
   * @param {Data} data
   */
  constructor(data) {
    this._name = data.value();
    // Unlike most data objects, an "interaction" may be embedded inside a room definition.
    // So, it doesn't need to be terminated by a newline; parsing will also terminate if a
    // line is encountered that is not a valid interaction attribute.
    while (data.next() && data.size()) {
      if (data.tag() === "position") {
        this._position = data.point(1);
      }
      else if (data.tag() === "offset") {
        this._iconCenter = data.point(1);
      }
      else if (data.tag() === "visible") {
        this._radius[VISIBLE] = data.point(1);
        if (data.size() >= 3) this._icon[VISIBLE] = Sprite.get(+data.arg(2));
      }
      else if (data.tag() === "active") {
        this._radius[ACTIVE] = data.point(1);
        if (data.size() >= 3) this._icon[ACTIVE] = Sprite.get(+data.arg(2));
        if (data.size() >= 4) this._icon[HOVER] = Sprite.get(+data.arg(3));
      }
      else if (data.tag() === "enter") {
        this._enterPosition = data.point(1);
        this._enterRoom = data.arg(2);
      }
      else if (data.tag() === "dialog") {
        this._dialog = data.value();
      }
      else break;
    }
    if (this._position) this._iconCenter = this._iconCenter.add(this._position);
  }

  /** Get the name of this interaction.
   * @returns {string} The interaction's name, or an empty string if no name is set.
   */
  name() {
    return this._name;
  }

  /** Set the state of the intersection. Return true if it must trigger immediately.
   * @param {Avatar} avatar
   * @returns {boolean} True if this interaction triggers immediately due to avatar movement.
   */
  setState(avatar) {
    let offset = avatar.sub(this._position);
    if (inRange(offset, this._radius[ACTIVE])) {
      if (this._state !== ACTIVE) {
        this._state = ACTIVE;
        if (!this._icon[ACTIVE]) return true;
      }
    }
    else if (this._icon[VISIBLE] && inRange(offset, this._radius[VISIBLE])) {
      this._state = VISIBLE;
    }
    else this._state = INACTIVE;
    // Return false unless we triggered an immediate action, above.
    return false;
  }

  /** Update this icon's hover state. Return true if its visible state changes.
   * @param {Point} mouse The mouse position in room coordinates.
   * @returns {boolean} True if the display needs to be updated.
   */
  setHover(mouse) {
    // Mouse hover does nothing if this interaction does not have both an active
    // icon and a hover icon.
    if (!this._icon[ACTIVE] || !this._icon[HOVER]) return false;

    const prev = this._hover;
    this._hover = this._click(mouse);
    return (this._state == ACTIVE && this._hover != prev)
  }

  /** Return true if the given mouse position is in the icon and the state is active.
   * @param {Point} mouse The mouse position in room coordinates.
   * @return {boolean} True if a click at that position activates this interaction.
   */
  click(mouse) {
    return (this._state == ACTIVE || this._state == VISIBLE) && this._click(mouse);
  }

  /** Return true if this interaction is active, i.e. clicking triggers it.
   * @return {boolean} True if the interaction is active.
   */
  isActive() {
    return this._state == ACTIVE;
  }

  /** Clear an interaction's state, e.g. due to the avatar leaving the room. */
  clearState() {
    this._state = INACTIVE;
    this._hover = false;
  }

  /** Get the interaction's position in the room.
   * @returns {Point} The position, in room coordinates.
   */
  position() {
    return this._position;
  }

  /** Get the icon to be drawn for this interaction in its current state.
   * @returns {?Sprite} The sprite to use, or null if none should be shown.
   */
  icon() {
    if (this._state == INACTIVE) return null;
    if (this._state == ACTIVE && this._hover && this._icon[HOVER]) return this._icon[HOVER];
    return this._icon[this._state];
  }

  /** Get the position where the icon should be drawn.
   * @returns {Point} The icon's center, in room coordinates.
   */
  iconCenter() {
    return this._iconCenter;
  }

  /** If this interaction triggers an "enter" event, get the point the avatar moves to.
   * @returns {?Point} The new coordinates, or null if there is no "enter" event.
   */
  enterPosition() {
    return this._enterPosition;
  }

  /** If this interaction triggers an "enter" event, get the room the avatar moves to.
   * @returns {?Room} The new room, or null if the avatar stays in the current room.
   */
  enterRoom() {
    return this._enterRoom;
  }

  /** If this interaction triggers a dialog, get the name of the dialog.
   * @returns {?string} The name of the dialog to begin.
   */
  dialog() {
    return this._dialog;
  }

  // Check if the given mouse location is inside where the active icon would be.
  _click(mouse) {
    const icon = this._icon[ACTIVE];
    if (!icon) return false;
    // Convert the mouse to an offset from the top left corner of the sprite.
    const off = this._iconCenter.sub(mouse).sub(icon.cornerOffset());
    return (off.x >= 0 && off.x < icon.width() && off.y >= 0 && off.y < icon.height());
  }

  /** @type {string} */
  _name = "";

  /** @type {Point} */
  _position = new Point();
  /** @type {Point} */
  _iconCenter = new Point();
  /** @type {Point[]} */
  _radius = [null, null];

  /** @type {number} */
  _state = INACTIVE;
  /** @type {boolean} */
  _hover = false;
  /** @type {Sprite[]} */
  _icon = [null, null, null];

  /** @type {?Point} */
  _enterPosition = null;
  /** @type {?Room} */
  _enterRoom = null;
  /** @type {?string} */
  _dialog = null;
}


// Check if the given point is within the given radius.
function inRange(point, radius) {
  // If the radius is (0, 0) it means the whole room is in range.
  if (!radius.x && !radius.y) return true;
  return (point.x / radius.x)**2 + (point.y / radius.y)**2 <= 1;
}
