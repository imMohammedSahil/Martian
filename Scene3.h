#ifndef SCENE3_H
#define SCENE3_H

#include "Common.h"

void initScene3();
void updateScene3(float dt);
void computeCameraScene3(float& eyeX, float& eyeY, float& eyeZ,
                        float& targetX, float& targetY, float& targetZ,
                        float& upX, float& upY, float& upZ);
void renderScene3();

#endif // SCENE3_H
