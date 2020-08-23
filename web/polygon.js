
import {Point} from "./point.js";

/** A polygon, made up of one or more "rings" of vertices.
 * @typedef {Object} Polygon
 */
export class Polygon {
  /** Add a "ring" of points to this polygon, without calculating intersections.
   * @param {number[]} vertices
   */
  push(points) {
    if (points.length) this._rings.push(expand(points));
  }
  /** Check if this polygon is empty.
   * @returns {boolean} True if the polygon is empty.
   */
  isEmpty() {
    return !this._rings.length;
  }
  /** Add the given vector offset to this polygon.
   * @param {Point} offset
   * @returns {Polygon}
   */
  add(point) {
    const polygon = new Polygon();
    for (const ring of this._rings) {
      const result = [];
      for (let i = 0; i < ring.length; i += 4) {
        result.push(ring[i] + point.x, ring[i + 1] + point.y, ring[i + 2], ring[i + 3]);
      }
      polygon._rings.push(result);
    }
    return polygon;
  }
  /** Add the given Polygon to this polygon, one "ring" at a time.
   * @param {Polygon} polygon
   */
  union(polygon) {
    for (const ring of polygon._rings) {
      this._union(ring);
    }
  }
  _union(ring) {
    // In the comments below, this polygon is called P, and the simple polygon
    // being added is called Q. The output is called R.

    // We're going to generate a new output polygon here:
    const result = [];

    // Remember all intersections with all edges on Q, so we can slot them in
    // at the right spots when generating the new Q.
    const qIntersections = [];

    // This is all the parts in one vector, with intersection vertices added
    // and links between polygons (including joining end to start points).
    const amplified = [];

    // Remember the smallest part of P that contains Q.
    let smallestContainer;

    // Entering and exiting calculations are reversed if Q is a hole.
    const qIsHole = isHole(ring);

    // Step 1: finding the intersections and generating copies of the P polygons
    // with those intersections inserted.

    // Loop through every segment of every part of this polygon, checking for
    // intersections with the given ring. If there are any, generate an
    // "amplified" version of the part with all the intersection vertices added.
    for (const part of this._rings) {
	  	// First, find any potential intersections. It's possible that we will
	  	// generate duplicates here, e.g. if an intersection occurs at a vertex.
      const pIntersections = [];

      for (let pi = 0; pi < part.length; pi += 4) {
        for (let qi = 0; qi < ring.length; qi += 4) {
          const cross = part[pi + 2] * ring[qi + 3] - part[pi + 3] * ring[qi + 2];
          if (!cross) continue;

          // Now, we know the two lines are not parallel, so they must
          // intersect. Calculate where that intersection occurs.
          const dx = ring[qi] - part[pi];
          const dy = ring[qi + 1] - part[pi + 1];
          // pT and qT are between 0 and 1 if the intersection is on p and q
          // respectively. As long as the vertices are all integer coordinates,
          // the intersection point's coordinates will be divisible by 0.25, but
          // that does not hold true for pT and qT.
          const pC = dx * ring[qi + 3] - dy * ring[qi + 2];
          const pT = pC / cross;
          if (pT < 0 || pT > 1) continue;
          const qC = dx * part[pi + 3] - dy * part[pi + 2];
          const qT = qC / cross;
          if (qT < 0 || qT > 1) continue;
          
          // Avoid floating point rounding error by multiplying by pC first
          // before dividing by cross.
          const point = new Point(
            part[pi] + part[pi + 2] * pC / cross,
            part[pi + 1] + part[pi + 3] * pC / cross);
          pIntersections.push({
            point,
            order: (pi * 0.25 + pT) % (part.length * 0.25),
            qOrder: (qi * 0.25 + qT) % (ring.length * 0.25),
            entering: (cross > 0) != qIsHole
          });
        }
      }
      // Sort the intersections so we can loop through them in order.
      pIntersections.sort((a, b) => a.order - b.order);
      // Remove "doubles" due to intersections exactly at a vertex.
      removeDoubles(pIntersections);

      // If there are no intersections left, figure out if this part of the
      // polygon is entirely within the new ring.
      if (!pIntersections.length) {
        if (!ringContainsRing(ring, part)) {
          result.push(part);
          if (ringContainsRing(part, ring) && (!smallestContainer || ringContainsRing(smallestContainer, part))) {
            smallestContainer = part;
          }
        }
        continue;
      }

      // Otherwise, we need to go back around the part to generate the
      // amplified list of vertices.
      amplify(part, pIntersections, amplified, it => {
        qIntersections.push({
          point: it.point,
          order: it.qOrder,
          index: amplified.length,
          entering: it.entering
        });
        amplified.push(-1, it.point.x, it.point.y);
      });
    }

    // Step 2: generating a copy of Q with the intersections inserted:
    // If there were no intersections at all:
    if (!qIntersections.length) {
      // Q should be added to P if it has the opposite polarity of the
      // smallest part of P that fully contains Q. (If none, the space outside
      // the polygon counts as having "hole" polarity.)
      const containerIsHole = (!smallestContainer || isHole(smallestContainer));
      if (containerIsHole != qIsHole) result.push(ring);
      this._rings = result;
      return;
    }
      
    // Sort the intersections so we can loop through them in order. This time,
    // we don't need to check for "doubles" since they were removed above.
    qIntersections.sort((a, b) => a.order - b.order);
    // Iterate through the vertices, linking them up with previous parts.
    const enteringIndices = [];
    amplify(ring, qIntersections, amplified, it => {
      // If this is an "entering" intersection, store it as one of the
      // possible places to start a trace.
      if (it.entering) enteringIndices.push(amplified.length);
      // If this is an exiting vertex, add a link to it.
      else amplified[it.index] = amplified.length;
      // Otherwise, if this is an entering vertex, add a link from it.
      amplified.push(it.entering ? it.index : -1, it.point.x, it.point.y);
    });
  
  	// Step 3: Tracing the output polygons.
    // Try each possible starting index.
    for (const index of enteringIndices) {
      // Make sure this link has not been cleared (while tracing a previous output).
      if (amplified[index] < 0) continue;

      const out = tracePolygon(amplified, index);
      if (out.length >= 6) result.push(expand(out));
    }
    this._rings = result;
  }

  /** Remove every component of this polygon that is not reachable from the given point.
   * @param {Point} point
   */
  floodFill(point) {
    const areas = [];
    for (const ring of this._rings) {
      areas.push(area(ring));
    }

    // First, find the smallest positive ring that contains the point.
    let smallestI = -1;
    for (let i = 0; i < areas.length; i++) {
      if (areas[i] > 0 && contains(this._rings[i], point) && (smallestI < 0 || areas[i] < areas[smallestI])) {
        smallestI = i;
      }
    }
    // If we didn't find a positive ring containing the point, clear the whole polygon.
    if (smallestI < 0) {
      this._rings.length = 0;
      return;
    }

    // Begin building up the output polygon.
    const smallest = this._rings[smallestI];
    const result = [smallest];

    // Now, find all holes that are inside that polygon. But, make sure we do
    // not include holes that are nested inside other holes.
    outer:
    for (let i = 0; i < areas.length; ++i) {
      const ring = this._rings[i];
      if (areas[i] < 0 && ringContainsRing(smallest, ring)) {
        // Find all holes larger than this one and see if they contain this
		  	// one. To contain it, the other hole must be larger, and since
        // holes have negative area that must mean a lower area.
        for (let j = 0; j < areas.length; ++j) {
          if (areas[j] < areas[i] && ringContainsRing(this._rings[j], ring)) continue outer;
        }
        result.push(ring);
      }
    }
    this._rings = result;
  }

  /** Check if this polygon contains the given point.
   * @param {Point} point
   * @returns {boolean} True if the given point is in this polygon (or on an edge of it).
   */
  contains(point) {
    let winding = 0;
    for (const ring of this._rings) {
      const result = windingNumber(ring, point.x, point.y);
      if (result.border) return true;
      winding += result.winding;
    }
    return winding != 0;
  }

  /** Check if the given line segment intersects this polygon - that is, the segment has
   * portions both inside and outside the polygon. The polygon's border counts as inside,
   * so it is not an intersection if the segment touches the border but is otherwise
   * entirely inside the polygon.
   * @param {Point} start
   * @param {Point} end
   * @returns {boolean} True if the segment is not entirely inside or outside.
   */
  intersects(start, end) {
    const vector = end.sub(start);
    for (const ring of this._rings) {
      // The first edge will be between vertices 0 and 1. Calculate the vector of the
      // previous edge, i.e. from the back of the ring to 0.
      let prev = (ring[0] - ring[ring.length - 4]) * vector.y - (ring[1] - ring[ring.length - 3]) * vector.x;
      for (let i = 0; i < ring.length; i += 4) {
        const cross = ring[i + 2] * vector.y - ring[i + 3] * vector.x;
        const pCross = prev;
        prev = cross;
        if (start.x == ring[i] && start.y == ring[i + 1]) {
          if (pCross < 0 && cross < 0) return true;
          continue;
        }
        if (end.x == ring[i] && end.y == ring[i + 1]) {
          if (pCross > 0 && cross > 0) return true;
          continue;
        }
        // Ignore edges that are either parallel to or collinear with the query segment.
        // This is safe because if there is an intersection, it will be detected at one
        // of the adjacent edges.
        if (!cross) continue;
  
        // Now, we know the two lines are not parallel, so they must intersect. Calculate
        // where that intersection occurs.
        const dx = start.x - ring[i];
        const dy = start.y - ring[i + 1];
        const segT = (dx * ring[i + 3] - dy * ring[i + 2]) / cross;
        const edgeT = (dx * vector.y - dy * vector.x) / cross;
        // The T values are the solution to: intersection = start + T * vector. That is,
        // the intersection is on the segment if T is between 0 and 1.

        if (edgeT < 0 || edgeT > 1 || segT < 0 || segT > 1) continue;
        // If the intersection point is strictly inside the query segment (not at one of
        // its endpoints), it always counts as an intersection.
        if (segT > 0 && segT < 1) return true;
        // If we're here, segT is exactly 0 or 1, i.e. there's an intersection at an
        // endpoint. If we're not at an endpoint of the polygon, the segment is outside
        // the polygon if segT is 0 and cross < 0, or segT is 1 and cross > 0.
        if (edgeT !== 0 && edgeT !== 1 && !segT === (cross < 0)) return true;
      }
    }
    return false;
  }

  /** Access the array of Ring components that make up this polygon.
   * @returns {number[][]} An array of rings.
   */
  rings() {
    return this._rings;
  }

  /** @type {number[][]} */
  _rings = [];
}


function removeDoubles(intersections) {
  let j = 0;
  for (let i = 0; i < intersections.length; ) {
    intersections[j] = intersections[i];
    const order = intersections[i].order;
    let entering = intersections[i].entering;
    let total = 1;
    for (i++; i < intersections.length && intersections[i].order == order; i++) {
      entering += intersections[i].entering;
      total++;
    }
    // Keep intersections[j] only if every intersection point at that
    // same location has the same "entering" value. This is to exclude
    // places where a corner of one polygon touches but does not cross
    // an edge of the other.
    if (entering == 0 || entering == total) j++;
  }
  // Erase any intersections that were removed.
  intersections.length = j;
  if (!intersections.length) return;

  // Now, also trim any adjacent points with the same "entering" value.
  j = 0;
  let wasEntering = intersections[intersections.length - 1].entering;
  for (let i = 0; i < intersections.length; i++) {
    intersections[j] = intersections[i];
    if (intersections[i].entering != wasEntering) {
      wasEntering = !wasEntering;
      j++;
    }
  }
  intersections.length = j;
}


function amplify(vertices, intersections, amplified, fun) {
  // Remember the index in the "amplified" vector where this part begins.
  // We'll need that later to construct the closing loop.
  const start = amplified.length;
  // When the pOrder reaches this value, we're at a new vertex.
  let index = 0;
  for (const it of intersections) {
    // Output any vertices that are before this intersection.
    for ( ; index * 0.25 <= it.order; index += 4) {
      if (index * 0.25 !== it.order) amplified.push(-1, vertices[index], vertices[index + 1]);
    }
    // Process this intersection.
    fun(it);
  }
  // Add the remaining vertices to the amplified list.
  for ( ; index < vertices.length; index += 4) {
    amplified.push(-1, vertices[index], vertices[index + 1]);
  }
  // Close out the amplified polygon by adding a "link" from the
  // last vertex back to the first one.
  amplified.push(start, 0, 0);
}


function tracePolygon(amplified, index) {
  let j = index;
  const out = [];
  do {
    const link = amplified[j];
    if (link >= 0) {
      amplified[j] = -1;
      j = link;
    }
    else {
      out.push(amplified[j + 1], amplified[j + 2]);
      j += 3;
    }
  } while (j != index);
  return out;
}


// Turn a set of vertices into a set of vertices plus edge vectors.
function expand(points) {
  const ring = [];
  let startX = points[0];
  let startY = points[1];
  for (let i = 2; i < points.length; i += 2) {
    const endX = points[i];
    const endY = points[i + 1];
    ring.push(startX, startY, endX - startX, endY - startY);
    startX = endX;
    startY = endY;
  }
  ring.push(startX, startY, points[0] - startX, points[1] - startY);
  return ring;
}


// Check if a ring contains a point.
function contains(ring, point) {
  const result = windingNumber(ring, point.x, point.y);
  return (result.winding || result.border);
}


// Get the area of a ring.
function area(ring) {
  // Break the polygon up into triangles and add up their areas. The triangles
  // are each formed using an edge of this ring as one edge, and connecting
  // them all to the same arbitrary point as the third vertex. That vertex
  // does not even need to be inside the polygon, so simplify the math by just
  // using the origin as the third point.
  
  // The area of a triangle is half the cross product of the two vectors
  // coming out of any of the vertices.
  let area = 0;
  for (let i = 0; i < ring.length; i += 4) {
    // The cross product is: startX * endY - startY * endX
    //   = x * (y + dy) - y * (x + dx)
    //   = x * y + x * dy - x * y - dx * y
    //   = x * dy - dx * y
    area += ring[i] * ring[i + 3] - ring[i + 1] * ring[i + 2];
  }
  return 0.5 * area;
}

function isHole(ring) {
  return area(ring) < 0;
}

// This helper function is only for use by Polygon. Given a ring that does not
// intersect this ring, check if it is inside this one.
function ringContainsRing(ring, other) {
  const result = windingNumber(ring, other[0], other[1]);
  return result.winding && (!result.border || area(ring) > area(other));
}

// Get the "winding number" and number of boundary intersections for the given point.
// This is returned as a {winding, border} object.
function windingNumber(ring, px, py) {
  let winding = 0;
  let border = 0;
  for (let i = 0; i < ring.length; i += 4) {
    // Special case: if this is a horizontal edge, the "border" value still needs to
    // be updated.
    const x = ring[i];
    const y = ring[i + 1];
    const dx = ring[i + 2];
    const dy = ring[i + 3];
    if (!dy) {
      if (y === py) border += (x + Math.min(0, dx) <= px && px <= x + Math.max(0, dx));
      continue;
    }
    const startsBelow = y <= py;
    const endsBelow = y + dy <= py;
    if (startsBelow == endsBelow) continue;
    
    const cross = (px - x) * dy - (py - y) * dx;
    winding += (!endsBelow && cross <= 0);
    winding -= (endsBelow && cross > 0);
    border += !cross;
  }
  return {winding, border};
}
