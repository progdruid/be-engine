#pragma once

#include <exception>
#include <iostream>
#include <string>
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <wrl/client.h>
#include <comdef.h>

namespace DxUtils {

    inline auto HResultToStr(const HRESULT hr) -> std::string {
        LPSTR messageBuffer = nullptr;
        DWORD size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            nullptr,
            hr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&messageBuffer,
            0,
            nullptr
        );
        std::string message(messageBuffer, size);
        LocalFree(messageBuffer);
        return message;
    }

    struct com_exception : public std::exception {
    private:
        HRESULT _hr;
    public:
        explicit com_exception(const HRESULT hr) : _hr(hr) {}
        const char* what() const noexcept override {
            static char str[64] = {};
            snprintf(str, sizeof(str), "Failure with HRESULT of 0x%08X", static_cast<unsigned int>(_hr));
            return str;
        }
    };

    inline auto ThrowIfFailed(const HRESULT hr) -> void {
        if (FAILED(hr))
            throw com_exception(hr);
    }

    struct ErrorStream {
        ErrorStream& operator<<(const HRESULT msg) {
            if (FAILED(msg)) {
                _com_error err(msg);
                std::wstring wstr(err.ErrorMessage());
                std::wcerr << L"Error: HRESULT 0x" << std::hex << msg << std::dec << L" Message: " << wstr << L"\n";
                throw com_exception(msg);
            }
            return *this;
        }
    };

    inline ErrorStream Check;

    inline ID3D11Buffer* NullBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = {};
    inline ID3D11SamplerState* NullSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT] = {};
    inline ID3D11ShaderResourceView* NullSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};
    inline ID3D11RenderTargetView* NullRTVs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};

    class DebugAnnotation {
    public:
        explicit DebugAnnotation(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, const std::string& label)
            : _context(context) {
            if (!_context) return;
            _context->QueryInterface(IID_PPV_ARGS(&_annotation));
            if (_annotation) {
                std::wstring wideLabel(label.begin(), label.end());
                _annotation->BeginEvent(wideLabel.c_str());
            }
        }
        ~DebugAnnotation() {
            if (_annotation) _annotation->EndEvent();
        }
        DebugAnnotation(const DebugAnnotation&) = delete;
        DebugAnnotation& operator=(const DebugAnnotation&) = delete;
        DebugAnnotation(DebugAnnotation&&) = delete;
        DebugAnnotation& operator=(DebugAnnotation&&) = delete;
    private:
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context;
        Microsoft::WRL::ComPtr<ID3DUserDefinedAnnotation> _annotation;
    };
}
