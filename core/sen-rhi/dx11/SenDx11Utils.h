#pragma once
#include <exception>
#include <iostream>
#include <string>
#define NOMINMAX
#include <windows.h>
#include <comdef.h>
#include <umbrellas/access-modifiers.hpp>

namespace Utils {
    struct com_exception : public std::exception {
        hide
        HRESULT _hr;

        expose
        explicit com_exception(const HRESULT hr) : _hr(hr) {}

        const char* what() const noexcept override {
            static char str[64] = {};
            snprintf(str, sizeof(str), "Failure with HRESULT of 0x%08X", static_cast<unsigned int>(_hr));
            return str;
        }
    };

    inline auto ThrowIfFailed(const HRESULT hr) -> void {
        if (FAILED(hr)) {
            throw com_exception(hr);
        }
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
}
