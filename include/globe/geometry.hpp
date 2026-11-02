// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Earth-centered unit vectors and geodetic coordinates on the unit sphere.
//
// Convention (shared with the camera): latitude 0, longitude 0 is (0, 0, 1); positive longitude
// turns toward +x, north is +y. The frame is right-handed (x cross y = z).
#pragma once

namespace globe {

struct Vec3 {
    double x;
    double y;
    double z;
};

struct LatLon {
    double lat_deg; // [-90, 90]
    double lon_deg; // any finite value; normalized by to_unit_vector
};

constexpr double kPi = 3.14159265358979323846;
constexpr double kDegToRad = kPi / 180.0;
constexpr double kRadToDeg = 180.0 / kPi;

constexpr Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
constexpr Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
constexpr Vec3 operator*(Vec3 a, double s) { return {a.x * s, a.y * s, a.z * s}; }
constexpr double dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
constexpr Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

double length(Vec3 v);

// Returns v scaled to unit length. A zero or non-finite vector is returned unchanged.
Vec3 normalized(Vec3 v);

// Latitude is clamped to [-90, 90]; longitude is wrapped, so 180 and -180 give the same vector
// (to rounding) and the poles do not depend on longitude. Non-finite input yields (0, 0, 1).
Vec3 to_unit_vector(LatLon p);

// Inverse of to_unit_vector for a non-zero vector. Longitude is in (-180, 180]; at the poles it
// is defined as 0. The vector need not be exactly unit length.
LatLon from_unit_vector(Vec3 v);

// Angle between two unit vectors, in radians, in [0, pi]. Stable for tiny and near-antipodal angles.
double angle_between(Vec3 a, Vec3 b);

} // namespace globe
