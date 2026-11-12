#ifndef SPACECRAFT_H
#define SPACECRAFT_H

#include "Common.h"

void renderEnduranceShip();
void renderRangerLander();
void renderParticles();
void renderSpeedDustStreaks(float camX, float camY, float camZ);
void spawnParticle(float x, float y, float z,
                   float vx, float vy, float vz,
                   float life, float size,
                   float r, float g, float b, bool additive);

#endif // SPACECRAFT_H

