import * as THREE from "three";

export class Character {
  constructor({ radius = 0.6, maxHealth = 100 } = {}) {
    this.group = new THREE.Group();
    this.radius = radius;
    this.maxHealth = maxHealth;
    this.health = maxHealth;
    this.isDead = false;
  }

  getHealthRatio() {
    return this.maxHealth > 0 ? this.health / this.maxHealth : 0;
  }

  takeDamage(amount) {
    if (this.isDead) return;
    this.health = Math.max(0, this.health - amount);
    if (this.health <= 0) {
      this.isDead = true;
      this.onDeath();
    }
  }

  onDeath() {}
}
