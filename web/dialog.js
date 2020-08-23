import {Data} from "./data.js";
import {Interaction} from "./interaction.js";
import {Sprite} from "./sprite.js";
import {Style} from "./style.js";
import {Variables} from "./variables.js";
import {World} from "./world.js";

/** An object for parsing and displaying dialog in the game.
 * @typedef {Object} Dialog
 */
export class Dialog {
  /** Load a dialog definition from the given data file.
   * @param {Data} data
   */
  static load(data) {
    const node = Dialog._nodes[data.value()] = new Node();
    while (data.next() && data.size()) {
      if (data.tag() == "ask") node.ask = data.value();
      else node.lines.push(data.line());
    }
  }

  /** Construct a dialog object that can make changes to the given World.
   * @param {World} world
   */
  constructor(world) {
    this._world = world;
  }

  /** Start the dialog with the given name, if it exists. If the dialog does not produce
   * any visible output, it will run immediately and return.
   * @param {string} name
   */
  begin(name) {
    if (name in Dialog._nodes) {
      this._data = new Data(Dialog._nodes[name].lines);
      this._step();
      if (this.isOpen()) {
        this._keyListener = event => this._keypress(event);
        document.addEventListener("keypress", this._keyListener);
      }
    }
  }

  /** Close the dialog, if it is open. */
  close() {
    this._clearOptions();
    this._optionText.length = 0;
    document.getElementById("options").innerHTML = "";
    this._icon = 0;
    this._scene = 0;
    this._display();
    if (this._keyListener) {
      document.removeEventListener("keypress", this._keyListener);
      this._keyListener = null;
    }
  }

  /** If the dialog is in the "click to continue" state, choose the only option. */
  continue() {
    if (this.isOpen() && this._optionText.length == 1) this._choose(0);
  }

  /** Check if the dialog is open, i.e. something is being displayed.
   * @returns {boolean} True if the current dialog state requires displaying anything.
   */
  isOpen() {
    return this._text.length || this._icon || this._scene || this._optionText.length;
  }

  // Step the dialog forward to the next "say" block.
  _step() {
    // Remove any options currently being displayed.
    this._optionText.length = 0;
    document.getElementById("options").innerHTML = "";
    // The "scene" resets with each page of the dialog. The icon does not.
    this._scene = 0;
    let spoke = false;
    while (this._data.isValid()) {
      if (this._data.tag() === "goto") {
        const name = this._data.value();
        if (name in Dialog._nodes) {
          this._data = new Data(Dialog._nodes[name].lines);
        }
        continue;
      }
      else if (this._data.tag() === "if") {
        if (!Variables.eval(this._data.value())) {
          skipBlock(this._data);
          // We are now at the first line after the end of the "if" block
          // that we skipped. If that line is an "else" we can skip that
          // line and move on into the contents of the else. Otherwise, we
          // need to "continue" to avoid advancing past this line.
          if (this._data.tag() !== "else") continue;
        }
      }
      else if (this._data.tag() === "else") {
        // If we got to this line, it means we didn't skip the previous "if"
        // clause, so we need to skip this one.
        skipBlock(this._data);
        // We don't need to call data.next() because it is already at the
        // first line after the block.
        continue;
      }
      else if (this._data.tag() === "end") {
        if (spoke) break;
        this._world.end();
        return;
      }
      else if (this._data.tag() === "option") {
        const name = this._data.value();
        if (name in Dialog._nodes && !this._visited.has(Dialog._nodes[name])) {
          this._options.push(Dialog._nodes[name]);
        }
      }
      else if (this._data.tag() === "exit") {
        this._exitText = (this._data.size() > 1 ? this._data.value() : "(End conversation.)");
      }
      else if (this._data.tag() === "icon") {
        this._icon = +this._data.arg(1);
      }
      else if (this._data.tag() === "scene") {
        this._scene = +this._data.arg(1);
      }
      else if (this._data.tag() == "add") {
        const room = this._data.value();
        const indent = this._data.indent();
        this._data.next();
        while (this._data.indent() > indent) {
          if (this._data.tag() == "interaction") {
            this._world.addInteraction(new Interaction(data), room);
          }
          else {
            this._world.addSprite(+data.arg(0), data.point(1), data.value(2), room);
            this._data.next();
          }
        }
        // Return to the top of the loop without calling data.next() again.
        continue;
      }
      else if (this._data.tag() == "remove") {
        const room = this._data.value();
        const indent = this._data.indent();
        while (this._data.next() && this._data.indent() > indent) {
          this._world.remove(this._data.value(0), room);
        }
        // Return to the top of the loop without calling data.next() again.
        continue;
      }
      else if (this._data.tag() == "enter") {
        this._world.enter(this._data.point(1), this._data.arg(2));
      }
      else if (this._data.tag() == "face") {
        this._world.face(+this._data.arg(1));
      }
      else if (this._data.tag() == "set") {
        Variables.set(this._data.value());
      }
      else if (this._data.tag() == "say") {
        // If we've reached another "say" line, that means the current
        // "paragraph" should be shown immediately, and we don't need to
        // show options because we're not yet at the end of the stream.
        if (spoke) break;
        spoke = true;

        this._text = this._data.value();
      }
      // If we didn't "continue" up above, advance to the next line.
      this._data.next();
    }

    // We've reached the point where we should update the dialog page.
    // Check if we reached the end of the node, meaning that any options
    // we have accumulated should be displayed.
    if (this._data.isValid()) this._optionText.push("(Click to continue.)");
    else {
      // When accumulating options, we already made sure that they are all valid.
      for (const node of this._options) {
        this._optionText.push(node.ask);
      }
      if (this._exitText) {
        this._optionText.push(this._exitText);
      }
      else if (this._text) {
        this._optionText.push("(Click to continue.)");
      }
    }

    this._display();
  }

  _display() {
    const dialog = document.getElementById("dialog");
    if (!this.isOpen()) {
      dialog.style.display = "none";
      return;
    }
    dialog.style.display = "block";

    // Get the four elements of the document that we can customize.
    const text = document.getElementById("text");
    const icon = document.getElementById("icon");
    const scene = document.getElementById("scene");
    const options = document.getElementById("options");
    // TODO: Avoid hard-coding this width and the padding.
    let width = 400;

    // Set the icon.
    if (this._icon) width += 10 + this._setSprite(icon, this._icon);
    else icon.style.display = "none";
    // Set the scene.
    if (this._scene) width = Math.max(width, this._setSprite(scene, this._scene));
    else scene.style.display = "none";
    // Set the text.
    text.innerHTML = Style.apply(this._text);
    // Add the padding to the width.
    width += 20;
    dialog.style.width = width + "px";
    dialog.style.marginLeft = -0.5 * width + "px";

    // Add the options.
    let index = 0;
    for (const option of this._optionText) {
      // Make a copy of the index for the arrow function to hold onto.
      const i = index++;
      let element = document.createElement("div");
      element.onmousedown = () => this._choose(i);
      element.className = "option box";
      element.innerHTML = Style.apply(index + ": " + option);
      options.append(element);
    }
  }

  _setSprite(element, index) {
    const sprite = Sprite.get(index);
    if (!sprite) return 0;

    element.style.display = "block";
    element.style.width = sprite.width() + "px";
    element.style.height = sprite.height() + "px";
    element.style.background = sprite.source();
    return sprite.width();
  }

  _choose(index) {
    if (index < this._options.length) {
      // Note: before adding theis option to the list, we already checked
      // that it exists and is not visited.
      const dialog = this._options[index];
      this._visited.add(dialog);
      this._data = new Data(dialog.lines);
      this._clearOptions();
      this._step();
    }
    else if (this._data.isValid()) this._step();
    else this.close();
  }

  /** Handle a keyboard event. This listener is only active when the dialog is visible.
   * @param {KeyboardEvent} event
   */
  _keypress(event) {
    const index = +String.fromCharCode(event.charCode);
    if (index >= 1 && index <= 9) this._choose(index - 1);
  }

  _clearOptions() {
    // Begin accumulating options from scratch.
    this._options.length = 0;
    this._exitText = "";
    // The text disappears, but the icon will remain until cleared manually or
	  // until the conversation ends.
	  this._text = "";
  }

  // Dictionary of all the dialog nodes.
  /** @type {Object.<string, Dialog>} */
  static _nodes = {};

  // Hold a reference to the World object, to apply changes to it.
  /** @type {World} */
  _world;
  // The data we are currently iterating through.
  /** @type {Data} */
  _data;
  // Track whether we're listening to key events.
  /** @type {function(KeyboardEvent) => void} */
  _keyListener;

  // Display state:
  /** @type {string} */
  _text = "";
  /** @type {number} */
  _icon = 0;
  /** @type {number} */
  _scene = 0;
  /** @type {string[]} */
  _optionText = [];

  // Remember which dialog nodes we have visited. They will not be presented as
  // an option again until the player leaves the room.
  /** @type {Set<Dialog>} */
  _visited = new Set();
  // These are the nodes that may be "visited" from the current one.
  /** @type {Dialog[]} */
  _options = [];
}


// This class represents a single "dialog" object in the data files.
class Node {
  ask;
  lines = [];
}

// Skip an indented block in a data file.
function skipBlock(data) {
  const indent = data.indent();
  while (data.next() && data.indent() > indent) continue;
}
