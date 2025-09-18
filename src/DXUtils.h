#pragma once
#include <exception>
#include <string>

namespace DX {
  
    struct com_exception : public std::exception {
    private:
        HRESULT _hr;

    public:
        explicit com_exception(const HRESULT hr) : _hr(hr) {}

        const char* what() const noexcept override {
            static char s_str[64] = {};
            snprintf(s_str, sizeof(s_str), "Failure with HRESULT of 0x%08X", static_cast<unsigned int>(_hr));
            return s_str;
        }


    };

    inline void ThrowIfFailed(const HRESULT hr) {
        if (FAILED(hr))
            throw com_exception(hr);
    }
}
