"""
smooth_astronauts.py - Smooths existing astronaut poses and increases polygon density.
Preserves existing poses, heights, and silhouettes while eliminating jagged stepping,
kinks, and faceted edges on arms, hands, elbows, and helmets.
"""

import struct, math, os
from collections import defaultdict

def smooth_pose_mesh(input_file, output_file, num_subdiv=1, smooth_iters=5):
    print(f"Processing {input_file} -> {output_file}...")
    with open(input_file, "rb") as f:
        magic = f.read(4)
        num_verts, = struct.unpack("<I", f.read(4))
        verts = [list(struct.unpack("<11f", f.read(44))) for _ in range(num_verts)]

    print(f"  Loaded {len(verts)} raw vertices ({len(verts)//3} triangles)")

    # 1. Midpoint Subdivision
    for s in range(num_subdiv):
        pos_map = {}
        unique_pos = []
        vert_to_unique = []
        for v in verts:
            k = (round(v[0], 5), round(v[1], 5), round(v[2], 5))
            if k not in pos_map:
                pos_map[k] = len(unique_pos)
                unique_pos.append(list(v))
            vert_to_unique.append(pos_map[k])

        edge_midpoints = {}
        new_unique = [list(u) for u in unique_pos]

        def get_edge_mid(u_a, u_b, v_a, v_b):
            edge_key = (min(u_a, u_b), max(u_a, u_b))
            if edge_key in edge_midpoints:
                return edge_midpoints[edge_key]
            # Interpolate all 11 attributes
            mid_v = [0.5 * (v_a[i] + v_b[i]) for i in range(11)]
            # Normalize normal
            nx, ny, nz = mid_v[3], mid_v[4], mid_v[5]
            l = math.sqrt(nx*nx + ny*ny + nz*nz)
            if l > 1e-6:
                mid_v[3], mid_v[4], mid_v[5] = nx/l, ny/l, nz/l
            mid_u = len(new_unique)
            new_unique.append(mid_v)
            edge_midpoints[edge_key] = (mid_u, mid_v)
            return mid_u, mid_v

        new_verts = []
        num_tri = len(verts) // 3
        for t in range(num_tri):
            i0, i1, i2 = t * 3, t * 3 + 1, t * 3 + 2
            v0, v1, v2 = verts[i0], verts[i1], verts[i2]
            u0, u1, u2 = vert_to_unique[i0], vert_to_unique[i1], vert_to_unique[i2]

            u01, m01 = get_edge_mid(u0, u1, v0, v1)
            u12, m12 = get_edge_mid(u1, u2, v1, v2)
            u20, m20 = get_edge_mid(u2, u0, v2, v0)

            new_verts.extend([v0, m01, m20])
            new_verts.extend([v1, m12, m01])
            new_verts.extend([v2, m20, m12])
            new_verts.extend([m01, m12, m20])

        verts = new_verts
        print(f"  Subdivided: {len(verts)} vertices ({len(verts)//3} triangles)")

    # 2. Build Adjacency Graph for Smoothing
    pos_map = {}
    unique_pos = []
    vert_to_unique = []
    for v in verts:
        k = (round(v[0], 5), round(v[1], 5), round(v[2], 5))
        if k not in pos_map:
            pos_map[k] = len(unique_pos)
            unique_pos.append([v[0], v[1], v[2]])
        vert_to_unique.append(pos_map[k])

    adj = defaultdict(set)
    for t in range(len(verts) // 3):
        u0 = vert_to_unique[t * 3 + 0]
        u1 = vert_to_unique[t * 3 + 1]
        u2 = vert_to_unique[t * 3 + 2]
        if u0 != u1: adj[u0].add(u1); adj[u1].add(u0)
        if u1 != u2: adj[u1].add(u2); adj[u2].add(u1)
        if u2 != u0: adj[u2].add(u0); adj[u0].add(u2)

    # 3. Taubin Non-Shrinking Mesh Smoothing
    pos = [list(p) for p in unique_pos]
    lam = 0.42
    mu = -0.44

    for it in range(smooth_iters):
        # Step 1: positive step (smoothing)
        new_pos = [list(p) for p in pos]
        for i, nbrs in adj.items():
            if not nbrs: continue
            avg = [sum(pos[n][c] for n in nbrs) / len(nbrs) for c in range(3)]
            for c in range(3):
                new_pos[i][c] += lam * (avg[c] - pos[i][c])
        pos = new_pos

        # Step 2: negative step (volume compensation)
        new_pos = [list(p) for p in pos]
        for i, nbrs in adj.items():
            if not nbrs: continue
            avg = [sum(pos[n][c] for n in nbrs) / len(nbrs) for c in range(3)]
            for c in range(3):
                new_pos[i][c] += mu * (avg[c] - pos[i][c])
        pos = new_pos

    # 4. Extra targeted arm/hand joint relaxation to remove sharp kinks
    for it in range(3):
        new_pos = [list(p) for p in pos]
        for i, nbrs in adj.items():
            if not nbrs: continue
            x, y = pos[i][0], pos[i][1]
            # If in arm/hand/elbow zone, smooth extra
            if abs(x) > 0.22 and 0.60 < y < 1.45:
                avg = [sum(pos[n][c] for n in nbrs) / len(nbrs) for c in range(3)]
                for c in range(3):
                    new_pos[i][c] += 0.25 * (avg[c] - pos[i][c])
        pos = new_pos

    # Re-apply smoothed positions to verts
    for i, v in enumerate(verts):
        u = vert_to_unique[i]
        v[0], v[1], v[2] = pos[u][0], pos[u][1], pos[u][2]

    # 5. Recompute Smooth Normals
    for t in range(len(verts) // 3):
        i0, i1, i2 = t * 3, t * 3 + 1, t * 3 + 2
        p0, p1, p2 = verts[i0], verts[i1], verts[i2]
        d1 = [p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]]
        d2 = [p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]]
        nx = d1[1] * d2[2] - d1[2] * d2[1]
        ny = d1[2] * d2[0] - d1[0] * d2[2]
        nz = d1[0] * d2[1] - d1[1] * d2[0]
        l = math.sqrt(nx*nx + ny*ny + nz*nz)
        if l > 1e-6:
            nx, ny, nz = nx/l, ny/l, nz/l
            for idx in [i0, i1, i2]:
                verts[idx][3] = nx
                verts[idx][4] = ny
                verts[idx][5] = nz

    # Save to binary file
    with open(output_file, "wb") as f:
        f.write(b"ASTR")
        f.write(struct.pack("<I", len(verts)))
        for v in verts:
            f.write(struct.pack("<11f", *v))

    print(f"  [OK] Successfully saved {output_file} ({len(verts)} smooth vertices)\n")

def main():
    poses = [
        ("astronaut_pose1.bin", "astronaut_pose1.bin"),
        ("astronaut_pose2.bin", "astronaut_pose2.bin"),
        ("astronaut_pose3.bin", "astronaut_pose3.bin"),
        ("astronaut_pose4.bin", "astronaut_pose4.bin")
    ]
    for inp, outp in poses:
        if os.path.exists(inp):
            smooth_pose_mesh(inp, outp, num_subdiv=1, smooth_iters=5)

if __name__ == "__main__":
    main()
