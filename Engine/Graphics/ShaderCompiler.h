#pragma once

#include <Windows.h>
#include <wrl/client.h>
#include <dxcapi.h>

#include <string>

namespace Engine {

class ShaderCompiler {
public:

    static ShaderCompiler* GetInstance();

    // DXCを初期化し、エンジンのShadersフォルダの場所を決める
    // （見つからない場合はassertで停止する）
    void Initialize();

    void Finalize();

    /// <summary>
    /// エンジンのShadersフォルダにあるシェーダーをコンパイルする
    /// </summary>
    /// <param name="fileName">Shadersフォルダからの相対パス（例: L"Object3d.VS.hlsl"）</param>
    /// <param name="profile">シェーダープロファイル（例: L"vs_6_0"）</param>
    Microsoft::WRL::ComPtr<IDxcBlob> Compile(
        const std::wstring& fileName,
        const wchar_t* profile
    );

    // 見つかったShadersフォルダ（末尾に/付き）
    const std::wstring& GetShaderDirectory() const { return shaderDirectory_; }

private:

    ShaderCompiler() = default;

    ~ShaderCompiler() = default;

    ShaderCompiler(const ShaderCompiler&) = delete;

    ShaderCompiler& operator=(const ShaderCompiler&) = delete;

private:

    Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;

    Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;

    Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;

    // エンジンのShadersフォルダ（実行時のカレントディレクトリからの相対パス）
    std::wstring shaderDirectory_;
};

} // namespace Engine
