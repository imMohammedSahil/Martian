#ifndef POST_PROCESS_H
#define POST_PROCESS_H

#include "Common.h"

void renderSunDisc3D(float& outWinX, float& outWinY, float& outWinZ, bool& outInFront);
void renderCinemaPost(float flareX, float flareY, bool flareInFront, float flareVisibility);

#endif // POST_PROCESS_H

