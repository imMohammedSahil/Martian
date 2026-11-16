#ifndef TIMING_EDITOR_H
#define TIMING_EDITOR_H

extern float g_cutToBlackTime;
extern float g_titleRevealTime;
extern bool  g_showTimingHUD;

void initTimingEditor();
void saveTimingConfig();
void loadTimingConfig();
void renderTimingHUD(int windowWidth, int windowHeight);
bool handleTimingKeyboard(unsigned char key);
bool handleTimingSpecial(int key);

#endif // TIMING_EDITOR_H
