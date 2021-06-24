#include "quad_loader.h"

using namespace qs;

QuadLoader::QuadLoader() : frame(std::nullopt) {}

QuadLoader::~QuadLoader() {}

void QuadLoader::open(std::string recDirPath) {
	// 今まで開いていたファイルをすべて閉じる
	close();

	// 各ファイルのファイルパス
	const std::string
		infoJsonPath        = recDirPath + "/info.json",
		cameraFrameInfoPath = recDirPath + "/cameraFrameInfo",
		cameraPath          = recDirPath + "/camera.mp4",
		depthPath           = recDirPath + "/depth",
		confidencePath      = recDirPath + "/confidence",
		imuPath             = recDirPath + "/imu",
		gpsPath             = recDirPath + "/gps";

	// 録画データ全体の情報を取得
	std::ifstream infoJsonFs(infoJsonPath, std::ios_base::in | std::ios_base::binary);
	if ((!infoJsonFs.is_open()) || (!infoJsonFs.good())) { close(); return; }
	std::string infoJsonString((std::istreambuf_iterator<char>(infoJsonFs)), std::istreambuf_iterator<char>());
	infoJsonFs.close();
	frame = Frame();
	Frame& frame_ = frame.value();
	try {
		nlohmann::json infoJson = nlohmann::json::parse(infoJsonString);

		int cameraWidth = infoJson.at("camera.mp4").at("width");
		int cameraHeight = infoJson.at("camera.mp4").at("height");
		frame_.camera = cv::Mat(cameraHeight, cameraWidth, CV_8UC3);

		int depthWidth = infoJson.at("depth").at("width");
		int depthHeight = infoJson.at("depth").at("height");
		frame_.depth = cv::Mat(depthHeight, depthWidth, CV_32FC1);

		int confidenceWidth = infoJson.at("confidence").at("width");
		int confidenceHeight = infoJson.at("confidence").at("height");
		frame_.confidence = cv::Mat(confidenceHeight, confidenceWidth, CV_8UC1);
	}
	catch(const nlohmann::detail::out_of_range& e) { close(); return; }

	// カメラ、デプス、信頼度に関する情報を取得
	arFs.open(cameraFrameInfoPath, std::ios_base::in | std::ios_base::binary);
	if (!arFs.is_open()) { close(); return; }

	// カメラの映像を取得
	cameraCap.open(cameraPath);
	if (!cameraCap.isOpened()) { close(); return; }

	// デプスの映像を取得
	depthFs.open(depthPath, std::ios_base::in | std::ios_base::binary);
	if (!depthFs.is_open()) { close(); return; }

	// 信頼度の映像を取得
	confidenceFs.open(confidencePath, std::ios_base::in | std::ios_base::binary);
	if (!confidenceFs.is_open()) { close(); return; }

	return;
}

void QuadLoader::close() {
	if (cameraCap.isOpened()) cameraCap.release();
	if (depthFs.is_open()) depthFs.close();
	if (confidenceFs.is_open()) confidenceFs.close();
	frame = std::nullopt;
}

bool QuadLoader::isOpened() {
	return frame.has_value();
}

std::optional<Frame> QuadLoader::next() {
	// ファイルが開かれていなければ処理を終了
	if (!frame.has_value()) { return std::nullopt; }
	Frame& frame_ = frame.value();

	// フレームのタイムスタンプなどを取得
	uint8_t arBytes[199];
	uint8_t cameraExists, depthExists, confidenceExists;
	uint64_t depthOffset, confidenceOffset;
	arFs.read((char*)arBytes, sizeof(arBytes));
	if (!arFs.good()) { close(); return std::nullopt; }
	size_t offset = 0, size = 0;
	std::memcpy(&frame_.frameNumber        , &arBytes[offset], size = sizeof(frame_.frameNumber)        ); offset += size;
	std::memcpy(&frame_.timestamp          , &arBytes[offset], size = sizeof(frame_.timestamp)          ); offset += size;
	std::memcpy(&cameraExists              , &arBytes[offset], size = sizeof(cameraExists)              ); offset += size;
	std::memcpy(&depthExists               , &arBytes[offset], size = sizeof(depthExists)               ); offset += size;
	std::memcpy(&confidenceExists          , &arBytes[offset], size = sizeof(confidenceExists)          ); offset += size;
	std::memcpy(&depthOffset               , &arBytes[offset], size = sizeof(depthOffset)               ); offset += size;
	std::memcpy(&confidenceOffset          , &arBytes[offset], size = sizeof(confidenceOffset)          ); offset += size;
	std::memcpy(&frame_.ar.intrinsics      , &arBytes[offset], size = sizeof(frame_.ar.intrinsics)      ); offset += size;
	std::memcpy(&frame_.ar.projectionMatrix, &arBytes[offset], size = sizeof(frame_.ar.projectionMatrix)); offset += size;
	std::memcpy(&frame_.ar.viewMatrix      , &arBytes[offset], size = sizeof(frame_.ar.viewMatrix)      ); offset += size;

	// カメラの取得
	if (1 == cameraExists) {
		cameraCap >> frame_.camera;
		if (frame_.camera.empty()) { cameraExists = 0; }
	}

	// デプスの取得
	if (1 == depthExists) [&](){
		// 注: ここはラムダ式です
		depthExists = 0;
		cv::Mat &depth = frame_.depth;
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
		stream.avail_out = depth.size().width * depth.size().height * 4;
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
		cv::Mat &confidence = frame_.confidence;
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
		stream.avail_out = confidence.size().width * confidence.size().height;
		inflateInit2(&stream, -15);
		int result = inflate(&stream, Z_FINISH);
		if (Z_STREAM_END != result && Z_OK != result) { return; }
		inflateEnd(&stream);
		confidenceExists = 1;
	}();

	// カメラ、デプス、信頼度の3つが揃っていなければ次のフレームを取得
	if (0 == (cameraExists & depthExists & confidenceExists)) { return next(); }

	return frame;
}
