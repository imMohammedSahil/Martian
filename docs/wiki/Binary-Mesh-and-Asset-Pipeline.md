# Binary Mesh and Asset Pipeline

## 1. Specification

To bypass runtime OBJ/STL parsing overhead, all 3D geometries are serialized into pre-baked binary formats (`.bin`).

### Header & Vertex Payload Layout

```
[ Magic Identifier: 4 bytes (e.g. 'MSHB') ]
[ Vertex Count: uint32 ]
[ Vertex Data: VertexCount * sizeof(PackedVertex) ]
```

Where `PackedVertex` is defined as:

```cpp
struct PackedVertex {
    float x, y, z;        // Position (12 bytes)
    float nx, ny, nz;     // Normal vector (12 bytes)
    float u, v;           // Texture coordinates (8 bytes)
    uint32_t color;       // Packed RGBA color (4 bytes)
};
```

---

## 2. Asset Conversion Scripts

- `apply_natural_arms.py`: Adjusts skeletal orientation for astronaut meshes.
- `smooth_astronauts.py`: Calculates averaged surface normals to eliminate hard polygonal creases.
- `pose_editor.py`: Generates keyframe poses (`astronaut_pose1.bin` through `astronaut_pose4.bin`).
