#pragma once
#include <optional>
#include <fstream>
#include <stdint.h>
#include "types.h"

namespace qs {
	struct IMULoader {
		IMULoader();
		virtual ~IMULoader();
		void open(std::string recDirPath);
		void close();
		bool isOpened();
		std::optional<IMU> next();

	private:
		std::optional<IMU> imu;
		std::ifstream imuFs;
	};
}
