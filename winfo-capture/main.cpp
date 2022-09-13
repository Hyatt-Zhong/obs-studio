#include "win-capture.h"

#include <shellscalingapi.h>

#pragma comment(lib, "Shcore.lib")
using namespace std;

void main()
{
	SetConsoleOutputCP(CP_UTF8);
	init();
	SetProcessDpiAwareness(
		PROCESS_SYSTEM_DPI_AWARE); //在系统设置缩放比的情况下，GetClientRect获取到正确的rect大小，以完全抓取窗口
	condition_variable cv;
	mutex mtx;

	wc_data dt;
	dt.source = nullptr;

	thread th(windows_msg_loop, &cv, &dt);
	th.detach();

	unique_lock<mutex> lk(mtx);
	cv.wait(lk);

	auto s = create_source();
	dt.source = s;
	auto window = select_source(s);
	modify_source(s, window.c_str());

	auto sc = create_scene();
	add_source_to_scene(s, sc);
	set_output_source(obs_scene_get_source(sc));

	while (true) {
		auto window = select_source(s);
		modify_source(s, window.c_str());
	}
}
