#pragma once

#include <cstddef>

namespace sr::math {

constexpr float kPi = 3.14159265358979323846f;

float Clamp(float value, float minValue, float maxValue);
float Lerp(float start, float end, float t);

class Vec2 {
public:
    float x;
    float y;

    constexpr Vec2() : x(0.0f), y(0.0f) {}
    constexpr Vec2(float xValue, float yValue) : x(xValue), y(yValue) {}

    Vec2 operator+(const Vec2& rhs) const;
    Vec2 operator-(const Vec2& rhs) const;
    Vec2 operator*(float scalar) const;
    Vec2 operator/(float scalar) const;

    float Dot(const Vec2& rhs) const;
    float Length() const;
    Vec2 Normalized() const;

    static Vec2 Lerp(const Vec2& start, const Vec2& end, float t);
};

Vec2 operator*(float scalar, const Vec2& value);

class Vec3 {
public:
    float x;
    float y;
    float z;

    constexpr Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    constexpr Vec3(float xValue, float yValue, float zValue)
        : x(xValue), y(yValue), z(zValue)
    {
    }

    Vec3 operator+(const Vec3& rhs) const;
    Vec3 operator-(const Vec3& rhs) const;
    Vec3 operator-() const;
    Vec3 operator*(float scalar) const;
    Vec3 operator/(float scalar) const;

    float Dot(const Vec3& rhs) const;
    Vec3 Cross(const Vec3& rhs) const;
    float Length() const;
    Vec3 Normalized() const;

    static Vec3 Lerp(const Vec3& start, const Vec3& end, float t);
};

Vec3 operator*(float scalar, const Vec3& value);

class Vec4 {
public:
    float x;
    float y;
    float z;
    float w;

    constexpr Vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    constexpr Vec4(float xValue, float yValue, float zValue, float wValue)
        : x(xValue), y(yValue), z(zValue), w(wValue)
    {
    }

    constexpr Vec4(const Vec3& xyz, float wValue)
        : x(xyz.x), y(xyz.y), z(xyz.z), w(wValue)
    {
    }

    Vec4 operator+(const Vec4& rhs) const;
    Vec4 operator-(const Vec4& rhs) const;
    Vec4 operator*(float scalar) const;
    Vec4 operator/(float scalar) const;

    float Dot(const Vec4& rhs) const;
    float Length() const;
    Vec4 Normalized() const;

    static Vec4 Lerp(const Vec4& start, const Vec4& end, float t);
};

Vec4 operator*(float scalar, const Vec4& value);

class Mat4 {
public:
    Mat4();

    float* operator[](std::size_t row);
    const float* operator[](std::size_t row) const;

    Mat4 operator*(const Mat4& rhs) const;
    Vec4 operator*(const Vec4& rhs) const;

    static Mat4 Identity();
    static Mat4 Translate(float x, float y, float z);
    static Mat4 Scale(float x, float y, float z);

private:
    float m[4][4];
};

Mat4 RotateX(float radians);
Mat4 RotateY(float radians);
Mat4 RotateZ(float radians);
Mat4 LookAt(const Vec3& eye, const Vec3& target, const Vec3& up);
Mat4 Perspective(float fovyRadians, float aspect, float nearPlane, float farPlane);

} // namespace sr::math
