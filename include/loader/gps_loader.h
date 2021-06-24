#pragma once
#include <optional>
#include <fstream>
#include <stdint.h>
#include "types.h"

namespace qs {
	struct GPSLoader {
		GPSLoader();
		virtual ~GPSLoader();
		void open(std::string recDirPath);
		void close();
		bool isOpened() const;
		std::optional<GPS> next();

	private:
		bool isOpened_ = false;
		std::ifstream gpsFs;
	};
}
