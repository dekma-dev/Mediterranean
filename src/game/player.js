import * as THREE from "three";
import { Character } from "../engine/character.js";

export class Player extends Character {
  constructor() {
    super({ radius: 0.6, maxHealth: 100, hitOffsetY: 1.2 });
    this.group.position.set(0, 0, 0);
    this.speed = 4;
    this.jumpSpeed = 6.5;
    this.gravity = -18;
    this.velocityY = 0;
    this.isGrounded = true;
    this.animTime = 0;
    this.parts = {};
    this.attackTimer = 0;
    this.dashTimer = 0;
    this.dashDirection = new THREE.Vector3(0, 0, 1);
    this.dashSpeed = 14;
    this.maxMana = 1000;
    this.mana = this.maxMana;
    this.manaRegen = 10;
    this.skills = {
      skill1: { name: "slash", cooldown: 1, remaining: 0 },
      skill2: { name: "bolt", cooldown: 4, remaining: 0 },
      skill3: { name: "dash", cooldown: 4, remaining: 0 },
      skill4: { name: "heal", cooldown: 10, remaining: 0 }
    };

    const skinMaterial = new THREE.MeshStandardMaterial({
      color: 0xd9b36c,
      roughness: 0.6
    });
    const clothMaterial = new THREE.MeshStandardMaterial({
      color: 0x5d2b2b,
      roughness: 0.8
    });
    const armorMaterial = new THREE.MeshStandardMaterial({
      color: 0x3a4b4a,
      roughness: 0.5,
      metalness: 0.2
    });

    const hips = new THREE.Mesh(
      new THREE.BoxGeometry(0.9, 0.5, 0.5),
      clothMaterial
    );
    hips.position.y = 0.75;
    hips.castShadow = true;

    const torso = new THREE.Mesh(
      new THREE.BoxGeometry(1.0, 1.2, 0.6),
      armorMaterial
    );
    torso.position.y = 1.55;
    torso.castShadow = true;

    const head = new THREE.Mesh(
      new THREE.BoxGeometry(0.5, 0.55, 0.5),
      skinMaterial
    );
    head.position.y = 2.35;
    head.castShadow = true;

    const leftArm = new THREE.Mesh(
      new THREE.BoxGeometry(0.25, 0.95, 0.25),
      armorMaterial
    );
    leftArm.position.set(-0.7, 1.55, 0);
    leftArm.castShadow = true;

    const rightArm = leftArm.clone();
    rightArm.position.x = 0.7;

    const leftLeg = new THREE.Mesh(
      new THREE.BoxGeometry(0.3, 1.0, 0.3),
      clothMaterial
    );
    leftLeg.position.set(-0.25, 0.25, 0);
    leftLeg.castShadow = true;

    const rightLeg = leftLeg.clone();
    rightLeg.position.x = 0.25;

    const cloak = new THREE.Mesh(
      new THREE.BoxGeometry(1.2, 1.6, 0.12),
      clothMaterial
    );
    cloak.position.set(0, 1.35, -0.32);
    cloak.castShadow = true;

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
      cloak
    };
  }

  update(input, delta, world, camera, enemies = [], targetEnemy = null) {
    if (this.isDead) {
      this._updateAnimation(delta, false);
      return;
    }
    this._regenMana(delta);
    const moveX =
      (input.isDown("right") ? 1 : 0) - (input.isDown("left") ? 1 : 0);
    const moveZ = (input.isDown("down") ? 1 : 0) - (input.isDown("up") ? 1 : 0);

    const direction = new THREE.Vector3();
    if (camera) {
      const forward = new THREE.Vector3(0, 0, -1).applyQuaternion(
        camera.quaternion
      );
      forward.y = 0;
      forward.normalize();

      const right = new THREE.Vector3().crossVectors(
        forward,
        new THREE.Vector3(0, 1, 0)
      );

      direction.addScaledVector(right, moveX);
      direction.addScaledVector(forward, -moveZ);
    } else {
      direction.set(moveX, 0, moveZ);
    }

    const moving = direction.lengthSq() > 0;
    const baseDirection = moving ? direction.clone().normalize() : null;
    const cameraForward = this._getCameraForward(camera);

    this._updateCooldowns(delta);
    this._handleSkills(
      input,
      world,
      camera,
      baseDirection,
      cameraForward,
      enemies,
      targetEnemy
    );

    if (this.dashTimer > 0) {
      this.dashTimer = Math.max(0, this.dashTimer - delta);
    }

    let moveDirection = null;
    let targetSpeed = 0;

    if (this.dashTimer > 0) {
      moveDirection = this.dashDirection;
      targetSpeed = this.dashSpeed;
    } else if (moving && baseDirection) {
      moveDirection = baseDirection;
      targetSpeed = this.speed;
    }

    if (moveDirection) {
      const distance = targetSpeed * delta;
      const moveVector = moveDirection.clone().multiplyScalar(distance);
      this._tryMove(world, moveVector, enemies);

      const heading = Math.atan2(moveDirection.x, moveDirection.z);
      this.group.rotation.y = heading;
    }

    if (input.isDown("jump") && this.isGrounded) {
      this.velocityY = this.jumpSpeed;
      this.isGrounded = false;
    }

    this.velocityY += this.gravity * delta;
    this.group.position.y += this.velocityY * delta;

    if (this.group.position.y <= 0) {
      this.group.position.y = 0;
      this.velocityY = 0;
      this.isGrounded = true;
    }

    this._updateAnimation(delta, moveDirection !== null);
  }

  getSkills() {
    return this.skills;
  }

  getManaRatio() {
    return this.maxMana > 0 ? this.mana / this.maxMana : 0;
  }

  _getCameraForward(camera) {
    const forward = new THREE.Vector3(0, 0, -1);
    if (!camera) return forward;
    forward.applyQuaternion(camera.quaternion);
    forward.y = 0;
    if (forward.lengthSq() === 0) return new THREE.Vector3(0, 0, 1);
    return forward.normalize();
  }

  _updateCooldowns(delta) {
    Object.values(this.skills).forEach((skill) => {
      if (skill.remaining > 0) {
        skill.remaining = Math.max(0, skill.remaining - delta);
      }
    });
  }

  _handleSkills(
    input,
    world,
    camera,
    baseDirection,
    cameraForward,
    enemies,
    targetEnemy
  ) {
    const hasTarget = !!targetEnemy && !targetEnemy.isDead;
    if (input.consumePressed("skill1")) {
      if (hasTarget && this._isTargetInRange(targetEnemy, 1.5)) {
        this._tryUseSkill("skill1", () =>
          this._doSlash(enemies, targetEnemy)
        );
      }
    }
    if (input.consumePressed("skill2")) {
      if (hasTarget && this._isTargetInRange(targetEnemy, 40)) {
        this._tryUseSkill("skill2", () =>
          this._doBolt(world, camera, cameraForward, targetEnemy)
        , 150);
      }
    }
    if (input.consumePressed("skill3")) {
      this._tryUseSkill("skill3", () =>
        this._doDash(baseDirection, cameraForward)
      );
    }
    if (input.consumePressed("skill4")) {
      this._tryUseSkill("skill4", () => this._doHeal());
    }
  }

  _tryUseSkill(key, action, manaCost = 0) {
    const skill = this.skills[key];
    if (!skill || skill.remaining > 0) return;
    if (manaCost > 0 && !this._consumeMana(manaCost)) return;
    skill.remaining = skill.cooldown;
    action();
  }

  _doSlash(enemies, targetEnemy) {
    this.attackTimer = 0.25;
    const slashRange = 1.5;
    const slashDamage = 25;

    const target = this._getSlashTarget(enemies, targetEnemy, slashRange);
    if (target) {
      target.takeDamage(slashDamage);
    }
  }

  _getSlashTarget(enemies, targetEnemy, range) {
    if (targetEnemy && !targetEnemy.isDead) {
      const distSq = targetEnemy.group.position.distanceToSquared(
        this.group.position
      );
      if (distSq <= range * range) {
        return targetEnemy;
      }
    }

    let closest = null;
    let closestDistSq = range * range;
    enemies.forEach((enemy) => {
      if (!enemy || enemy.isDead) return;
      const distSq = enemy.group.position.distanceToSquared(
        this.group.position
      );
      if (distSq <= closestDistSq) {
        closestDistSq = distSq;
        closest = enemy;
      }
    });
    return closest;
  }

  _isTargetInRange(targetEnemy, range) {
    if (!targetEnemy) return false;
    const distSq = targetEnemy.group.position.distanceToSquared(
      this.group.position
    );
    return distSq <= range * range;
  }

  _doBolt(world, camera, cameraForward, targetEnemy) {
    if (!world) return;
    const spawn = this.group.position.clone().add(new THREE.Vector3(0, 1.6, 0));
    const aim = new THREE.Vector3(0, 0, -1);
    if (targetEnemy) {
      aim.copy(targetEnemy.group.position);
      aim.y += targetEnemy.hitOffsetY ?? 1;
      aim.sub(spawn);
    } else if (camera) {
      camera.getWorldDirection(aim);
    } else {
      aim.copy(cameraForward);
    }
    world.spawnProjectile(spawn, aim, {
      owner: "player",
      damage: 20,
      maxAge: 40 / 14,
      target: targetEnemy ?? null,
      targetOffsetY: targetEnemy?.hitOffsetY ?? 0,
      turnRate: 8
    });
  }

  _doDash(baseDirection, cameraForward) {
    const dir =
      baseDirection && baseDirection.lengthSq() > 0 ? baseDirection : cameraForward;
    if (dir.lengthSq() === 0) return;
    this.dashDirection = dir.clone().normalize();
    this.dashTimer = 0.2;
  }

  _doHeal() {
    const amount = 35;
    this.health = Math.min(this.maxHealth, this.health + amount);
  }

  _tryMove(world, moveVector, enemies) {
    if (!world) return;
    const nextPosition = this.group.position.clone().add(moveVector);
    if (world.isWalkable(nextPosition, this.radius, enemies, this)) {
      this.group.position.copy(nextPosition);
      return;
    }

    const slideX = new THREE.Vector3(moveVector.x, 0, 0);
    const slideZ = new THREE.Vector3(0, 0, moveVector.z);
    const nextX = this.group.position.clone().add(slideX);
    if (slideX.lengthSq() > 0 && world.isWalkable(nextX, this.radius, enemies, this)) {
      this.group.position.copy(nextX);
      return;
    }

    const nextZ = this.group.position.clone().add(slideZ);
    if (slideZ.lengthSq() > 0 && world.isWalkable(nextZ, this.radius, enemies, this)) {
      this.group.position.copy(nextZ);
    }
  }

  _regenMana(delta) {
    if (this.mana >= this.maxMana) return;
    this.mana = Math.min(this.maxMana, this.mana + this.manaRegen * delta);
  }

  _consumeMana(cost) {
    if (this.mana < cost) return false;
    this.mana -= cost;
    return true;
  }

  _updateAnimation(delta, moving) {
    this.animTime += delta * (moving ? 8 : 2);

    const swing = Math.sin(this.animTime) * (moving ? 0.8 : 0.15);
    const oppositeSwing =
      Math.sin(this.animTime + Math.PI) * (moving ? 0.8 : 0.15);
    const jumpBlend = this.isGrounded ? 0 : 1;

    const runArm = swing;
    const runLeg = oppositeSwing;
    const jumpArm = -0.8;
    const jumpLeg = 0.6;

    const attackBlend = this.attackTimer > 0 ? 1 : 0;
    if (this.attackTimer > 0) {
      this.attackTimer = Math.max(0, this.attackTimer - delta);
    }
    const attackArm = -1.4;

    const leftArmX = THREE.MathUtils.lerp(
      THREE.MathUtils.lerp(runArm, jumpArm, jumpBlend),
      attackArm,
      attackBlend
    );
    const rightArmX = THREE.MathUtils.lerp(
      THREE.MathUtils.lerp(oppositeSwing, jumpArm, jumpBlend),
      attackArm,
      attackBlend
    );
    const leftLegX = THREE.MathUtils.lerp(runLeg, jumpLeg, jumpBlend);
    const rightLegX = THREE.MathUtils.lerp(swing, jumpLeg, jumpBlend);

    this.parts.leftArm.rotation.x = leftArmX;
    this.parts.rightArm.rotation.x = rightArmX;
    this.parts.leftLeg.rotation.x = leftLegX;
    this.parts.rightLeg.rotation.x = rightLegX;

    const bob = moving ? Math.abs(Math.sin(this.animTime)) * 0.05 : 0.02;
    this.parts.torso.position.y = 1.55 + bob;
    this.parts.head.position.y = 2.35 + bob * 0.5;
    this.parts.cloak.position.y = 1.35 + bob * 0.4;
  }

}
