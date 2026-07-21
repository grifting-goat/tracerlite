#include "camera.h"

Camera create_cam(float fov, float aspect) {
    Camera cam = {0};
    cam.pos[0] = 50.0;
    cam.pos[1] = 50.0;
    cam.pos[2] = 50.0;
    cam.fov = fov;
    cam.aspect = aspect;
    cam.fov_v = fov / 2.0f;
    cam.fov_h = atanf(tanf(cam.fov_v) * aspect);

    return cam;
}