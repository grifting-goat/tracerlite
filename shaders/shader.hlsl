[[vk::binding(1, 0)]] ByteAddressBuffer voxelsIn;

struct Material {
    float4 color;
};
[[vk::binding(2, 0)]] StructuredBuffer<Material> materials;

[[vk::binding(3, 0)]]

cbuffer shader_data {
    float4 _camera_position;
    float4 _camera_rotation;

    int2 _screen_size;
    int pixel_count;

    float _tan_fov_v;
    float _tan_fov_h;

    int3 _voxel_grid_size;
};

[[vk::binding(4, 0)]]
[[vk::image_format("rgba8")]]
RWTexture2D<float4> outputImage;

struct PushConstants {
    uint voxelCount;
};
[[vk::push_constant]] PushConstants pc;

static const float FLT_INF = asfloat(0x7F800000);
static const float MAX_RAY_DISTANCE = 40.0f;

struct Hit {
    int type;
    float dist;
};

struct Ray {
    float3 origin;
    float3 direction;
    float3 color;
};


float3 RotateVector(float3 vect, float3 rotation) {
    float xr = rotation.x;
    float yr = rotation.y;
    float zr = rotation.z;

    float3x3 Rx = float3x3(
        1,  0,  0,
        0,  cos(xr),    -sin(xr),
        0,  sin(xr),    cos(xr)
    );

    float3x3 Ry = float3x3(
        cos(yr),    0,    sin(yr),
        0,  1,  0,
        -sin(yr),   0,   cos(yr)
    );

    float3x3 Rz = float3x3(
        cos(zr), -sin(zr),  0,
        sin(zr),  cos(zr),  0,
        0,  0,  1
    );

    return mul(mul(mul(Ry, Rx), Rz), vect);
}

Ray CreateRay(float3 origin, float3 direction) {
    Ray ray;
    ray.origin = origin;
    ray.direction = normalize(direction);
    return ray;
}

Ray CreateCameraRay(float x, float y) {
    x = (x - 0.5f) * 2.0f;
    y = (y - 0.5f) * 2.0f;

    float3 plane = float3(x * _tan_fov_h, y * _tan_fov_v, 1.0f);
    float3 direction = RotateVector(normalize(plane), _camera_rotation.xyz);

    return CreateRay(_camera_position.xyz, direction);
}

uint ReadVoxel(uint index) {
    uint word = voxelsIn.Load(index & ~3u);
    uint shift = (index & 3u) * 8u;
    return (word >> shift) & 0xFFu;
}

Hit CastRay(Ray ray) {

    int x = int(ray.origin.x);
    int y = int(ray.origin.y);
    int z = int(ray.origin.z);

    int stepX = (ray.direction.x > 0) ? 1 : ((ray.direction.x < 0) ? -1 : 0);
    int stepY = (ray.direction.y > 0) ? 1 : ((ray.direction.y < 0) ? -1 : 0);
    int stepZ = (ray.direction.z > 0) ? 1 : ((ray.direction.z < 0) ? -1 : 0);

    float tDeltaX = (ray.direction.x != 0) ? abs(1.0f / ray.direction.x) : FLT_INF;
    float tDeltaY = (ray.direction.y != 0) ? abs(1.0f / ray.direction.y) : FLT_INF;
    float tDeltaZ = (ray.direction.z != 0) ? abs(1.0f / ray.direction.z) : FLT_INF;

    float nextBoundaryX = (stepX > 0) ? floor(ray.origin.x) + 1.0f : floor(ray.origin.x);
    float nextBoundaryY = (stepY > 0) ? floor(ray.origin.y) + 1.0f : floor(ray.origin.y);
    float nextBoundaryZ = (stepZ > 0) ? floor(ray.origin.z) + 1.0f : floor(ray.origin.z);

    float tMaxX = (ray.direction.x != 0) ? (nextBoundaryX - ray.origin.x) / ray.direction.x : FLT_INF;
    float tMaxY = (ray.direction.y != 0) ? (nextBoundaryY - ray.origin.y) / ray.direction.y : FLT_INF;
    float tMaxZ = (ray.direction.z != 0) ? (nextBoundaryZ - ray.origin.z) / ray.direction.z : FLT_INF;

    Hit hit;
    hit.type = 0;
    hit.dist = FLT_INF;

    if (x < 0 || x >= _voxel_grid_size.x ||
        y < 0 || y >= _voxel_grid_size.y ||
        z < 0 || z >= _voxel_grid_size.z) {
        return hit;
    }

    float tCurrent = 0.0f;

    while (hit.type == 0) {
        if (tCurrent > MAX_RAY_DISTANCE) {
            return hit;
        }

        uint voxelIndex = uint(x) + uint(y) * uint(_voxel_grid_size.x) + uint(z) * uint(_voxel_grid_size.x) * uint(_voxel_grid_size.y);

        hit.type = int(ReadVoxel(voxelIndex));
        if (hit.type != 0) {
            hit.dist = tCurrent;
            break;
        }

        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                tCurrent = tMaxX;
                x = x + stepX;
                tMaxX = tMaxX + tDeltaX;
                if (x < 0 || x >= _voxel_grid_size.x) { return hit; }
            }
            else {
                tCurrent = tMaxZ;
                z = z + stepZ;
                tMaxZ = tMaxZ + tDeltaZ;
                if (z < 0 || z >= _voxel_grid_size.z) { return hit; }
            }
        }
        else {
            if (tMaxY < tMaxZ) {
                tCurrent = tMaxY;
                y = y + stepY;
                tMaxY = tMaxY + tDeltaY;
                if (y < 0 || y >= _voxel_grid_size.y) { return hit; }
            }
            else {
                tCurrent = tMaxZ;
                z = z + stepZ;
                tMaxZ = tMaxZ + tDeltaZ;
                if (z < 0 || z >= _voxel_grid_size.z) { return hit; }
            }
        }
    }

    return hit;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID) {
    int2 pixel = int2(dispatchThreadID.xy);
    if (pixel.x >= _screen_size.x || pixel.y >= _screen_size.y) {
        return;
    }

    float x = (float(pixel.x) + 0.5f) / float(_screen_size.x);
    float y = (float(pixel.y) + 0.5f) / float(_screen_size.y);

    Ray r = CreateCameraRay(x, y);
    Hit hit = CastRay(r);

    float alpha = hit.dist / MAX_RAY_DISTANCE;
    if (alpha > 1.0) {alpha = 1.0;}

    float shade = 1.0f - alpha;

    float4 color = hit.type ? float4(shade * materials[hit.type].color.rgb, 1.0f) : float4(0.0f, 0.0f, 0.0f, 1.0f);
    outputImage[pixel] = color;
}
