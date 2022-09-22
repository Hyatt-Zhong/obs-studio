#include "audio-capture.h"
#include <filesystem>
#include <codecvt>
using namespace std;
namespace ns_audio_capture {

static void LoadAudioDevice(const char *name, int channel, obs_data_t *parent)
{
	obs_data_t *data = obs_data_get_obj(parent, name);
	if (!data)
		return;

	obs_source_t *source = obs_load_source(data);
	if (source) {
		obs_set_output_source(channel, source);

		const char *name = obs_source_get_name(source);
		//blog(LOG_INFO, "[Loaded global audio device]: '%s'", name);
		//obs_source_enum_filters(source, LogFilter, (void *)(intptr_t)1);
		obs_monitoring_type monitoring_type =
			obs_source_get_monitoring_type(source);
		if (monitoring_type != OBS_MONITORING_TYPE_NONE) {
			const char *type = (monitoring_type ==
					    OBS_MONITORING_TYPE_MONITOR_ONLY)
						   ? "monitor only"
						   : "monitor and output";

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

	auto x = obs_reset_audio(&ai);
}
const wstring audio_config = L"\\audio_config.json";

#define DESKTOP_AUDIO_1 ("DesktopAudioDevice1")
#define DESKTOP_AUDIO_2 ("DesktopAudioDevice2")
#define AUX_AUDIO_1 ("AuxAudioDevice1")
#define AUX_AUDIO_2 ("AuxAudioDevice2")
#define AUX_AUDIO_3 ("AuxAudioDevice3")
#define AUX_AUDIO_4 ("AuxAudioDevice4")

void load_audio()
{
	ResetAudio();

	wstring cfg = filesystem::current_path().c_str() + audio_config;
	wstring_convert<codecvt_utf8<wchar_t>> conv;

	auto file = conv.to_bytes(cfg);
	obs_data_t *data =
		obs_data_create_from_json_file_safe(file.c_str(), nullptr);

	LoadAudioDevice(DESKTOP_AUDIO_1, 1, data);
	LoadAudioDevice(DESKTOP_AUDIO_2, 2, data);
	LoadAudioDevice(AUX_AUDIO_1, 3, data);
	LoadAudioDevice(AUX_AUDIO_2, 4, data);
	LoadAudioDevice(AUX_AUDIO_3, 5, data);
	LoadAudioDevice(AUX_AUDIO_4, 6, data);
}
};
