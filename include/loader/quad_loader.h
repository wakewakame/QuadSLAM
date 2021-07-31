#pragma once
#include <optional>
#include "types.h"
#include "opencv2/opencv.hpp"
#include "opencv2/videoio.hpp"
#include "qs_zlib/zlib.h"

namespace qs {
	struct CameraForORM {
		uint64_t id;
		double timestamp;
		std::optional<uint64_t> colorFrame;
		std::optional<std::vector<char>> depthZlib;
		std::optional<std::vector<char>> confidenceZlib;
		std::optional<std::vector<char>> intrinsicsMatrix;
		std::optional<std::vector<char>> projectionMatrix;
		std::optional<std::vector<char>> viewMatrix;
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
				make_column("id", &CameraForORM::id, autoincrement(), primary_key()),
				make_column("timestamp", &CameraForORM::timestamp),
				make_column("color_frame", &CameraForORM::colorFrame, unique()),
				make_column("depth_zlib", &CameraForORM::depthZlib),
				make_column("confidence_zlib", &CameraForORM::confidenceZlib),
				make_column("intrinsics_matrix_3x3", &CameraForORM::intrinsicsMatrix),
				make_column("projection_matrix_4x4", &CameraForORM::projectionMatrix),
				make_column("view_matrix_4x4", &CameraForORM::viewMatrix)
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
			)
		);
	}

	struct QuadLoader {
		QuadLoader();
		virtual ~QuadLoader();
		void open(std::string recDirPath);
		void close();
		bool isOpened() const;
		std::optional<QuadFrame> next();

	private:
		using QSStorage = decltype(makeQSStorage(std::declval<std::string>()));
		Description description;
		std::unique_ptr<QSStorage> storage;
		cv::VideoCapture video;

		template<class T>
		struct SQLIterator {
			using Range = sqlite_orm::internal::view_t<T, QSStorage>;
			using Iterator = sqlite_orm::internal::iterator_t<Range>;
			std::unique_ptr<Range> range;
			std::unique_ptr<Iterator> current, end;
			void init(QSStorage& storage) {
				range = std::make_unique<Range>(storage.iterate<T>());
				current = std::make_unique<Iterator>(range->begin());
				end = std::make_unique<Iterator>(range->end());
			}
			std::optional<T> next() {
				if (*current == *end) { return std::nullopt; }
				(*current)++;
				return **current;
			}
		};

		SQLIterator<CameraForORM> cameraItr;
		SQLIterator<Gps> gpsItr;
		SQLIterator<Imu> imuItr;
	};
}
