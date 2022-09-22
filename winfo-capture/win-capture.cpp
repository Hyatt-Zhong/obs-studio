#include "win-capture.h"
#include "screenshot.h"

#include <iostream>
#include <map>
#include <string>
#include <filesystem>

using namespace std;

namespace ns_win_capture {

static const char *module_bin = "../../win-capture";

static const char *module_data = "../../data/obs-plugins/%module%";

const int window_width = 1440;
const int window_hight = 720;

static void do_log(int log_level, const char *msg, va_list args, void *param)
{
	char out_put[4096];
	vsnprintf(out_put, 4095, msg, args);

	OutputDebugStringA(out_put);
	OutputDebugStringA("\n");

	//if (log_level < LOG_WARNING)
	//__debugbreak();

	UNUSED_PARAMETER(param);
}

void init()
{
	base_set_log_handler(do_log, nullptr);

	if (!obs_startup("en-US", nullptr, nullptr))
		return;

	obs_remove_all_module_path();
	obs_add_module_path(module_bin, module_data);

	/* load modules */
	obs_load_all_modules();
}

obs_source_t *create_source()
{
	return obs_source_create("window_capture", "window_capture", NULL,
				 nullptr);
}

obs_scene_t *create_scene()
{
	return obs_scene_create("window_capture scene");
}

void add_source_to_scene(obs_source_t *s, obs_scene_t *sc)
{
	obs_sceneitem_t *item = NULL;
	struct vec2 scale;

	//vec2_set(&scale, .66f, .66f);
	vec2_set(&scale, 1.f, 1.f);

	item = obs_scene_add(sc, s);
	obs_sceneitem_set_scale(item, &scale);
}
obs_display_t *create_display(HWND hwnd)
{
	RECT rc;
	GetClientRect(hwnd, &rc);

	gs_init_data info = {};
	info.cx = rc.right;
	info.cy = rc.bottom;
	info.format = GS_RGBA;
	info.zsformat = GS_ZS_NONE;
	info.window.hwnd = hwnd;

	return obs_display_create(&info, 0);
}

void draw_func(void *data, uint32_t cx, uint32_t cy)
{
	obs_render_main_texture();
	//obs_render_main_view();
}

void set_output_source(obs_source_t *source)
{
	/* set the scene as the primary draw source and go */
	obs_set_output_source(0, source);
}

void GetScaleAndCenterPos(int baseCX, int baseCY, int windowCX, int windowCY,
			  int &x, int &y, float &scale)
{
	double windowAspect, baseAspect;
	int newCX, newCY;

	windowAspect = double(windowCX) / double(windowCY);
	baseAspect = double(baseCX) / double(baseCY);

	if (windowAspect > baseAspect) {
		scale = float(windowCY) / float(baseCY);
		newCX = int(double(windowCY) * baseAspect);
		newCY = windowCY;
	} else {
		scale = float(windowCX) / float(baseCX);
		newCX = windowCX;
		newCY = int(float(windowCX) / baseAspect);
	}

	x = windowCX / 2 - newCX / 2;
	y = windowCY / 2 - newCY / 2;
}

//绘制回调，每帧调用
void DrawPreview(void *data, uint32_t cx, uint32_t cy)
{
	wc_data *dt = (wc_data *)data;
	auto source = dt->source;
	auto hw = dt->hw;
	if (!source)
		return;

	CRect rt;
	GetClientRect(hw, &rt);

	uint32_t sourceCX = max(obs_source_get_width(source), 1u);
	uint32_t sourceCY = max(obs_source_get_height(source), 1u);

	int x, y;
	int newCX, newCY;
	float scale;

	GetScaleAndCenterPos(sourceCX, sourceCY, cx, cy, x, y, scale);

	newCX = int(scale * float(sourceCX));
	newCY = int(scale * float(sourceCY));
	x = y = 0;
	newCX = sourceCX /** 1.5*/;
	newCY = sourceCY /** 1.5*/;

	newCX = rt.Width(); /** 1.5*/
	;
	newCY = rt.Height(); /** 1.5*/
	;

	gs_viewport_push();
	gs_projection_push();
	const bool previous = gs_set_linear_srgb(true);

	gs_ortho(0.0f, float(sourceCX), 0.0f, float(sourceCY), -100.0f, 100.0f);
	gs_set_viewport(x, y, newCX, newCY);
	obs_source_video_render(source); //这里真正的绘制数据

	gs_set_linear_srgb(previous);
	gs_projection_pop();
	gs_viewport_pop();
}

void display(obs_display_t *ds, wc_data *data)
{
	//obs_display_add_draw_callback(ds, draw_func, nullptr);
	obs_display_add_draw_callback(ds, DrawPreview, data);
}

HWND create_display_window(windowx &w)
{
	CRect rt;
	auto offset = 10;
	rt.top = offset;
	rt.bottom = window_hight + offset;
	rt.left = offset;
	rt.right = window_width + offset;
	w.Create(NULL, rt, L"win-capture", WS_VISIBLE);
	return w.m_hWnd;
}

void add_cmd(std::map<int, std::pair<const char *, const char *>> *mp,
	     const char *cmd, void (*pfunc)(void *))
{
	auto index = mp->size();
	auto pr = std::make_pair(cmd, (char *)pfunc);
	(*mp)[(int)index] = pr;
	cout << index << ": " << cmd << endl;
}

string select_source(obs_source_t *source)
{
	auto pp = obs_source_properties(source);
	auto p = obs_properties_get(pp, "window");
	size_t count = obs_property_list_item_count(p);

	std::map<int, std::pair<const char *, const char *>> mp;
	for (size_t i = 0; i < count; i++) {
		const char *name = obs_property_list_item_name(p, i);
		const char *str = obs_property_list_item_string(p, i);
		auto pr = std::make_pair(name, str);
		mp[(int)i] = pr;
		cout << i << ": " << name << endl;
	}
	auto cur_path = filesystem::current_path();
	wstring save_dir = wstring(cur_path.c_str()) + L"\\screenshot";
	if (!filesystem::is_directory(save_dir)) {
		filesystem::create_directory(save_dir);
	}

	struct ss_param {
		obs_source_t *source;
		wstring save_path;
	};
	add_cmd(&mp, "screenshot", [](void *param) {
		ss_param *sp = (ss_param *)param;
		new ScreenshotObj(sp->source, sp->save_path);
		delete sp;
	});
	auto mp_count = mp.size();
	int no = -1;
	auto con = no < 0 || no >= mp_count;
	while (con) {
		cout << "select window or cmd:";
		cin >> no;
		con = no < 0 || no >= mp_count;

		if (no >= count && no < mp_count) {
			auto func = mp[no].second;
			auto sp = new ss_param;
			sp->save_path = save_dir;
			sp->source = source;
			((void (*)(void *))func)(sp);
			cout << mp[no].first << " done!" << endl;
			con = true;
		}
	}

	auto window = mp[no].second;

	return window;
}

void modify_source(obs_source_t *source, const char *window)
{
	auto pp = obs_source_properties(source);
	auto p = obs_properties_get(pp, "window");

	auto setting = obs_source_get_settings(source);

	obs_data_set_int(setting, "method", 0);
	obs_data_set_string(setting, "window", window);
	obs_data_set_int(setting, "priority", 0);

	obs_data_set_bool(setting, "cursor", true);
	obs_data_set_bool(setting, "use_wildcards", false);
	obs_data_set_bool(setting, "compatibility", false);
	obs_data_set_bool(setting, "client_area", true);

	obs_property_modified(p, setting);
	obs_source_update(source, setting);
}

void handle_cmd(obs_source_t *source)
{
	string cmd;
	while (true) {
		cin >> cmd;
	}
}

void reset_video_info(HWND hwnd)
{
	RECT rc;
	GetClientRect(hwnd, &rc);

	struct obs_video_info ovi;
	ovi.adapter = 0;
	ovi.base_width = rc.right;
	ovi.base_height = rc.bottom;
	ovi.fps_num = 30000;
	ovi.fps_den = 1001;
	ovi.graphics_module = DL_D3D11; //DL_OPENGL;
	ovi.output_format = VIDEO_FORMAT_RGBA;
	ovi.output_width = rc.right;
	ovi.output_height = rc.bottom;

	if (obs_reset_video(&ovi) != 0)
		throw "Couldn't initialize video";
}

void windows_msg_loop(condition_variable *cv, wc_data *data)
{
	windowx winx;
	auto hwnd = create_display_window(winx);
	reset_video_info(hwnd);
	auto ds = create_display(hwnd);

	data->hw = hwnd;

	display(ds, data);
	cv->notify_all();

	MSG msg;
	while (GetMessage(&msg, hwnd, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
};


