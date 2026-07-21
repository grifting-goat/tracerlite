#ifndef CAMERA_H
#define CAMERA_H

#include <math.h>


typedef struct Camera {

    float pos[3];
    float angle[3];

    float fov;
    float fov_v;
    float fov_h;
    float aspect;

} Camera;


Camera create_cam(float fov, float aspect);

#endif //CAMERA_H