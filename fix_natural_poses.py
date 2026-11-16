"""
fix_natural_poses.py - Gives Astronaut 2 and Astronaut 4 completely natural arm postures.
Arms hang straight down relaxed along the sides of the thighs, perfectly matching 
the natural look of Astronauts 1 and 3.
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

def rot_z(x, y, z, cx, cy, deg):
    a = math.radians(deg)
    ca, sa = math.cos(a), math.sin(a)
    dx, dy = x - cx, y - cy
    return cx + dx * ca - dy * sa, cy + dx * sa + dy * ca, z

def build_natural_pose2(base_verts):
    # Person 2 (Center-Left / Commander):
    # Tall dignified posture. Arms hang naturally along thighs with gentle drape.
    verts = [list(v) for v in base_verts]
    for v in verts:
        x, y, z = v[0], v[1], v[2]
        # Head
        if y > 1.62:
            v[0], v[1], v[2] = rot_y(x, y, z, 0.0, 0.0, 3.0)
            v[0], v[1], v[2] = rot_x(v[0], v[1], v[2], 1.70, 0.0, -2.0)
        # Right Arm (screen left when viewed from behind):
        # In base mesh, arm is at X > 0.32. Tuck it slightly inward towards body
        elif x > 0.32 and y < 1.42:
            # Subtle tuck inward so arm is close to hip (flare = -4 deg)
            v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], 0.36, 1.35, -4.0)
            # Forearm hangs relaxed with natural drape
            if v[1] < 1.15:
                v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], 0.38, 1.12, -3.0)
                v[0], v[1], v[2] = rot_y(v[0], v[1], v[2], 0.38, 0.0, 5.0)
        # Left Arm (screen right when viewed from behind):
        elif x < -0.32 and y < 1.42:
            # Subtle tuck inward towards body
            v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], -0.36, 1.35, 4.0)
            if v[1] < 1.15:
                v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], -0.38, 1.12, 3.0)
                v[0], v[1], v[2] = rot_y(v[0], v[1], v[2], -0.38, 0.0, -5.0)
    return verts

def build_natural_pose4(base_verts):
    # Person 4 (Far-Right / Engineer):
    # Relaxed natural standing posture. Both arms hang naturally straight down.
    verts = [list(v) for v in base_verts]
    for v in verts:
        x, y, z = v[0], v[1], v[2]
        # Head slightly turned scanning the horizon
        if y > 1.62:
            v[0], v[1], v[2] = rot_y(x, y, z, 0.0, 0.0, -10.0)
            v[0], v[1], v[2] = rot_x(v[0], v[1], v[2], 1.70, 0.0, 1.0)
        # Right Arm (screen left from behind):
        # Tuck arm inward so it does not flare out!
        elif x > 0.32 and y < 1.42:
            v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], 0.36, 1.35, -5.0)
            if v[1] < 1.15:
                v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], 0.38, 1.12, -4.0)
        # Left Arm (screen right from behind):
        elif x < -0.32 and y < 1.42:
            v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], -0.36, 1.35, 3.0)
            if v[1] < 1.15:
                v[0], v[1], v[2] = rot_z(v[0], v[1], v[2], -0.38, 1.12, 2.0)
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
    print("Natural poses 2 and 4 saved and smoothed successfully!")

if __name__ == "__main__":
    main()
