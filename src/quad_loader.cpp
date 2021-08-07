#include "quad_loader.h"
#include <cassert>
#include <cstring>

using namespace qs;

QuadLoader::QuadLoader() {}

QuadLoader::~QuadLoader() {}

void QuadLoader::open(const std::string& recDirPath) {
	using namespace sqlite_orm;

	// カメラ
	video.open(recDirPath + "/camera.mp4");
	if (!video.isOpened()) { close(); return; }

	// データベース
	try {
		storagePtr = std::make_unique<QSStorage>(makeQSStorage(recDirPath + "/db.sqlite3"));
		QSStorage& storage = *storagePtr;
		storage.sync_schema();
		std::optional<Description> descriptionOpt;
		for (auto& desc : storage.iterate<Description>()) {
			descriptionOpt = std::move(desc);
			break;
		}
		if (!descriptionOpt.has_value()) { close(); return; }
		description = descriptionOpt.value();
	}
	catch(const std::system_error&) { close(); return; }

	// 1フレーム前のタイムスタンプを表す変数をリセット
	preTimestamp = 0.0;
}

void QuadLoader::close() {
	video.release();
	storagePtr.release();
}

bool QuadLoader::isOpened() const {
	return (storagePtr && video.isOpened());
}

std::optional<QuadFrame> QuadLoader::next(bool withImu, bool withGps) {
	using namespace sqlite_orm;

	// ファイルが開かれていなければ処理を終了
	if (!isOpened()) { return std::nullopt; }
	QSStorage& storage = *storagePtr;

	// 動画の次のフレームを取得
	const uint64_t colorFrame = static_cast<uint64_t>(video.get(cv::CAP_PROP_POS_FRAMES));
	cv::Mat color;
	if (!video.read(color)) { return std::nullopt; }

	// 現在のフレーム番号の情報をデータベースから取得
	std::optional<CameraForOrm> cameraForOrmOpt;
	for(auto& camera : storage.iterate<CameraForOrm>(
		where(c(&CameraForOrm::colorFrame) == colorFrame)
	)) {
		cameraForOrmOpt = camera;
	}
	if (!cameraForOrmOpt.has_value()) { return std::nullopt; }
	CameraForOrm cameraForOrm = cameraForOrmOpt.value();

	// CameraIterでは検索条件に'where color_frame is not null'を指定しているので
	// colorFrameにnulloptが返ることはないはず
	assert(cameraForOrm.colorFrame.has_value());

	// CameraForOrmからCameraに変換
	Camera camera = std::move(cameraForOrm.toCamera(description, color));

	// IMU
	std::vector<Imu> imu;
	if (withImu) {
		imu = std::move(storage.get_all<Imu>(
			where(
				preTimestamp < c(&Imu::timestamp) and
				c(&Imu::timestamp) <= camera.timestamp
			)
		));
	}

	// GPS
	std::vector<Gps> gps;
	if (withGps) {
		gps = std::move(storage.get_all<Gps>(
			where(
				preTimestamp < c(&Gps::timestamp) and
				c(&Gps::timestamp) <= camera.timestamp
			)
		));
	}

	preTimestamp = camera.timestamp;

	return QuadFrame {
		std::move(camera), imu, gps
	};
}

const std::unique_ptr<QSStorage>& QuadLoader::getStorage() const {
	return storagePtr;
}

void QuadLoader::seek(const uint64_t frameNumber) {
	using namespace sqlite_orm;

	// ファイルが開かれていなければ処理を終了
	if (!isOpened()) { return; }
	QSStorage& storage = *storagePtr;

	// 動画のシーク
	video.set(cv::CAP_PROP_POS_FRAMES, static_cast<uint64_t>(frameNumber));
	const uint64_t postFrameNumber = static_cast<uint64_t>(video.get(cv::CAP_PROP_POS_FRAMES));

	// 1フレーム前が存在する場合、そのタイムスタンプを取得
	preTimestamp = 0.0;
	std::unique_ptr<double> preTimestampOpt = storage.max(&CameraForOrm::timestamp, where(
		is_not_null(&CameraForOrm::colorFrame) and
		c(&CameraForOrm::colorFrame) < frameNumber
	));
	if (preTimestampOpt) { preTimestamp = *preTimestampOpt; }
}
