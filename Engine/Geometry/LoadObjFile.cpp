#include <cassert>
#include <charconv>
#include <fstream>
#include <sstream>
#include <string_view>
#include "Engine/Geometry/LoadObjFile.h"
#include "Engine/Math/Vector2.h"
#include "Engine/Math/Vector3.h"
#include "Engine/Math/Vector4.h"

namespace Engine {

namespace {

// bunny級（29万行）のファイルでも高速に読めるよう、頻出行（v/vt/vn/f）は
// istringstreamを使わずfrom_charsで直接解析する（ロケール処理やヒープ確保が無い）。

// pからendの範囲で空白（スペース・タブ・CR）を読み飛ばした位置を返す
const char* SkipSpaces(const char* p, const char* end) {
	while (p < end && (*p == ' ' || *p == '\t' || *p == '\r')) {
		++p;
	}
	return p;
}

// 空白区切りのfloatを最大count個読む。読めた個数を返す
int32_t ParseFloats(const char* p, const char* end, float* out, int32_t count) {
	int32_t parsed = 0;
	while (parsed < count) {
		p = SkipSpaces(p, end);
		if (p >= end) {
			break;
		}
		std::from_chars_result result = std::from_chars(p, end, out[parsed]);
		if (result.ec != std::errc{}) {
			break;
		}
		p = result.ptr;
		++parsed;
	}
	return parsed;
}

// 面の頂点定義（"1"・"1/2"・"1//3"・"1/2/3"のいずれか）を位置/UV/法線のインデックスに分解する。
// 省略された要素は0のままにする（OBJのインデックスは1始まりなので、0を「未指定」として使える）
void ParseFaceVertex(const char* begin, const char* end, uint32_t elementIndices[3]) {
	elementIndices[0] = 0;
	elementIndices[1] = 0;
	elementIndices[2] = 0;

	const char* p = begin;
	for (int32_t element = 0; element < 3; ++element) {
		const char* tokenEnd = p;
		while (tokenEnd < end && *tokenEnd != '/') {
			++tokenEnd;
		}
		if (tokenEnd > p) {
			// stoiと違い空文字列でも例外にならない（その場合は0=未指定のまま）
			[[maybe_unused]] std::from_chars_result result =
				std::from_chars(p, tokenEnd, elementIndices[element]);
			assert(result.ec == std::errc{});
		}
		if (tokenEnd >= end) {
			break;
		}
		p = tokenEnd + 1; // '/'の次へ
	}
}

// ファイル全体を一括で読み込んで返す（Debugビルドのgetlineは1文字ずつ読むため
// bunny級の15MB・29万行では数秒かかる。一括読み→行走査で回避する）
std::string ReadFileAll(const std::string& filepath) {
	std::ifstream file(filepath, std::ios::binary);
	assert(file.is_open()); // とりあえず開けなかったら止める
	file.seekg(0, std::ios::end);
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);
	std::string content(size_t(size), '\0');
	file.read(content.data(), size);
	return content;
}

} // namespace

std::unordered_map<std::string, MaterialData> LoadMaterialTemplateFile(
	const std::string& directoryPath, const std::string& filename) {
	std::unordered_map<std::string, MaterialData> materials; // 構築するマテリアル辞書
	std::string currentName; // 現在編集中のマテリアル名（newmtlで切り替わる）
	std::string line; // ファイルから読んだ1行を格納するもの
	std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
	assert(file.is_open()); //とりあえず開けなかったら止める

	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		//identifierに応じた処理
		if (identifier == "newmtl") {
			s >> currentName;
			materials[currentName] = MaterialData{};
		} else if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			if (!currentName.empty()) {
				//連結してファイルパスにする
				materials[currentName].textureFilePath = directoryPath + "/" + textureFilename;
			}
		}
	}

	return materials;
}

ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename) {
	ModelData modelData;//構築するModelData
	std::vector<Vector4> positions; // 位置
	std::vector<Vector3> normals; // 法線
	std::vector<Vector2> texcoords; //テクスチャ座標
	const std::string content = ReadFileAll(directoryPath + "/" + filename); // ファイル全体

	// o/g/usemtlで宣言された「現在のサブメッシュ属性」。実際のサブメッシュ生成は
	// 最初のf行まで遅延させる（Blenderの「o → usemtl → f」の並びを1つに畳み、
	// o/gを持たないファイル（teapot等）でも面があれば必ず1つは作られる）
	std::string currentName;      // 直近のo/gの名前
	std::string currentMaterial;  // 直近のusemtlのマテリアル名
	bool pendingSubMesh = true;   // 次のf行で新しいサブメッシュを開始するか

	const char* cursor = content.data();
	const char* fileEnd = cursor + content.size();
	while (cursor < fileEnd) {
		// 1行を切り出す（改行コードLF/CRLFの両対応。行末のCRは落とす）
		const char* lineBegin = cursor;
		while (cursor < fileEnd && *cursor != '\n') {
			++cursor;
		}
		const char* lineEnd = cursor;
		if (cursor < fileEnd) {
			++cursor; // '\n'の次の行頭へ
		}
		if (lineEnd > lineBegin && lineEnd[-1] == '\r') {
			--lineEnd;
		}
		const std::string_view line(lineBegin, size_t(lineEnd - lineBegin));

		// 行頭の識別子に応じた処理（頻出行はfrom_charsで直接解析する）

		if (line.starts_with("v ")) {
			float elements[3] = {};
			[[maybe_unused]] int32_t parsed = ParseFloats(line.data() + 2, lineEnd, elements, 3);
			assert(parsed == 3);
			// 右手系→左手系に合わせてzを反転する
			positions.push_back({ elements[0], elements[1], -elements[2], 1.0f });
		} else if (line.starts_with("vt ")) {
			float elements[2] = {};
			[[maybe_unused]] int32_t parsed = ParseFloats(line.data() + 3, lineEnd, elements, 2);
			assert(parsed == 2);
			// 画像の上下原点の違いに合わせてvを反転する
			texcoords.push_back({ elements[0], 1.0f - elements[1] });
		} else if (line.starts_with("vn ")) {
			float elements[3] = {};
			[[maybe_unused]] int32_t parsed = ParseFloats(line.data() + 3, lineEnd, elements, 3);
			assert(parsed == 3);
			// 位置と同様にzを反転する
			normals.push_back({ elements[0], elements[1], -elements[2] });
		} else if (line.starts_with("f ")) {

			// サブメッシュの遅延生成。属性が変わってから最初の面でだけ切り替える
			if (pendingSubMesh) {
				if (modelData.subMeshes.empty() || modelData.subMeshes.back().vertexCount > 0) {
					modelData.subMeshes.push_back(
						{ currentName, currentMaterial, uint32_t(modelData.vertices.size()), 0 });
				} else {
					// まだ面を持たないサブメッシュは作り直さず属性だけ更新する
					modelData.subMeshes.back().name = currentName;
					modelData.subMeshes.back().materialName = currentMaterial;
				}
				pendingSubMesh = false;
			}

			// 大きいモデル（bunny等）で再確保のコピーを避ける。
			// 閉じた三角形メッシュは面数≒頂点数×2なので、頂点数×2面×3頂点で見積もる
			if (modelData.vertices.capacity() == 0) {
				modelData.vertices.reserve(positions.size() * 6);
			}

			VertexData triangle[3];

			//面は三角形限定。その他は未対応
			const char* p = line.data() + 2;
			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
				// 空白区切りで頂点定義（例:"1/2/3"）を1つ切り出す
				p = SkipSpaces(p, lineEnd);
				const char* tokenBegin = p;
				while (p < lineEnd && *p != ' ' && *p != '\t' && *p != '\r') {
					++p;
				}
				assert(p > tokenBegin && "面の頂点が3つに満たない");

				//頂点の要素へのIndexは「位置/UV/法線」で格納されているので、分解してIndexを取得する
				//（UV・法線は省略されていることがある。省略時は0が入る）
				uint32_t elementIndices[3];
				ParseFaceVertex(tokenBegin, p, elementIndices);
				assert(elementIndices[0] >= 1 && elementIndices[0] <= positions.size());

				Vector4 position = positions[elementIndices[0] - 1];
				// UVを持たないモデル（suzanne等）は(0,0)を補う（白テクスチャと組み合わせて単色で描ける）
				Vector2 texcoord =
					(elementIndices[1] != 0) ? texcoords[elementIndices[1] - 1] : Vector2{ 0.0f, 0.0f };
				// 法線を持たないモデルは手前向きを補う
				Vector3 normal =
					(elementIndices[2] != 0) ? normals[elementIndices[2] - 1] : Vector3{ 0.0f, 0.0f, -1.0f };
				triangle[faceVertex] = { position, texcoord, normal };
			}

			modelData.vertices.push_back(triangle[2]);
			modelData.vertices.push_back(triangle[1]);
			modelData.vertices.push_back(triangle[0]);
			modelData.subMeshes.back().vertexCount += 3;

		} else {
			// 低頻度の行（mtllib/o/g/usemtl等）は従来通りistringstreamで解析する
			std::string identifier;
			std::istringstream s{ std::string(line) };
			s >> identifier;//先頭の識別子を読む

			if (identifier == "mtllib") {
				// materialTemplateLibraryファイルの名前を取得する
				std::string materialFilename;
				s >> materialFilename;
				// 基本的にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
				modelData.materials = LoadMaterialTemplateFile(directoryPath, materialFilename);
			} else if (identifier == "o" || identifier == "g") {
				s >> currentName;
				pendingSubMesh = true;
			} else if (identifier == "usemtl") {
				s >> currentMaterial;
				pendingSubMesh = true;
			}
		}
	}
	return modelData;
}

} // namespace Engine
