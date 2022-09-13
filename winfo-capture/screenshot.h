#include <obs.h>
#include <string>

class ScreenshotObj  {
	

public:
	ScreenshotObj(obs_source_t *source, const std::wstring &save_path);
	~ScreenshotObj() ;
	void Screenshot();
	void Download();
	void Copy();
	void MuxAndFinish();

	gs_texrender_t *texrender = nullptr;
	gs_stagesurf_t *stagesurf = nullptr;

	uint32_t cx;
	uint32_t cy;

	int stage = 0;
	obs_source_t *source_;
	std::wstring save_path_;
};
