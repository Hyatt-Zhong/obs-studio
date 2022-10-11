#include "audio-capture.h"
#include <filesystem>
#include <graphics.h>
#include <map>
#include <iostream>

using namespace std;
namespace ns_audio_capture {

wstring s2ws(const string &str)
{
	if (str.empty()) {
		return L"";
	}
	int nLen = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str.c_str(), -1,
				       NULL, 0);
	if (nLen == 0) {
		return L"";
	}
	wchar_t *pResult = new wchar_t[nLen];
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str.c_str(), -1, pResult,
			    nLen);
	wstring ret = pResult;
	delete[] pResult;
	return ret;
}

string ws2s(const wstring &str)
{
	if (str.empty()){
		return "";
	}
	int nLen = WideCharToMultiByte(CP_ACP, 0, str.c_str(), -1, NULL, 0,
				       NULL, NULL);
	if (nLen == 0) {
		return "";
	}
	char *pResult = new char[nLen];
	WideCharToMultiByte(CP_ACP, 0, str.c_str(), -1, pResult, nLen, NULL,
			    NULL);
	string ret = pResult;
	delete[] pResult;
	return ret;
}

static void LoadAudioDevice(const char *name, int channel, obs_data_t *parent)
{
	obs_data_t *data = obs_data_get_obj(parent, name);
	if (!data)
		return;

	obs_source_t *source = obs_load_source(data);
	if (source) {
		obs_set_output_source(channel, source);

		/*const char *name =*/ obs_source_get_name(source);
		//blog(LOG_INFO, "[Loaded global audio device]: '%s'", name);
		//obs_source_enum_filters(source, LogFilter, (void *)(intptr_t)1);
		obs_monitoring_type monitoring_type =
			obs_source_get_monitoring_type(source);
		if (monitoring_type != OBS_MONITORING_TYPE_NONE) {
			/*const char *type = (monitoring_type ==
					    OBS_MONITORING_TYPE_MONITOR_ONLY)
						   ? "monitor only"
						   : "monitor and output";*/

			//blog(LOG_INFO, "    - monitoring: %s", type);
		}
		obs_source_release(source);
	}

	obs_data_release(data);
}

static void ResetAudio()
{
	struct obs_audio_info ai;
	ai.samples_per_sec = 48000;

	const char *channelSetupStr = "STEREO";

	if (strcmp(channelSetupStr, "Mono") == 0)
		ai.speakers = SPEAKERS_MONO;
	else if (strcmp(channelSetupStr, "2.1") == 0)
		ai.speakers = SPEAKERS_2POINT1;
	else if (strcmp(channelSetupStr, "4.0") == 0)
		ai.speakers = SPEAKERS_4POINT0;
	else if (strcmp(channelSetupStr, "4.1") == 0)
		ai.speakers = SPEAKERS_4POINT1;
	else if (strcmp(channelSetupStr, "5.1") == 0)
		ai.speakers = SPEAKERS_5POINT1;
	else if (strcmp(channelSetupStr, "7.1") == 0)
		ai.speakers = SPEAKERS_7POINT1;
	else
		ai.speakers = SPEAKERS_STEREO;

	obs_reset_audio(&ai);
}
const wstring audio_config = L"\\audio_config.json";

#define DESKTOP_AUDIO_1 ("DesktopAudioDevice1")
#define DESKTOP_AUDIO_2 ("DesktopAudioDevice2")
#define AUX_AUDIO_1 ("AuxAudioDevice1")
#define AUX_AUDIO_2 ("AuxAudioDevice2")
#define AUX_AUDIO_3 ("AuxAudioDevice3")
#define AUX_AUDIO_4 ("AuxAudioDevice4")

#define WM_CREATE_VIEWER (WM_USER+0x100)
class audio_handle : public ns_windowx::Loop {
public:
	BOOL Init() {
		comm_wind_.Create(NULL, NULL, L"communicat_window", WS_CAPTION);
		comm_wind_.ShowWindow(SW_HIDE);
		return TRUE;
	}
	BOOL PreTranslateMessage(MSG* pMsg) {
		switch (pMsg->message) {
		case WM_CREATE_VIEWER: {
			auto viewer = new AudioViewer(800, 300, 2, 8, 4);
			*(AudioViewer **)pMsg->wParam = viewer;
			cv_.notify_all();
			return TRUE;
		}
		default:
			break;
		}
		return FALSE;
	}
	AudioViewer *CreateViewer()
	{
		AudioViewer *viewer = nullptr;
		PostMessage(comm_wind_.m_hWnd, WM_CREATE_VIEWER,
			    (WPARAM)&viewer, 0);
		unique_lock<mutex> lk(mtx_);
		cv_.wait(lk);
		return viewer;
	}
	void UnInit() {}
	BOOL OnIdle() {
		for (auto &itor : mp) {
			auto viewer = (AudioViewer *)itor.second;
			viewer->draw();
		}
		Push();
		return FALSE;
	}
	void handle(obs_source_t *source, void *data, int len)
	{
		auto it = mp.find(source);
		if (it != mp.end()) {
			auto viewer = (AudioViewer *)it->second;
			viewer->copy_data((const char *)data, len);
		} else {
			AudioViewer *viewer = CreateViewer();
			if (viewer) {
				mp.insert(make_pair(source, viewer));
				viewer->copy_data((const char *)data, len);
			}
		}
	}
	audio_handle() {}
	~audio_handle()
	{
		for (auto &itor : mp) {
			delete itor.second;
		}
	}

private:
	map<obs_source_t *, void *> mp;
	mutex mtx_;
	condition_variable cv_;
	ns_windowx::HideWindow comm_wind_;
};

//struct audio_handle 
//{
//	void handle(obs_source_t *source, void *data, int len)
//	{
//		auto it = mp.find(source);
//		if (it != mp.end()) {
//			auto viewer = (AudioViewer *)it->second;
//			viewer->copy_data((const char *)data, len);
//		} else {
//			if (!mp.empty()) {
//				return;
//			}
//			auto viewer = new AudioViewer(700, 300, 2, 8, 4);
//			mp.insert(make_pair(source, viewer));
//			viewer->run();
//			viewer->copy_data((const char *)data, len);
//		}
//	}
//	~audio_handle() {
//		for (auto &itor:mp)
//		{
//			delete itor.second;
//		}
//	}
//	map<obs_source_t *, void *> mp;
//};
static void data_handle_cbk(obs_source_t *source, void *data, int len,
			    void *param)
{
	auto hdl = (audio_handle *)param;
	hdl->handle(source, data, len);
}
static audio_handle *hdl = 0;
void audio_clear() {
	delete hdl;
}
void load_audio()
{
	hdl = new audio_handle;
	hdl->Run();
	obs_source_set_data_handle(data_handle_cbk, hdl);
	ResetAudio();

	wstring cfg = filesystem::current_path().c_str() + audio_config;

	auto file = ws2s(cfg);
	obs_data_t *data =
		obs_data_create_from_json_file_safe(file.c_str(), nullptr);

	LoadAudioDevice(DESKTOP_AUDIO_1, 1, data);
	LoadAudioDevice(DESKTOP_AUDIO_2, 2, data);
	LoadAudioDevice(AUX_AUDIO_1, 3, data);
	LoadAudioDevice(AUX_AUDIO_2, 4, data);
	LoadAudioDevice(AUX_AUDIO_3, 5, data);
	LoadAudioDevice(AUX_AUDIO_4, 6, data);
}
buffer::buffer(int len) : len_(len), point_(0)
{
	buf_ = new char[len];
	memset(buf_, 0, len);
}
buffer::~buffer()
{
	delete[] buf_;
}
shared_ptr<char> buffer::get_data(const int &len)
{
	if (point_ < len) {
		return nullptr;
	}
	shared_ptr<char> ret(new char[len]);
	memcpy((void *)ret.get(), buf_, len);
	memcpy(buf_, buf_ + len, point_ - len);
	point_ = point_ - len;
	return ret;
}
bool buffer::push_data(const char *data, const int &len)
{
	if (point_ + len < len_) {
		memcpy(buf_ + point_, data, len);
		point_ += len;
		return true;
	}
	if (point_>len_)
	{
		cout << point_ << endl;
	}
	return false;
}

AudioViewer::AudioViewer(const int &w, const int &h, const int &channel,
			 const int &frame_len, const int &bit_deep)
	: kW_(w),
	  kH_(h),
	  kH_half_(kH_ / 2),
	  channel_(channel),
	  frame_len_(frame_len),
	  bit_deep_(bit_deep),
	  bufs_(len_),
	  quit_(false)
{
	auto x = kH_ / (channel_ + 1);
	for (auto i = 0; i < channel_; i++) {
		ch[i] = kH_half_ - x * (i + 1);
	}

	init();
}
AudioViewer::~AudioViewer() {}

bool AudioViewer::copy_data(const char *data, int len)
{
	lock_guard<mutex> lk(mtx);
	return bufs_.push_data(data, len);
}

void AudioViewer::init()
{
	wind_.create(kW_, kH_);
	wind_.setorigin(0, kH_half_);
	wind_.setcolor(0xffffff);
}

void AudioViewer::draw()
{
	mtx.lock();
	auto data = bufs_.get_data(kW_ * frame_len_ *
				   catch_); //每几个声音帧画一个像素
	mtx.unlock();
	if (!data) {
		return;
	}
	wind_.clear();

	for (auto k = 0; k < channel_; k++) {
		auto p = (char *)(data.get()) + k * bit_deep_;
		wind_.moveto(0, ch[k]);
		for (auto i = 0; i < kW_ * catch_; i += catch_) {
			float h = 0;
			memcpy(&h, (const void *)(p + i * frame_len_),
			       bit_deep_);
			auto hn = h * scale_ + ch[k];
			wind_.lineto(i, (int)hn);
		}
	}

	wind_.refresh();
}

};
