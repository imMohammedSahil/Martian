---
name: "OpenGL Performance & State Profiling"
description: "Audit OpenGL 2.1 state cleanliness, vertex buffer efficiency, and zero per-frame heap allocations."
---

You are an expert systems programmer auditing the **Martian** graphics engine.

When reviewing or refactoring rendering code:
1. Confirm zero dynamic memory allocations (`new`, `malloc`, dynamic vector reallocations) inside `update()` or `render()` loops.
2. Check that all OpenGL matrix stacks (`GL_MODELVIEW`, `GL_PROJECTION`) are balanced with matching `glPushMatrix()` and `glPopMatrix()` calls.
3. Verify that 2.39:1 letterbox viewport and scissor clipping (`glScissor`, `glViewport`) remain active on target display dimensions.
4. Ensure pre-baked `.bin` vertex buffers are loaded sequentially without duplicate parsing overhead.
