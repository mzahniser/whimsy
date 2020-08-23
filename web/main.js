import {Data} from "./data.js";
import {Sprite} from "./sprite.js";
import {World} from "./world.js";

// The "world" object holds the current state of the game world.
let world;
// Because module scripts are always deferred, this code will not be run
// until the full HTML page has been parsed.
fetch("data.txt").then(response => response.text()).then(text => loadData(text));
function loadData(text) {
  // Start the game as soon as all sprites are loaded.
  Sprite.setLoadCallback(startGame);

  // Parse the game data, which is embedded in the HTML.
  const data = new Data(text);
  World.load(data);

  // Initialize the world object.
  world = new World();
  world.new();
}

function startGame()
{
  // Link up the World's event listeners.
  const canvas = document.getElementById("game");
  canvas.onmousemove = (event => world.onmousemove(event));
  canvas.onmousedown = (event => world.onmousedown(event));
  // Begin the event loop.
  world.start(canvas);
}
