#pragma once
#include <cmath>
#include <cstring> // for memcpy if needed

// --------------------------------------------------------
// Vec3: 3D vector with float components
// --------------------------------------------------------
struct Vec3 {
    float x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(float xx, float yy, float zz) : x(xx), y(yy), z(zz) {}

    Vec3 operator+(const Vec3& v) const {
        return Vec3(x + v.x, y + v.y, z + v.z);
    }
    Vec3 operator-(const Vec3& v) const {
        return Vec3(x - v.x, y - v.y, z - v.z);
    }
    Vec3 operator*(float s) const {
        return Vec3(x * s, y * s, z * s);
    }
    Vec3 operator/(float s) const {
        float inv = 1.0f / s;
        return Vec3(x * inv, y * inv, z * inv);
    }

    float length() const {
        return std::sqrt(x * x + y * y + z * z);
    }
    Vec3 normalized() const {
        float len = length();
        if (len == 0) return *this;
        return *this / len;
    }
    float dot(const Vec3& v) const {
        return x * v.x + y * v.y + z * v.z;
    }
    Vec3 cross(const Vec3& v) const {
        return Vec3(
            y * v.z - z * v.y,
            z * v.x - x * v.z,
            x * v.y - y * v.x
        );
    }
};

// --------------------------------------------------------
// Quaternion: represents a rotation
// --------------------------------------------------------
struct Quaternion {
    float w, x, y, z;

    Quaternion() : w(1), x(0), y(0), z(0) {}
    Quaternion(float ww, float xx, float yy, float zz) : w(ww), x(xx), y(yy), z(zz) {}

    // Construct from axis-angle (angle in radians)
    static Quaternion fromAxisAngle(const Vec3& axis, float angleRad) {
        float halfAngle = angleRad * 0.5f;
        float s = std::sin(halfAngle);
        return Quaternion(std::cos(halfAngle), axis.x * s, axis.y * s, axis.z * s);
    }

    Quaternion normalized() const {
        float len = std::sqrt(w * w + x * x + y * y + z * z);
        return Quaternion(w / len, x / len, y / len, z / len);
    }

    Quaternion operator*(const Quaternion& q) const {
        return Quaternion(
            w * q.w - x * q.x - y * q.y - z * q.z,
            w * q.x + x * q.w + y * q.z - z * q.y,
            w * q.y - x * q.z + y * q.w + z * q.x,
            w * q.z + x * q.y - y * q.x + z * q.w
        );
    }

    // Convert to 4x4 rotation matrix (column-major for OpenGL)
    // Matrix layout:
    // [ 0  4  8 12 ]
    // [ 1  5  9 13 ]
    // [ 2  6 10 14 ]
    // [ 3  7 11 15 ]
    void toMatrix(float out[16]) const {
        float xx = x * x;
        float yy = y * y;
        float zz = z * z;
        float xy = x * y;
        float xz = x * z;
        float yz = y * z;
        float wx = w * x;
        float wy = w * y;
        float wz = w * z;

        // Column-major order
        out[0] = 1.0f - 2.0f * (yy + zz);
        out[1] = 2.0f * (xy + wz);
        out[2] = 2.0f * (xz - wy);
        out[3] = 0.0f;

        out[4] = 2.0f * (xy - wz);
        out[5] = 1.0f - 2.0f * (xx + zz);
        out[6] = 2.0f * (yz + wx);
        out[7] = 0.0f;

        out[8] = 2.0f * (xz + wy);
        out[9] = 2.0f * (yz - wx);
        out[10] = 1.0f - 2.0f * (xx + yy);
        out[11] = 0.0f;

        out[12] = 0.0f;
        out[13] = 0.0f;
        out[14] = 0.0f;
        out[15] = 1.0f;
    }
};

// --------------------------------------------------------
// Mat4: 4x4 matrix in column-major order (OpenGL style)
// Stored as float data[16]:
// data[0], data[1], data[2], data[3]   - first column
// data[4], data[5], data[6], data[7]   - second column
// data[8], data[9], data[10], data[11] - third column
// data[12], data[13], data[14], data[15] - fourth column
// --------------------------------------------------------
struct Mat4 {
    float data[16];

    // Initialize as identity matrix
    Mat4() {
        for (int i = 0; i < 16; ++i) data[i] = 0.0f;
        data[0] = 1.0f;
        data[5] = 1.0f;
        data[10] = 1.0f;
        data[15] = 1.0f;
    }

    // Access element by row and column (row-major indexing)
    float& operator()(int row, int col) {
        return data[col * 4 + row]; // column-major order
    }
    const float& operator()(int row, int col) const {
        return data[col * 4 + row];
    }

    // Matrix multiplication (this * other)
    Mat4 operator*(const Mat4& other) const {
        Mat4 result;
        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    sum += (*this)(row, k) * other(k, col);
                }
                result(row, col) = sum;
            }
        }
        return result;
    }

    // Create translation matrix
    static Mat4 translate(const Vec3& t) {
        Mat4 m;
        m(0, 3) = t.x;
        m(1, 3) = t.y;
        m(2, 3) = t.z;
        return m;
    }

    // Create scale matrix
    static Mat4 scale(const Vec3& s) {
        Mat4 m;
        m(0, 0) = s.x;
        m(1, 1) = s.y;
        m(2, 2) = s.z;
        return m;
    }

    // Create rotation matrix from quaternion
    static Mat4 fromQuaternion(const Quaternion& q) {
        Mat4 m;
        q.toMatrix(m.data);
        return m;
    }

    // Create perspective projection matrix (right-handed, OpenGL clip space)
    // fovyRadians: vertical fov in radians
    // aspect: width / height
    // nearZ, farZ: near and far planes
    static Mat4 perspective(float fovyRadians, float aspect, float nearZ, float farZ) {
        Mat4 m;
        float f = 1.0f / std::tan(fovyRadians / 2.0f);
        m(0, 0) = f / aspect;
        m(1, 1) = f;
        m(2, 2) = (farZ + nearZ) / (nearZ - farZ);
        m(3, 2) = -1.0f;
        m(2, 3) = (2.0f * farZ * nearZ) / (nearZ - farZ);
        m(3, 3) = 0.0f;
        return m;
    }

    // Create lookAt matrix (right-handed, OpenGL)
    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
        Vec3 f = (center - eye).normalized();
        Vec3 s = f.cross(up).normalized();
        Vec3 u = s.cross(f);

        Mat4 m;
        m(0, 0) = s.x;
        m(1, 0) = s.y;
        m(2, 0) = s.z;
        m(3, 0) = 0.0f;

        m(0, 1) = u.x;
        m(1, 1) = u.y;
        m(2, 1) = u.z;
        m(3, 1) = 0.0f;

        m(0, 2) = -f.x;
        m(1, 2) = -f.y;
        m(2, 2) = -f.z;
        m(3, 2) = 0.0f;

        m(0, 3) = 0.0f;
        m(1, 3) = 0.0f;
        m(2, 3) = 0.0f;
        m(3, 3) = 1.0f;

        Mat4 translation = Mat4::translate(Vec3(-eye.x, -eye.y, -eye.z));
        return m * translation;
    }

    // Return pointer to raw data for OpenGL
    const float* raw() const { return data; }
};
