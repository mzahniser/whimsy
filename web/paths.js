import {Heap} from "./heap.js";
import {Polygon} from "./polygon.js";
import {Point} from "./point.js";
import {Room} from "./room.js";

/** An object for performing pathfinding in a room.
 * @typedef {Object} Paths
 */
export class Paths {
  /** Initialize pathfinding, beginning at the given location in the given room.
   * @param {Room} room The room to initialize pathfinding for.
   * @param {Point} point Any part of the room not reachable from this point is ignored.
   */
  init(room, point) {
    this._passable = new Polygon();
    this._waypoints = [];

    // Generate the passable polygon.
    for (const entry of room.sprites()) {
      if (!entry.sprite.mask().isEmpty()) {
        const mask = entry.sprite.mask().add(entry.center);
        this._passable.union(mask);
      }
    }
    // Cull out any parts of the passable polygon that the avatar can't reach.
    this._passable.floodFill(point);

    // Now, calculate all the sightlines in the room.
    for (const part of this._passable.rings()) {
      // Skip degenerate polygons. (Just in case, although there should be none.)
      const length = part.length;
      if (length < 12) continue;
      
      let beforeX = part[length - 2];
      let beforeY = part[length - 1];
      for (let i = 0; i < length; i += 4) {
        const afterX = part[i + 2];
        const afterY = part[i + 3];
        const cross = beforeX * afterY - beforeY * afterX;
        if (cross <= 0) this._waypoints.push(new Waypoint(new Point(part[i], part[i + 1])));
        beforeX = afterX;
        beforeY = afterY;
      }
    }
  }

  /** Find the shortest path between the two given room coordiantes.
   * @param from The starting coordinate.
   * @param to The target coordinate. If it's not reachable, the closest reachable vertex is used.
   * @returns {Point[]} A list of waypoints in stack form, i.e. the last one is the first destination.
   */
  find(from, to) {
    // Make sure we have a collision mask.
    if (!this._passable.rings().length) return [];
    // If the given point is not inside the mask, replace it with the closest vertex.
    if (!this._passable.contains(to)) to = this._closestVertex(to);
    // If there's a direct line of sight between the start and end points, there is
    // no need for pathfinding.
    if (!this._passable.intersects(from, to)) return [to];

    // Calculate the distance from every waypoint to the target point, and also
    // check which waypoints are in direct sight of the target.
    this._calculateDistances(to);
    
    // Now, we know there's no direct point to the target, but an indirect path
    // must exist. Use A* search to find it.
    const queue = new Heap((a, b) => a.length < b.length);

    // Start by adding the sightlines from the start node to each waypoint.
    for (let index = 0; index < this._waypoints.length; ++index) {
      const it = this._waypoints[index];
      if (!this._passable.intersects(from, it.point)) {
        let length = from.distance(it.point);
        // Remember that we can get straight back to the beginning from here.
        it.backtrack = -1;
        it.shortest = length;
        // Calculate the A* heuristic and queue this point.
        length += it.targetDistance;
        queue.insert({length, index});
      }
    }
    // Now, iterate until we've found the best path.
    let bestDistance = Infinity;
    let best;
    while (!queue.empty()) {
      // Get the most promising path from the queue.
      const node = queue.pop();
      // If that path cannot be shorter than the best we've found, there is no need
      // to do any more searching.
      if (node.length >= bestDistance) break;

      // Check if we can reach the end point directly from this node.
      const waypoint = this._waypoints[node.index];
      if (waypoint.visible) {
        bestDistance = node.length;
        best = node;
        continue;
      }
      
      // Otherwise, queue up paths through all the waypoints visible from
      // here. Remove the heuristic segment from the path length first.
      node.length -= waypoint.targetDistance;
      if (!waypoint.sightlines.length) this._findSightlines(node.index);
      for (const line of waypoint.sightlines) {
        const next = this._waypoints[line.link];
        // Don't bother trying a path through this waypoint if a shorter
        // one has already passed through it.
        const length = node.length + line.distance;
        if (length >= next.shortest) continue;
        // Remember how short this path was, and where it came from.
        next.shortest = length;
        next.backtrack = node.index;
        queue.insert({
          length: length + next.targetDistance,
          index: line.link,
          previous: node.index
        });
      }
    }
    // If we were unable to find a path, return an empty vector.
    if (!best) return [];
    
    // Backtrack to find the path.
    const path = [to];
    for (let i = best.index; i >= 0; i = this._waypoints[i].backtrack) {
      path.push(this._waypoints[i].point);
    }
    // Note: the path is returned in stack format, i.e. with the first waypoint
    // at the end because it is the first one we will want to pop.
    return path;
  }

  _findSightlines(index) {
    // Check sightlines to all other waypoints.
    const here = this._waypoints[index];
    for (let i = 0; i < this._waypoints.length; ++i) {
      if (i === index) continue;
      const other = this._waypoints[i];
      if (!this._passable.intersects(here.point, other.point)) {
        here.sightlines.push(new Sightline(i, other.point.distance(here.point)));
      }
    }
  }

  // Find the closest vertex of the polygon.
  _closestVertex(target) {
    let bestDistance = Infinity;
    let best;
    for (const ring of this._passable.rings()) {
      for (let i = 0; i < ring.length; i += 4) {
        const distance = Math.hypot(target.x - ring[i], target.y - ring[i + 1]);
        if (distance < bestDistance) {
          bestDistance = distance;
          best = new Point(ring[i], ring[i + 1]);
        }
      }
    }
    return best;
  }

  // For each waypoint, calculate its distance and visibility to the given point.
  _calculateDistances(target) {
    for (const it of this._waypoints) {
      it.targetDistance = target.distance(it.point);
      it.visible = !this._passable.intersects(target, it.point);
      it.backtrack = -1;
      it.shortest = Infinity;
    }
  }

  /** @type {Polygon} */
  _passable;
  /** @type {Waypoint[]} */
  _waypoints;
}


/** A point that pathfinding might pass through.
 * @typedef {Object} Waypoint
 * @property {Point} point Coordinates of this waypoint.
 * @property {Sightline[]} sightlines Waypoints that are reachable directly from here.
 * @property {number} targetDistance Straight-line distance to the target endpoint.
 * @property {boolean} visible True if there's a straight path to the endpoint from here.
 * @property {number} backtrack The index of the waypoint that the shortest path comes from.
 * @property {number} shortest The length of the shortest path to here from the starting point.
 */
class Waypoint {
  constructor(vertex) {
    this.point = vertex;
  }

  point;
  sightlines = [];
  targetDistance;
  visible;
  backtrack;
  shortest;
}


/** A path between two Waypoints.
 * @typedef {Object} Sightline
 * @property {number} link The index of the waypoint at the other end.
 * @property {number} distance The distance to that waypoint.
 */
class Sightline {
  constructor(link, distance) {
    this.link = link;
    this.distance = distance;
  }

  link;
  distance;
}
