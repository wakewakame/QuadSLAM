#include "loader/imu_loader.h"

using namespace qs;

IMULoader::IMULoader() {}

IMULoader::~IMULoader() {}

void IMULoader::open(std::string recDirPath) {
	// 今まで開いていたファイルを閉じる
	close();

	// ファイルのファイルパス
	const std::string imuPath = recDirPath + "/imu";

	// デプスの映像を取得
	imuFs.open(imuPath, std::ios_base::in | std::ios_base::binary);
	if (!imuFs.is_open()) { close(); return; }

	isOpened_ = true;

	return;
}

void IMULoader::close() {
	if (imuFs.is_open()) imuFs.close();
	isOpened_ = false;
}

bool IMULoader::isOpened() const {
	return isOpened_;
}

std::optional<IMU> IMULoader::next() {
	// ファイルが開かれていなければ処理を終了
	if (!isOpened()) { return std::nullopt; }
	IMU imu;

	// IMUの値を取得
	uint8_t imuBytes[80];
	imuFs.read((char*)imuBytes, sizeof(imuBytes));
	if (!imuFs.good()) { close(); return std::nullopt; }
	size_t offset = 0, size = 0;
	std::memcpy(&imu.timestamp      , &imuBytes[offset], size = sizeof(imu.timestamp      )); offset += size;
	std::memcpy(&imu.gravity        , &imuBytes[offset], size = sizeof(imu.gravity        )); offset += size;
	std::memcpy(&imu.userAccleration, &imuBytes[offset], size = sizeof(imu.userAccleration)); offset += size;
	std::memcpy(&imu.attitude       , &imuBytes[offset], size = sizeof(imu.attitude       )); offset += size;

	return imu;
}
