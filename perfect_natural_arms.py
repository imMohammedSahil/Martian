"""
perfect_natural_arms.py - Rotates both arms of Astronaut 2 and Astronaut 4 inward
so they hang naturally straight down under the shoulder contour, with hands naturally 
relaxed along the thigh seams.
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
    # Dignified upright posture. Both arms hang naturally straight down.
    # Level head gaze looking out across the dunes.
    verts = [list(v) for v in base_verts]
    for v in verts:
        x, y, z = v[0], v[1], v[2]
        # Head (Y > 1.62)
        if y > 1.62:
            v[0], v[1], v[2] = rot_y(x, y, z, 0.0, 0.0, 1.0)
            v[0], v[1], v[2] = rot_x(v[0], v[1], v[2], 1.70, 0.0, 3.0) # reveal round helmet dome
        # Right Arm (X > 0.28, 0.72 < Y < 1.45)
        elif x > 0.28 and 0.72 < y < 1.45:
            # Smooth shoulder blending
            w = 1.0 - smoothstep(1.24, 1.42, y)
            # Full 24 degree tuck brings arm directly under shoulder line
            v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], 0.36, 1.35, -24.0 * w)
            # Forearm and hand curl naturally inward along thigh
            if y < 1.05:
                v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], 0.34, 1.00, -5.0)
                v[0], v[1], v[2] = rot_y(v[0], v[1], v[2], 0.34, 0.0, 8.0)
        # Left Arm (X < -0.28, 0.72 < Y < 1.45)
        elif x < -0.28 and 0.72 < y < 1.45:
            w = 1.0 - smoothstep(1.24, 1.42, y)
            v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], -0.36, 1.35, 24.0 * w)
            if y < 1.05:
                v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], -0.34, 1.00, 5.0)
                v[0], v[1], v[2] = rot_y(v[0], v[1], v[2], -0.34, 0.0, -8.0)
    return verts

def build_natural_pose4(base_verts):
    # Astronaut 4 (Engineer - Far Right):
    # Relaxed natural stance. Both arms hang naturally straight down.
    # Head turned slightly scanning horizon.
    verts = [list(v) for v in base_verts]
    for v in verts:
        x, y, z = v[0], v[1], v[2]
        if y > 1.62:
            v[0], v[1], v[2] = rot_y(x, y, z, 0.0, 0.0, -8.0)
            v[0], v[1], v[2] = rot_x(v[0], v[1], v[2], 1.70, 0.0, 2.0)
        # Right Arm (screen left arm from behind)
        elif x > 0.28 and 0.72 < y < 1.45:
            w = 1.0 - smoothstep(1.24, 1.42, y)
            v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], 0.36, 1.35, -23.0 * w)
            if y < 1.05:
                v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], 0.34, 1.00, -4.0)
        # Left Arm (screen right arm from behind)
        elif x < -0.28 and 0.72 < y < 1.45:
            w = 1.0 - smoothstep(1.24, 1.42, y)
            v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], -0.36, 1.35, 23.0 * w)
            if y < 1.05:
                v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], -0.34, 1.00, 4.0)
    return verts

def save_bin(filename, verts):
    with open(filename, "wb") as f:
        f.write(b"ASTR")
        f.write(struct.pack("<I", len(verts)))
        for v in verts:
            f.write(struct.pack("<11f", *v))

def main():
    base = load_base_verts()
    print("Generating natural hanging arm poses for Astronaut 2 and Astronaut 4...")

    p2 = build_natural_pose2(base)
    p4 = build_natural_pose4(base)
    save_bin("astronaut_pose2.bin", p2)
    save_bin("astronaut_pose4.bin", p4)

    # Subdivide & smooth
    smooth_astronauts.smooth_pose_mesh("astronaut_pose2.bin", "astronaut_pose2.bin", num_subdiv=1, smooth_iters=5)
    smooth_astronauts.smooth_pose_mesh("astronaut_pose4.bin", "astronaut_pose4.bin", num_subdiv=1, smooth_iters=5)
    print("Done! Both Person 2 and Person 4 arms updated and smoothed.")

if __name__ == "__main__":
    main()
