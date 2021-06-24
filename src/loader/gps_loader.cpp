#include "loader/gps_loader.h"

using namespace qs;

GPSLoader::GPSLoader() {}

GPSLoader::~GPSLoader() {}

void GPSLoader::open(std::string recDirPath) {
	// 今まで開いていたファイルを閉じる
	close();

	// ファイルのファイルパス
	const std::string gpsPath = recDirPath + "/gps";

	// デプスの映像を取得
	gpsFs.open(gpsPath, std::ios_base::in | std::ios_base::binary);
	if (!gpsFs.is_open()) { close(); return; }

	isOpened_ = true;

	return;
}

void GPSLoader::close() {
	if (gpsFs.is_open()) gpsFs.close();
	isOpened_ = false;
}

bool GPSLoader::isOpened() const {
	return isOpened_;
}

std::optional<GPS> GPSLoader::next() {
	// ファイルが開かれていなければ処理を終了
	if (!isOpened()) { return std::nullopt; }
	GPS gps;

	// GPSの値を取得
	uint8_t gpsBytes[48];
	gpsFs.read((char*)gpsBytes, sizeof(gpsBytes));
	if (!gpsFs.good()) { close(); return std::nullopt; }
	size_t offset = 0, size = 0;
	std::memcpy(&gps.timestamp         , &gpsBytes[offset], size = sizeof(gps.timestamp         )); offset += size;
	std::memcpy(&gps.latitude          , &gpsBytes[offset], size = sizeof(gps.latitude          )); offset += size;
	std::memcpy(&gps.longitude         , &gpsBytes[offset], size = sizeof(gps.longitude         )); offset += size;
	std::memcpy(&gps.altitude          , &gpsBytes[offset], size = sizeof(gps.altitude          )); offset += size;
	std::memcpy(&gps.horizontalAccuracy, &gpsBytes[offset], size = sizeof(gps.horizontalAccuracy)); offset += size;
	std::memcpy(&gps.verticalAccuracy  , &gpsBytes[offset], size = sizeof(gps.verticalAccuracy  )); offset += size;

	return gps;
}
