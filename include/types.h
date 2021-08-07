#pragma once
#include <stdint.h>
#include <optional>
#include "opencv2/opencv.hpp"
#include "sqlite_orm/sqlite_orm.h"
#include "qs_zlib/zlib.h"

namespace qs {
	struct Description {
		std::string date;
		uint64_t colorWidth;
		uint64_t colorHeight;
		std::optional<uint64_t> depthWidth, depthHeight;
		std::optional<uint64_t> confidenceWidth, confidenceHeight;
	};

	struct Camera {
		/*
			メモ
			OpenCV 4.x より、cv::Matにムーブコンストラクタとムーブ代入演算子が実装された。
			そのため、Cameraには暗黙的にコピーコンストラクタ、ムーブコンストラクタ、コピー代入演算子、ムーブ代入演算子が定義される。
			また、cv::Matは内部で参照カウンタを持っており、通常のコピーではデータの参照先は同じになる。
			そのため、深いコピーを行うときにはCamera::clone()メソッドを使用する。
		*/

		uint64_t frameNumber;
		double timestamp;
		cv::Mat color;
		cv::Mat depth;
		cv::Mat confidence;
		cv::Mat intrinsicsMatrix;
		cv::Mat projectionMatrix;
		cv::Mat viewMatrix;

		Camera clone() const;
	};

	struct Imu {
		uint64_t id;
		double timestamp;
		double gravityX;
		double gravityY;
		double gravityZ;
		double userAcclerationX;
		double userAcclerationY;
		double userAcclerationZ;
		double attitudeX;
		double attitudeY;
		double attitudeZ;

		cv::Vec3d cvGravity() const;
		cv::Vec3d cvUserAccleration() const;
		cv::Vec3d cvAttitude() const;
	};

	struct Gps {
		uint64_t id;
		double timestamp;
		double latitude;
		double longitude;
		double altitude;
		double horizontalAccuracy;
		double verticalAccuracy;

		cv::Vec3d cvGps() const;
	};

	struct QuadFrame {
		Camera camera;
		std::vector<Imu> imu;
		std::vector<Gps> gps;

		QuadFrame clone() const;
	};

	struct CameraForOrm {
		uint64_t id;
		double timestamp;
		std::optional<uint64_t> colorFrame;
		std::optional<std::vector<char>> depthZlib;
		std::optional<std::vector<char>> confidenceZlib;
		std::optional<std::vector<char>> intrinsicsMatrix;
		std::optional<std::vector<char>> projectionMatrix;
		std::optional<std::vector<char>> viewMatrix;

		Camera toCamera(Description description, cv::Mat color) const;
	};

	inline auto makeQSStorage(const std::string& filepath) {
		using namespace sqlite_orm;
		return make_storage(filepath,
			make_table("description",
				make_column("date", &Description::date),
				make_column("color_width", &Description::colorWidth),
				make_column("color_height", &Description::colorHeight),
				make_column("depth_width", &Description::depthWidth),
				make_column("depth_height", &Description::depthHeight),
				make_column("confidence_width", &Description::confidenceWidth),
				make_column("confidence_height", &Description::confidenceHeight)
			),
			make_table("camera",
				make_column("id", &CameraForOrm::id, autoincrement(), primary_key()),
				make_column("timestamp", &CameraForOrm::timestamp),
				make_column("color_frame", &CameraForOrm::colorFrame, unique()),
				make_column("depth_zlib", &CameraForOrm::depthZlib),
				make_column("confidence_zlib", &CameraForOrm::confidenceZlib),
				make_column("intrinsics_matrix_3x3", &CameraForOrm::intrinsicsMatrix),
				make_column("projection_matrix_4x4", &CameraForOrm::projectionMatrix),
				make_column("view_matrix_4x4", &CameraForOrm::viewMatrix)
			),
			make_table("gps",
				make_column("id", &Gps::id, autoincrement(), primary_key()),
				make_column("timestamp", &Gps::timestamp),
				make_column("latitude", &Gps::latitude),
				make_column("longitude", &Gps::longitude),
				make_column("altitude", &Gps::altitude),
				make_column("horizontal_accuracy", &Gps::horizontalAccuracy),
				make_column("vertical_accuracy", &Gps::verticalAccuracy)
			),
			make_table("imu",
				make_column("id", &Imu::id, autoincrement(), primary_key()),
				make_column("timestamp", &Imu::timestamp),
				make_column("gravity_x", &Imu::gravityX),
				make_column("gravity_y", &Imu::gravityY),
				make_column("gravity_z", &Imu::gravityZ),
				make_column("user_accleration_x", &Imu::userAcclerationX),
				make_column("user_accleration_y", &Imu::userAcclerationY),
				make_column("user_accleration_z", &Imu::userAcclerationZ),
				make_column("attitude_x", &Imu::attitudeX),
				make_column("attitude_y", &Imu::attitudeY),
				make_column("attitude_z", &Imu::attitudeZ)
			),
			make_unique_index("idx_camera_color_frame", &CameraForOrm::colorFrame),
			make_unique_index("idx_imu_timestamp", &Imu::timestamp),
			make_unique_index("idx_gps_timestamp", &Gps::timestamp)
		);
	}
	using QSStorage = decltype(makeQSStorage(std::declval<std::string>()));
}
