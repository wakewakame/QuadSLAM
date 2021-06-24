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
		bool isOpened() const;
		std::optional<IMU> next();

	private:
		bool isOpened_ = false;
		std::ifstream imuFs;
	};
}
