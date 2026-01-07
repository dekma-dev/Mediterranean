import * as THREE from "three";
import { Input } from "./engine/input.js";
import { startLoop } from "./engine/loop.js";
import { ThirdPersonCamera } from "./engine/third_person_camera.js";
import { World } from "./game/world.js";
import { Player } from "./game/player.js";
import { Enemy } from "./game/enemy.js";

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
const enemies = [new Enemy()];
enemies.forEach((enemy) => scene.add(enemy.group));

const camera = new THREE.PerspectiveCamera(
  60,
  window.innerWidth / window.innerHeight,
  0.1,
  200
);
const followCam = new ThirdPersonCamera(camera, player);

const input = new Input();
const hpValue = document.getElementById("hp");
const skillSlots = Array.from(document.querySelectorAll("[data-skill]")).map(
  (slot) => ({
    slot,
    key: slot.dataset.skill,
    cooldown: slot.querySelector(".cooldown"),
    text: slot.querySelector(".cooldown-text")
  })
);

function onResize() {
  camera.aspect = window.innerWidth / window.innerHeight;
  camera.updateProjectionMatrix();
  renderer.setSize(window.innerWidth, window.innerHeight);
}

window.addEventListener("resize", onResize);

startLoop((delta) => {
  player.update(input, delta, world, camera);
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

  if (hpValue) {
    hpValue.textContent = Math.round(player.getHealthRatio() * 100);
  }
});
