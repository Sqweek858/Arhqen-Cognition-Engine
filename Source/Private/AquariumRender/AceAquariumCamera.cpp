#include "ArhqenCognitionEngine/AquariumRender/AceAquariumCamera.h"

#include <algorithm>
#include <cmath>

namespace ace::aquarium_render
{
    namespace
    {
        constexpr float kPi = 3.14159265358979323846f;
        constexpr float kHalfPi = kPi * 0.5f;

        float clampPitch(float pitch)
        {
            return std::clamp(pitch, -1.48f, 1.48f);
        }

        AceAqMat4 identity()
        {
            AceAqMat4 result{};
            result.m = {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            };
            return result;
        }
    }

    AceAqVec3 Add(AceAqVec3 lhs, AceAqVec3 rhs)
    {
        return { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z };
    }

    AceAqVec3 Sub(AceAqVec3 lhs, AceAqVec3 rhs)
    {
        return { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z };
    }

    AceAqVec3 Mul(AceAqVec3 value, float scalar)
    {
        return { value.x * scalar, value.y * scalar, value.z * scalar };
    }

    float Dot(AceAqVec3 lhs, AceAqVec3 rhs)
    {
        return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
    }

    AceAqVec3 Cross(AceAqVec3 lhs, AceAqVec3 rhs)
    {
        return {
            lhs.y * rhs.z - lhs.z * rhs.y,
            lhs.z * rhs.x - lhs.x * rhs.z,
            lhs.x * rhs.y - lhs.y * rhs.x
        };
    }

    float Length(AceAqVec3 value)
    {
        return std::sqrt(Dot(value, value));
    }

    AceAqVec3 Normalize(AceAqVec3 value)
    {
        const float len = Length(value);
        if (len <= 0.00001f)
        {
            return {};
        }
        return Mul(value, 1.0f / len);
    }

    AceAqMat4 Mul(AceAqMat4 lhs, AceAqMat4 rhs)
    {
        AceAqMat4 result{};
        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                float value = 0.0f;
                for (int k = 0; k < 4; ++k)
                {
                    value += lhs.m[static_cast<std::size_t>(row * 4 + k)] * rhs.m[static_cast<std::size_t>(k * 4 + col)];
                }
                result.m[static_cast<std::size_t>(row * 4 + col)] = value;
            }
        }
        return result;
    }

    AceAquariumRealCamera::AceAquariumRealCamera()
    {
        Reset();
    }

    void AceAquariumRealCamera::Reset()
    {
        position_ = { 6.5f, 5.0f, -8.0f };
        movementVelocity_ = {};
        yaw_ = 0.78f;
        pitch_ = -0.38f;
    }

    void AceAquariumRealCamera::SetPosition(AceAqVec3 position)
    {
        // A programmatic camera teleport starts a new movement segment. Carrying
        // old flight velocity through it produces the same one-frame lurch UE
        // avoids by resetting its editor camera controller on view teleports.
        position_ = position;
        movementVelocity_ = {};
    }

    AceAqVec3 AceAquariumRealCamera::WorldUp()
    {
        return { 0.0f, 1.0f, 0.0f };
    }

    void AceAquariumRealCamera::SetYawPitch(float yawRadians, float pitchRadians)
    {
        yaw_ = yawRadians;
        pitch_ = clampPitch(pitchRadians);
    }

    AceAqVec3 AceAquariumRealCamera::Forward() const
    {
        const float cp = std::cos(pitch_);
        return Normalize({
            std::cos(yaw_) * cp,
            std::sin(pitch_),
            std::sin(yaw_) * cp
        });
    }

    AceAqVec3 AceAquariumRealCamera::Right() const
    {
        // ACE-AQ3D11: right is derived from camera orientation, not world X.
        return Normalize(Cross(WorldUp(), Forward()));
    }

    AceAqVec3 AceAquariumRealCamera::Up() const
    {
        return Normalize(Cross(Forward(), Right()));
    }

    void AceAquariumRealCamera::UpdateFromInput(const AceAqCameraInput& input, float deltaSeconds)
    {
        AceAqVec3 movement{};

        const AceAqVec3 forward = Forward();
        const AceAqVec3 right = Right();
        const AceAqVec3 worldUp = WorldUp();

        // ACE-AQ3D11: W/S are camera-forward, A/D are camera-right. Only Q/E use hardcoded world vertical.
        if (input.moveForward) { movement = Add(movement, forward); }
        if (input.moveBackward) { movement = Sub(movement, forward); }
        if (input.moveRight) { movement = Add(movement, right); }
        if (input.moveLeft) { movement = Sub(movement, right); }
        if (input.moveUp) { movement = Add(movement, worldUp); }
        if (input.moveDown) { movement = Sub(movement, worldUp); }

        const float len = Length(movement);
        if (len > 1.0f)
        {
            movement = Mul(movement, 1.0f / len);
        }

        // UE editor camera model, adapted to this engine's scale: user input is an
        // impulse which accelerates persistent velocity, then damping and a speed
        // cap are applied. Small fixed upper-bound substeps make the result stable
        // when UI presentation has a long/short frame pair. Braking is deliberately
        // stronger than powered damping so keyboard flight stays crisp, not floaty.
        float remaining = std::clamp(deltaSeconds, 0.0f, 0.050f);
        constexpr float kMaxMovementStep = 1.0f / 120.0f;
        const bool hasMovementImpulse = len > 0.00001f;
        while (remaining > 0.0f)
        {
            const float step = std::min(remaining, kMaxMovementStep);
            remaining -= step;

            if (hasMovementImpulse)
            {
                movementVelocity_ = Add(movementVelocity_, Mul(movement, movementAcceleration_ * step));
            }

            const float damping = hasMovementImpulse ? movementDamping_ : movementBrakingDamping_;
            const float dampingFactor = std::clamp(damping * step, 0.0f, 0.75f);
            movementVelocity_ = Mul(movementVelocity_, 1.0f - dampingFactor);

            const float speed = Length(movementVelocity_);
            if (speed > moveSpeed_)
            {
                movementVelocity_ = Mul(movementVelocity_, moveSpeed_ / speed);
            }
            else if (speed < 0.0001f)
            {
                movementVelocity_ = {};
            }

            position_ = Add(position_, Mul(movementVelocity_, step));
        }
    }

    void AceAquariumRealCamera::ApplyMouseDelta(float deltaX, float deltaY)
    {
        // ACE-AQ3D11R2: screen-space mouse X is inverted relative to the camera yaw convention.
        // Dragging right must turn the view right, not mirror it like a cursed bathroom camera.
        yaw_ -= deltaX * mouseSensitivity_;
        pitch_ = clampPitch(pitch_ - deltaY * mouseSensitivity_);
    }

    AceAqMat4 AceAquariumRealCamera::ViewMatrix() const
    {
        const AceAqVec3 f = Forward();
        const AceAqVec3 r = Right();
        const AceAqVec3 u = Up();

        AceAqMat4 view = identity();
        view.m = {
            r.x, u.x, f.x, 0.0f,
            r.y, u.y, f.y, 0.0f,
            r.z, u.z, f.z, 0.0f,
            -Dot(r, position_), -Dot(u, position_), -Dot(f, position_), 1.0f
        };
        return view;
    }

    AceAqMat4 AceAquariumRealCamera::ProjectionMatrix(float aspectRatio) const
    {
        const float aspect = std::max(0.05f, aspectRatio);
        const float yScale = 1.0f / std::tan(fovYRadians_ * 0.5f);
        const float xScale = yScale / aspect;
        const float zScale = farPlane_ / (farPlane_ - nearPlane_);

        AceAqMat4 proj{};
        proj.m = {
            xScale, 0.0f, 0.0f, 0.0f,
            0.0f, yScale, 0.0f, 0.0f,
            0.0f, 0.0f, zScale, 1.0f,
            0.0f, 0.0f, -nearPlane_ * zScale, 0.0f
        };
        return proj;
    }

    AceAqMat4 AceAquariumRealCamera::ViewProjectionMatrix(float aspectRatio) const
    {
        return Mul(ViewMatrix(), ProjectionMatrix(aspectRatio));
    }
}
