#include "loader/quad_loader.h"

using namespace qs;

QuadLoader::QuadLoader() : quadFrame(std::nullopt) {}

QuadLoader::~QuadLoader() {}

void QuadLoader::open(std::string recDirPath) {
	// 今まで開いていたファイルをすべて閉じる
	close();

	// カメラ
	cameraLoader.open(recDirPath);
	if (!cameraLoader.isOpened()) { close(); return; }

	// IMU
	imuLoader.open(recDirPath);
	if (!imuLoader.isOpened()) { close(); return; }

	// GPS
	gpsLoader.open(recDirPath);
	if (!gpsLoader.isOpened()) { close(); return; }

	quadFrame = QuadFrame();

	return;
}

void QuadLoader::close() {
	if (cameraLoader.isOpened()) cameraLoader.close();
	if (imuLoader.isOpened()) imuLoader.close();
	if (gpsLoader.isOpened()) gpsLoader.close();
	quadFrame = std::nullopt;
}

bool QuadLoader::isOpened() {
	return quadFrame.has_value();
}

std::optional<QuadFrame> QuadLoader::next() {
	// ファイルが開かれていなければ処理を終了
	if (!quadFrame.has_value()) { return std::nullopt; }
	QuadFrame& quadFrame_ = quadFrame.value();

	// カメラ
	std::optional<Camera> camera = cameraLoader.next();
	if (!camera.has_value()) { close(); return std::nullopt; }

	return quadFrame_;
}
