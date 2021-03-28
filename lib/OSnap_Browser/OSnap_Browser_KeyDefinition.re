type t = {
  keyCode: option(int),
  key: string,
  code: string,
  text: string,
  location: int,
};

let make =
  fun
  | '0' =>
    Some({
      keyCode: Some(48),
      key: "0",
      code: "Digit0",
      text: "0",
      location: 0,
    })
  | '1' =>
    Some({
      keyCode: Some(49),
      key: "1",
      code: "Digit1",
      text: "1",
      location: 0,
    })
  | '2' =>
    Some({
      keyCode: Some(50),
      key: "2",
      code: "Digit2",
      text: "2",
      location: 0,
    })
  | '3' =>
    Some({
      keyCode: Some(51),
      key: "3",
      code: "Digit3",
      text: "3",
      location: 0,
    })
  | '4' =>
    Some({
      keyCode: Some(52),
      key: "4",
      code: "Digit4",
      text: "4",
      location: 0,
    })
  | '5' =>
    Some({
      keyCode: Some(53),
      key: "5",
      code: "Digit5",
      text: "5",
      location: 0,
    })
  | '6' =>
    Some({
      keyCode: Some(54),
      key: "6",
      code: "Digit6",
      text: "6",
      location: 0,
    })
  | '7' =>
    Some({
      keyCode: Some(55),
      key: "7",
      code: "Digit7",
      text: "7",
      location: 0,
    })
  | '8' =>
    Some({
      keyCode: Some(56),
      key: "8",
      code: "Digit8",
      text: "8",
      location: 0,
    })
  | '9' =>
    Some({
      keyCode: Some(57),
      key: "9",
      code: "Digit9",
      text: "9",
      location: 0,
    })
  | ' ' =>
    Some({
      keyCode: Some(32),
      key: " ",
      code: "Space",
      text: " ",
      location: 0,
    })
  | 'a' =>
    Some({keyCode: Some(65), key: "a", code: "KeyA", text: "a", location: 0})
  | 'b' =>
    Some({keyCode: Some(66), key: "b", code: "KeyB", text: "b", location: 0})
  | 'c' =>
    Some({keyCode: Some(67), key: "c", code: "KeyC", text: "c", location: 0})
  | 'd' =>
    Some({keyCode: Some(68), key: "d", code: "KeyD", text: "d", location: 0})
  | 'e' =>
    Some({keyCode: Some(69), key: "e", code: "KeyE", text: "e", location: 0})
  | 'f' =>
    Some({keyCode: Some(70), key: "f", code: "KeyF", text: "f", location: 0})
  | 'g' =>
    Some({keyCode: Some(71), key: "g", code: "KeyG", text: "g", location: 0})
  | 'h' =>
    Some({keyCode: Some(72), key: "h", code: "KeyH", text: "h", location: 0})
  | 'i' =>
    Some({keyCode: Some(73), key: "i", code: "KeyI", text: "i", location: 0})
  | 'j' =>
    Some({keyCode: Some(74), key: "j", code: "KeyJ", text: "j", location: 0})
  | 'k' =>
    Some({keyCode: Some(75), key: "k", code: "KeyK", text: "k", location: 0})
  | 'l' =>
    Some({keyCode: Some(76), key: "l", code: "KeyL", text: "l", location: 0})
  | 'm' =>
    Some({keyCode: Some(77), key: "m", code: "KeyM", text: "m", location: 0})
  | 'n' =>
    Some({keyCode: Some(78), key: "n", code: "KeyN", text: "n", location: 0})
  | 'o' =>
    Some({keyCode: Some(79), key: "o", code: "KeyO", text: "o", location: 0})
  | 'p' =>
    Some({keyCode: Some(80), key: "p", code: "KeyP", text: "p", location: 0})
  | 'q' =>
    Some({keyCode: Some(81), key: "q", code: "KeyQ", text: "q", location: 0})
  | 'r' =>
    Some({keyCode: Some(82), key: "r", code: "KeyR", text: "r", location: 0})
  | 's' =>
    Some({keyCode: Some(83), key: "s", code: "KeyS", text: "s", location: 0})
  | 't' =>
    Some({keyCode: Some(84), key: "t", code: "KeyT", text: "t", location: 0})
  | 'u' =>
    Some({keyCode: Some(85), key: "u", code: "KeyU", text: "u", location: 0})
  | 'v' =>
    Some({keyCode: Some(86), key: "v", code: "KeyV", text: "v", location: 0})
  | 'w' =>
    Some({keyCode: Some(87), key: "w", code: "KeyW", text: "w", location: 0})
  | 'x' =>
    Some({keyCode: Some(88), key: "x", code: "KeyX", text: "x", location: 0})
  | 'y' =>
    Some({keyCode: Some(89), key: "y", code: "KeyY", text: "y", location: 0})
  | 'z' =>
    Some({keyCode: Some(90), key: "z", code: "KeyZ", text: "z", location: 0})
  | '*' =>
    Some({
      keyCode: Some(106),
      key: "*",
      code: "NumpadMultiply",
      text: "*",
      location: 3,
    })
  | '+' =>
    Some({
      keyCode: Some(107),
      key: "+",
      code: "NumpadAdd",
      text: "+",
      location: 3,
    })
  | '-' =>
    Some({
      keyCode: Some(109),
      key: "-",
      code: "NumpadSubtract",
      text: "-",
      location: 3,
    })
  | '/' =>
    Some({
      keyCode: Some(111),
      key: "/",
      code: "NumpadDivide",
      text: "/",
      location: 3,
    })
  | ';' =>
    Some({
      keyCode: Some(186),
      key: ";",
      code: "Semicolon",
      text: ";",
      location: 0,
    })
  | '=' =>
    Some({
      keyCode: Some(187),
      key: "=",
      code: "Equal",
      text: "=",
      location: 0,
    })
  | ',' =>
    Some({
      keyCode: Some(188),
      key: ",",
      code: "Comma",
      text: ",",
      location: 0,
    })
  | '.' =>
    Some({
      keyCode: Some(190),
      key: ".",
      code: "Period",
      text: ".",
      location: 0,
    })
  | '`' =>
    Some({
      keyCode: Some(192),
      key: "`",
      code: "Backquote",
      text: "`",
      location: 0,
    })
  | '[' =>
    Some({
      keyCode: Some(219),
      key: "[",
      code: "BracketLeft",
      text: "[",
      location: 0,
    })
  | '\\' =>
    Some({
      keyCode: Some(220),
      key: "\\",
      code: "Backslash",
      text: "\\",
      location: 0,
    })
  | ']' =>
    Some({
      keyCode: Some(221),
      key: "]",
      code: "BracketRight",
      text: "]",
      location: 0,
    })
  | '\'' =>
    Some({
      keyCode: Some(222),
      key: "'",
      code: "Quote",
      text: "'",
      location: 0,
    })
  | ')' =>
    Some({
      keyCode: Some(48),
      key: ")",
      code: "Digit0",
      text: ")",
      location: 0,
    })
  | '!' =>
    Some({
      keyCode: Some(49),
      key: "!",
      code: "Digit1",
      text: "!",
      location: 0,
    })
  | '@' =>
    Some({
      keyCode: Some(50),
      key: "@",
      code: "Digit2",
      text: "@",
      location: 0,
    })
  | '#' =>
    Some({
      keyCode: Some(51),
      key: "#",
      code: "Digit3",
      text: "#",
      location: 0,
    })
  | '$' =>
    Some({
      keyCode: Some(52),
      key: "$",
      code: "Digit4",
      text: "$",
      location: 0,
    })
  | '%' =>
    Some({
      keyCode: Some(53),
      key: "%",
      code: "Digit5",
      text: "%",
      location: 0,
    })
  | '^' =>
    Some({
      keyCode: Some(54),
      key: "^",
      code: "Digit6",
      text: "^",
      location: 0,
    })
  | '&' =>
    Some({
      keyCode: Some(55),
      key: "&",
      code: "Digit7",
      text: "&",
      location: 0,
    })
  | '(' =>
    Some({
      keyCode: Some(57),
      key: "(",
      code: "Digit9",
      text: "(",
      location: 0,
    })
  | 'A' =>
    Some({keyCode: Some(65), key: "A", code: "KeyA", text: "A", location: 0})
  | 'B' =>
    Some({keyCode: Some(66), key: "B", code: "KeyB", text: "B", location: 0})
  | 'C' =>
    Some({keyCode: Some(67), key: "C", code: "KeyC", text: "C", location: 0})
  | 'D' =>
    Some({keyCode: Some(68), key: "D", code: "KeyD", text: "D", location: 0})
  | 'E' =>
    Some({keyCode: Some(69), key: "E", code: "KeyE", text: "E", location: 0})
  | 'F' =>
    Some({keyCode: Some(70), key: "F", code: "KeyF", text: "F", location: 0})
  | 'G' =>
    Some({keyCode: Some(71), key: "G", code: "KeyG", text: "G", location: 0})
  | 'H' =>
    Some({keyCode: Some(72), key: "H", code: "KeyH", text: "H", location: 0})
  | 'I' =>
    Some({keyCode: Some(73), key: "I", code: "KeyI", text: "I", location: 0})
  | 'J' =>
    Some({keyCode: Some(74), key: "J", code: "KeyJ", text: "J", location: 0})
  | 'K' =>
    Some({keyCode: Some(75), key: "K", code: "KeyK", text: "K", location: 0})
  | 'L' =>
    Some({keyCode: Some(76), key: "L", code: "KeyL", text: "L", location: 0})
  | 'M' =>
    Some({keyCode: Some(77), key: "M", code: "KeyM", text: "M", location: 0})
  | 'N' =>
    Some({keyCode: Some(78), key: "N", code: "KeyN", text: "N", location: 0})
  | 'O' =>
    Some({keyCode: Some(79), key: "O", code: "KeyO", text: "O", location: 0})
  | 'P' =>
    Some({keyCode: Some(80), key: "P", code: "KeyP", text: "P", location: 0})
  | 'Q' =>
    Some({keyCode: Some(81), key: "Q", code: "KeyQ", text: "Q", location: 0})
  | 'R' =>
    Some({keyCode: Some(82), key: "R", code: "KeyR", text: "R", location: 0})
  | 'S' =>
    Some({keyCode: Some(83), key: "S", code: "KeyS", text: "S", location: 0})
  | 'T' =>
    Some({keyCode: Some(84), key: "T", code: "KeyT", text: "T", location: 0})
  | 'U' =>
    Some({keyCode: Some(85), key: "U", code: "KeyU", text: "U", location: 0})
  | 'V' =>
    Some({keyCode: Some(86), key: "V", code: "KeyV", text: "V", location: 0})
  | 'W' =>
    Some({keyCode: Some(87), key: "W", code: "KeyW", text: "W", location: 0})
  | 'X' =>
    Some({keyCode: Some(88), key: "X", code: "KeyX", text: "X", location: 0})
  | 'Y' =>
    Some({keyCode: Some(89), key: "Y", code: "KeyY", text: "Y", location: 0})
  | 'Z' =>
    Some({keyCode: Some(90), key: "Z", code: "KeyZ", text: "Z", location: 0})
  | ':' =>
    Some({
      keyCode: Some(186),
      key: ":",
      code: "Semicolon",
      text: ":",
      location: 0,
    })
  | '<' =>
    Some({
      keyCode: Some(188),
      key: "<",
      code: "Comma",
      text: "<",
      location: 0,
    })
  | '_' =>
    Some({
      keyCode: Some(189),
      key: "_",
      code: "Minus",
      text: "_",
      location: 0,
    })
  | '>' =>
    Some({
      keyCode: Some(190),
      key: ">",
      code: "Period",
      text: ">",
      location: 0,
    })
  | '?' =>
    Some({
      keyCode: Some(191),
      key: "?",
      code: "Slash",
      text: "?",
      location: 0,
    })
  | '~' =>
    Some({
      keyCode: Some(192),
      key: "~",
      code: "Backquote",
      text: "~",
      location: 0,
    })
  | '{' =>
    Some({
      keyCode: Some(219),
      key: "{",
      code: "BracketLeft",
      text: "{",
      location: 0,
    })
  | '|' =>
    Some({
      keyCode: Some(220),
      key: "|",
      code: "Backslash",
      text: "|",
      location: 0,
    })
  | '}' =>
    Some({
      keyCode: Some(221),
      key: "}",
      code: "BracketRight",
      text: "}",
      location: 0,
    })
  | '"' =>
    Some({
      keyCode: Some(222),
      key: "\"",
      code: "Quote",
      text: "\"",
      location: 0,
    })
  | _ => None;
