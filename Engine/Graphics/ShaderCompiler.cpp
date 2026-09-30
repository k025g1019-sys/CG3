#include "Engine/Graphics/ShaderCompiler.h"

#include <cassert>
#include <format>

#include "Engine/String/ConvertString.h"
#include "Engine/Diagnostics/Log.h"
#include <combaseapi.h>

#pragma comment(lib, "dxcompiler.lib")

using Microsoft::WRL::ComPtr;

namespace Engine {

ShaderCompiler* ShaderCompiler::GetInstance() {

    static ShaderCompiler instance;

    return &instance;
}

void ShaderCompiler::Initialize() {

    HRESULT hr;

    hr = DxcCreateInstance(
        CLSID_DxcUtils,
        IID_PPV_ARGS(&dxcUtils_)
    );

    assert(SUCCEEDED(hr));

    hr = DxcCreateInstance(
        CLSID_DxcCompiler,
        IID_PPV_ARGS(&dxcCompiler_)
    );

    assert(SUCCEEDED(hr));

    hr = dxcUtils_->CreateDefaultIncludeHandler(
        &includeHandler_
    );

    assert(SUCCEEDED(hr));

    // エンジンのShadersフォルダを探す（実行時のカレントディレクトリ基準）。
    //   Shaders/    : 配布用フォルダ（exeと同じ場所に Shaders/ と resources/ を置いて起動）
    //   ../Shaders/ : 開発時（VSから起動するとカレントが Sandbox/ や Game/ になる）
    const wchar_t* const kCandidates[] = {
        L"Shaders/",
        L"../Shaders/",
    };
    for (const wchar_t* candidate : kCandidates) {
        const std::wstring probe = std::wstring(candidate) + L"Object3d.VS.hlsl";
        if (GetFileAttributesW(probe.c_str()) != INVALID_FILE_ATTRIBUTES) {
            shaderDirectory_ = candidate;
            break;
        }
    }

    Log(ConvertString(std::format(L"Shader directory: {}\n", shaderDirectory_)));

    // Shadersフォルダが見つからない（カレントディレクトリが想定外）
    assert(!shaderDirectory_.empty());
}

ComPtr<IDxcBlob> ShaderCompiler::Compile(
    const std::wstring& fileName,
    const wchar_t* profile
) {

    const std::wstring filePath = shaderDirectory_ + fileName;

    Log(ConvertString(
        std::format(
        L"Begin CompileShader, path:{}, profile:{}\n",
        filePath,
        profile
    )
    ));

    HRESULT hr;

    ComPtr<IDxcBlobEncoding> shaderSource;

    hr = dxcUtils_->LoadFile(
        filePath.c_str(),
        nullptr,
        &shaderSource
    );

    assert(SUCCEEDED(hr));

    DxcBuffer shaderSourceBuffer{};

    shaderSourceBuffer.Ptr =
        shaderSource->GetBufferPointer();

    shaderSourceBuffer.Size =
        shaderSource->GetBufferSize();

    shaderSourceBuffer.Encoding =
        DXC_CP_UTF8;

    LPCWSTR arguments[] = {
        filePath.c_str(),
        L"-E", L"main",
        L"-T", profile,
        L"-Zi", L"-Qembed_debug",
        L"-Od",
        L"-Zpr",
    };

    ComPtr<IDxcResult> shaderResult;

    hr = dxcCompiler_->Compile(
        &shaderSourceBuffer,
        arguments,
        _countof(arguments),
        includeHandler_.Get(),
        IID_PPV_ARGS(&shaderResult)
    );

    assert(SUCCEEDED(hr));

    ComPtr<IDxcBlobUtf8> shaderError;

    shaderResult->GetOutput(
        DXC_OUT_ERRORS,
        IID_PPV_ARGS(&shaderError),
        nullptr
    );

    if (shaderError != nullptr &&
        shaderError->GetStringLength() != 0) {

        Log(shaderError->GetStringPointer());

        assert(false);
    }

    ComPtr<IDxcBlob> shaderBlob;

    hr = shaderResult->GetOutput(
        DXC_OUT_OBJECT,
        IID_PPV_ARGS(&shaderBlob),
        nullptr
    );

    assert(SUCCEEDED(hr));

    Log(ConvertString(
        std::format(
        L"Compile Succeeded, path:{}, profile:{}\n",
        filePath,
        profile
    )
    ));

    return shaderBlob;
}

void ShaderCompiler::Finalize() {

    includeHandler_.Reset();

    dxcCompiler_.Reset();

    dxcUtils_.Reset();
}

} // namespace Engine
