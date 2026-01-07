import * as THREE from "three";
import { Character } from "../engine/character.js";

export class Enemy extends Character {
  constructor(position = new THREE.Vector3(12, 0, -6)) {
    super({ radius: 0.7, maxHealth: 120 });
    this.group.position.copy(position);

    this.animTime = 0;
    this.castInterval = 2.8;
    this.castDuration = 0.6;
    this.castFireDelay = 0.25;
    this.castCooldown = 1.2;
    this.castElapsed = 0;
    this.castFired = false;
    this.isCasting = false;

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

  update(delta, world, player) {
    if (!world || !player || this.isDead || player.isDead) return;

    const toPlayer = player.group.position.clone().sub(this.group.position);
    toPlayer.y = 0;
    if (toPlayer.lengthSq() > 0.001) {
      const heading = Math.atan2(toPlayer.x, toPlayer.z);
      this.group.rotation.y = heading;
    }

    const seesPlayer = world.hasLineOfSight(
      this.group.position,
      player.group.position,
      0.2
    );

    if (seesPlayer) {
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
    }

    this._updateAnimation(delta);
  }

  onDeath() {
    this.group.visible = false;
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
      size: 0.22
    });
  }

  _updateAnimation(delta) {
    this.animTime += delta;
    const castBlend = this.isCasting
      ? Math.min(1, this.castElapsed / 0.35)
      : 0;
    const idleBob = Math.sin(this.animTime * 2.4) * 0.04;

    const armCast = -1.1;
    const armIdle = Math.sin(this.animTime * 1.2) * 0.12;

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
    this.parts.torso.rotation.x = -castBlend * 0.2;

    const orbPulse = castBlend * (0.7 + Math.sin(this.animTime * 8) * 0.2);
    this.parts.castOrb.visible = castBlend > 0.05;
    this.parts.castOrb.scale.setScalar(orbPulse);

    this.parts.torso.position.y = 1.6 + idleBob;
    this.parts.head.position.y = 2.45 + idleBob * 0.5;
    this.parts.cloak.position.y = 1.45 + idleBob * 0.4;
  }
}
