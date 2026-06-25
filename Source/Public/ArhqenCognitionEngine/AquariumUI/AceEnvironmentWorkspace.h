#pragma once

namespace ace::aquarium_ui
{
    struct AceEnvironmentWorkspaceRect
    {
        float left = 0.0f;
        float top = 0.0f;
        float right = 0.0f;
        float bottom = 0.0f;

        float Width() const { return right - left; }
        float Height() const { return bottom - top; }
    };

    struct AceEnvironmentWorkspaceLayout
    {
        AceEnvironmentWorkspaceRect workspace{};
        AceEnvironmentWorkspaceRect toolbar{};
        AceEnvironmentWorkspaceRect viewport{};
        AceEnvironmentWorkspaceRect inspector{};
        AceEnvironmentWorkspaceRect logs{};
    };

    class AceEnvironmentWorkspace
    {
    public:
        AceEnvironmentWorkspaceLayout Compute(float clientWidth, float clientHeight, float appBarHeight) const;
        bool ViewportIsLargeEnough(const AceEnvironmentWorkspaceLayout& layout, float clientWidth, float clientHeight) const;
    };
}
