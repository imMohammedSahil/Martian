"""
tweak_poses.py - Easily adjust astronaut arm, hand, and head angles.
Usage:
    python tweak_poses.py

Edit the angles (in degrees) in the POSE_CONFIG dictionary below.
"""

import struct, math, os

# ==============================================================================
# CONFIGURATION: Tweak angles (in degrees) here!
# ==============================================================================
POSE_CONFIG = {
    # --- Astronaut 1 (Left / Scout): Hand-on-hip pose ---
    "astro_1": {
        "head_yaw": -9.0,        # negative = look left, positive = look right
        "head_pitch": 0.0,       # positive = look up, negative = look down
        "r_shoulder_flare": 16.0,# right arm outward flare
        "r_elbow_bend": -42.0,   # right elbow bend (negative bends forearm inward toward hip)
        "r_forearm_twist": 24.0, # right forearm twist towards waist
        "l_arm_swing": -5.0,     # left arm relaxed swing
    },

    # --- Astronaut 2 (Center-Left / Commander): Parade rest pose ---
    "astro_2": {
        "head_yaw": 3.0,
        "head_pitch": -2.0,
        "r_shoulder_pitch": -22.0, # right arm pulled back behind torso
        "l_shoulder_pitch": -22.0, # left arm pulled back behind torso
        "r_elbow_inward": -42.0,   # right hand towards lower spine
        "l_elbow_inward": 42.0,    # left hand towards lower spine
    },

    # --- Astronaut 3 (Center-Right / Scientist): Looking up / telemetry ---
    "astro_3": {
        "head_yaw": 10.0,
        "head_pitch": 14.0,        # looking up at the sky / landscape
        "r_shoulder_lift": 28.0,   # right arm raised forward
        "r_elbow_lift": 45.0,      # right forearm bent holding scanner
        "l_arm_flare": -10.0,      # left arm outward balance
    },

    # --- Astronaut 4 (Right / Engineer): Relaxed stance ---
    "astro_4": {
        "head_yaw": -12.0,
        "head_pitch": 2.0,
        "r_arm_swing": -8.0,
        "l_arm_swing": 10.0,
        "r_elbow_bend": 15.0,
    }
}

# ==============================================================================
# 3D ROTATION HELPERS
# ==============================================================================
def rotate_pt_y(x, y, z, cx, cz, angle_deg):
    a = math.radians(angle_deg)
    ca, sa = math.cos(a), math.sin(a)
    dx, dz = x - cx, z - cz
    return cx + dx * ca - dz * sa, y, cz + dx * sa + dz * ca

def rotate_pt_x(x, y, z, cy, cz, angle_deg):
    a = math.radians(angle_deg)
    ca, sa = math.cos(a), math.sin(a)
    dy, dz = y - cy, z - cz
    return x, cy + dy * ca - dz * sa, cz + dy * sa + dz * ca

def rotate_pt_z(x, y, z, cx, cy, angle_deg):
    a = math.radians(angle_deg)
    ca, sa = math.cos(a), math.sin(a)
    dx, dy = x - cx, y - cy
    return cx + dx * ca - dy * sa, cy + dx * sa + dy * ca, z

def load_base_mesh():
    if not os.path.exists("astronaut.bin"):
        raise FileNotFoundError("astronaut.bin not found!")
    verts = []
    with open("astronaut.bin", "rb") as f:
        magic = f.read(4)
        num_verts, = struct.unpack("<I", f.read(4))
        for _ in range(num_verts):
            verts.append(list(struct.unpack("<11f", f.read(44))))
    return verts

def save_pose_mesh(verts, filename):
    with open(filename, "wb") as f:
        f.write(b"ASTR")
        f.write(struct.pack("<I", len(verts)))
        for v in verts:
            f.write(struct.pack("<11f", *v))
    print(f"  [OK] Saved {filename} ({len(verts)} vertices)")

def main():
    print("Loading base astronaut mesh...")
    base_verts = load_base_mesh()

    # --- Build Pose 1 ---
    cfg1 = POSE_CONFIG["astro_1"]
    p1 = [list(v) for v in base_verts]
    for v in p1:
        x, y, z = v[0], v[1], v[2]
        if y > 1.62: # Head
            v[0], v[1], v[2] = rotate_pt_y(x, y, z, 0.0, 0.0, cfg1["head_yaw"])
            v[0], v[1], v[2] = rotate_pt_x(v[0], v[1], v[2], 1.70, 0.0, cfg1["head_pitch"])
        elif x > 0.32 and 0.72 < y < 1.42: # Right Arm
            v[0], v[1], v[2] = rotate_pt_z(x, y, z, 0.36, 1.35, cfg1["r_shoulder_flare"])
            if v[1] < 1.15: # Right Forearm & Hand
                v[0], v[1], v[2] = rotate_pt_z(v[0], v[1], v[2], 0.44, 1.12, cfg1["r_elbow_bend"])
                v[0], v[1], v[2] = rotate_pt_y(v[0], v[1], v[2], 0.44, 0.0, cfg1["r_forearm_twist"])
        elif x < -0.32 and 0.72 < y < 1.42: # Left Arm
            v[0], v[1], v[2] = rotate_pt_z(x, y, z, -0.36, 1.35, cfg1["l_arm_swing"])
    save_pose_mesh(p1, "astronaut_pose1.bin")

    # --- Build Pose 2 ---
    cfg2 = POSE_CONFIG["astro_2"]
    p2 = [list(v) for v in base_verts]
    for v in p2:
        x, y, z = v[0], v[1], v[2]
        if y > 1.62: # Head
            v[0], v[1], v[2] = rotate_pt_y(x, y, z, 0.0, 0.0, cfg2["head_yaw"])
            v[0], v[1], v[2] = rotate_pt_x(v[0], v[1], v[2], 1.70, 0.0, cfg2["head_pitch"])
        elif x > 0.32 and 0.72 < y < 1.42: # Right Arm
            v[0], v[1], v[2] = rotate_pt_x(x, y, z, 1.35, 0.0, cfg2["r_shoulder_pitch"])
            if v[1] < 1.15: # Forearm
                v[0], v[1], v[2] = rotate_pt_z(v[0], v[1], v[2], 0.35, 1.12, cfg2["r_elbow_inward"])
                v[2] -= 0.08
        elif x < -0.32 and 0.72 < y < 1.42: # Left Arm
            v[0], v[1], v[2] = rotate_pt_x(x, y, z, 1.35, 0.0, cfg2["l_shoulder_pitch"])
            if v[1] < 1.15: # Forearm
                v[0], v[1], v[2] = rotate_pt_z(v[0], v[1], v[2], -0.35, 1.12, cfg2["l_elbow_inward"])
                v[2] -= 0.08
    save_pose_mesh(p2, "astronaut_pose2.bin")

    # --- Build Pose 3 ---
    cfg3 = POSE_CONFIG["astro_3"]
    p3 = [list(v) for v in base_verts]
    for v in p3:
        x, y, z = v[0], v[1], v[2]
        if y > 1.62: # Head
            v[0], v[1], v[2] = rotate_pt_y(x, y, z, 0.0, 0.0, cfg3["head_yaw"])
            v[0], v[1], v[2] = rotate_pt_x(v[0], v[1], v[2], 1.70, 0.0, cfg3["head_pitch"])
        elif x > 0.32 and 0.72 < y < 1.42: # Right Arm
            v[0], v[1], v[2] = rotate_pt_x(x, y, z, 1.35, 0.0, cfg3["r_shoulder_lift"])
            if v[1] < 1.18: # Forearm holding scanner
                v[0], v[1], v[2] = rotate_pt_x(v[0], v[1], v[2], 1.15, 0.12, cfg3["r_elbow_lift"])
                v[0], v[1], v[2] = rotate_pt_z(v[0], v[1], v[2], 0.38, 1.15, -18.0)
        elif x < -0.32 and 0.72 < y < 1.42: # Left Arm
            v[0], v[1], v[2] = rotate_pt_z(x, y, z, -0.36, 1.35, cfg3["l_arm_flare"])
    save_pose_mesh(p3, "astronaut_pose3.bin")

    # --- Build Pose 4 ---
    cfg4 = POSE_CONFIG["astro_4"]
    p4 = [list(v) for v in base_verts]
    for v in p4:
        x, y, z = v[0], v[1], v[2]
        if y > 1.62: # Head
            v[0], v[1], v[2] = rotate_pt_y(x, y, z, 0.0, 0.0, cfg4["head_yaw"])
            v[0], v[1], v[2] = rotate_pt_x(v[0], v[1], v[2], 1.70, 0.0, cfg4["head_pitch"])
        elif x > 0.32 and 0.72 < y < 1.42: # Right Arm
            v[0], v[1], v[2] = rotate_pt_z(x, y, z, 0.36, 1.35, cfg4["r_arm_swing"])
            if v[1] < 1.15:
                v[0], v[1], v[2] = rotate_pt_x(v[0], v[1], v[2], 1.12, 0.0, cfg4["r_elbow_bend"])
        elif x < -0.32 and 0.72 < y < 1.42: # Left Arm
            v[0], v[1], v[2] = rotate_pt_z(x, y, z, -0.36, 1.35, cfg4["l_arm_swing"])
    save_pose_mesh(p4, "astronaut_pose4.bin")

    import smooth_astronauts
    for i in range(1, 5):
        fn = f"astronaut_pose{i}.bin"
        smooth_astronauts.smooth_pose_mesh(fn, fn, num_subdiv=1, smooth_iters=4)

    print("\nAll 4 poses updated and smoothed successfully!")

if __name__ == "__main__":
    main()
