#include <sqlite_orm/sqlite_orm.h>
#include <iostream>
#include <string>
#include <memory>
#include <ctime>

struct Description {
	std::string date;
	uint64_t colorWidth;
	uint64_t colorHeight;
	std::unique_ptr<uint64_t> depthWidth;
	std::unique_ptr<uint64_t> depthHeight;
	std::unique_ptr<uint64_t> confidenceWidth;
	std::unique_ptr<uint64_t> confidenceHeight;
};

struct Camera {
	uint64_t id;
	double timestamp;
	std::unique_ptr<uint64_t> colorFrame;
	std::unique_ptr<std::vector<char>> depthZlib;
	std::unique_ptr<std::vector<char>> confidenceZlib;
	std::unique_ptr<std::vector<char>> intrinsicsMatrix;
	std::unique_ptr<std::vector<char>> projectionMatrix;
	std::unique_ptr<std::vector<char>> viewMatrix;
};

struct Gps {
	uint64_t id;
	double timestamp;
	double latitude;
	double longitude;
	double altitude;
	double horizontalAccuracy;
	double verticalAccuracy;
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
};

inline auto make_qs_storage(const std::string& filename) {
	using namespace sqlite_orm;
	return make_storage(filename,
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
			make_column("id", &Camera::id, autoincrement(), primary_key()),
			make_column("timestamp", &Camera::timestamp),
			make_column("color_frame", &Camera::colorFrame, unique()),
			make_column("depth_zlib", &Camera::depthZlib),
			make_column("confidence_zlib", &Camera::confidenceZlib),
			make_column("intrinsics_matrix_3x3", &Camera::intrinsicsMatrix),
			make_column("projection_matrix_4x4", &Camera::projectionMatrix),
			make_column("view_matrix_4x4", &Camera::viewMatrix)
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
			make_column("user_acclerationX", &Imu::userAcclerationX),
			make_column("user_acclerationY", &Imu::userAcclerationY),
			make_column("user_acclerationZ", &Imu::userAcclerationZ),
			make_column("attitude_x", &Imu::attitudeX),
			make_column("attitude_y", &Imu::attitudeY),
			make_column("attitude_z", &Imu::attitudeZ)
		)
	);
}

int main(int argc, char* argv[]) {
	using namespace sqlite_orm;

	if (2 != argc) {
		std::cout
			<< "example_db_read version 0.0.1\n"
			<< "\n"
			<< "usage: example_db_read input_path\n"
			<< "  input_path: Directory containing QuadDump recording files"
			<< "\n"
			<< std::endl;
		return 0;
	}

	std::string recDirPath = argv[1];

	using QSStorage = decltype(make_qs_storage(std::declval<std::string>()));
	QSStorage storage = make_qs_storage(recDirPath + "/db.sqlite3");
	//storage->sync_schema();

	for (auto& description : storage.iterate<Description>()) {
		std::cout << storage.dump(description) << std::endl;
	}
	std::cout << "camera frame: " << storage.count<Camera>() << std::endl;
	std::cout << "gps frame: " << storage.count<Gps>() << std::endl;
	std::cout << "imu frame: " << storage.count<Imu>() << std::endl;

	return 0;
}
