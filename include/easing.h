#ifndef EASING_H
#define EASING_H


typedef float (*easingFunc)(float, float, float);

typedef struct {
    const char *name;
    easingFunc func ;
} EasingEntry;

extern float linear(float t, float start, float end);
extern float smoothstep(float t, float start, float end);
extern float smootherstep(float t, float start, float end);

extern const EasingEntry easingTable[];
extern const int easingTableCount;

EasingEntry getEasingFunc(const char* name);

#endif