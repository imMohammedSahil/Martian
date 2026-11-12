#ifndef SCENE_DIRECTOR_H
#define SCENE_DIRECTOR_H

#include "Common.h"

void updateSceneDirector(float dt);
void computeCinematicCamera(float& eyeX, float& eyeY, float& eyeZ,
                            float& targetX, float& targetY, float& targetZ,
                            float& upX, float& upY, float& upZ);
float computeSunMarsOcclusion(float camX, float camY, float camZ);

// Scene 1 specific handlers
void updateScene1(float dt);
void computeCameraScene1(float& eyeX, float& eyeY, float& eyeZ,
                        float& targetX, float& targetY, float& targetZ,
                        float& upX, float& upY, float& upZ);

#endif // SCENE_DIRECTOR_H

