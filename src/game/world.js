import * as THREE from "three";

export class World {
  constructor(scene) {
    this.scene = scene;
    this.mapSize = { width: 80, depth: 80 };
    this.obstacles = [];
    this.obstacleBounds = [];
    this.projectiles = [];

    this._createLights();
    this._createGround();
    this._createProps();
    this._createWalls();
  }

  update(delta, entities = {}) {
    this._updateProjectiles(delta, entities);
  }

  _createLights() {
    const ambient = new THREE.AmbientLight(0x9bb7b0, 0.9);
    this.scene.add(ambient);

    const sun = new THREE.DirectionalLight(0xdff6ff, 1.6);
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
      20,
      20
    );
    const material = new THREE.MeshStandardMaterial({
      color: 0x3f7a3b,
      roughness: 0.85
    });
    const ground = new THREE.Mesh(geometry, material);
    ground.receiveShadow = true;
    ground.rotation.x = -Math.PI / 2;
    this.scene.add(ground);

  }

  _createObstacles() {}

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

  _createProps() {
    const treeTrunk = new THREE.MeshStandardMaterial({
      color: 0x5a3b27,
      roughness: 0.85
    });
    const treeLeaves = new THREE.MeshStandardMaterial({
      color: 0x2f6d3f,
      roughness: 0.8
    });
    const rockMaterial = new THREE.MeshStandardMaterial({
      color: 0x4a5a5c,
      roughness: 0.7
    });
    const bushMaterial = new THREE.MeshStandardMaterial({
      color: 0x2f6b45,
      roughness: 0.85
    });

    this._scatterProps(treeTrunk, treeLeaves, rockMaterial, bushMaterial);
    this._createGrassPatches();
  }

  _scatterProps(treeTrunk, treeLeaves, rockMaterial, bushMaterial) {
    const halfWidth = this.mapSize.width / 2 - 6;
    const halfDepth = this.mapSize.depth / 2 - 6;
    const minDistance = 2;
    const spawnClearRadius = 4;
    const placed = [];
    const tryPlace = (radius) => {
      for (let attempt = 0; attempt < 40; attempt += 1) {
        const x = THREE.MathUtils.randFloat(-halfWidth, halfWidth);
        const z = THREE.MathUtils.randFloat(-halfDepth, halfDepth);
        if (x * x + z * z < spawnClearRadius * spawnClearRadius) {
          continue;
        }
        const ok = placed.every((p) => {
          const dx = x - p.x;
          const dz = z - p.z;
          const minDist = p.radius + radius + minDistance;
          return dx * dx + dz * dz >= minDist * minDist;
        });
        if (ok) {
          placed.push({ x, z, radius });
          return { x, z };
        }
      }
      return null;
    };

    for (let i = 0; i < 40; i += 1) {
      const scale = THREE.MathUtils.randFloat(0.8, 1.3);
      const radius = 0.85 * scale;
      const spot = tryPlace(radius);
      if (!spot) continue;
      const trunk = new THREE.Mesh(
        new THREE.CylinderGeometry(0.35, 0.5, 2.8, 8),
        treeTrunk
      );
      const crown = new THREE.Mesh(
        new THREE.ConeGeometry(1.4, 2.5, 8),
        treeLeaves
      );
      trunk.position.set(spot.x, 1.4, spot.z);
      crown.position.set(spot.x, 3.4, spot.z);
      trunk.castShadow = true;
      crown.castShadow = true;
      trunk.scale.setScalar(scale);
      crown.scale.setScalar(scale);
      this.scene.add(trunk, crown);
      this._addObstacleBounds(
        spot.x - radius,
        spot.x + radius,
        spot.z - radius,
        spot.z + radius
      );
    }

    for (let i = 0; i < 48; i += 1) {
      const size = THREE.MathUtils.randFloat(0.6, 1.3);
      const radius = 0.7;
      const spot = tryPlace(radius);
      if (!spot) continue;
      const rock = new THREE.Mesh(
        new THREE.DodecahedronGeometry(size, 0),
        rockMaterial
      );
      rock.position.set(spot.x, 0.8, spot.z);
      rock.castShadow = true;
      rock.receiveShadow = true;
      this.scene.add(rock);
      this._addObstacleBounds(
        spot.x - radius,
        spot.x + radius,
        spot.z - radius,
        spot.z + radius
      );
    }

    for (let i = 0; i < 46; i += 1) {
      const size = THREE.MathUtils.randFloat(0.8, 1.4);
      const radius = 0.65;
      const spot = tryPlace(radius);
      if (!spot) continue;
      const bush = new THREE.Mesh(
        new THREE.SphereGeometry(size, 10, 10),
        bushMaterial
      );
      bush.position.set(spot.x, 0.9, spot.z);
      bush.castShadow = true;
      bush.receiveShadow = true;
      this.scene.add(bush);
      this._addObstacleBounds(
        spot.x - radius,
        spot.x + radius,
        spot.z - radius,
        spot.z + radius
      );
    }
  }

  _createGrassPatches() {
    const bladeMaterial = new THREE.MeshStandardMaterial({
      color: 0x3c7a4e,
      roughness: 0.85
    });
    const bladeGeometry = new THREE.ConeGeometry(0.12, 0.6, 5);
    const halfWidth = this.mapSize.width / 2 - 3;
    const halfDepth = this.mapSize.depth / 2 - 3;
    const count = 2600;

    const grass = new THREE.InstancedMesh(
      bladeGeometry,
      bladeMaterial,
      count
    );
    grass.castShadow = true;
    grass.receiveShadow = true;
    const dummy = new THREE.Object3D();
    for (let i = 0; i < count; i += 1) {
      const x = THREE.MathUtils.randFloat(-halfWidth, halfWidth);
      const z = THREE.MathUtils.randFloat(-halfDepth, halfDepth);
      const scale = THREE.MathUtils.randFloat(0.7, 1.25);
      dummy.position.set(x, 0.3, z);
      dummy.rotation.y = THREE.MathUtils.randFloat(0, Math.PI * 2);
      dummy.scale.set(scale, scale, scale);
      dummy.updateMatrix();
      grass.setMatrixAt(i, dummy.matrix);
    }
    this.scene.add(grass);
  }

  _createWalls() {
    const inset = 3;
    const halfWidth = this.mapSize.width / 2 - inset;
    const halfDepth = this.mapSize.depth / 2 - inset;
    const wallThickness = 2;
    const wallHeight = 4;
    const logRadius = 0.45;
    const logLength = 1.8;
    const logMaterial = new THREE.MeshStandardMaterial({
      color: 0x7a542f,
      roughness: 0.8
    });
    const logGeometry = new THREE.CylinderGeometry(
      logRadius,
      logRadius,
      logLength,
      10
    );

    const createLogRow = (count, startX, startZ, dx, dz, y, rotY) => {
      for (let i = 0; i < count; i += 1) {
        const log = new THREE.Mesh(logGeometry, logMaterial);
        log.position.set(startX + dx * i, y, startZ + dz * i);
        log.rotation.z = Math.PI / 2;
        log.rotation.y = rotY;
        log.castShadow = true;
        log.receiveShadow = true;
        this.scene.add(log);
      }
    };

    const rows = Math.floor(wallHeight / (logRadius * 1.5));
    const northZ = -halfDepth - wallThickness / 2;
    const southZ = halfDepth + wallThickness / 2;
    const westX = -halfWidth - wallThickness / 2;
    const eastX = halfWidth + wallThickness / 2;
    const spanX = halfWidth * 2;
    const spanZ = halfDepth * 2;
    const countX = Math.ceil(spanX / logLength) + 1;
    const countZ = Math.ceil(spanZ / logLength) + 1;
    const actualSpanX = (countX - 1) * logLength;
    const actualSpanZ = (countZ - 1) * logLength;
    const offset = logRadius * 1.1;

    for (let r = 0; r < rows; r += 1) {
      const y = logRadius + r * logRadius * 1.5;
      const startX = -actualSpanX / 2;
      const startZ = -actualSpanZ / 2;
      createLogRow(countX, startX, northZ - offset, logLength, 0, y, 0);
      createLogRow(countX, startX, northZ + offset, logLength, 0, y, 0);
      createLogRow(countX, startX, southZ - offset, logLength, 0, y, 0);
      createLogRow(countX, startX, southZ + offset, logLength, 0, y, 0);

      createLogRow(countZ, westX - offset, startZ, 0, logLength, y, Math.PI / 2);
      createLogRow(countZ, westX + offset, startZ, 0, logLength, y, Math.PI / 2);
      createLogRow(countZ, eastX - offset, startZ, 0, logLength, y, Math.PI / 2);
      createLogRow(countZ, eastX + offset, startZ, 0, logLength, y, Math.PI / 2);
    }

    this._addObstacleBounds(
      -halfWidth - wallThickness * 0.4,
      halfWidth + wallThickness * 0.4,
      northZ - wallThickness * 0.4,
      northZ + wallThickness * 0.4
    );
    this._addObstacleBounds(
      -halfWidth - wallThickness * 0.4,
      halfWidth + wallThickness * 0.4,
      southZ - wallThickness * 0.4,
      southZ + wallThickness * 0.4
    );
    this._addObstacleBounds(
      westX - wallThickness * 0.4,
      westX + wallThickness * 0.4,
      -halfDepth - wallThickness * 0.4,
      halfDepth + wallThickness * 0.4
    );
    this._addObstacleBounds(
      eastX - wallThickness * 0.4,
      eastX + wallThickness * 0.4,
      -halfDepth - wallThickness * 0.4,
      halfDepth + wallThickness * 0.4
    );
  }

  _addObstacleBounds(minX, maxX, minZ, maxZ) {
    this.obstacleBounds.push({ minX, maxX, minZ, maxZ });
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

  isWalkable(position, radius = 0.6, entities = [], self = null) {
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

    if (this._hitsObstacle(position, radius)) {
      return false;
    }

    if (entities && entities.length) {
      for (const entity of entities) {
        if (!entity || entity === self || entity.isDead) continue;
        const dx = position.x - entity.group.position.x;
        const dz = position.z - entity.group.position.z;
        const minDist = radius + (entity.radius ?? 0.6);
        if (dx * dx + dz * dz < minDist * minDist) {
          return false;
        }
      }
    }

    return true;
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
