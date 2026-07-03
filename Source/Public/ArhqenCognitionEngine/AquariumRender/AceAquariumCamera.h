#pragma once

#include <array>

namespace ace::aquarium_render
{
    struct AceAquariumCamera
    {
        float Zoom = 30.0f;
        float OriginX = 0.0f;
        float OriginY = 0.0f;
        float HeightScale = 0.58f;
        float Tilt = 0.55f;
    };

    struct AceAqVec3
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    struct AceAqMat4
    {
        // Row-major matrix used by the DX12/HLSL row_major MVP path.
        std::array<float, 16> m{};
    };

    struct AceAqCameraInput
    {
        bool moveForward = false;
        bool moveBackward = false;
        bool moveLeft = false;
        bool moveRight = false;
        bool moveDown = false;
        bool moveUp = false;
    };

    class AceAquariumRealCamera
    {
    public:
        AceAquariumRealCamera();

        void Reset();
        void UpdateFromInput(const AceAqCameraInput& input, float deltaSeconds);
        void ApplyMouseDelta(float deltaX, float deltaY);

        AceAqVec3 Forward() const;
        AceAqVec3 Right() const;
        AceAqVec3 Up() const;

        AceAqMat4 ViewMatrix() const;
        AceAqMat4 ProjectionMatrix(float aspectRatio) const;
        AceAqMat4 ViewProjectionMatrix(float aspectRatio) const;

        const AceAqVec3& Position() const { return position_; }
        float Yaw() const { return yaw_; }
        float Pitch() const { return pitch_; }
        float MoveSpeed() const { return moveSpeed_; }
        float MouseSensitivity() const { return mouseSensitivity_; }

        void SetPosition(AceAqVec3 position);
        void SetYawPitch(float yawRadians, float pitchRadians);
        void SetMoveSpeed(float unitsPerSecond);

        static AceAqVec3 WorldUp();

    private:
        AceAqVec3 position_{};
        AceAqVec3 movementVelocity_{};
        float yaw_ = 0.0f;
        float pitch_ = 0.0f;
        float moveSpeed_ = 5.25f;
        float movementAcceleration_ = 115.0f;
        float movementDamping_ = 18.0f;
        float movementBrakingDamping_ = 35.0f;
        float mouseSensitivity_ = 0.0035f;
        float fovYRadians_ = 1.0471975512f;
        float nearPlane_ = 0.05f;
        float farPlane_ = 250.0f;
    };

    AceAqVec3 Add(AceAqVec3 lhs, AceAqVec3 rhs);
    AceAqVec3 Sub(AceAqVec3 lhs, AceAqVec3 rhs);
    AceAqVec3 Mul(AceAqVec3 value, float scalar);
    float Dot(AceAqVec3 lhs, AceAqVec3 rhs);
    AceAqVec3 Cross(AceAqVec3 lhs, AceAqVec3 rhs);
    float Length(AceAqVec3 value);
    AceAqVec3 Normalize(AceAqVec3 value);
    AceAqMat4 Mul(AceAqMat4 lhs, AceAqMat4 rhs);
}
