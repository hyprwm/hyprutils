#include <hyprutils/math/Matrix.hpp>
#include <unordered_map>
#include <cstring>
#include <cmath>

void matrixIdentity(float mat[9]) {
    static constexpr float identity[9] = {
        1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    };
    memcpy(mat, identity, sizeof(identity));
}

void matrixMultiply(float mat[9], const float a[9], const float b[9]) {
    float product[9];

    product[0] = a[0] * b[0] + a[1] * b[3] + a[2] * b[6];
    product[1] = a[0] * b[1] + a[1] * b[4] + a[2] * b[7];
    product[2] = a[0] * b[2] + a[1] * b[5] + a[2] * b[8];

    product[3] = a[3] * b[0] + a[4] * b[3] + a[5] * b[6];
    product[4] = a[3] * b[1] + a[4] * b[4] + a[5] * b[7];
    product[5] = a[3] * b[2] + a[4] * b[5] + a[5] * b[8];

    product[6] = a[6] * b[0] + a[7] * b[3] + a[8] * b[6];
    product[7] = a[6] * b[1] + a[7] * b[4] + a[8] * b[7];
    product[8] = a[6] * b[2] + a[7] * b[5] + a[8] * b[8];

    memcpy(mat, product, sizeof(product));
}

void matrixTranspose(float mat[9], const float a[9]) {
    const float transposition[9] = {
        a[0], a[3], a[6], a[1], a[4], a[7], a[2], a[5], a[8],
    };
    memcpy(mat, transposition, sizeof(transposition));
}

void matrixTranslate(float mat[9], float x, float y) {
    const float translate[9] = {
        1.0f, 0.0f, x, 0.0f, 1.0f, y, 0.0f, 0.0f, 1.0f,
    };
    matrixMultiply(mat, mat, translate);
}

void matrixScale(float mat[9], float x, float y) {
    const float scale[9] = {
        x, 0.0f, 0.0f, 0.0f, y, 0.0f, 0.0f, 0.0f, 1.0f,
    };
    matrixMultiply(mat, mat, scale);
}

void matrixRotate(float mat[9], float rad) {
    const float rotate[9] = {
        (float)cos(rad), (float)-sin(rad), 0.0f, (float)sin(rad), (float)cos(rad), 0.0f, 0.0f, 0.0f, 1.0f,
    };
    matrixMultiply(mat, mat, rotate);
}

std::unordered_map<eTransform, std::array<float, 9>> transforms = {
    {HYPRUTILS_TRANSFORM_NORMAL,
     {
         1.0f,
         0.0f,
         0.0f,
         0.0f,
         1.0f,
         0.0f,
         0.0f,
         0.0f,
         1.0f,
     }},
    {HYPRUTILS_TRANSFORM_90,
     {
         0.0f,
         1.0f,
         0.0f,
         -1.0f,
         0.0f,
         0.0f,
         0.0f,
         0.0f,
         1.0f,
     }},
    {HYPRUTILS_TRANSFORM_180,
     {
         -1.0f,
         0.0f,
         0.0f,
         0.0f,
         -1.0f,
         0.0f,
         0.0f,
         0.0f,
         1.0f,
     }},
    {HYPRUTILS_TRANSFORM_270,
     {
         0.0f,
         -1.0f,
         0.0f,
         1.0f,
         0.0f,
         0.0f,
         0.0f,
         0.0f,
         1.0f,
     }},
    {HYPRUTILS_TRANSFORM_FLIPPED,
     {
         -1.0f,
         0.0f,
         0.0f,
         0.0f,
         1.0f,
         0.0f,
         0.0f,
         0.0f,
         1.0f,
     }},
    {HYPRUTILS_TRANSFORM_FLIPPED_90,
     {
         0.0f,
         1.0f,
         0.0f,
         1.0f,
         0.0f,
         0.0f,
         0.0f,
         0.0f,
         1.0f,
     }},
    {HYPRUTILS_TRANSFORM_FLIPPED_180,
     {
         1.0f,
         0.0f,
         0.0f,
         0.0f,
         -1.0f,
         0.0f,
         0.0f,
         0.0f,
         1.0f,
     }},
    {HYPRUTILS_TRANSFORM_FLIPPED_270,
     {
         0.0f,
         -1.0f,
         0.0f,
         -1.0f,
         0.0f,
         0.0f,
         0.0f,
         0.0f,
         1.0f,
     }},
};

void matrixTransform(float mat[9], const eTransform transform) {
    matrixMultiply(mat, mat, transforms.at(transform).data());
}

void matrixProjection(float mat[9], const int width, const int height, const eTransform transform) {
    memset(mat, 0, sizeof(*mat) * 9);

    const float* t = transforms.at(transform).data();
    const float  x = 2.0f / width;
    const float  y = 2.0f / height;

    // Rotation + reflection
    mat[0] = x * t[0];
    mat[1] = x * t[1];
    mat[3] = y * -t[3];
    mat[4] = y * -t[4];

    // Translation
    mat[2] = -copysign(1.0f, mat[0] + mat[1]);
    mat[5] = -copysign(1.0f, mat[3] + mat[4]);

    // Identity
    mat[8] = 1.0f;
}

void projectBox(float mat[9], const CBox& box, const eTransform transform, const float rotation, const float projection[9]) {
    const double x      = box.x;
    const double y      = box.y;
    const double width  = box.width;
    const double height = box.height;

    matrixIdentity(mat);
    matrixTranslate(mat, x, y);

    if (rotation != 0) {
        matrixTranslate(mat, width / 2, height / 2);
        matrixRotate(mat, rotation);
        matrixTranslate(mat, -width / 2, -height / 2);
    }

    matrixScale(mat, width, height);

    if (transform != HYPRUTILS_TRANSFORM_NORMAL) {
        matrixTranslate(mat, 0.5, 0.5);
        matrixTransform(mat, transform);
        matrixTranslate(mat, -0.5, -0.5);
    }

    matrixMultiply(mat, projection, mat);
}
