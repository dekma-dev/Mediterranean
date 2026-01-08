import * as THREE from "three";

export class ThirdPersonCamera {
  constructor(camera, target, domElement = null) {
    this.camera = camera;
    this.target = target;
    this.domElement = domElement;
    this.currentPosition = new THREE.Vector3();
    this.currentLookAt = new THREE.Vector3();

    this.distance = 10;
    this.height = 2.2;
    this.pitch = 0.35;
    this.yaw = Math.PI;
    this.sensitivity = 0.003;
    this.minDistance = 4;
    this.maxDistance = 16;
    this.isRotating = false;
    this.lastMouse = null;
    this.isPointerLocked = false;

    this._bindMouse();
  }

  _bindMouse() {
    window.addEventListener("contextmenu", (event) => event.preventDefault());

    window.addEventListener("mousedown", (event) => {
      if (event.button === 2) {
        this.isRotating = true;
        if (this.domElement && this.domElement.requestPointerLock) {
          this.domElement.requestPointerLock();
        } else {
          this.lastMouse = { x: event.clientX, y: event.clientY };
        }
      }
    });

    window.addEventListener("mouseup", (event) => {
      if (event.button === 2) {
        this.isRotating = false;
        this.lastMouse = null;
        if (document.pointerLockElement) {
          document.exitPointerLock();
        }
      }
    });

    document.addEventListener("pointerlockchange", () => {
      this.isPointerLocked = document.pointerLockElement === this.domElement;
      if (!this.isPointerLocked) {
        this.lastMouse = null;
      }
    });

    window.addEventListener("mousemove", (event) => {
      if (!this.isRotating) return;
      let dx = 0;
      let dy = 0;
      if (this.isPointerLocked) {
        dx = event.movementX;
        dy = event.movementY;
      } else {
        if (!this.lastMouse) return;
        dx = event.clientX - this.lastMouse.x;
        dy = event.clientY - this.lastMouse.y;
        this.lastMouse = { x: event.clientX, y: event.clientY };
      }

      this.yaw -= dx * this.sensitivity;
      this.pitch += dy * this.sensitivity;
      this.pitch = Math.max(0.1, Math.min(1.2, this.pitch));
    });

    window.addEventListener("wheel", (event) => {
      const delta = Math.sign(event.deltaY);
      if (delta === 0) return;
      this.distance = THREE.MathUtils.clamp(
        this.distance + delta * 1.2,
        this.minDistance,
        this.maxDistance
      );
    }, { passive: true });
  }

  update(delta) {
    const idealPosition = this._calculateIdealPosition();
    const idealLookAt = this._calculateIdealLookAt();

    const positionLerp = 1 - Math.pow(0.001, delta);
    const lookAtLerp = 1 - Math.pow(0.001, delta);

    this.currentPosition.lerp(idealPosition, positionLerp);
    this.currentLookAt.lerp(idealLookAt, lookAtLerp);

    this.camera.position.copy(this.currentPosition);
    this.camera.lookAt(this.currentLookAt);
  }

  _calculateIdealPosition() {
    const orbit = new THREE.Vector3(
      Math.sin(this.yaw) * Math.cos(this.pitch),
      Math.sin(this.pitch),
      Math.cos(this.yaw) * Math.cos(this.pitch)
    );

    const targetPosition = this.target.group.position.clone();
    return targetPosition
      .clone()
      .addScaledVector(orbit, this.distance)
      .add(new THREE.Vector3(0, this.height, 0));
  }

  _calculateIdealLookAt() {
    return this.target.group.position.clone().add(new THREE.Vector3(0, 1.8, 0));
  }
}
