// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Research code for #145 (section 7): can a compiler autovectorize batched orthographic vertex
// projection, in the repository's own layout (array of 3 doubles = 24 bytes per vertex), in float,
// and in a structure-of-arrays layout? Compile-only: see simd_probe_report.sh.
struct Vec3d {
    double x, y, z;
};
struct Vec3f {
    float x, y, z;
};
struct Basis {
    double rx, ry, rz, ux, uy, uz, fx, fy, fz, cx, cy, radius;
};
struct BasisF {
    float rx, ry, rz, ux, uy, uz, fx, fy, fz, cx, cy, radius;
};

// Scalar reference, exactly the repository's arithmetic (double, AoS): to_camera_space + to_screen.
extern "C" void project_aos_double(const Vec3d* v, int n, Basis b, double* sx, double* sy,
                                   unsigned char* front) {
    for (int i = 0; i < n; ++i) {
        const double x = b.rx * v[i].x + b.ry * v[i].y + b.rz * v[i].z;
        const double y = b.ux * v[i].x + b.uy * v[i].y + b.uz * v[i].z;
        const double z = b.fx * v[i].x + b.fy * v[i].y + b.fz * v[i].z;
        sx[i] = b.cx + b.radius * x;
        sy[i] = b.cy - b.radius * y;
        front[i] = z >= 0.0;
    }
}
// Same in float (what a 4-lane AltiVec kernel would have to use).
extern "C" void project_aos_float(const Vec3f* v, int n, BasisF b, float* sx, float* sy,
                                  unsigned char* front) {
    for (int i = 0; i < n; ++i) {
        const float x = b.rx * v[i].x + b.ry * v[i].y + b.rz * v[i].z;
        const float y = b.ux * v[i].x + b.uy * v[i].y + b.uz * v[i].z;
        const float z = b.fx * v[i].x + b.fy * v[i].y + b.fz * v[i].z;
        sx[i] = b.cx + b.radius * x;
        sy[i] = b.cy - b.radius * y;
        front[i] = z >= 0.0f;
    }
}
// SoA float: the layout vectorizers like (needs a one-off transpose or a changed Scene layout).
extern "C" void project_soa_float(const float* __restrict vx, const float* __restrict vy,
                                  const float* __restrict vz, int n, BasisF b, float* __restrict sx,
                                  float* __restrict sy, unsigned char* __restrict front) {
    for (int i = 0; i < n; ++i) {
        const float x = b.rx * vx[i] + b.ry * vy[i] + b.rz * vz[i];
        const float y = b.ux * vx[i] + b.uy * vy[i] + b.uz * vz[i];
        const float z = b.fx * vx[i] + b.fy * vy[i] + b.fz * vz[i];
        sx[i] = b.cx + b.radius * x;
        sy[i] = b.cy - b.radius * y;
        front[i] = z >= 0.0f;
    }
}
