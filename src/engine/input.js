const KEY_MAP = {
  ArrowUp: "up",
  ArrowDown: "down",
  ArrowLeft: "left",
  ArrowRight: "right",
  KeyW: "up",
  KeyS: "down",
  KeyA: "left",
  KeyD: "right",
  Digit1: "skill1",
  Digit2: "skill2",
  Digit3: "skill3",
  Digit4: "skill4",
  Space: "jump",
  ShiftLeft: "sprint",
  ShiftRight: "sprint"
};

export class Input {
  constructor() {
    this.state = new Map();
    this.pressed = new Set();
    this._onKeyDown = (event) => this._setKey(event.code, true);
    this._onKeyUp = (event) => this._setKey(event.code, false);
    window.addEventListener("keydown", this._onKeyDown);
    window.addEventListener("keyup", this._onKeyUp);
  }

  _setKey(code, pressed) {
    const mapped = KEY_MAP[code];
    if (!mapped) return;
    if (pressed && !this.state.get(mapped)) {
      this.pressed.add(mapped);
    }
    this.state.set(mapped, pressed);
  }

  isDown(key) {
    return this.state.get(key) === true;
  }

  consumePressed(key) {
    if (!this.pressed.has(key)) return false;
    this.pressed.delete(key);
    return true;
  }
}
