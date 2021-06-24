#include "loader/camera_loader.h"

using namespace qs;

CameraLoader::CameraLoader() : camera(std::nullopt) {}

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
	camera = Camera();
	Camera& camera_ = camera.value();
	try {
		nlohmann::json infoJson = nlohmann::json::parse(infoJsonString);

		int colorWidth = infoJson.at("camera.mp4").at("width");
		int colorHeight = infoJson.at("camera.mp4").at("height");
		camera_.color = cv::Mat(colorHeight, colorWidth, CV_8UC3);

		int depthWidth = infoJson.at("depth").at("width");
		int depthHeight = infoJson.at("depth").at("height");
		camera_.depth = cv::Mat(depthHeight, depthWidth, CV_32FC1);

		int confidenceWidth = infoJson.at("confidence").at("width");
		int confidenceHeight = infoJson.at("confidence").at("height");
		camera_.confidence = cv::Mat(confidenceHeight, confidenceWidth, CV_8UC1);
	}
	catch(const nlohmann::detail::out_of_range& e) { close(); return; }

	// カメラ、デプス、信頼度に関する情報を取得
	arFs.open(cameraFrameInfoPath, std::ios_base::in | std::ios_base::binary);
	if (!arFs.is_open()) { close(); return; }

	// カメラの映像を取得
	colorCap.open(cameraPath);
	if (!colorCap.isOpened()) { close(); return; }

	// デプスの映像を取得
	depthFs.open(depthPath, std::ios_base::in | std::ios_base::binary);
	if (!depthFs.is_open()) { close(); return; }

	// 信頼度の映像を取得
	confidenceFs.open(confidencePath, std::ios_base::in | std::ios_base::binary);
	if (!confidenceFs.is_open()) { close(); return; }

	return;
}

void CameraLoader::close() {
	if (colorCap.isOpened()) colorCap.release();
	if (depthFs.is_open()) depthFs.close();
	if (confidenceFs.is_open()) confidenceFs.close();
	camera = std::nullopt;
}

bool CameraLoader::isOpened() {
	return camera.has_value();
}

std::optional<Camera> CameraLoader::next() {
	// ファイルが開かれていなければ処理を終了
	if (!camera.has_value()) { return std::nullopt; }
	Camera& camera_ = camera.value();

	// フレームのタイムスタンプなどを取得
	uint8_t arBytes[199];
	uint8_t colorExists, depthExists, confidenceExists;
	uint64_t depthOffset, confidenceOffset;
	arFs.read((char*)arBytes, sizeof(arBytes));
	if (!arFs.good()) { close(); return std::nullopt; }
	size_t offset = 8, size = 0;
	std::memcpy(&camera_.timestamp          , &arBytes[offset], size = sizeof(camera_.timestamp)          ); offset += size;
	std::memcpy(&colorExists                , &arBytes[offset], size = sizeof(colorExists)                ); offset += size;
	std::memcpy(&depthExists                , &arBytes[offset], size = sizeof(depthExists)                ); offset += size;
	std::memcpy(&confidenceExists           , &arBytes[offset], size = sizeof(confidenceExists)           ); offset += size;
	std::memcpy(&depthOffset                , &arBytes[offset], size = sizeof(depthOffset)                ); offset += size;
	std::memcpy(&confidenceOffset           , &arBytes[offset], size = sizeof(confidenceOffset)           ); offset += size;
	std::memcpy(&camera_.ar.intrinsics      , &arBytes[offset], size = sizeof(camera_.ar.intrinsics)      ); offset += size;
	std::memcpy(&camera_.ar.projectionMatrix, &arBytes[offset], size = sizeof(camera_.ar.projectionMatrix)); offset += size;
	std::memcpy(&camera_.ar.viewMatrix      , &arBytes[offset], size = sizeof(camera_.ar.viewMatrix)      ); offset += size;

	// カメラの取得
	if (1 == colorExists) {
		colorCap >> camera_.color;
		if (camera_.color.empty()) { colorExists = 0; }
	}

	// デプスの取得
	if (1 == depthExists) [&](){
		// 注: ここはラムダ式です
		depthExists = 0;
		cv::Mat &depth = camera_.depth;
		depthFs.seekg(depthOffset);
		int64_t src_size;
		depthFs.read((char*)&src_size, sizeof(src_size));
		if (!depthFs.good()) { return; }
		std::vector<uint8_t> src(src_size);
		depthFs.read((char*)src.data(), src_size);
		if (!depthFs.good()) { return; }
		z_stream stream;
		memset(&stream, 0, sizeof(z_stream));
		stream.next_in = (Bytef*)src.data();
		stream.avail_in = src.size();
		stream.next_out = (Bytef*)depth.data;
		stream.avail_out = depth.total() * depth.elemSize();
		inflateInit2(&stream, -15);
		int result = inflate(&stream, Z_FINISH);
		if (Z_STREAM_END != result && Z_OK != result) { return; }
		inflateEnd(&stream);
		depthExists = 1;
	}();

	// 信頼度の取得
	if (1 == confidenceExists) [&](){
		// 注: ここはラムダ式です
		confidenceExists = 0;
		cv::Mat &confidence = camera_.confidence;
		confidenceFs.seekg(confidenceOffset);
		int64_t src_size;
		confidenceFs.read((char*)&src_size, sizeof(src_size));
		if (!confidenceFs.good()) { return; }
		std::vector<uint8_t> src(src_size);
		confidenceFs.read((char*)src.data(), src_size);
		if (!confidenceFs.good()) { return; }
		z_stream stream;
		memset(&stream, 0, sizeof(z_stream));
		stream.next_in = (Bytef*)src.data();
		stream.avail_in = src.size();
		stream.next_out = (Bytef*)confidence.data;
		stream.avail_out = confidence.total() * confidence.elemSize();
		inflateInit2(&stream, -15);
		int result = inflate(&stream, Z_FINISH);
		if (Z_STREAM_END != result && Z_OK != result) { return; }
		inflateEnd(&stream);
		confidenceExists = 1;
	}();

	// カメラ、デプス、信頼度の3つが揃っていなければ次のフレームを取得
	if (0 == (colorExists & depthExists & confidenceExists)) { return next(); }

	return camera_;
}
