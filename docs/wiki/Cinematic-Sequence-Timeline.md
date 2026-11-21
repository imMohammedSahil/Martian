# Cinematic Sequence and Timeline Pacing

## 1. Timeline Overview

The entire presentation executes along a single continuous 84.3-second timeline driven by `SceneDirector`.

```
0.0s          24.0s                 44.0s                 66.0s           84.3s
 |--------------|---------------------|---------------------|---------------|
    Scene 1            Scene 2               Scene 3             Scene 4
 Orbit & Title       Separation          Hypersonic Entry     Dune Horizon
```

---

## 2. Scene Breakdown

### Scene 1: Orbit & Title (0.00s - 24.00s)
- **Camera**: Slow spherical interpolation orbiting the Endurance spacecraft.
- **Visuals**: Deep starfield, Uranus, Jupiter, Saturn with procedural alpha-tested rings, and anamorphic lens flare projection from the primary solar source.
- **Title Card**: Fades in at 12.0s with smoothed Gaussian alpha ramp.

### Scene 2: Separation & RCS Thrust (24.00s - 44.00s)
- **Maneuver**: Ranger 1 undocks from the central Endurance ring.
- **Kinematics**: Cold-gas RCS thruster bursts rendered using additive billboard alpha particles. Camera pans to capture separation vector.

### Scene 3: Hypersonic Entry & Descent (44.00s - 66.00s)
- **Transition**: Hard cut at 44.0s aligned with soundtrack surge.
- **Atmosphere**: Aerodynamic heating envelope, shockwave plasma sheath, and supersonic deceleration over Gale Crater terrain (STL binary mesh).
- **Parachute**: High-altitude supersonic drogue deployment with dynamic cloth deformation.

### Scene 4: The Dune Horizon (66.00s - 84.30s)
- **Composition**: Wide 2.39:1 low-angle shot across Martian sand dunes.
- **Lighting**: Low-altitude sunset with ray-marched terrain shadows and edge-to-edge Martian horizon coverage.
- **Finale**: Astronaut silhouette with subtle idle helmet reflection; freeze-frame hold at 84.3s.
