#include "loader/quad_loader.h"

using namespace qs;

QuadLoader::QuadLoader() {}

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

	isOpened_ = true;

	return;
}

void QuadLoader::close() {
	if (cameraLoader.isOpened()) cameraLoader.close();
	if (imuLoader.isOpened()) imuLoader.close();
	if (gpsLoader.isOpened()) gpsLoader.close();
	isOpened_ = false;
}

bool QuadLoader::isOpened() const {
	return isOpened_;
}

std::optional<QuadFrame> QuadLoader::next() {
	// ファイルが開かれていなければ処理を終了
	if (!isOpened()) { return std::nullopt; }
	QuadFrame quadFrame;

	// カメラ
	std::optional<Camera> camera = cameraLoader.next();
	if (!camera.has_value()) { close(); return std::nullopt; }
	quadFrame.camera = camera.value();

	/*
		TODO: カメラのタイムスタンプに合わせたIMUとGPSの値を計算し代入する
	*/

	return quadFrame;
}
