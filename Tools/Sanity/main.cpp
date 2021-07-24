#include "Assistance.h"
#include "Updater.h"
#include "Logger.hpp"

int main(int argc, char** argv)
{
	using namespace asst;

	bool up = Updater::get_instance().has_new_version();
	if (up) {
		auto && info = Updater::get_instance().get_version_info();
		std::cout << "���°汾��" << info.tag_name << std::endl;
		std::cout << "��ַ��" << info.html_url << std::endl;
	}

	Assistance asst;

	auto ret = asst.set_emulator();
	if (!ret) {
		DebugTraceError("Can't Find Emulator or Permission denied.");
		getchar();
		return -1;
	}
	else {
		DebugTraceInfo("Find Emulator:", ret.value());
	}

	DebugTraceInfo("Start");
	asst.start("SanityBegin");

	getchar();

	DebugTraceInfo("Stop");
	asst.stop();

	return 0;
}