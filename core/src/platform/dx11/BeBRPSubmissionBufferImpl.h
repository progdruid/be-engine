#pragma once

#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct BeBRPSubmissionBufferImpl {
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11Buffer> sharedVertexBuffer;
    ComPtr<ID3D11Buffer> sharedIndexBuffer;
};
