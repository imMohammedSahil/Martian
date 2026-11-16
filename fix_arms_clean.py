"""
fix_arms_clean.py - Cleanly articulates the arms of Astronaut 2 and Astronaut 4
without touching the legs, giving them completely natural, relaxed downward-hanging arms.
"""

import struct, math
import smooth_astronauts

def load_base_verts():
    with open("astronaut.bin", "rb") as f:
        f.read(4)
        n, = struct.unpack("<I", f.read(4))
        return [list(struct.unpack("<11f", f.read(44))) for _ in range(n)]

def rot_z(x, y, z, cx, cy, deg):
    a = math.radians(deg)
    ca, sa = math.cos(a), math.sin(a)
    dx, dy = x - cx, y - cy
    return cx + dx * ca - dy * sa, cy + dx * sa + dy * ca, z

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

def clampf(v, lo, hi):
    return max(lo, min(hi, v))

def smoothstep(e0, e1, x):
    t = clampf((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)

def build_natural_pose2(base_verts):
    # Astronaut 2 (Commander - Center Left):
    # Stately upright posture. Arms hang naturally down along thighs.
    # Rounded helmet dome visible above chest collar.
    verts = [list(v) for v in base_verts]
    for v in verts:
        x, y, z = v[0], v[1], v[2]
        # Head (Y > 1.62): natural level gaze looking out over dunes
        if y > 1.62:
            v[0], v[1], v[2] = rot_y(x, y, z, 0.0, 0.0, 2.0)
            v[0], v[1], v[2] = rot_x(v[0], v[1], v[2], 1.70, 0.0, 2.0) # slight tilt up to reveal round helmet
        # Right Arm ONLY (X > 0.28 and 0.72 < Y < 1.45):
        elif x > 0.28 and 0.72 < y < 1.45:
            w = 1.0 - smoothstep(1.25, 1.42, y)
            v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], 0.36, 1.35, -17.5 * w)
            # Forearm and hand hang straight down naturally
            if y < 1.12:
                v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], 0.38, 1.10, -3.0)
                v[0], v[1], v[2] = rot_y(v[0], v[1], v[2], 0.38, 0.0, 6.0)
        # Left Arm ONLY (X < -0.28 and 0.72 < Y < 1.45):
        elif x < -0.28 and 0.72 < y < 1.45:
            w = 1.0 - smoothstep(1.25, 1.42, y)
            v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], -0.36, 1.35, 17.5 * w)
            if y < 1.12:
                v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], -0.38, 1.10, 3.0)
                v[0], v[1], v[2] = rot_y(v[0], v[1], v[2], -0.38, 0.0, -6.0)
    return verts

def build_natural_pose4(base_verts):
    # Astronaut 4 (Engineer - Far Right):
    # Relaxed natural standing posture. Both arms hang naturally straight down.
    verts = [list(v) for v in base_verts]
    for v in verts:
        x, y, z = v[0], v[1], v[2]
        # Head (Y > 1.62): head turned scanning left
        if y > 1.62:
            v[0], v[1], v[2] = rot_y(x, y, z, 0.0, 0.0, -9.0)
            v[0], v[1], v[2] = rot_x(v[0], v[1], v[2], 1.70, 0.0, 2.0)
        # Right Arm ONLY (X > 0.28 and 0.72 < Y < 1.45):
        # Tuck inward from the flared A-pose so it hangs straight down!
        elif x > 0.28 and 0.72 < y < 1.45:
            w = 1.0 - smoothstep(1.25, 1.42, y)
            v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], 0.36, 1.35, -16.5 * w)
            if y < 1.12:
                v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], 0.38, 1.10, -2.5)
        # Left Arm ONLY (X < -0.28 and 0.72 < Y < 1.45):
        elif x < -0.28 and 0.72 < y < 1.45:
            w = 1.0 - smoothstep(1.25, 1.42, y)
            v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], -0.36, 1.35, 16.5 * w)
            if y < 1.12:
                v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], -0.38, 1.10, 2.5)
    return verts

def save_bin(filename, verts):
    with open(filename, "wb") as f:
        f.write(b"ASTR")
        f.write(struct.pack("<I", len(verts)))
        for v in verts:
            f.write(struct.pack("<11f", *v))

def main():
    base = load_base_verts()
    print("Generating natural poses for Person 2 and Person 4...")

    p2 = build_natural_pose2(base)
    p4 = build_natural_pose4(base)
    save_bin("astronaut_pose2.bin", p2)
    save_bin("astronaut_pose4.bin", p4)

    # Subdivide & smooth
    smooth_astronauts.smooth_pose_mesh("astronaut_pose2.bin", "astronaut_pose2.bin", num_subdiv=1, smooth_iters=5)
    smooth_astronauts.smooth_pose_mesh("astronaut_pose4.bin", "astronaut_pose4.bin", num_subdiv=1, smooth_iters=5)
    print("Done! Person 2 and Person 4 now have natural, relaxed arms.")

if __name__ == "__main__":
    main()
