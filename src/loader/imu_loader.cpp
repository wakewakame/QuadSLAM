#include "loader/imu_loader.h"

using namespace qs;

IMULoader::IMULoader() : imu(std::nullopt) {}

IMULoader::~IMULoader() {}

void IMULoader::open(std::string recDirPath) {
	// 今まで開いていたファイルを閉じる
	close();

	// ファイルのファイルパス
	const std::string imuPath = recDirPath + "/imu";

	// デプスの映像を取得
	imuFs.open(imuPath, std::ios_base::in | std::ios_base::binary);
	if (!imuFs.is_open()) { close(); return; }

	imu = IMU();

	return;
}

void IMULoader::close() {
	if (imuFs.is_open()) imuFs.close();
	imu = std::nullopt;
}

bool IMULoader::isOpened() {
	return imu.has_value();
}

std::optional<IMU> IMULoader::next() {
	// ファイルが開かれていなければ処理を終了
	if (!imu.has_value()) { return std::nullopt; }
	IMU& imu_ = imu.value();

	// IMUの値を取得
	uint8_t imuBytes[80];
	imuFs.read((char*)imuBytes, sizeof(imuBytes));
	if (!imuFs.good()) { close(); return std::nullopt; }
	size_t offset = 0, size = 0;
	std::memcpy(&imu_.timestamp      , &imuBytes[offset], size = sizeof(imu_.timestamp      )); offset += size;
	std::memcpy(&imu_.gravity        , &imuBytes[offset], size = sizeof(imu_.gravity        )); offset += size;
	std::memcpy(&imu_.userAccleration, &imuBytes[offset], size = sizeof(imu_.userAccleration)); offset += size;
	std::memcpy(&imu_.attitude       , &imuBytes[offset], size = sizeof(imu_.attitude       )); offset += size;

	return imu_;
}
