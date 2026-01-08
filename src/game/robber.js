import * as THREE from "three";
import { Character } from "../engine/character.js";

export class Robber extends Character {
  constructor(position = new THREE.Vector3(12, 0, -6)) {
    super({ radius: 0.7, maxHealth: 120, hitOffsetY: 1.25 });
    this.group.position.copy(position);

    this.animTime = 0;
    this.name = "Robber";
    this.castInterval = 2.8;
    this.castDuration = 0.6;
    this.castFireDelay = 0.25;
    this.castCooldown = 1.2;
    this.castElapsed = 0;
    this.castFired = false;
    this.isCasting = false;
    this.patrolSpeed = 1.6;
    this.angrySpeed = 2.4;
    this.patrolThreshold = 0.25;
    this.aggroRange = 10;
    this.preferredRange = 12;
    this.patrolPoints = [];
    this.patrolIndex = 0;
    this.avoidDirection = new THREE.Vector3(1, 0, 0);
    this.avoidTimer = 0;
    this.stuckTimer = 0;
    this.desiredMove = false;

    const skinMaterial = new THREE.MeshStandardMaterial({
      color: 0x6d3b3b,
      roughness: 0.7
    });
    const robeMaterial = new THREE.MeshStandardMaterial({
      color: 0x3b1d1d,
      roughness: 0.9
    });
    const armorMaterial = new THREE.MeshStandardMaterial({
      color: 0x5a1a1a,
      roughness: 0.4,
      metalness: 0.25
    });
    const orbMaterial = new THREE.MeshStandardMaterial({
      color: 0xff7a3d,
      emissive: 0x8a1e00,
      roughness: 0.2
    });

    const hips = new THREE.Mesh(
      new THREE.BoxGeometry(0.95, 0.55, 0.6),
      robeMaterial
    );
    hips.position.y = 0.78;
    hips.castShadow = true;

    const torso = new THREE.Mesh(
      new THREE.BoxGeometry(1.1, 1.3, 0.7),
      armorMaterial
    );
    torso.position.y = 1.6;
    torso.castShadow = true;

    const head = new THREE.Mesh(
      new THREE.BoxGeometry(0.55, 0.6, 0.55),
      skinMaterial
    );
    head.position.y = 2.45;
    head.castShadow = true;

    const leftArm = new THREE.Mesh(
      new THREE.BoxGeometry(0.28, 1.05, 0.28),
      armorMaterial
    );
    leftArm.position.set(-0.75, 1.6, 0);
    leftArm.castShadow = true;

    const rightArm = leftArm.clone();
    rightArm.position.x = 0.75;

    const leftLeg = new THREE.Mesh(
      new THREE.BoxGeometry(0.33, 1.05, 0.33),
      robeMaterial
    );
    leftLeg.position.set(-0.28, 0.25, 0);
    leftLeg.castShadow = true;

    const rightLeg = leftLeg.clone();
    rightLeg.position.x = 0.28;

    const cloak = new THREE.Mesh(
      new THREE.BoxGeometry(1.3, 1.7, 0.14),
      robeMaterial
    );
    cloak.position.set(0, 1.45, -0.36);
    cloak.castShadow = true;

    const castOrb = new THREE.Mesh(
      new THREE.SphereGeometry(0.14, 12, 12),
      orbMaterial
    );
    castOrb.position.set(0, -0.35, 0.25);
    castOrb.visible = false;

    rightArm.add(castOrb);

    this.group.add(
      hips,
      torso,
      head,
      leftArm,
      rightArm,
      leftLeg,
      rightLeg,
      cloak
    );

    this.parts = {
      torso,
      head,
      leftArm,
      rightArm,
      leftLeg,
      rightLeg,
      cloak,
      castOrb
    };
  }

  update(delta, world, player, enemies = []) {
    if (!world || !player) return;
    if (this.isDead) {
      this._updateDeath(delta);
      return;
    }
    if (player.isDead) return;
    this._ensurePatrolPoints(world);

    const distanceSq = this.group.position.distanceToSquared(
      player.group.position
    );
    const seesPlayer =
      distanceSq <= this.aggroRange * this.aggroRange &&
      world.hasLineOfSight(this.group.position, player.group.position, 0.2);
    let isMoving = false;

    if (seesPlayer) {
      const toPlayer = player.group.position.clone().sub(this.group.position);
      toPlayer.y = 0;
      if (toPlayer.lengthSq() > 0.001) {
        const heading = Math.atan2(toPlayer.x, toPlayer.z);
        this.group.rotation.y = heading;
      }
    }

    if (seesPlayer) {
      isMoving = this._updateChase(delta, world, player, enemies);
      if (this.isCasting) {
        this.castElapsed += delta;
        if (!this.castFired && this.castElapsed >= this.castFireDelay) {
          this._fireball(world, player);
          this.castFired = true;
        }
        if (this.castElapsed >= this.castDuration) {
          this.isCasting = false;
          this.castCooldown = this.castInterval;
        }
      } else {
        this.castCooldown = Math.max(0, this.castCooldown - delta);
        if (this.castCooldown <= 0) {
          this.isCasting = true;
          this.castElapsed = 0;
          this.castFired = false;
        }
      }
    } else {
      this.isCasting = false;
      this.castElapsed = 0;
      isMoving = this._updatePatrol(delta, world, enemies, player);
    }

    if (this.desiredMove && !isMoving) {
      this.stuckTimer += delta;
      if (this.stuckTimer > 1.2) {
        this._startAvoidance(world, enemies, player);
        if (this.patrolPoints.length > 0) {
          this.patrolIndex = (this.patrolIndex + 1) % this.patrolPoints.length;
        }
        this.stuckTimer = 0;
      }
    } else {
      this.stuckTimer = 0;
    }

    this._updateAnimation(delta, seesPlayer, isMoving);
  }

  onDeath() {
    this.isCasting = false;
    this.castElapsed = 0;
    this.castCooldown = this.castInterval;
    this.deathQuat = this.group.quaternion.clone();
    const axis = new THREE.Vector3(Math.random() - 0.5, 0, Math.random() - 0.5);
    if (axis.lengthSq() < 0.01) {
      axis.set(1, 0, 0);
    }
    this.fallAxis = axis.normalize();
    this.fallAngle = 0;
  }

  _fireball(world, player) {
    const spawn = this.group.position
      .clone()
      .add(new THREE.Vector3(0, 1.65, 0));
    const toPlayer = player.group.position.clone().sub(spawn);
    toPlayer.y = Math.max(0.2, toPlayer.y);
    const direction =
      toPlayer.lengthSq() > 0.001
        ? toPlayer.normalize()
        : new THREE.Vector3(0, 0, 1).applyQuaternion(this.group.quaternion);

    spawn.addScaledVector(direction, 0.7);

    world.spawnProjectile(spawn, direction, {
      color: 0xff6b4a,
      emissive: 0x7a1300,
      owner: "enemy",
      damage: 15,
      speed: 9,
      size: 0.22,
      type: "fireball",
      target: player,
      targetOffsetY: player.hitOffsetY ?? 1,
      turnRate: 6
    });
  }

  _updateAnimation(delta, seesPlayer, isMoving) {
    const animSpeed = seesPlayer ? 1 : 1.6;
    this.animTime += delta * animSpeed;
    const castBlend = this.isCasting
      ? Math.min(1, this.castElapsed / 0.35)
      : 0;
    const bobStrength = isMoving ? 0.08 : 0.04;
    const idleBob = Math.sin(this.animTime * 2.4) * bobStrength;

    const walkSwing = isMoving ? Math.sin(this.animTime * 6) * 0.7 : 0;
    const armCast = -1.1;
    const armIdle = Math.sin(this.animTime * 1.2) * 0.12 + walkSwing * 0.2;

    this.parts.leftArm.rotation.x = THREE.MathUtils.lerp(
      armIdle,
      armCast,
      castBlend
    );
    this.parts.rightArm.rotation.x = THREE.MathUtils.lerp(
      -armIdle,
      armCast,
      castBlend
    );
    this.parts.leftLeg.rotation.x = walkSwing;
    this.parts.rightLeg.rotation.x = -walkSwing;
    this.parts.torso.rotation.x = -castBlend * 0.2;

    const orbPulse = castBlend * (0.7 + Math.sin(this.animTime * 8) * 0.2);
    this.parts.castOrb.visible = castBlend > 0.05;
    this.parts.castOrb.scale.setScalar(orbPulse);

    this.parts.torso.position.y = 1.6 + idleBob;
    this.parts.head.position.y = 2.45 + idleBob * 0.5;
    this.parts.cloak.position.y = 1.45 + idleBob * 0.4;
  }

  _buildPatrolPoints(origin, world) {
    const points = [];
    const baseAngle = Math.random() * Math.PI * 2;
    const radii = [3.5, 4.5, 3, 4];
    for (let i = 0; i < 4; i += 1) {
      const angle = baseAngle + (Math.PI / 2) * i + (Math.random() - 0.5) * 0.7;
      const radius = radii[i] + (Math.random() - 0.5) * 1.2;
      const candidate = origin
        .clone()
        .add(
          new THREE.Vector3(Math.cos(angle) * radius, 0, Math.sin(angle) * radius)
        );
      if (!world || world.isWalkable(candidate, this.radius)) {
        points.push(candidate);
      }
    }
    return points;
  }

  _ensurePatrolPoints(world) {
    if (this.patrolPoints.length > 0) return;
    let points = this._buildPatrolPoints(this.group.position, world);
    if (points.length === 0) {
      const backup = [];
      for (let i = 0; i < 4; i += 1) {
        const angle = Math.random() * Math.PI * 2;
        const radius = 2 + Math.random() * 3;
        const candidate = this.group.position
          .clone()
          .add(
            new THREE.Vector3(
              Math.cos(angle) * radius,
              0,
              Math.sin(angle) * radius
            )
          );
        if (world.isWalkable(candidate, this.radius)) {
          backup.push(candidate);
        }
      }
      points = backup;
    }
    this.patrolPoints = points;
    this.patrolIndex = 0;
  }

  _updatePatrol(delta, world, enemies, player) {
    if (this.patrolPoints.length === 0) return false;
    if (this._updateAvoidance(delta, world, enemies, player)) {
      this.desiredMove = true;
      return true;
    }
    const target = this.patrolPoints[this.patrolIndex];
    const toTarget = target.clone().sub(this.group.position);
    toTarget.y = 0;
    const distance = toTarget.length();
    if (distance <= this.patrolThreshold) {
      this.desiredMove = false;
      this.patrolIndex = (this.patrolIndex + 1) % this.patrolPoints.length;
      return false;
    }

    this.desiredMove = true;
    const direction = toTarget.normalize();
    const lookAhead = this.group.position
      .clone()
      .addScaledVector(direction, 1.2);
    const blockers = [player, ...enemies];
    if (!world.isWalkable(lookAhead, this.radius, blockers, this)) {
      this._startAvoidance(world, enemies, player);
      return this._updateAvoidance(delta, world, enemies, player);
    }
    const speed = this.angrySpeed;
    const step = speed * delta;
    const moveVector = direction.clone().multiplyScalar(step);
    const moved = this._tryMove(world, moveVector, blockers);
    if (moved) {
      const heading = Math.atan2(direction.x, direction.z);
      this.group.rotation.y = heading;
      return true;
    }

    this._startAvoidance(world, enemies, player);
    this.patrolIndex = (this.patrolIndex + 1) % this.patrolPoints.length;
    return false;
  }

  _updateChase(delta, world, player, enemies) {
    const toTarget = player.group.position.clone().sub(this.group.position);
    toTarget.y = 0;
    const distance = toTarget.length();
    if (distance <= this.preferredRange) {
      this.desiredMove = false;
      return false;
    }

    this.desiredMove = true;
    if (this._updateAvoidance(delta, world, enemies, player)) {
      return true;
    }
    const direction = toTarget.normalize();
    const lookAhead = this.group.position
      .clone()
      .addScaledVector(direction, 1.4);
    const blockers = [player, ...enemies];
    if (!world.isWalkable(lookAhead, this.radius, blockers, this)) {
      this._startAvoidance(world, enemies, player);
      return this._updateAvoidance(delta, world, enemies, player);
    }
    const step = this.angrySpeed * delta;
    const moveVector = direction.clone().multiplyScalar(step);
    const moved = this._tryMove(world, moveVector, blockers);
    if (moved) {
      const heading = Math.atan2(direction.x, direction.z);
      this.group.rotation.y = heading;
      return true;
    }

    this._startAvoidance(world, enemies, player);
    return false;
  }

  _updateDeath(delta) {
    if (!this.fallAxis) return;
    if (this.fallAngle >= Math.PI / 2) return;
    this.fallAngle = Math.min(Math.PI / 2, this.fallAngle + delta * 2.2);
    const fallQuat = new THREE.Quaternion().setFromAxisAngle(
      this.fallAxis,
      this.fallAngle
    );
    this.group.quaternion.copy(this.deathQuat).multiply(fallQuat);
  }

  _tryMove(world, moveVector, blockers) {
    if (!world) return false;
    const nextPosition = this.group.position.clone().add(moveVector);
    if (world.isWalkable(nextPosition, this.radius, blockers, this)) {
      this.group.position.copy(nextPosition);
      return true;
    }

    const slideX = new THREE.Vector3(moveVector.x, 0, 0);
    const slideZ = new THREE.Vector3(0, 0, moveVector.z);
    const nextX = this.group.position.clone().add(slideX);
    if (slideX.lengthSq() > 0 && world.isWalkable(nextX, this.radius, blockers, this)) {
      this.group.position.copy(nextX);
      return true;
    }

    const nextZ = this.group.position.clone().add(slideZ);
    if (slideZ.lengthSq() > 0 && world.isWalkable(nextZ, this.radius, blockers, this)) {
      this.group.position.copy(nextZ);
      return true;
    }
    return false;
  }

  _startAvoidance(world, enemies, player) {
    const blockers = [player, ...enemies];
    const base = Math.random() * Math.PI * 2;
    for (let i = 0; i < 6; i += 1) {
      const angle = base + (i - 3) * 0.45;
      const dir = new THREE.Vector3(Math.sin(angle), 0, Math.cos(angle));
      const testPos = this.group.position.clone().addScaledVector(dir, 1.2);
      if (world.isWalkable(testPos, this.radius, blockers, this)) {
        this.avoidDirection = dir;
        this.avoidTimer = THREE.MathUtils.randFloat(0.5, 1.2);
        return;
      }
    }
    this.avoidDirection = new THREE.Vector3(
      Math.sin(base),
      0,
      Math.cos(base)
    );
    this.avoidTimer = 0.4;
  }

  _updateAvoidance(delta, world, enemies, player) {
    if (this.avoidTimer <= 0) return false;
    const blockers = [player, ...enemies];
    const step = this.angrySpeed * delta;
    const moveVector = this.avoidDirection.clone().multiplyScalar(step);
    const moved = this._tryMove(world, moveVector, blockers);
    if (!moved) {
      this._startAvoidance(world, enemies, player);
    }
    this.avoidTimer = Math.max(0, this.avoidTimer - delta);
    return moved;
  }
}
