import * as THREE from "three";

export class World {
  constructor(scene) {
    this.scene = scene;
    this.mapSize = { width: 40, depth: 40 };
    this.obstacles = [];
    this.obstacleBounds = [];
    this.projectiles = [];

    this._createLights();
    this._createGround();
    this._createObstacles();
  }

  update(delta, entities = {}) {
    this._updateProjectiles(delta, entities);
  }

  _createLights() {
    const ambient = new THREE.AmbientLight(0x9bb7b0, 0.5);
    this.scene.add(ambient);

    const sun = new THREE.DirectionalLight(0xdff6ff, 1.1);
    sun.position.set(12, 18, 8);
    sun.castShadow = true;
    sun.shadow.mapSize.set(1024, 1024);
    sun.shadow.camera.left = -25;
    sun.shadow.camera.right = 25;
    sun.shadow.camera.top = 25;
    sun.shadow.camera.bottom = -25;
    this.scene.add(sun);
  }

  _createGround() {
    const geometry = new THREE.PlaneGeometry(
      this.mapSize.width,
      this.mapSize.depth,
      10,
      10
    );
    const material = new THREE.MeshStandardMaterial({
      color: 0x1b3a36,
      roughness: 0.9
    });
    const ground = new THREE.Mesh(geometry, material);
    ground.receiveShadow = true;
    ground.rotation.x = -Math.PI / 2;
    this.scene.add(ground);

    const grid = new THREE.GridHelper(
      this.mapSize.width,
      20,
      0x3a5e59,
      0x223632
    );
    grid.position.y = 0.01;
    this.scene.add(grid);
  }

  _createObstacles() {
    const obstacleData = [
      { x: -10, z: -6, w: 4, d: 6, h: 4 },
      { x: 8, z: -2, w: 6, d: 4, h: 3 },
      { x: 4, z: 10, w: 8, d: 3, h: 2.5 },
      { x: -6, z: 10, w: 3, d: 6, h: 5 }
    ];

    const material = new THREE.MeshStandardMaterial({
      color: 0x2b5c55,
      roughness: 0.7
    });

    obstacleData.forEach((data) => {
      const geometry = new THREE.BoxGeometry(data.w, data.h, data.d);
      const mesh = new THREE.Mesh(geometry, material);
      mesh.position.set(data.x, data.h / 2, data.z);
      mesh.castShadow = true;
      mesh.receiveShadow = true;
      this.scene.add(mesh);
      this.obstacles.push(mesh);
      this.obstacleBounds.push({
        minX: data.x - data.w / 2,
        maxX: data.x + data.w / 2,
        minZ: data.z - data.d / 2,
        maxZ: data.z + data.d / 2
      });
    });
  }

  spawnProjectile(position, direction, options = {}) {
    const {
      color = 0x6be5ff,
      emissive = 0x1b6a78,
      roughness = 0.3,
      speed = 14,
      size = 0.18,
      owner = "neutral",
      damage = 0,
      hitRadius = null,
      type = "orb",
      maxAge = null,
      target = null,
      targetOffsetY = 0,
      turnRate = 6
    } = options;
    const mesh =
      type === "fireball"
        ? this._createFireballMesh(size, color, emissive)
        : this._createOrbMesh(size, color, emissive, roughness);
    mesh.position.copy(position);
    mesh.castShadow = true;
    this.scene.add(mesh);

    const dir = direction.clone().normalize();
    const velocity = dir.clone().multiplyScalar(speed);
    const quat = new THREE.Quaternion().setFromUnitVectors(
      new THREE.Vector3(0, 0, 1),
      dir
    );
    mesh.quaternion.copy(quat);
    this.projectiles.push({
      mesh,
      velocity,
      age: 0,
      radius: hitRadius ?? size * 1.1,
      owner,
      damage,
      pulseSpeed: type === "fireball" ? 9 : 0,
      maxAge,
      target,
      targetOffsetY,
      turnRate
    });
  }

  _updateProjectiles(delta, { player, enemies = [] } = {}) {
    this.projectiles = this.projectiles.filter((projectile) => {
      projectile.age += delta;
      if (projectile.target && !projectile.target.isDead) {
        const targetPos = projectile.target.group.position.clone();
        targetPos.y += projectile.targetOffsetY || 0;
        const toTarget = targetPos.sub(projectile.mesh.position);
        if (toTarget.lengthSq() > 0.0001) {
          const desiredDir = toTarget.normalize();
          const currentDir = projectile.velocity.clone().normalize();
          const turn = Math.min(1, delta * projectile.turnRate);
          currentDir.lerp(desiredDir, turn).normalize();
          const speed = projectile.velocity.length();
          projectile.velocity.copy(currentDir.multiplyScalar(speed));
          projectile.mesh.quaternion.setFromUnitVectors(
            new THREE.Vector3(0, 0, 1),
            currentDir
          );
        }
      }

      projectile.mesh.position.addScaledVector(projectile.velocity, delta);
      if (projectile.pulseSpeed) {
        const pulse = 1 + Math.sin(projectile.age * projectile.pulseSpeed) * 0.12;
        projectile.mesh.scale.setScalar(pulse);
      }
      if (this._hitsObstacle(projectile.mesh.position, projectile.radius)) {
        this._disposeProjectile(projectile);
        return false;
      }

      if (
        projectile.owner === "enemy" &&
        player &&
        !player.isDead &&
        this._hitsEntity(
          projectile,
          player.group.position,
          player.radius,
          player.hitOffsetY
        )
      ) {
        player.takeDamage(projectile.damage);
        this._disposeProjectile(projectile);
        return false;
      }

      if (projectile.owner === "player" && enemies.length > 0) {
        const hitEnemy = enemies.find(
          (enemy) =>
            enemy &&
            !enemy.isDead &&
            this._hitsEntity(
              projectile,
              enemy.group.position,
              enemy.radius,
              enemy.hitOffsetY
            )
        );
        if (hitEnemy) {
          hitEnemy.takeDamage(projectile.damage);
          this._disposeProjectile(projectile);
          return false;
        }
      }

      const maxAge = projectile.maxAge ?? 2.5;
      if (projectile.age > maxAge) {
        this._disposeProjectile(projectile);
        return false;
      }
      return true;
    });
  }

  hasLineOfSight(from, to, padding = 0.1) {
    const ax = from.x;
    const az = from.z;
    const bx = to.x;
    const bz = to.z;
    return !this.obstacleBounds.some((box) =>
      this._segmentIntersectsBox2D(ax, az, bx, bz, box, padding)
    );
  }

  isWalkable(position, radius = 0.6) {
    const halfWidth = this.mapSize.width / 2 - radius;
    const halfDepth = this.mapSize.depth / 2 - radius;
    if (
      position.x < -halfWidth ||
      position.x > halfWidth ||
      position.z < -halfDepth ||
      position.z > halfDepth
    ) {
      return false;
    }

    return !this._hitsObstacle(position, radius);
  }

  _hitsObstacle(position, radius) {
    const minX = position.x - radius;
    const maxX = position.x + radius;
    const minZ = position.z - radius;
    const maxZ = position.z + radius;

    return this.obstacleBounds.some((box) => {
      const overlapX = maxX > box.minX && minX < box.maxX;
      const overlapZ = maxZ > box.minZ && minZ < box.maxZ;
      return overlapX && overlapZ;
    });
  }

  _hitsEntity(projectile, targetPosition, targetRadius, offsetY = 0) {
    const dx = projectile.mesh.position.x - targetPosition.x;
    const dy = projectile.mesh.position.y - (targetPosition.y + offsetY);
    const dz = projectile.mesh.position.z - targetPosition.z;
    const radius = projectile.radius + targetRadius;
    return dx * dx + dy * dy + dz * dz <= radius * radius;
  }

  _segmentIntersectsBox2D(ax, az, bx, bz, box, padding) {
    const minX = box.minX - padding;
    const maxX = box.maxX + padding;
    const minZ = box.minZ - padding;
    const maxZ = box.maxZ + padding;

    const dx = bx - ax;
    const dz = bz - az;
    let tMin = 0;
    let tMax = 1;

    if (Math.abs(dx) < 1e-6) {
      if (ax < minX || ax > maxX) return false;
    } else {
      const invDx = 1 / dx;
      let t1 = (minX - ax) * invDx;
      let t2 = (maxX - ax) * invDx;
      if (t1 > t2) [t1, t2] = [t2, t1];
      tMin = Math.max(tMin, t1);
      tMax = Math.min(tMax, t2);
      if (tMin > tMax) return false;
    }

    if (Math.abs(dz) < 1e-6) {
      if (az < minZ || az > maxZ) return false;
    } else {
      const invDz = 1 / dz;
      let t1 = (minZ - az) * invDz;
      let t2 = (maxZ - az) * invDz;
      if (t1 > t2) [t1, t2] = [t2, t1];
      tMin = Math.max(tMin, t1);
      tMax = Math.min(tMax, t2);
      if (tMin > tMax) return false;
    }

    return true;
  }

  _disposeProjectile(projectile) {
    this.scene.remove(projectile.mesh);
    projectile.mesh.traverse((child) => {
      if (!child.isMesh) return;
      child.geometry?.dispose();
      if (Array.isArray(child.material)) {
        child.material.forEach((mat) => mat.dispose());
      } else {
        child.material?.dispose();
      }
    });
  }

  _createOrbMesh(size, color, emissive, roughness) {
    const geometry = new THREE.SphereGeometry(size, 12, 12);
    const material = new THREE.MeshStandardMaterial({
      color,
      emissive,
      roughness
    });
    return new THREE.Mesh(geometry, material);
  }

  _createFireballMesh(size, color, emissive) {
    const group = new THREE.Group();

    const core = new THREE.Mesh(
      new THREE.SphereGeometry(size * 0.55, 14, 14),
      new THREE.MeshStandardMaterial({
        color: 0xffd1a1,
        emissive: 0xff7a3d,
        roughness: 0.2
      })
    );
    const shell = new THREE.Mesh(
      new THREE.SphereGeometry(size, 16, 16),
      new THREE.MeshStandardMaterial({
        color,
        emissive,
        roughness: 0.35,
        transparent: true,
        opacity: 0.75
      })
    );
    const tail = new THREE.Mesh(
      new THREE.ConeGeometry(size * 0.7, size * 2.2, 12),
      new THREE.MeshStandardMaterial({
        color: 0xff5b2e,
        emissive,
        transparent: true,
        opacity: 0.65,
        roughness: 0.6
      })
    );
    tail.rotation.x = Math.PI / 2;
    tail.position.z = -size * 1.35;

    const glow = new THREE.PointLight(0xff6b4a, 0.8, 4);

    group.add(shell, core, tail, glow);
    return group;
  }
}
