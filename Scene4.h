#ifndef SCENE4_H
#define SCENE4_H

#include "Common.h"

void updateScene4(float dt);
void computeCameraScene4(float& eyeX, float& eyeY, float& eyeZ,
                         float& targetX, float& targetY, float& targetZ,
                         float& upX, float& upY, float& upZ);
void renderScene4();
float getScene4BloomStrength();
void reloadScene4Lighting();
void renderScene4HUD(int windowWidth, int windowHeight);
bool handleScene4Keyboard(unsigned char key);
bool handleScene4Special(int key);

#endif // SCENE4_H
