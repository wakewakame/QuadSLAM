#include "loader/gps_loader.h"

using namespace qs;

GPSLoader::GPSLoader() : gps(std::nullopt) {}

GPSLoader::~GPSLoader() {}

void GPSLoader::open(std::string recDirPath) {
	// 今まで開いていたファイルを閉じる
	close();

	// ファイルのファイルパス
	const std::string gpsPath = recDirPath + "/gps";

	// デプスの映像を取得
	gpsFs.open(gpsPath, std::ios_base::in | std::ios_base::binary);
	if (!gpsFs.is_open()) { close(); return; }

	gps = GPS();

	return;
}

void GPSLoader::close() {
	if (gpsFs.is_open()) gpsFs.close();
	gps = std::nullopt;
}

bool GPSLoader::isOpened() {
	return gps.has_value();
}

std::optional<GPS> GPSLoader::next() {
	// ファイルが開かれていなければ処理を終了
	if (!gps.has_value()) { return std::nullopt; }
	GPS& gps_ = gps.value();

	// GPSの値を取得
	uint8_t gpsBytes[48];
	gpsFs.read((char*)gpsBytes, sizeof(gpsBytes));
	if (!gpsFs.good()) { close(); return std::nullopt; }
	size_t offset = 0, size = 0;
	std::memcpy(&gps_.timestamp         , &gpsBytes[offset], size = sizeof(gps_.timestamp         )); offset += size;
	std::memcpy(&gps_.latitude          , &gpsBytes[offset], size = sizeof(gps_.latitude          )); offset += size;
	std::memcpy(&gps_.longitude         , &gpsBytes[offset], size = sizeof(gps_.longitude         )); offset += size;
	std::memcpy(&gps_.altitude          , &gpsBytes[offset], size = sizeof(gps_.altitude          )); offset += size;
	std::memcpy(&gps_.horizontalAccuracy, &gpsBytes[offset], size = sizeof(gps_.horizontalAccuracy)); offset += size;
	std::memcpy(&gps_.verticalAccuracy  , &gpsBytes[offset], size = sizeof(gps_.verticalAccuracy  )); offset += size;

	return gps_;
}
