#pragma once
#include <cmath>
#include <algorithm>

struct Vec3 {
    float x, y, z;
    Vec3 operator+(const Vec3& other) const { return {x + other.x, y + other.y, z + other.z}; }
    Vec3 operator-(const Vec3& other) const { return {x - other.x, y - other.y, z - other.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3& operator+=(const Vec3& other) { x += other.x; y += other.y; z += other.z; return *this; }
};

inline Vec3 normalize(Vec3 v) {
    float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len == 0) return {0, 0, 0};
    return {v.x / len, v.y / len, v.z / len};
}

inline Vec3 cross(Vec3 a, Vec3 b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

class Camera {
public:
    Vec3 position = {0.0f, 2.0f, 5.0f};
    float yaw = 3.14f; // Facing towards -Z roughly
    float pitch = 0.0f;
    
    Vec3 getForward() const {
        // Assuming Y is up
        return normalize({
            std::sin(yaw) * std::cos(pitch),
            std::sin(pitch),
            std::cos(yaw) * std::cos(pitch)
        });
    }

    Vec3 getRight() const {
        return normalize(cross({0.0f, 1.0f, 0.0f}, getForward()));
    }

    void move(float forward, float right) {
        Vec3 fwd = getForward();
        Vec3 rgt = getRight();
        // Project forward to XZ plane for walking
        Vec3 walkFwd = normalize({fwd.x, 0.0f, fwd.z});
        
        position = position + walkFwd * forward + rgt * right;
    }

    void rotate(float dYaw, float dPitch) {
        yaw += dYaw;
        pitch += dPitch;
        // Clamp pitch
        pitch = std::max(-1.5f, std::min(1.5f, pitch));
    }
};
