#include "base/Math.h"

#include <cmath>

namespace sr::math {

namespace {

Vec3 FallbackAxis(const Vec3& forward)
{
    Vec3 right = forward.Cross(Vec3{ 1.0f, 0.0f, 0.0f });
    if (right.Length() != 0.0f) {
        return right.Normalized();
    }

    right = forward.Cross(Vec3{ 0.0f, 1.0f, 0.0f });
    if (right.Length() != 0.0f) {
        return right.Normalized();
    }

    return Vec3{ 1.0f, 0.0f, 0.0f };
}

} // namespace

float Clamp(float value, float minValue, float maxValue)
{
    if (value < minValue) {
        return minValue;
    }

    if (value > maxValue) {
        return maxValue;
    }

    return value;
}

float Lerp(float start, float end, float t)
{
    return start + (end - start) * t;
}

Vec2 Vec2::operator+(const Vec2& rhs) const
{
    return { x + rhs.x, y + rhs.y };
}

Vec2 Vec2::operator-(const Vec2& rhs) const
{
    return { x - rhs.x, y - rhs.y };
}

Vec2 Vec2::operator*(float scalar) const
{
    return { x * scalar, y * scalar };
}

Vec2 Vec2::operator/(float scalar) const
{
    return { x / scalar, y / scalar };
}

float Vec2::Dot(const Vec2& rhs) const
{
    return x * rhs.x + y * rhs.y;
}

float Vec2::Length() const
{
    return std::sqrt(Dot(*this));
}

Vec2 Vec2::Normalized() const
{
    const float length = Length();
    if (length == 0.0f) {
        return {};
    }

    return *this / length;
}

Vec2 Vec2::Lerp(const Vec2& start, const Vec2& end, float t)
{
    return start + (end - start) * t;
}

Vec2 operator*(float scalar, const Vec2& value)
{
    return value * scalar;
}

Vec3 Vec3::operator+(const Vec3& rhs) const
{
    return { x + rhs.x, y + rhs.y, z + rhs.z };
}

Vec3 Vec3::operator-(const Vec3& rhs) const
{
    return { x - rhs.x, y - rhs.y, z - rhs.z };
}

Vec3 Vec3::operator-() const
{
    return { -x, -y, -z };
}

Vec3 Vec3::operator*(float scalar) const
{
    return { x * scalar, y * scalar, z * scalar };
}

Vec3 Vec3::operator/(float scalar) const
{
    return { x / scalar, y / scalar, z / scalar };
}

float Vec3::Dot(const Vec3& rhs) const
{
    return x * rhs.x + y * rhs.y + z * rhs.z;
}

Vec3 Vec3::Cross(const Vec3& rhs) const
{
    return {
        y * rhs.z - z * rhs.y,
        z * rhs.x - x * rhs.z,
        x * rhs.y - y * rhs.x,
    };
}

float Vec3::Length() const
{
    return std::sqrt(Dot(*this));
}

Vec3 Vec3::Normalized() const
{
    const float length = Length();
    if (length == 0.0f) {
        return {};
    }

    return *this / length;
}

Vec3 Vec3::Lerp(const Vec3& start, const Vec3& end, float t)
{
    return start + (end - start) * t;
}

Vec3 operator*(float scalar, const Vec3& value)
{
    return value * scalar;
}

Vec4 Vec4::operator+(const Vec4& rhs) const
{
    return { x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w };
}

Vec4 Vec4::operator-(const Vec4& rhs) const
{
    return { x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w };
}

Vec4 Vec4::operator*(float scalar) const
{
    return { x * scalar, y * scalar, z * scalar, w * scalar };
}

Vec4 Vec4::operator/(float scalar) const
{
    return { x / scalar, y / scalar, z / scalar, w / scalar };
}

float Vec4::Dot(const Vec4& rhs) const
{
    return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w;
}

float Vec4::Length() const
{
    return std::sqrt(Dot(*this));
}

Vec4 Vec4::Normalized() const
{
    const float length = Length();
    if (length == 0.0f) {
        return {};
    }

    return *this / length;
}

Vec4 Vec4::Lerp(const Vec4& start, const Vec4& end, float t)
{
    return start + (end - start) * t;
}

Vec4 operator*(float scalar, const Vec4& value)
{
    return value * scalar;
}

Mat4::Mat4() : m{}
{
}

float* Mat4::operator[](std::size_t row)
{
    return m[row];
}

const float* Mat4::operator[](std::size_t row) const
{
    return m[row];
}

Mat4 Mat4::operator*(const Mat4& rhs) const
{
    Mat4 result;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            float sum = 0.0f;
            for (int index = 0; index < 4; ++index) {
                sum += m[row][index] * rhs[index][col];
            }
            result[row][col] = sum;
        }
    }
    return result;
}

Vec4 Mat4::operator*(const Vec4& rhs) const
{
    return {
        m[0][0] * rhs.x + m[0][1] * rhs.y + m[0][2] * rhs.z + m[0][3] * rhs.w,
        m[1][0] * rhs.x + m[1][1] * rhs.y + m[1][2] * rhs.z + m[1][3] * rhs.w,
        m[2][0] * rhs.x + m[2][1] * rhs.y + m[2][2] * rhs.z + m[2][3] * rhs.w,
        m[3][0] * rhs.x + m[3][1] * rhs.y + m[3][2] * rhs.z + m[3][3] * rhs.w,
    };
}

Mat4 Mat4::Identity()
{
    Mat4 result;
    result[0][0] = 1.0f;
    result[1][1] = 1.0f;
    result[2][2] = 1.0f;
    result[3][3] = 1.0f;
    return result;
}

Mat4 Mat4::Translate(float x, float y, float z)
{
    Mat4 result = Identity();
    result[0][3] = x;
    result[1][3] = y;
    result[2][3] = z;
    return result;
}

Mat4 Mat4::Scale(float x, float y, float z)
{
    Mat4 result;
    result[0][0] = x;
    result[1][1] = y;
    result[2][2] = z;
    result[3][3] = 1.0f;
    return result;
}

Mat4 RotateX(float radians)
{
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);

    Mat4 result = Mat4::Identity();
    result[1][1] = cosine;
    result[1][2] = -sine;
    result[2][1] = sine;
    result[2][2] = cosine;
    return result;
}

Mat4 RotateY(float radians)
{
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);

    Mat4 result = Mat4::Identity();
    result[0][0] = cosine;
    result[0][2] = sine;
    result[2][0] = -sine;
    result[2][2] = cosine;
    return result;
}

Mat4 RotateZ(float radians)
{
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);

    Mat4 result = Mat4::Identity();
    result[0][0] = cosine;
    result[0][1] = -sine;
    result[1][0] = sine;
    result[1][1] = cosine;
    return result;
}

Mat4 LookAt(const Vec3& eye, const Vec3& target, const Vec3& up)
{
    const Vec3 forward = (target - eye).Normalized();
    if (forward.Length() == 0.0f) {
        return Mat4::Identity();
    }

    Vec3 right = forward.Cross(up);
    if (right.Length() == 0.0f) {
        right = FallbackAxis(forward);
    } else {
        right = right.Normalized();
    }

    const Vec3 cameraUp = right.Cross(forward).Normalized();

    Mat4 result = Mat4::Identity();
    result[0][0] = right.x;
    result[0][1] = right.y;
    result[0][2] = right.z;
    result[0][3] = -right.Dot(eye);

    result[1][0] = cameraUp.x;
    result[1][1] = cameraUp.y;
    result[1][2] = cameraUp.z;
    result[1][3] = -cameraUp.Dot(eye);

    result[2][0] = -forward.x;
    result[2][1] = -forward.y;
    result[2][2] = -forward.z;
    result[2][3] = forward.Dot(eye);

    return result;
}

Mat4 Perspective(float fovyRadians, float aspect, float nearPlane, float farPlane)
{
    if (aspect == 0.0f || nearPlane <= 0.0f || farPlane <= nearPlane) {
        return Mat4::Identity();
    }

    const float tanHalfFovy = std::tan(fovyRadians * 0.5f);
    if (tanHalfFovy == 0.0f) {
        return Mat4::Identity();
    }

    Mat4 result;
    result[0][0] = 1.0f / (aspect * tanHalfFovy);
    result[1][1] = 1.0f / tanHalfFovy;
    result[2][2] = (farPlane + nearPlane) / (nearPlane - farPlane);
    result[2][3] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
    result[3][2] = -1.0f;
    return result;
}

} // namespace sr::math
