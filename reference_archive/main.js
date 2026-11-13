import { createMars } from './mars.js';
import { createSun } from './sun.js';

const container = document.getElementById('canvas-container');
const width = window.innerWidth;
const height = window.innerHeight;

// 1. Scene & Standard Coordinate Setup (Y is UP, Z is Toward Camera, X is Left/Right)
const scene = new THREE.Scene();
scene.background = new THREE.Color(0x000000); // Deep pitch black void

const camera = new THREE.PerspectiveCamera(40, width / height, 0.1, 1000);
// Ensure camera UP vector is strictly world UP (+Y) to prevent any tilting/roll
camera.up.set(0, 1, 0);

const renderer = new THREE.WebGLRenderer({ antialias: true, powerPreference: "high-performance" });
renderer.setSize(width, height);
renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
renderer.toneMapping = THREE.ACESFilmicToneMapping;
renderer.toneMappingExposure = 1.35;
container.appendChild(renderer.domElement);

// Ambient space fill
const ambient = new THREE.AmbientLight(0x40322d, 1.2);
scene.add(ambient);

// 2. Mars at World Origin (0, 0, 0)
const mars = createMars(THREE, 10.0);
scene.add(mars.group);

// 3. Sun in Standard Coordinates: Front-Left (-X), Up (+Y), and in front (+Z) illuminating Mars
const sunPos = new THREE.Vector3(-24.0, 14.0, 48.0);
const sun = createSun(THREE, sunPos);
sun.light.target = mars.group;
scene.add(sun.group);

// 4. Clean Horizontal Track Camera Keyframes (Straight eye-level push-in)
// Camera starts at Z = +62 (looking towards Mars at Z = 0)
// Negative X offsets camera to the left, so Mars appears on the RIGHT.
const keyframes = [
  // 0s: Wide shot - Mars on right (~60%), Sun in left void (~40%)
  { t: 0.0, pos: new THREE.Vector3(-12.0, 3.0, 62.0), lookAt: new THREE.Vector3(5.0, 1.0, 0.0), flareOp: 1.0, act: "ACT I: COSMIC ESTABLISHING SHOT" },
  // 3s: Slow cosmic drift
  { t: 3.0, pos: new THREE.Vector3(-10.5, 2.8, 58.0), lookAt: new THREE.Vector3(4.8, 1.0, 0.0), flareOp: 1.0, act: "ACT I: IMMENSE SCALE HOLD" },
  // 5.4s: Accelerate forward, rotate toward Mars, Sun slides out of left frame
  { t: 5.4, pos: new THREE.Vector3(-4.0, 1.8, 44.0), lookAt: new THREE.Vector3(3.0, 0.5, 0.0), flareOp: 0.35, act: "ACT II: CINEMATIC RIGHT TURN" },
  // 8s: Close approach, Mars dominates the screen
  { t: 8.0, pos: new THREE.Vector3(2.5, 0.5, 26.0), lookAt: new THREE.Vector3(-0.5, 0.0, 0.0), flareOp: 0.0, act: "ACT III: STATELY MARS APPROACH" }
];

function evaluateCamera(progress) {
  let seg = 0;
  for (let i = 0; i < keyframes.length - 1; i++) {
    if (progress >= keyframes[i].t && progress <= keyframes[i + 1].t) {
      seg = i;
      break;
    }
  }
  const k0 = keyframes[seg];
  const k1 = keyframes[seg + 1];
  const u = (progress - k0.t) / (k1.t - k0.t);
  const ease = u * u * (3.0 - 2.0 * u);

  return {
    pos: new THREE.Vector3().lerpVectors(k0.pos, k1.pos, ease),
    lookAt: new THREE.Vector3().lerpVectors(k0.lookAt, k1.lookAt, ease),
    flareOp: THREE.MathUtils.lerp(k0.flareOp, k1.flareOp, ease),
    act: k0.act
  };
}

// 5. UI Controls & Playback
let currentTime = 0.0;
const duration = 8.0;
let isPlaying = true;
let isFreeCam = false;

const playBtn = document.getElementById('play-btn');
const restartBtn = document.getElementById('restart-btn');
const timeline = document.getElementById('timeline');
const timeDisplay = document.getElementById('time-display');
const actLabel = document.getElementById('act-label');
const camModeBtn = document.getElementById('cam-mode-btn');

playBtn.addEventListener('click', () => {
  isPlaying = !isPlaying;
  playBtn.textContent = isPlaying ? "❚❚ PAUSE" : "▶ PLAY";
});

restartBtn.addEventListener('click', () => {
  currentTime = 0.0;
  timeline.value = 0;
});

timeline.addEventListener('input', (e) => {
  currentTime = parseFloat(e.target.value);
});

camModeBtn.addEventListener('click', () => {
  isFreeCam = !isFreeCam;
  camModeBtn.classList.toggle('active', isFreeCam);
  camModeBtn.textContent = isFreeCam ? "CINEMATIC CAM" : "FREE CAM";
});

// Free Cam Orbit Controls
let isDragging = false;
let prevMouseX = 0, prevMouseY = 0;
let orbitTheta = 0, orbitPhi = 0;

window.addEventListener('mousedown', (e) => {
  if (e.target.tagName !== 'BUTTON' && e.target.tagName !== 'INPUT') {
    isDragging = true;
    prevMouseX = e.clientX;
    prevMouseY = e.clientY;
  }
});

window.addEventListener('mousemove', (e) => {
  if (isDragging && isFreeCam) {
    const dx = e.clientX - prevMouseX;
    const dy = e.clientY - prevMouseY;
    orbitTheta -= dx * 0.005;
    orbitPhi = Math.max(-1.4, Math.min(1.4, orbitPhi + dy * 0.005));
    prevMouseX = e.clientX;
    prevMouseY = e.clientY;
  }
});

window.addEventListener('mouseup', () => isDragging = false);

window.addEventListener('keydown', (e) => {
  if (e.key === 'ArrowRight' || e.key === 'f' || e.key === 'F') {
    currentTime = Math.min(duration, currentTime + 5.0);
  } else if (e.key === 'ArrowLeft' || e.key === 'b' || e.key === 'B') {
    currentTime = Math.max(0.0, currentTime - 5.0);
  } else if (e.code === 'Space') {
    isPlaying = !isPlaying;
    playBtn.textContent = isPlaying ? "❚❚ PAUSE" : "▶ PLAY";
  }
});

// 6. Animation Loop
const clock = new THREE.Clock();

function animate() {
  requestAnimationFrame(animate);
  const delta = clock.getDelta();

  if (isPlaying) {
    currentTime += delta;
    if (currentTime > duration) {
      currentTime = 0.0;
    }
    timeline.value = currentTime.toFixed(2);
  }

  timeDisplay.textContent = currentTime.toFixed(1) + "s";

  const state = evaluateCamera(currentTime);
  actLabel.textContent = state.act;

  // Update Components
  mars.update(delta);
  sun.setFlareOpacity(state.flareOp);

  // Camera Update with locked UP vector (zero tilt / Dutch roll)
  if (!isFreeCam) {
    camera.up.set(0, 1, 0);
    camera.position.copy(state.pos);
    camera.lookAt(state.lookAt);
  } else {
    const r = 55.0;
    camera.position.x = r * Math.sin(orbitTheta) * Math.cos(orbitPhi);
    camera.position.y = r * Math.sin(orbitPhi);
    camera.position.z = r * Math.cos(orbitTheta) * Math.cos(orbitPhi);
    camera.up.set(0, 1, 0);
    camera.lookAt(0, 0, 0);
  }

  renderer.render(scene, camera);
}

animate();

window.addEventListener('resize', () => {
  camera.aspect = window.innerWidth / window.innerHeight;
  camera.updateProjectionMatrix();
  renderer.setSize(window.innerWidth, window.innerHeight);
});
