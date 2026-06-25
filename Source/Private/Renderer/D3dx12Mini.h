#pragma once

#include <d3d12.h>

// Tiny local replacement for the only d3dx12 helper M3 needs.
// Do not name this helper `Transition`, because D3D12_RESOURCE_BARRIER already
// has a union member named `Transition`. C++ name hiding is adorable if your idea
// of adorable is a compiler throwing furniture.
inline D3D12_RESOURCE_BARRIER MakeTransitionBarrier(
    ID3D12Resource* resource,
    D3D12_RESOURCE_STATES stateBefore,
    D3D12_RESOURCE_STATES stateAfter)
{
    D3D12_RESOURCE_BARRIER result{};
    result.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    result.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    result.Transition.pResource = resource;
    result.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    result.Transition.StateBefore = stateBefore;
    result.Transition.StateAfter = stateAfter;
    return result;
}
