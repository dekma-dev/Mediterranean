import * as THREE from "three";
import { Input } from "./engine/input.js";
import { startLoop } from "./engine/loop.js";
import { ThirdPersonCamera } from "./engine/third_person_camera.js";
import { World } from "./game/world.js";
import { Player } from "./game/player.js";
import { Robber } from "./game/robber.js";

const renderer = new THREE.WebGLRenderer({ antialias: true });
renderer.setPixelRatio(window.devicePixelRatio || 1);
renderer.setSize(window.innerWidth, window.innerHeight);
renderer.shadowMap.enabled = true;
document.body.appendChild(renderer.domElement);

const scene = new THREE.Scene();
scene.fog = new THREE.Fog(0x0d1216, 18, 45);

const player = new Player();
scene.add(player.group);

const world = new World(scene);
const enemies = [
  new Robber(new THREE.Vector3(14, 0, -12)),
  new Robber(new THREE.Vector3(-14, 0, -10)),
  new Robber(new THREE.Vector3(12, 0, 12)),
  new Robber(new THREE.Vector3(-12, 0, 8))
];
enemies.forEach((enemy) => scene.add(enemy.group));
let targetEnemy = null;

const camera = new THREE.PerspectiveCamera(
  60,
  window.innerWidth / window.innerHeight,
  0.1,
  200
);
const followCam = new ThirdPersonCamera(camera, player);

const input = new Input();
const hpFill = document.getElementById("hp-fill");
const hpText = document.getElementById("hp-text");
const enemyHud = document.getElementById("enemy-hud");
const enemyName = document.getElementById("enemy-name");
const enemyHpFill = document.getElementById("enemy-hp-fill");
const enemyHpText = document.getElementById("enemy-hp-text");
const skillDesc = document.getElementById("skill-desc");
const skillSlots = Array.from(document.querySelectorAll("[data-skill]")).map(
  (slot) => ({
    slot,
    key: slot.dataset.skill,
    requiresTarget: slot.dataset.requiresTarget === "true",
    range: slot.dataset.range ? Number.parseFloat(slot.dataset.range) : null,
    desc: slot.dataset.desc || "",
    label: slot.childNodes[0]?.textContent?.trim() || "Skill",
    cooldown: slot.querySelector(".cooldown"),
    text: slot.querySelector(".cooldown-text")
  })
);

skillSlots.forEach(({ slot, key, desc, label, requiresTarget, range }) => {
  slot.addEventListener("mouseenter", () => showSkillDesc(label, desc));
  slot.addEventListener("mouseleave", hideSkillDesc);
  slot.addEventListener("click", (event) => {
    event.preventDefault();
    if (
      requiresTarget &&
      (!targetEnemy || targetEnemy.isDead || !isTargetInRange(range))
    ) {
      return;
    }
    input.trigger(key);
    showSkillDesc(label, desc);
  });
});

function onResize() {
  camera.aspect = window.innerWidth / window.innerHeight;
  camera.updateProjectionMatrix();
  renderer.setSize(window.innerWidth, window.innerHeight);
}

window.addEventListener("resize", onResize);

startLoop((delta) => {
  if (input.consumePressed("target")) {
    targetEnemy = findTargetEnemy(player, enemies, 40);
  }
  if (input.consumePressed("clearTarget")) {
    targetEnemy = null;
  }

  if (targetEnemy && targetEnemy.isDead) {
    targetEnemy = null;
  }
  if (!targetEnemy) {
    input.clearPressed(["skill1", "skill2"]);
  }

  player.update(input, delta, world, camera, enemies, targetEnemy);
  enemies.forEach((enemy) => enemy.update(delta, world, player));
  world.update(delta, { player, enemies });
  followCam.update(delta);
  renderer.render(scene, camera);

  const skills = player.getSkills();
  skillSlots.forEach(({ key, cooldown, text }) => {
    const skill = skills[key];
    if (!skill) return;
    const ratio = skill.remaining / skill.cooldown;
    const percent = Math.min(100, Math.max(0, ratio * 100));
    cooldown.style.height = `${percent}%`;
    text.textContent = skill.remaining > 0 ? `${skill.remaining.toFixed(1)}s` : "";
  });

  const hpRatio = player.getHealthRatio();
  const hpPercent = Math.round(hpRatio * 100);
  if (hpFill) {
    hpFill.style.width = `${Math.max(0, Math.min(100, hpPercent))}%`;
  }
  if (hpText) {
    hpText.textContent = `${hpPercent}%`;
  }

  updateEnemyHud(targetEnemy);
  updateSkillAvailability(!!targetEnemy);
});

function showSkillDesc(label, desc) {
  if (!skillDesc) return;
  const text = desc ? `${label}: ${desc}` : label;
  skillDesc.textContent = text;
  skillDesc.classList.add("visible");
}

function hideSkillDesc() {
  if (!skillDesc) return;
  skillDesc.classList.remove("visible");
}

function updateSkillAvailability(hasTarget) {
  skillSlots.forEach(({ slot, requiresTarget, range }) => {
    const outOfRange = requiresTarget && hasTarget && !isTargetInRange(range);
    slot.classList.toggle(
      "disabled",
      requiresTarget && (!hasTarget || outOfRange)
    );
  });
}

function isTargetInRange(range) {
  if (!range || !targetEnemy) return true;
  const distSq = targetEnemy.group.position.distanceToSquared(
    player.group.position
  );
  return distSq <= range * range;
}

function findTargetEnemy(player, enemies, maxDistance) {
  let closest = null;
  let closestDistSq = maxDistance * maxDistance;
  enemies.forEach((enemy) => {
    if (!enemy || enemy.isDead) return;
    const distSq = enemy.group.position.distanceToSquared(
      player.group.position
    );
    if (distSq <= closestDistSq) {
      closestDistSq = distSq;
      closest = enemy;
    }
  });
  return closest;
}

function updateEnemyHud(enemy) {
  if (!enemyHud) return;
  if (!enemy) {
    enemyHud.classList.add("hidden");
    return;
  }

  enemyHud.classList.remove("hidden");
  if (enemyName) {
    enemyName.textContent = enemy.name || "Enemy";
  }
  const hpRatio = enemy.getHealthRatio();
  const hpPercent = Math.round(hpRatio * 100);
  if (enemyHpFill) {
    enemyHpFill.style.width = `${Math.max(0, Math.min(100, hpPercent))}%`;
  }
  if (enemyHpText) {
    enemyHpText.textContent = `${hpPercent}%`;
  }
}
