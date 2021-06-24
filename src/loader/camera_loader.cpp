#include "loader/camera_loader.h"

using namespace qs;

CameraLoader::CameraLoader() {}

CameraLoader::~CameraLoader() {}

void CameraLoader::open(std::string recDirPath) {
	// 今まで開いていたファイルをすべて閉じる
	close();

	// 各ファイルのファイルパス
	const std::string
		infoJsonPath        = recDirPath + "/info.json",
		cameraFrameInfoPath = recDirPath + "/cameraFrameInfo",
		cameraPath          = recDirPath + "/camera.mp4",
		depthPath           = recDirPath + "/depth",
		confidencePath      = recDirPath + "/confidence";

	// 録画データ全体の情報を取得
	std::ifstream infoJsonFs(infoJsonPath, std::ios_base::in | std::ios_base::binary);
	if ((!infoJsonFs.is_open()) || (!infoJsonFs.good())) { close(); return; }
	std::string infoJsonString((std::istreambuf_iterator<char>(infoJsonFs)), std::istreambuf_iterator<char>());
	infoJsonFs.close();
	try {
		nlohmann::json infoJson = nlohmann::json::parse(infoJsonString);

		depthZlib.width = infoJson.at("depth").at("width");
		depthZlib.height = infoJson.at("depth").at("height");
		depthZlib.dst.resize(depthZlib.width * depthZlib.height * sizeof(float));

		confidenceZlib.width = infoJson.at("confidence").at("width");
		confidenceZlib.height = infoJson.at("confidence").at("height");
		confidenceZlib.dst. resize(confidenceZlib.width * confidenceZlib.height * sizeof(uint8_t));
	}
	catch(const nlohmann::detail::out_of_range& e) { close(); return; }

	// カメラ、デプス、信頼度に関する情報を取得
	arFs.open(cameraFrameInfoPath, std::ios_base::in | std::ios_base::binary);
	if (!arFs.is_open()) { close(); return; }

	// カメラの映像を取得
	colorCap.open(cameraPath);
	if (!colorCap.isOpened()) { close(); return; }

	// デプスの映像を取得
	depthZlib.fs.open(depthPath, std::ios_base::in | std::ios_base::binary);
	if (!depthZlib.fs.is_open()) { close(); return; }

	// 信頼度の映像を取得
	confidenceZlib.fs.open(confidencePath, std::ios_base::in | std::ios_base::binary);
	if (!confidenceZlib.fs.is_open()) { close(); return; }

	isOpened_ = true;

	return;
}

void CameraLoader::close() {
	if (colorCap.isOpened()) colorCap.release();
	if (depthZlib.fs.is_open()) depthZlib.fs.close();
	if (confidenceZlib.fs.is_open()) confidenceZlib.fs.close();
	isOpened_ = false;
}

bool CameraLoader::isOpened() const {
	return isOpened_;
}

std::optional<Camera> CameraLoader::next() {
	// ファイルが開かれていなければ処理を終了
	if (!isOpened()) { return std::nullopt; }
	Camera camera;

	// フレームのタイムスタンプなどを取得
	uint8_t arBytes[199];
	uint8_t colorExists, depthExists, confidenceExists;
	uint64_t depthOffset, confidenceOffset;
	arFs.read((char*)arBytes, sizeof(arBytes));
	if (!arFs.good()) { close(); return std::nullopt; }
	size_t offset = 8, size = 0;
	std::memcpy(&camera.timestamp          , &arBytes[offset], size = sizeof(camera.timestamp)          ); offset += size;
	std::memcpy(&colorExists               , &arBytes[offset], size = sizeof(colorExists)               ); offset += size;
	std::memcpy(&depthExists               , &arBytes[offset], size = sizeof(depthExists)               ); offset += size;
	std::memcpy(&confidenceExists          , &arBytes[offset], size = sizeof(confidenceExists)          ); offset += size;
	std::memcpy(&depthOffset               , &arBytes[offset], size = sizeof(depthOffset)               ); offset += size;
	std::memcpy(&confidenceOffset          , &arBytes[offset], size = sizeof(confidenceOffset)          ); offset += size;
	std::memcpy(&camera.ar.intrinsics      , &arBytes[offset], size = sizeof(camera.ar.intrinsics)      ); offset += size;
	std::memcpy(&camera.ar.projectionMatrix, &arBytes[offset], size = sizeof(camera.ar.projectionMatrix)); offset += size;
	std::memcpy(&camera.ar.viewMatrix      , &arBytes[offset], size = sizeof(camera.ar.viewMatrix)      ); offset += size;

	// カメラの取得
	if (1 == colorExists) {
		colorCap >> camera.color;
		if (camera.color.empty()) { colorExists = 0; }
	}

	// デプスの取得
	if (1 == depthExists) [&](){
		// 注: ここはラムダ式です
		depthExists = 0;
		depthZlib.fs.seekg(depthOffset);
		int64_t src_size;
		depthZlib.fs.read((char*)&src_size, sizeof(src_size));
		if (!depthZlib.fs.good()) { return; }
		depthZlib.src.resize(src_size);
		depthZlib.fs.read((char*)depthZlib.src.data(), depthZlib.src.size());
		if (!depthZlib.fs.good()) { return; }
		z_stream stream;
		memset(&stream, 0, sizeof(z_stream));
		stream.next_in = (Bytef*)depthZlib.src.data();
		stream.avail_in = depthZlib.src.size();
		stream.next_out = (Bytef*)depthZlib.dst.data();
		stream.avail_out = depthZlib.dst.size();
		inflateInit2(&stream, -15);
		int result = inflate(&stream, Z_FINISH);
		if (Z_STREAM_END != result && Z_OK != result) { return; }
		inflateEnd(&stream);
		camera.depth = cv::Mat(depthZlib.height, depthZlib.width, CV_32FC1, depthZlib.dst.data());
		depthExists = 1;
	}();

	// 信頼度の取得
	if (1 == confidenceExists) [&](){
		// 注: ここはラムダ式です
		confidenceExists = 0;
		confidenceZlib.fs.seekg(confidenceOffset);
		int64_t src_size;
		confidenceZlib.fs.read((char*)&src_size, sizeof(src_size));
		if (!confidenceZlib.fs.good()) { return; }
		confidenceZlib.src.resize(src_size);
		confidenceZlib.fs.read((char*)confidenceZlib.src.data(), confidenceZlib.src.size());
		if (!confidenceZlib.fs.good()) { return; }
		z_stream stream;
		memset(&stream, 0, sizeof(z_stream));
		stream.next_in = (Bytef*)confidenceZlib.src.data();
		stream.avail_in = confidenceZlib.src.size();
		stream.next_out = (Bytef*)confidenceZlib.dst.data();
		stream.avail_out = confidenceZlib.dst.size();
		inflateInit2(&stream, -15);
		int result = inflate(&stream, Z_FINISH);
		if (Z_STREAM_END != result && Z_OK != result) { return; }
		inflateEnd(&stream);
		camera.confidence = cv::Mat(confidenceZlib.height, confidenceZlib.width, CV_8UC1, confidenceZlib.dst.data());
		confidenceExists = 1;
	}();

	// カメラ、デプス、信頼度の3つが揃っていなければ次のフレームを取得
	if (0 == (colorExists & depthExists & confidenceExists)) { return next(); }

	return camera;
}
