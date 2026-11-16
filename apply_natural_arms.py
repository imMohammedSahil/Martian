"""
apply_natural_arms.py - Perfectly drapes the arms of Astronaut 2 and Astronaut 4
along their thighs with smooth skinning weights, completely eliminating flared A/V poses.
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

def clampf(v, lo, hi):
    return max(lo, min(hi, v))

def smoothstep(e0, e1, x):
    t = clampf((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)

def build_natural_standing_mesh(base_verts, r_tuck_deg, l_tuck_deg, head_yaw=0.0):
    verts = [list(v) for v in base_verts]
    for v in verts:
        x, y, z = v[0], v[1], v[2]
        # Head
        if y > 1.62 and head_yaw != 0.0:
            v[0], v[1], v[2] = rot_y(x, y, z, 0.0, 0.0, head_yaw)
        # Right Arm (X > 0.25): tuck inward towards body with smooth shoulder weight
        elif x > 0.25 and y < 1.45:
            # Weight: 0 at shoulder top (1.42), 1 below armpit (1.22)
            w = 1.0 - smoothstep(1.22, 1.42, y)
            eff_angle = -r_tuck_deg * w
            v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], 0.36, 1.35, eff_angle)
        # Left Arm (X < -0.25): tuck inward towards body with smooth shoulder weight
        elif x < -0.25 and y < 1.45:
            w = 1.0 - smoothstep(1.22, 1.42, y)
            eff_angle = l_tuck_deg * w
            v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], -0.36, 1.35, eff_angle)
    return verts

def save_bin(filename, verts):
    with open(filename, "wb") as f:
        f.write(b"ASTR")
        f.write(struct.pack("<I", len(verts)))
        for v in verts:
            f.write(struct.pack("<11f", *v))

def main():
    base = load_base_verts()
    print("Generating smoothly draped natural arms for Person 2 and Person 4...")

    # Person 2 (Commander): arms hang naturally straight along thighs
    p2 = build_natural_standing_mesh(base, r_tuck_deg=17.5, l_tuck_deg=17.5, head_yaw=2.0)
    save_bin("astronaut_pose2.bin", p2)

    # Person 4 (Engineer): arms hang naturally straight along thighs, slight head turn
    p4 = build_natural_standing_mesh(base, r_tuck_deg=16.0, l_tuck_deg=16.0, head_yaw=-8.0)
    save_bin("astronaut_pose4.bin", p4)

    # Subdivide & smooth both to 19,248 vertices
    smooth_astronauts.smooth_pose_mesh("astronaut_pose2.bin", "astronaut_pose2.bin", num_subdiv=1, smooth_iters=5)
    smooth_astronauts.smooth_pose_mesh("astronaut_pose4.bin", "astronaut_pose4.bin", num_subdiv=1, smooth_iters=5)
    print("Done! Both Person 2 and Person 4 now have perfectly natural, relaxed arms.")

if __name__ == "__main__":
    main()
