#include "loader/quad_loader.h"
#include <iostream>

using namespace qs;

QuadLoader::QuadLoader() {}

QuadLoader::~QuadLoader() {}

void QuadLoader::open(std::string recDirPath) {
	// データベース
	using namespace sqlite_orm;
	storage = std::make_unique<QSStorage>(makeQSStorage(recDirPath + "/db.sqlite3"));
	for (auto& desc : storage->iterate<Description>()) { description = std::move(desc); }
	cameraItr.init(*storage);
	gpsItr.init(*storage);
	imuItr.init(*storage);

	// カメラ
	video.open(recDirPath + "/camera.mp4");
	if (!video.isOpened()) { close(); return; }

	return;
}

void QuadLoader::close() {
	storage.release();
	video.release();
}

bool QuadLoader::isOpened() const {
	return static_cast<bool>(storage);
}

std::optional<QuadFrame> QuadLoader::next() {
	// ファイルが開かれていなければ処理を終了
	if (!isOpened()) { return std::nullopt; }
	QuadFrame quadFrame;

	std::optional<CameraForORM> cam = cameraItr.next();
	if (!static_cast<bool>(cam)) { return std::nullopt; }

	/*
		TODO: カメラのタイムスタンプに合わせたIMUとGPSの値を計算し代入する
	*/

	return quadFrame;
}
