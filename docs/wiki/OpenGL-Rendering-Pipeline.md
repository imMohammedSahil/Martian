# OpenGL Rendering and Optical Pipeline

## 1. Anamorphic Viewport Setup

The display engine forces a 2.39:1 cinema letterbox on any native monitor resolution:

```cpp
int screenWidth = glutGet(GLUT_SCREEN_WIDTH);
int screenHeight = glutGet(GLUT_SCREEN_HEIGHT);
int targetHeight = (int)(screenWidth / 2.39f);
int offsetY = (screenHeight - targetHeight) / 2;

glViewport(0, offsetY, screenWidth, targetHeight);
glScissor(0, offsetY, screenWidth, targetHeight);
glEnable(GL_SCISSOR_TEST);
```

---

## 2. Solar Flare Optical Projection

Solar lens flares are calculated dynamically in screen space:

1. Transform the light vector into clip coordinates using `gluProject()`.
2. Check depth buffer occlusion to compute solar visibility factor $V \in [0.0, 1.0]$.
3. Draw camera-facing quad strips with custom horizontal stretch factors to emulate anamorphic lens flare distortion.

---

## 3. Terrain Ray-Marching & Shadow Calculation

For Scene 4's dune terrain:
- Height values are sampled from an analytical procedural wave function:
  $$h(x, z) = A_1 \sin(f_1 x + \phi_1) \cos(f_2 z) + A_2 \sin(f_3 x + f_4 z)$$
- Hard shadow rays cast toward the low-altitude sun determine surface self-shadowing without requiring a full shadow map pass.
