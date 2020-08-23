/** Static class for remembering user-defined text styles.
 * @typedef {object} Style
 */
export class Style {
  /** Load a dialog definition from the given data file.
   * @param {Data} data
   */
  static load(data) {
    const name = data.value();
    // If this style has no name, it is the default paragraph style.
    let rule = (name.length ? ".text-" + name : "p") + " {";
    while (data.next() && data.size()) {
      if (data.tag() == "family") {
        rule += `font-family:'${data.value()}';`
      }
      else if (data.tag() === "size") {
        rule += `font-size:${+data.arg(1)}px;`;
      }
      else if (data.tag() === "bold") {
        rule += "font-weight:bold;";
      }
      else if (data.tag() === "italic") {
        rule += "font-style:italic;";
      }
      else if (data.tag() === "normal") {
        rule += "font-weight:normal;font-style:normal;";
      }
      else if (data.tag() === "color") {
        rule += `color:rgb(${+data.arg(1)}, ${+data.arg(2)}, ${+data.arg(3)});`;
      }
    }
    // Close the style rule, and add it to the document.
    rule += "}";
    const style = document.createElement("style");
    style.innerHTML = rule;
    document.body.append(style);
  }
  /** Apply styling to the given text to produce one or more paragraphs.
   * @param {string} text The text, which may contain tags in curly brackets.
   * @param {string=} initial The initial style to use.
   * @returns {string} An HTML string with styles applied.
   */
  static apply(text, initial = "") {
    let current = initial;
    let result = startP(current);

    // Parse the string, finding {} tags one at a time.
    const length = text.length;
    let wasSpace = true;
    for (let i = 0; i < length; i++) {
      // First, find any text before the next {} tag.
      // Also break out if a single or double quote is found.
      let start = i;
      while (i < length && !`{'"`.includes(text[i])) {
        wasSpace = (text[i] == ' ');
        i++;
      }
      result += text.slice(start, i);

      if (i == length) break;
      if (text[i] !== '{') {
        // Insert curly quotes. This is a UTF-8 HTML document, so there is no need
        // to use HTML entities; instead just use the proper UTF-8 characters:
        result += (text[i] == '"' ? (wasSpace ? '“' : '”') : (wasSpace ? '‘' : '’'));
        continue;
      }
      // Skip past the opening bracket.
      i++;
      start = i;
      while (i < length && text[i] != '}') i++;
      if (i == length) break;
      // Get the "tag" within the brackets.
      const tag = text.slice(start, i);

      // Apply the tag.
      if (tag === "br") result += endP(current) + startP(current);
      else {
        result += endSpan(current);
        // If a tag is repeated, it toggles between that style and the default one.
        current = (tag === current ? initial : tag);
        // Add a style tag only if not using the default style.
        result += startSpan(current);
      }
    }
    return result + endP(current);
  }
}


function startP(style) {
  return "<p>" + startSpan(style);
}

function endP(style) {
  return endSpan(style) + "</p>";
}

function startSpan(style) {
  return style.length ? `<span class="text-${style}">` : "";
}

function endSpan(style) {
  return style.length ? "</span>" : "";
}
