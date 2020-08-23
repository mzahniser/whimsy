import {Avatar} from "./avatar.js";
import {Data} from "./data.js";
import {Dialog} from "./dialog.js";
import {Paths} from "./paths.js";
import {Point} from "./point.js";
import {Room} from "./room.js";
import {Sprite} from "./sprite.js";
import {Style} from "./style.js";

/** Class for tracking the whole state of the game world.
 * @typedef {Object} World
 */
export class World {
  /** Load the game data from the given data file.
   * @param {Data} data
   */
  static load(data) {
    while (data.isValid()) {
      if (!data.size()) {
        data.next();
      }
      else if (data.tag() === "game") {
        World._loadConfig(data);
      }
      else if (data.tag() === "index") {
        Sprite.setIndex(data);
      }
      else if (data.tag() === "sheet") {
        Sprite.loadSheet(data);
      }
      else if (data.tag() === "sprite") {
        Sprite.loadSprite(data);
      }
      else if (data.tag() === "style") {
        Style.load(data);
      }
      else if (data.tag() === "avatar") {
        Avatar.load(data);
      }
      else if (data.tag() === "init" || data.tag() == "dialog") {
        Dialog.load(data);
      }
      else if (data.tag() === "room") {
        World._initRooms[data.value()] = new Room(data);
      }
      else {
        while (data.next() && data.size()) continue;
      }
    }
  }
  
  /** Initialize the world from scratch.
   */
  new() {
    this._reset();

    // Run the "init" dialog, which should place the avatar.
    this._dialog.begin("");
  }

  start(canvas) {
    this._canvas = canvas;
    this._context = canvas.getContext("2d", {alpha: false});
    // Begin animating.
    this._draw();
    this._step();
  }

  /** The game has reached its end. Show the "the end" text.
   */
  end() {
    document.getElementById("game").style.display = "none";
    document.getElementById("dialog").style.display = "none";
    document.getElementById("end").style.display = "block";
  }

  /** Handle a mouse movement event.
   * @param {MouseEvent} event
   */
  onmousemove(event) {
    if (this._dialog.isOpen()) return;
    const room = this._avatar.room();
    if (!room) return;

    // Get the mouse location, then convert it to room coordinates.
    const mouse = new Point(event.clientX, event.clientY);
    const point = this._avatar.position().add(mouse).sub(this._center);

    let changed = false;
    for (const it of room.interactions()) {
      changed |= it.setHover(point);
    }
    if (changed) requestAnimationFrame(() => this._draw());
  }

  /** Handle a mouse click event.
   * @param {MouseEvent} event
   */
  onmousedown(event) {
    if (this._dialog.isOpen()) {
      this._dialog.continue();
      return;
    }
    const room = this._avatar.room();
    if (!room) return;

    // Get the mouse location, then convert it to room coordinates.
    const mouse = new Point(event.clientX, event.clientY);
    let point = this._avatar.position().add(mouse).sub(this._center);

    for (const it of room.interactions()) {
      if (it.click(point)) {
        if (it.isActive()) {
          this._trigger(it);
          return;
        }
        else {
          point = it.position();
          break;
        }
      }
    }
    // If the click was not on an interaction icon, it's a movement command.
    this._path = this._paths.find(this._avatar.position(), point);
  }

  /** Move the avatar to the given place.
   * @param {Point} position The position to move to.
   * @param {?string} room Optionally, the room to move to.
   */
  enter(position, room) {
    // Before leaving the current location, disable all interactions in this room.
    if (this._avatar.room()) {
      for (const it of this._avatar.room().interactions()) {
        it.clearState();
      }
    }
    this._avatar.enter(position, this._findRoom(room));
    // Racalculate the pathfinding information.
    this._initPathfinding();
  
  	// Entering a room may trigger interactions, but if so those interactions
    // are not allowed to directly trigger another "entering" action. (Dialogs
    // run by the interaction may still use an "entering" command, so it is
    // still possible, but less likely, to cause an infinite loop.)
    if (this._avatar.room()) {
      for (const it of this._avatar.room().interactions()) {
        if (it.setState(this._avatar.position())) {
          this._trigger(it, true);
        }
      }
    }
  }

  /** Set the avatar's sprite to the one facing the given direction.
   * @param {number} degrees A direction, in "compass" degrees.
   */
  face(degrees) {
    this._avatar.face(degrees);
  }

  /** Add a sprite to the named room, or the current room if no room is given.
   * @param {number} index The sprite index.
   * @param {Point} position The position, in room coordinates.
   * @param {?string} name Optionally, a name to assign to the sprite.
   * @param {?string} room Optionally, the name of the room to move to.
   */
  addSprite(index, position, name, room) {
    this._findRoom(room).addSprite(Sprite.get(index), position, name);
  }

  /** Add an interaction to the named room, or the current room if no name is given. 
   * @param {Interaction} interaction The interaction to add.
   * @param {?string} room Optionally, the name of the room.
   */
  addInteraction(interaction, room) {
    this._findRoom(room).addInteraction(interaction);
  }

  /** Remove any sprites or interactions with the given name from the named room,
   * or from the current room if no room is given.
   * @param {string} name The name of the sprite(s) to remove.
   * @param {?string} room Optionally, the name of the room to operate on.
   */
  remove(name, room) {
    this._findRoom(room).remove(name);
  }

  static _loadConfig(data) {
    while (data.next() && data.size()) {
      if (data.tag() === "fps") {
        // Avoid divide by zero by not allowing frame rates below 1.
        World._fps = Math.max(1, +data.arg(1));
      }
    }
  }

  _findRoom(name) {
    return name in this._rooms ? this._rooms[name] : this._avatar.room();
  }

  // Draw the world. This is a private function because it is only done in
  // response to an event or a timer.
  _draw() {
    // Make sure the canvas internal size in pixels matches its size on the screen.
    if (this._canvas.width != this._canvas.clientWidth || this._canvas.height != this._canvas.clientHeight) {
      this._canvas.width = this._canvas.clientWidth;
      this._canvas.height = this._canvas.clientHeight;
    }
    // Store the center of the canvas, for mapping events.
    this._center = new Point(this._canvas.width >> 1, this._canvas.height >> 1);
  
    // Test drawing.
    const room = this._avatar.room();
    // Round the avatar's position to an integer value, so the sprites will all
    // be aligned with the pixels.
    const position = this._avatar.position().round();
    const index = room.addSprite(this._avatar.sprite(), position);
    room.draw(this._context, position.sub(this._center));
    room.removeSprite(index);
  }

  _step() {
    // TODO: Pause if the dialog is open.
    let position = this._avatar.position();
    let step = this._avatar.speed();
    while (this._path.length && step > 0) {
      const d = this._path[this._path.length - 1].sub(position);
      const length = d.length();
      if (length <= step) {
        position = this._path.pop();
        step -= length;
      }
      else {
        position = position.add(d.mul(step / length));
        step = 0;
      }
    }
    // Call avatar.move() only if we've changed the position, i.e. it no
    // longer points to the avatar's position.
    if (position !== this._avatar.position()) {
      this._avatar.move(position);
      for (const it of this._avatar.room().interactions()) {
        if (it.setState(position)) {
          this._trigger(it);
        }
        // We're about to redraw the frame anyway, so don't bother to check
        // if the state change caused the icon to need updating.
      }
    }
    requestAnimationFrame(() => this._draw());
    this._timer = setTimeout(() => this._step(), 1000 / World._fps);
  }

  _trigger(interaction, entering = false) {
    // Only trigger "enter" events if this event is not being triggered by
    // entering a room. (This is to avoid endless loops.)
    if (interaction.enterPosition() && !entering) {
      this.enter(interaction.enterPosition(), interaction.enterRoom());
    }
    if (interaction.dialog()) {
      this._dialog.begin(interaction.dialog());
    }
  }

  _reset() {
    // Disable any previously active frame timer.
    if (this._timer) {
      clearTimeout(this._timer);
    }
    this._rooms = {};
    for (const key in World._initRooms) {
      this._rooms[key] = World._initRooms[key].clone();
    }
    this._avatar = new Avatar();
    this._path = [];
    this._dialog = new Dialog(this);
  }

  _initPathfinding() {
    this._path.length = 0;
    if (this._avatar.room()) {
      this._paths.init(this._avatar.room(), this._avatar.position());
    }
  }

  // Initial state of the rooms.
  /** @type {Object.<string, Room>} */
  static _initRooms = {};
  // Frames per second.
  /** @type {number} */
  static _fps = 8;

  // The animation timer.
  /** @type {number} */
  _timer;

  // The canvas and draw context. We keep a copy of these here because they are
  // needed in the draw() function.
  /** @type {HTMLCanvasElement} */
  _canvas;
  /** @type {CanvasRenderingContext2D} */
  _context;
  // This is the center of the canvas. It's stored so that mouse events can be
  // mapped to points relative to the avatar's position.
  /** @type {Point} */
  _center;

  // Current dialog state.
  /** @type {Dialog} */
  _dialog;
  
  // The state of the rooms, with any local changes.
  /** @type {Object.<string, Room>} */
  _rooms;
  // The position of the avatar.
  /** @type {Avatar} */
  _avatar;

  // Pathfinding variables:
  /** @type {Paths} */
  _paths = new Paths();
  /** @type {Point[]} */
  _path = [];
}
