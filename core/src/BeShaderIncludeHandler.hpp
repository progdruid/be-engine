#pragma once
#include <d3dcompiler.h>
#include <string>
#include <fstream>
#include <filesystem>

class BeShaderIncludeHandler : public ID3DInclude
{
private:
    std::filesystem::path _shaderDir;
    std::filesystem::path _globalIncludeDir;

public:
    BeShaderIncludeHandler(const std::string& shaderDir, const std::string& globalIncludeDir)
    :   _shaderDir(shaderDir),
        _globalIncludeDir(globalIncludeDir) {}

    HRESULT STDMETHODCALLTYPE Open(
        D3D_INCLUDE_TYPE IncludeType,
        LPCSTR pFileName,
        LPCVOID pParentData,
        LPCVOID* ppData,
        UINT* pBytes) override {

        std::filesystem::path filePath;
        const std::string fileName = pFileName;
        
        if (IncludeType == D3D_INCLUDE_SYSTEM) {
            filePath = _globalIncludeDir / fileName;
        }
        else if (IncludeType == D3D_INCLUDE_LOCAL) {
            filePath = _shaderDir / fileName;
        }
        
        if (!std::filesystem::exists(filePath)) {
            return E_FAIL;
        }


        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file) return E_FAIL;

        const auto fileSize = file.tellg();
        file.seekg(0);

        auto buffer = std::make_unique<char[]>(fileSize);
        file.read(buffer.get(), fileSize);
        
        if (!file) return E_FAIL;

        *ppData = buffer.release();  // Transfer ownership
        *pBytes = static_cast<UINT>(fileSize);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Close(LPCVOID pData) override {
        delete[] static_cast<const char*>(pData);
        return S_OK;
    }
};
