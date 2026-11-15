"""
Interactive 3D Visual Pose Editor for Martian Astronauts
Run: python pose_editor.py
Opens your browser automatically with 3D viewport, sliders, and live save button.
"""

import http.server
import socketserver
import webbrowser
import struct
import json
import math
import os
import threading

PORT = 8088

def load_base_verts():
    verts = []
    with open("astronaut.bin", "rb") as f:
        magic = f.read(4)
        num_verts, = struct.unpack("<I", f.read(4))
        for _ in range(num_verts):
            data = struct.unpack("<11f", f.read(44))
            verts.append(list(data))
    return verts

BASE_VERTS = load_base_verts()

# Helper rotation math
def rot_y(x, y, z, cx, cz, deg):
    a = math.radians(deg)
    ca, sa = math.cos(a), math.sin(a)
    dx, dz = x - cx, z - cz
    return cx + dx * ca - dz * sa, y, cz + dx * sa + dz * ca

def rot_x(x, y, z, cy, cz, deg):
    a = math.radians(deg)
    ca, sa = math.cos(a), math.sin(a)
    dy, dz = y - cy, z - cz
    return x, cy + dy * ca - dz * sa, cz + dy * sa + dz * ca

def rot_z(x, y, z, cx, cy, deg):
    a = math.radians(deg)
    ca, sa = math.cos(a), math.sin(a)
    dx, dy = x - cx, y - cy
    return cx + dx * ca - dy * sa, cy + dx * sa + dy * ca, z

def deform_mesh(params):
    verts = [list(v) for v in BASE_VERTS]
    for v in verts:
        x, y, z = v[0], v[1], v[2]
        # Head
        if y > 1.62:
            v[0], v[1], v[2] = rot_y(x, y, z, 0.0, 0.0, params.get("head_yaw", 0.0))
            v[0], v[1], v[2] = rot_x(v[0], v[1], v[2], 1.70, 0.0, params.get("head_pitch", 0.0))
        # Right Arm
        elif x > 0.32 and y < 1.42:
            if params.get("r_shoulder_flare", 0.0) != 0:
                v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], 0.36, 1.35, params.get("r_shoulder_flare", 0.0))
            if params.get("r_shoulder_pitch", 0.0) != 0:
                v[0], v[1], v[2] = rot_x(v[0], v[1], v[2], 1.35, 0.0, params.get("r_shoulder_pitch", 0.0))
            # Forearm & Hand
            if v[1] < 1.15:
                if params.get("r_elbow_bend", 0.0) != 0:
                    v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], 0.44, 1.12, params.get("r_elbow_bend", 0.0))
                if params.get("r_forearm_twist", 0.0) != 0:
                    v[0], v[1], v[2] = rot_y(v[0], v[1], v[2], 0.44, 0.0, params.get("r_forearm_twist", 0.0))
                if params.get("r_elbow_pitch", 0.0) != 0:
                    v[0], v[1], v[2] = rot_x(v[0], v[1], v[2], 1.15, 0.12, params.get("r_elbow_pitch", 0.0))
                if params.get("r_hand_offset_z", 0.0) != 0:
                    v[2] += params.get("r_hand_offset_z", 0.0)
        # Left Arm
        elif x < -0.32 and y < 1.42:
            if params.get("l_shoulder_flare", 0.0) != 0:
                v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], -0.36, 1.35, params.get("l_shoulder_flare", 0.0))
            if params.get("l_shoulder_pitch", 0.0) != 0:
                v[0], v[1], v[2] = rot_x(v[0], v[1], v[2], 1.35, 0.0, params.get("l_shoulder_pitch", 0.0))
            # Forearm & Hand
            if v[1] < 1.15:
                if params.get("l_elbow_bend", 0.0) != 0:
                    v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], -0.44, 1.12, params.get("l_elbow_bend", 0.0))
                if params.get("l_forearm_twist", 0.0) != 0:
                    v[0], v[1], v[2] = rot_y(v[0], v[1], v[2], -0.44, 0.0, params.get("l_forearm_twist", 0.0))
                if params.get("l_elbow_pitch", 0.0) != 0:
                    v[0], v[1], v[2] = rot_x(v[0], v[1], v[2], 1.15, 0.12, params.get("l_elbow_pitch", 0.0))
                if params.get("l_hand_offset_z", 0.0) != 0:
                    v[2] += params.get("l_hand_offset_z", 0.0)
    return verts

def save_bin(filename, verts):
    with open(filename, "wb") as f:
        f.write(b"ASTR")
        f.write(struct.pack("<I", len(verts)))
        for v in verts:
            f.write(struct.pack("<11f", *v))
    print(f"Saved {filename}")

HTML_CONTENT = """<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>Martian Astronaut Pose Studio</title>
    <style>
        * { box-sizing: border-box; }
        body { margin: 0; padding: 0; background: #111; color: #eee; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; overflow: hidden; display: flex; height: 100vh; }
        #viewport { flex: 1; height: 100vh; position: relative; }
        #canvas3d { width: 100%; height: 100%; display: block; }
        #sidebar { width: 380px; height: 100vh; background: #1a1a1f; border-left: 1px solid #333; overflow-y: auto; padding: 20px; display: flex; flex-direction: column; gap: 16px; }
        h1 { margin: 0 0 4px 0; font-size: 20px; color: #ff5722; }
        .subtitle { font-size: 12px; color: #888; margin-bottom: 8px; }
        .crew-tabs { display: flex; gap: 6px; }
        .tab-btn { flex: 1; padding: 8px 4px; background: #282830; border: 1px solid #444; color: #aaa; cursor: pointer; border-radius: 4px; font-size: 12px; font-weight: 600; text-align: center; }
        .tab-btn.active { background: #ff5722; color: #fff; border-color: #ff5722; }
        .panel-group { background: #22222a; border-radius: 6px; padding: 12px; border: 1px solid #333; }
        .group-title { font-size: 13px; font-weight: 700; color: #ff9800; margin-bottom: 10px; text-transform: uppercase; letter-spacing: 0.5px; }
        .control { display: flex; flex-direction: column; gap: 4px; margin-bottom: 12px; }
        .control:last-child { margin-bottom: 0; }
        .label-row { display: flex; justify-content: space-between; font-size: 12px; color: #ccc; }
        .val-badge { font-weight: bold; color: #4fc3f7; font-family: monospace; }
        input[type=range] { width: 100%; cursor: pointer; accent-color: #ff5722; }
        .btn-save { background: #4caf50; color: white; border: none; padding: 14px; font-size: 15px; font-weight: bold; border-radius: 6px; cursor: pointer; transition: 0.2s; box-shadow: 0 4px 12px rgba(76,175,80,0.3); }
        .btn-save:hover { background: #43a047; transform: translateY(-1px); }
        .btn-save:active { transform: translateY(1px); }
        .render-toggles { display: flex; gap: 8px; }
        .mode-btn { flex: 1; padding: 6px; background: #282830; border: 1px solid #444; color: #ccc; border-radius: 4px; font-size: 12px; cursor: pointer; }
        .mode-btn.active { background: #3f51b5; color: white; border-color: #5c6bc0; }
        #status-msg { font-size: 13px; text-align: center; color: #81c784; min-height: 20px; font-weight: 600; }
    </style>
    <!-- Three.js from unpkg -->
    <script src="https://cdnjs.cloudflare.com/ajax/libs/three.js/r128/three.min.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/three@0.128.0/examples/js/controls/OrbitControls.js"></script>
</head>
<body>
    <div id="viewport">
        <canvas id="canvas3d"></canvas>
    </div>

    <div id="sidebar">
        <div>
            <h1>Astronaut Pose Studio</h1>
            <div class="subtitle">Live 3D Hand, Arm & Silhouette Adjuster</div>
            <div class="crew-tabs">
                <button class="tab-btn active" onclick="selectCrew(0)">Crew 1<br><span style="font-size:10px;font-weight:normal;">Scout</span></button>
                <button class="tab-btn" onclick="selectCrew(1)">Crew 2<br><span style="font-size:10px;font-weight:normal;">Commander</span></button>
                <button class="tab-btn" onclick="selectCrew(2)">Crew 3<br><span style="font-size:10px;font-weight:normal;">Scientist</span></button>
                <button class="tab-btn" onclick="selectCrew(3)">Crew 4<br><span style="font-size:10px;font-weight:normal;">Engineer</span></button>
            </div>
        </div>

        <div class="panel-group">
            <div class="group-title">Lighting & View Mode</div>
            <div class="render-toggles">
                <button class="mode-btn active" id="btn-sil" onclick="setMode('silhouette')">Dune Silhouette</button>
                <button class="mode-btn" id="btn-clay" onclick="setMode('clay')">3D Shaded</button>
            </div>
        </div>

        <div class="panel-group">
            <div class="group-title">Head & Gaze</div>
            <div class="control">
                <div class="label-row"><span>Head Yaw (Left / Right)</span><span class="val-badge" id="val-head_yaw">0°</span></div>
                <input type="range" id="slider-head_yaw" min="-45" max="45" step="1" oninput="onSlider('head_yaw', this.value)">
            </div>
            <div class="control">
                <div class="label-row"><span>Head Pitch (Up / Down)</span><span class="val-badge" id="val-head_pitch">0°</span></div>
                <input type="range" id="slider-head_pitch" min="-30" max="30" step="1" oninput="onSlider('head_pitch', this.value)">
            </div>
        </div>

        <div class="panel-group">
            <div class="group-title">Right Arm & Hand</div>
            <div class="control">
                <div class="label-row"><span>Shoulder Flare (Out / In)</span><span class="val-badge" id="val-r_shoulder_flare">0°</span></div>
                <input type="range" id="slider-r_shoulder_flare" min="-45" max="60" step="1" oninput="onSlider('r_shoulder_flare', this.value)">
            </div>
            <div class="control">
                <div class="label-row"><span>Shoulder Swing (Front / Back)</span><span class="val-badge" id="val-r_shoulder_pitch">0°</span></div>
                <input type="range" id="slider-r_shoulder_pitch" min="-60" max="60" step="1" oninput="onSlider('r_shoulder_pitch', this.value)">
            </div>
            <div class="control">
                <div class="label-row"><span>Elbow Bend</span><span class="val-badge" id="val-r_elbow_bend">0°</span></div>
                <input type="range" id="slider-r_elbow_bend" min="-70" max="70" step="1" oninput="onSlider('r_elbow_bend', this.value)">
            </div>
            <div class="control">
                <div class="label-row"><span>Forearm Twist (Towards Hip / Out)</span><span class="val-badge" id="val-r_forearm_twist">0°</span></div>
                <input type="range" id="slider-r_forearm_twist" min="-60" max="60" step="1" oninput="onSlider('r_forearm_twist', this.value)">
            </div>
            <div class="control">
                <div class="label-row"><span>Hand Forward Lift</span><span class="val-badge" id="val-r_elbow_pitch">0°</span></div>
                <input type="range" id="slider-r_elbow_pitch" min="-50" max="70" step="1" oninput="onSlider('r_elbow_pitch', this.value)">
            </div>
        </div>

        <div class="panel-group">
            <div class="group-title">Left Arm & Hand</div>
            <div class="control">
                <div class="label-row"><span>Shoulder Flare (Out / In)</span><span class="val-badge" id="val-l_shoulder_flare">0°</span></div>
                <input type="range" id="slider-l_shoulder_flare" min="-60" max="45" step="1" oninput="onSlider('l_shoulder_flare', this.value)">
            </div>
            <div class="control">
                <div class="label-row"><span>Shoulder Swing (Front / Back)</span><span class="val-badge" id="val-l_shoulder_pitch">0°</span></div>
                <input type="range" id="slider-l_shoulder_pitch" min="-60" max="60" step="1" oninput="onSlider('l_shoulder_pitch', this.value)">
            </div>
            <div class="control">
                <div class="label-row"><span>Elbow Bend</span><span class="val-badge" id="val-l_elbow_bend">0°</span></div>
                <input type="range" id="slider-l_elbow_bend" min="-70" max="70" step="1" oninput="onSlider('l_elbow_bend', this.value)">
            </div>
            <div class="control">
                <div class="label-row"><span>Forearm Twist</span><span class="val-badge" id="val-l_forearm_twist">0°</span></div>
                <input type="range" id="slider-l_forearm_twist" min="-60" max="60" step="1" oninput="onSlider('l_forearm_twist', this.value)">
            </div>
        </div>

        <button class="btn-save" onclick="savePosesToGame()">💾 SAVE ALL POSES TO GAME</button>
        <div id="status-msg"></div>
    </div>

    <script>
        let currentCrew = 0;
        let viewMode = 'silhouette';
        let rawBaseVerts = [];
        
        // Default pose configs
        let poses = [
            { head_yaw: -9, head_pitch: 0, r_shoulder_flare: 16, r_shoulder_pitch: 0, r_elbow_bend: -42, r_forearm_twist: 24, r_elbow_pitch: 0, l_shoulder_flare: -5, l_shoulder_pitch: 0, l_elbow_bend: 0, l_forearm_twist: 0 },
            { head_yaw: 3, head_pitch: -2, r_shoulder_flare: 0, r_shoulder_pitch: -22, r_elbow_bend: -42, r_forearm_twist: 0, r_elbow_pitch: 0, l_shoulder_flare: 0, l_shoulder_pitch: -22, l_elbow_bend: 42, l_forearm_twist: 0, r_hand_offset_z: -0.08, l_hand_offset_z: -0.08 },
            { head_yaw: 10, head_pitch: 14, r_shoulder_flare: 0, r_shoulder_pitch: 28, r_elbow_bend: -18, r_forearm_twist: 0, r_elbow_pitch: 45, l_shoulder_flare: -10, l_shoulder_pitch: 0, l_elbow_bend: 0, l_forearm_twist: 0 },
            { head_yaw: -12, head_pitch: 2, r_shoulder_flare: -8, r_shoulder_pitch: 0, r_elbow_bend: 15, r_forearm_twist: 0, r_elbow_pitch: 0, l_shoulder_flare: 10, l_shoulder_pitch: 0, l_elbow_bend: 0, l_forearm_twist: 0 }
        ];

        // Three.js Setup
        const canvas = document.getElementById('canvas3d');
        const renderer = new THREE.WebGLRenderer({ canvas, antialias: true });
        renderer.setPixelRatio(window.devicePixelRatio);
        const scene = new THREE.Scene();
        scene.background = new THREE.Color(0xd2753a); // Martian sky default

        const camera = new THREE.PerspectiveCamera(40, canvas.clientWidth / canvas.clientHeight, 0.1, 100);
        camera.position.set(0, 1.2, 3.2);

        const controls = new THREE.OrbitControls(camera, renderer.domElement);
        controls.target.set(0, 1.1, 0);
        controls.update();

        // Lights
        const ambLight = new THREE.AmbientLight(0xffffff, 0.15);
        scene.add(ambLight);
        const dirLight = new THREE.DirectionalLight(0xffffff, 0.8);
        dirLight.position.set(-2, 3, -5); // Back-light for dramatic silhouette edge
        scene.add(dirLight);

        let mesh = null;
        let geometry = null;
        let silMaterial = new THREE.MeshBasicMaterial({ color: 0x050404 });
        let clayMaterial = new THREE.MeshStandardMaterial({ color: 0xdddddd, roughness: 0.5, metalness: 0.1 });

        // Resize
        function resize() {
            const w = canvas.clientWidth;
            const h = canvas.clientHeight;
            if (canvas.width !== w || canvas.height !== h) {
                renderer.setSize(w, h, false);
                camera.aspect = w / h;
                camera.updateProjectionMatrix();
            }
        }

        // Fetch base verts from server
        fetch('/api/base_verts')
            .then(res => res.json())
            .then(data => {
                rawBaseVerts = data;
                initMesh();
                updateUIFromState();
            });

        function initMesh() {
            geometry = new THREE.BufferGeometry();
            const positions = new Float32Array(rawBaseVerts.length * 3);
            const normals = new Float32Array(rawBaseVerts.length * 3);
            geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
            geometry.setAttribute('normal', new THREE.BufferAttribute(normals, 3));

            mesh = new THREE.Mesh(geometry, silMaterial);
            scene.add(mesh);
            rebuildCurrentMesh();
        }

        function rotY(x, y, z, cx, cz, deg) {
            const rad = deg * Math.PI / 180;
            const ca = Math.cos(rad), sa = Math.sin(rad);
            const dx = x - cx, dz = z - cz;
            return [cx + dx * ca - dz * sa, y, cz + dx * sa + dz * ca];
        }

        function rotX(x, y, z, cy, cz, deg) {
            const rad = deg * Math.PI / 180;
            const ca = Math.cos(rad), sa = Math.sin(rad);
            const dy = y - cy, dz = z - cz;
            return [x, cy + dy * ca - dz * sa, cz + dy * sa + dz * ca];
        }

        function rotZ(x, y, z, cx, cy, deg) {
            const rad = deg * Math.PI / 180;
            const ca = Math.cos(rad), sa = Math.sin(rad);
            const dx = x - cx, dy = y - cy;
            return [cx + dx * ca - dy * sa, cy + dx * sa + dy * ca, z];
        }

        function rebuildCurrentMesh() {
            if (!geometry || rawBaseVerts.length === 0) return;
            const posAttr = geometry.attributes.position;
            const normAttr = geometry.attributes.normal;
            const p = poses[currentCrew];

            for (let i = 0; i < rawBaseVerts.length; i++) {
                let v = rawBaseVerts[i];
                let x = v[0], y = v[1], z = v[2];
                let nx = v[3], ny = v[4], nz = v[5];

                // Head
                if (y > 1.62) {
                    if (p.head_yaw) [x, y, z] = rotY(x, y, z, 0.0, 0.0, p.head_yaw);
                    if (p.head_pitch) [x, y, z] = rotX(x, y, z, 1.70, 0.0, p.head_pitch);
                }
                // Right Arm
                else if (x > 0.32 && y < 1.42) {
                    if (p.r_shoulder_flare) [x, y, z] = rotZ(x, y, z, 0.36, 1.35, p.r_shoulder_flare);
                    if (p.r_shoulder_pitch) [x, y, z] = rotX(x, y, z, 1.35, 0.0, p.r_shoulder_pitch);
                    if (y < 1.15) {
                        if (p.r_elbow_bend) [x, y, z] = rotZ(x, y, z, 0.44, 1.12, p.r_elbow_bend);
                        if (p.r_forearm_twist) [x, y, z] = rotY(x, y, z, 0.44, 0.0, p.r_forearm_twist);
                        if (p.r_elbow_pitch) [x, y, z] = rotX(x, y, z, 1.15, 0.12, p.r_elbow_pitch);
                        if (p.r_hand_offset_z) z += p.r_hand_offset_z;
                    }
                }
                // Left Arm
                else if (x < -0.32 && y < 1.42) {
                    if (p.l_shoulder_flare) [x, y, z] = rotZ(x, y, z, -0.36, 1.35, p.l_shoulder_flare);
                    if (p.l_shoulder_pitch) [x, y, z] = rotX(x, y, z, 1.35, 0.0, p.l_shoulder_pitch);
                    if (y < 1.15) {
                        if (p.l_elbow_bend) [x, y, z] = rotZ(x, y, z, -0.44, 1.12, p.l_elbow_bend);
                        if (p.l_forearm_twist) [x, y, z] = rotY(x, y, z, -0.44, 0.0, p.l_forearm_twist);
                        if (p.l_elbow_pitch) [x, y, z] = rotX(x, y, z, 1.15, 0.12, p.l_elbow_pitch);
                        if (p.l_hand_offset_z) z += p.l_hand_offset_z;
                    }
                }

                posAttr.setXYZ(i, x, y, z);
                normAttr.setXYZ(i, nx, ny, nz);
            }
            posAttr.needsUpdate = true;
            normAttr.needsUpdate = true;
            if (viewMode === 'clay') geometry.computeVertexNormals();
        }

        const sliderKeys = [
            'head_yaw', 'head_pitch',
            'r_shoulder_flare', 'r_shoulder_pitch', 'r_elbow_bend', 'r_forearm_twist', 'r_elbow_pitch',
            'l_shoulder_flare', 'l_shoulder_pitch', 'l_elbow_bend', 'l_forearm_twist'
        ];

        function updateUIFromState() {
            const p = poses[currentCrew];
            sliderKeys.forEach(k => {
                const el = document.getElementById('slider-' + k);
                const badge = document.getElementById('val-' + k);
                const val = p[k] || 0;
                if (el) el.value = val;
                if (badge) badge.innerText = val + '°';
            });
        }

        function onSlider(key, val) {
            poses[currentCrew][key] = parseFloat(val);
            const badge = document.getElementById('val-' + key);
            if (badge) badge.innerText = val + '°';
            rebuildCurrentMesh();
        }

        function selectCrew(idx) {
            currentCrew = idx;
            document.querySelectorAll('.tab-btn').forEach((btn, i) => {
                btn.classList.toggle('active', i === idx);
            });
            updateUIFromState();
            rebuildCurrentMesh();
        }

        function setMode(mode) {
            viewMode = mode;
            document.getElementById('btn-sil').classList.toggle('active', mode === 'silhouette');
            document.getElementById('btn-clay').classList.toggle('active', mode === 'clay');
            if (mode === 'silhouette') {
                scene.background = new THREE.Color(0xd2753a);
                mesh.material = silMaterial;
            } else {
                scene.background = new THREE.Color(0x1a1a24);
                mesh.material = clayMaterial;
            }
        }

        function savePosesToGame() {
            const msgEl = document.getElementById('status-msg');
            msgEl.innerText = 'Saving poses to game...';
            fetch('/api/save_poses', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(poses)
            })
            .then(res => res.json())
            .then(data => {
                msgEl.innerText = '✅ Saved directly to astronaut_pose1..4.bin!';
                setTimeout(() => { msgEl.innerText = ''; }, 4000);
            })
            .catch(err => {
                msgEl.innerText = '❌ Error saving: ' + err;
            });
        }

        // Render loop
        function animate() {
            requestAnimationFrame(animate);
            resize();
            controls.update();
            renderer.render(scene, camera);
        }
        animate();
    </script>
</body>
</html>
"""

class PoseEditorHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/" or self.path == "/index.html":
            self.send_response(200)
            self.send_header("Content-Type", "text/html")
            self.end_headers()
            self.wfile.write(HTML_CONTENT.encode("utf-8"))
        elif self.path == "/api/base_verts":
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps(BASE_VERTS).encode("utf-8"))
        else:
            self.send_error(404)

    def do_POST(self):
        if self.path == "/api/save_poses":
            content_length = int(self.headers.get('Content-Length', 0))
            body = self.rfile.read(content_length).decode('utf-8')
            poses = json.loads(body)
            for i, p in enumerate(poses):
                fname = f"astronaut_pose{i+1}.bin"
                v = deform_mesh(p)
                save_bin(fname, v)
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps({"status": "ok"}).encode("utf-8"))
        else:
            self.send_error(404)

    def log_message(self, format, *args):
        pass # Quiet server log

def run_server():
    server = socketserver.TCPServer(("127.0.0.1", PORT), PoseEditorHandler)
    print(f"\n[OK] Visual Pose Editor running at http://127.0.0.1:{PORT}")
    server.serve_forever()

if __name__ == "__main__":
    t = threading.Thread(target=run_server, daemon=True)
    t.start()
    webbrowser.open(f"http://127.0.0.1:{PORT}")
    print(f"Press Enter in this terminal to stop the editor...")
    try:
        input()
    except:
        pass
