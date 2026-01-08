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
scene.background = new THREE.Color(0x86b6e8);
scene.fog = new THREE.Fog(0x86b6e8, 30, 140);

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
  400
);
const followCam = new ThirdPersonCamera(camera, player, renderer.domElement);

const input = new Input();
const hpFill = document.getElementById("hp-fill");
const hpText = document.getElementById("hp-text");
const mpFill = document.getElementById("mp-fill");
const mpText = document.getElementById("mp-text");
const enemyHud = document.getElementById("enemy-hud");
const enemyName = document.getElementById("enemy-name");
const enemyHpFill = document.getElementById("enemy-hp-fill");
const enemyHpText = document.getElementById("enemy-hp-text");
const skillDesc = document.getElementById("skill-desc");
const menuOverlay = document.getElementById("menu-overlay");
const gameMenu = document.getElementById("game-menu");
const menuTabs = Array.from(document.querySelectorAll("[data-menu-tab]"));
const menuPanels = Array.from(document.querySelectorAll("[data-menu-panel]"));
const panelCharacter = document.getElementById("panel-character");
const panelInventory = document.getElementById("panel-inventory");
const panelQuests = document.getElementById("panel-quests");
const panelMap = document.getElementById("panel-map");
const characterCanvas = document.getElementById("character-canvas");
const mapCanvas = document.getElementById("map-canvas");
const exitGameButton = document.getElementById("exit-game");
let isMenuOpen = false;
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
  resizeMapCanvas();
}

window.addEventListener("resize", onResize);

let characterRenderer = null;
let characterScene = null;
let characterCamera = null;
let characterModel = null;
let mapCtx = null;
let mapBaseCanvas = null;
let mapBaseCtx = null;
let mapTexture = null;
let mapNeedsRedraw = true;

function initCharacterPreview() {
  if (!characterCanvas) return;
  characterRenderer = new THREE.WebGLRenderer({
    canvas: characterCanvas,
    alpha: true,
    antialias: true
  });
  characterRenderer.setPixelRatio(window.devicePixelRatio || 1);
  characterRenderer.setSize(
    characterCanvas.clientWidth,
    characterCanvas.clientHeight,
    false
  );
  characterScene = new THREE.Scene();
  characterCamera = new THREE.PerspectiveCamera(35, 1, 0.1, 100);
  const ambient = new THREE.AmbientLight(0xffffff, 0.7);
  characterScene.add(ambient);
  const keyLight = new THREE.DirectionalLight(0xffe0b8, 0.8);
  keyLight.position.set(2, 4, 3);
  characterScene.add(keyLight);
  characterModel = player.group.clone(true);
  const box = new THREE.Box3().setFromObject(characterModel);
  const size = new THREE.Vector3();
  const center = new THREE.Vector3();
  box.getSize(size);
  box.getCenter(center);
  characterModel.position.sub(center);
  characterScene.add(characterModel);
  const maxDim = Math.max(size.x, size.y, size.z);
  characterCamera.position.set(0, maxDim * 0.6, maxDim * 2.2);
  characterCamera.lookAt(0, maxDim * 0.4, 0);
  characterCamera.updateProjectionMatrix();
}

function initMap() {
  if (!mapCanvas) return;
  mapCtx = mapCanvas.getContext("2d");
  mapBaseCanvas = document.createElement("canvas");
  mapBaseCtx = mapBaseCanvas.getContext("2d");
  mapTexture = new Image();
  mapTexture.src = "./public/assets/map/world-map.png";
  mapTexture.onload = () => {
    mapNeedsRedraw = true;
  };
  mapTexture.onerror = () => {
    mapTexture = null;
    mapNeedsRedraw = true;
  };
  resizeMapCanvas();
  drawMapBase();
}

function resizeMapCanvas() {
  if (!mapCanvas || !mapCtx) return;
  const dpr = window.devicePixelRatio || 1;
  const width = Math.max(1, Math.floor(mapCanvas.clientWidth * dpr));
  const height = Math.max(1, Math.floor(mapCanvas.clientHeight * dpr));
  mapCanvas.width = width;
  mapCanvas.height = height;
  mapCtx.setTransform(1, 0, 0, 1, 0, 0);
  mapCtx.scale(dpr, dpr);
  const baseSize = 2048;
  mapBaseCanvas.width = baseSize;
  mapBaseCanvas.height = baseSize;
  mapNeedsRedraw = true;
}

function worldToMap(point, width, height) {
  const halfWidth = world.mapSize.width / 2;
  const halfDepth = world.mapSize.depth / 2;
  const x = ((-point.x / halfWidth) + 1) * 0.5 * width;
  const y = ((-point.z / halfDepth) + 1) * 0.5 * height;
  return { x, y };
}

function drawMapBase() {
  if (!mapBaseCtx) return;
  const width = mapBaseCanvas.width;
  const height = mapBaseCanvas.height;
  mapBaseCtx.clearRect(0, 0, width, height);

  if (mapTexture && mapTexture.complete) {
    mapBaseCtx.drawImage(mapTexture, 0, 0, width, height);
  } else {
    const gradient = mapBaseCtx.createLinearGradient(0, 0, width, height);
    gradient.addColorStop(0, "#223b2f");
    gradient.addColorStop(1, "#0e1512");
    mapBaseCtx.fillStyle = gradient;
    mapBaseCtx.fillRect(0, 0, width, height);
  }

  mapBaseCtx.strokeStyle = "rgba(120, 150, 120, 0.35)";
  mapBaseCtx.lineWidth = 1;
  const gridCount = 10;
  for (let i = 1; i < gridCount; i += 1) {
    const x = (width / gridCount) * i;
    const y = (height / gridCount) * i;
    mapBaseCtx.beginPath();
    mapBaseCtx.moveTo(x, 0);
    mapBaseCtx.lineTo(x, height);
    mapBaseCtx.stroke();
    mapBaseCtx.beginPath();
    mapBaseCtx.moveTo(0, y);
    mapBaseCtx.lineTo(width, y);
    mapBaseCtx.stroke();
  }

  mapBaseCtx.fillStyle = "rgba(80, 120, 95, 0.7)";
  world.obstacleBounds.forEach((box) => {
    const min = worldToMap(
      { x: box.minX, z: box.minZ },
      width,
      height
    );
    const max = worldToMap(
      { x: box.maxX, z: box.maxZ },
      width,
      height
    );
    const rectX = Math.min(min.x, max.x);
    const rectY = Math.min(min.y, max.y);
    const rectW = Math.abs(max.x - min.x);
    const rectH = Math.abs(max.y - min.y);
    mapBaseCtx.fillRect(rectX, rectY, rectW, rectH);
  });

  mapBaseCtx.strokeStyle = "rgba(220, 200, 150, 0.6)";
  mapBaseCtx.lineWidth = 2;
  mapBaseCtx.strokeRect(4, 4, width - 8, height - 8);
  mapNeedsRedraw = false;
}

function renderMap() {
  if (!mapCtx || !mapCanvas) return;
  const width = mapCanvas.clientWidth;
  const height = mapCanvas.clientHeight;
  if (width <= 0 || height <= 0) return;
  if (mapNeedsRedraw) drawMapBase();
  mapCtx.clearRect(0, 0, width, height);
  mapCtx.drawImage(mapBaseCanvas, 0, 0, width, height);

  const playerPos = worldToMap(player.group.position, width, height);
  mapCtx.fillStyle = "#f5e3b2";
  mapCtx.strokeStyle = "#3a2d20";
  mapCtx.lineWidth = 2;
  mapCtx.beginPath();
  mapCtx.arc(playerPos.x, playerPos.y, 6, 0, Math.PI * 2);
  mapCtx.fill();
  mapCtx.stroke();
  mapCtx.beginPath();
  mapCtx.moveTo(playerPos.x, playerPos.y);
  mapCtx.lineTo(playerPos.x, playerPos.y - 14);
  mapCtx.stroke();
}

function renderCharacterPreview(delta) {
  if (!characterRenderer || !characterScene || !characterCamera) return;
  if (!panelCharacter || !panelCharacter.classList.contains("active")) return;
  if (characterModel) {
    characterModel.rotation.y += delta * 0.4;
  }
  const width = characterCanvas.clientWidth;
  const height = characterCanvas.clientHeight;
  if (width > 0 && height > 0) {
    characterRenderer.setSize(width, height, false);
    characterCamera.aspect = width / height;
    characterCamera.updateProjectionMatrix();
  }
  characterRenderer.render(characterScene, characterCamera);
}

initCharacterPreview();
initMap();

if (exitGameButton) {
  exitGameButton.addEventListener("click", () => {
    window.close();
  });
}

function setMenuPanel(panel) {
  menuTabs.forEach((tab) => {
    tab.classList.toggle("active", tab.dataset.menuTab === panel);
  });
  menuPanels.forEach((page) => {
    page.classList.toggle("active", page.dataset.menuPanel === panel);
  });
}

function setMenuOpen(open) {
  isMenuOpen = open;
  document.body.classList.toggle("menu-open", isMenuOpen);
  if (menuOverlay) {
    menuOverlay.setAttribute("aria-hidden", isMenuOpen ? "false" : "true");
  }
  if (gameMenu) {
    gameMenu.setAttribute("aria-hidden", isMenuOpen ? "false" : "true");
  }
}

menuTabs.forEach((tab) => {
  tab.addEventListener("click", () => setMenuPanel(tab.dataset.menuTab));
});

function togglePanel(panel) {
  if (!panel) return;
  const willOpen = !panel.classList.contains("active");
  panel.classList.toggle("active");
  if (panel === panelMap && willOpen) {
    resizeMapCanvas();
    mapNeedsRedraw = true;
  }
}

startLoop((delta) => {
  if (input.consumePressed("menu")) {
    if (isMenuOpen) {
      setMenuOpen(false);
    } else {
      targetEnemy = null;
      setMenuOpen(true);
      setMenuPanel("character");
    }
    input.clearPressed(["skill1", "skill2", "skill3", "skill4", "target"]);
  }
  if (input.consumePressed("map")) {
    togglePanel(panelMap);
  }
  if (input.consumePressed("inventory")) {
    togglePanel(panelInventory);
  }
  if (input.consumePressed("quests")) {
    togglePanel(panelQuests);
  }
  if (input.consumePressed("character")) {
    togglePanel(panelCharacter);
  }
  if (input.consumePressed("target")) {
    targetEnemy = findTargetEnemy(player, enemies, 40);
  }

  if (targetEnemy && targetEnemy.isDead) {
    targetEnemy = null;
  }
  if (!targetEnemy) {
    input.clearPressed(["skill1", "skill2"]);
  }

  player.update(input, delta, world, camera, enemies, targetEnemy);
  enemies.forEach((enemy) => enemy.update(delta, world, player, enemies));
  world.update(delta, { player, enemies });
  followCam.update(delta);
  renderer.render(scene, camera);
  if (panelMap && panelMap.classList.contains("active")) {
    renderMap();
  }
  renderCharacterPreview(delta);

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
  const mpRatio = player.getManaRatio();
  const mpPercent = Math.round(mpRatio * 100);
  if (mpFill) {
    mpFill.style.width = `${Math.max(0, Math.min(100, mpPercent))}%`;
  }
  if (mpText) {
    mpText.textContent = `${Math.round(player.mana)} / ${player.maxMana}`;
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
