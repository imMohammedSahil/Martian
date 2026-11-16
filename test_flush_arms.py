"""
test_flush_arms.py - Pulls the arms inward against the torso/thighs
to completely eliminate the unnatural V-shape gap.
"""

import struct, math
import smooth_astronauts

def load_base_verts():
    with open("astronaut.bin", "rb") as f:
        f.read(4)
        n, = struct.unpack("<I", f.read(4))
        return [list(struct.unpack("<11f", f.read(44))) for _ in range(n)]

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

def build_flush_arms_mesh(base_verts, head_yaw=0.0):
    verts = [list(v) for v in base_verts]
    for v in verts:
        x, y, z = v[0], v[1], v[2]
        # Head
        if y > 1.62:
            if head_yaw != 0.0:
                v[0], v[1], v[2] = rot_y(x, y, z, 0.0, 0.0, head_yaw)
            v[0], v[1], v[2] = rot_x(v[0], v[1], v[2], 1.70, 0.0, 3.0) # slight tilt up to show round helmet
        # Right Arm (X > 0.26 and 0.72 < Y < 1.45):
        # Pull arm inward against the torso/thigh to eliminate V-gap
        elif x > 0.26 and 0.72 < y < 1.45:
            w = 1.0 - smoothstep(1.22, 1.42, y)
            # Pull inward by 0.13m and slightly back
            v[0] -= 0.13 * w
            v[2] -= 0.03 * w
        # Left Arm (X < -0.26 and 0.72 < Y < 1.45):
        elif x < -0.26 and 0.72 < y < 1.45:
            w = 1.0 - smoothstep(1.22, 1.42, y)
            # Pull inward by 0.13m and slightly back
            v[0] += 0.13 * w
            v[2] -= 0.03 * w
    return verts

def save_bin(filename, verts):
    with open(filename, "wb") as f:
        f.write(b"ASTR")
        f.write(struct.pack("<I", len(verts)))
        for v in verts:
            f.write(struct.pack("<11f", *v))

def main():
    base = load_base_verts()
    print("Generating flush, natural arms for Astronaut 2 and Astronaut 4...")

    p2 = build_flush_arms_mesh(base, head_yaw=1.0)
    p4 = build_flush_arms_mesh(base, head_yaw=-8.0)

    save_bin("astronaut_pose2.bin", p2)
    save_bin("astronaut_pose4.bin", p4)

    smooth_astronauts.smooth_pose_mesh("astronaut_pose2.bin", "astronaut_pose2.bin", num_subdiv=1, smooth_iters=5)
    smooth_astronauts.smooth_pose_mesh("astronaut_pose4.bin", "astronaut_pose4.bin", num_subdiv=1, smooth_iters=5)
    print("Done! Astronaut 2 and 4 arms are now flush and natural.")

if __name__ == "__main__":
    main()
