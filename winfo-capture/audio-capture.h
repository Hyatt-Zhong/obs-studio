#include <obs.h>
#include "windowx.h"
#include <memory>

using std::shared_ptr;
using std::make_shared;
using std::mutex;

#include <string>

namespace ns_audio_capture {
struct buffer {
	buffer(int len);
	~buffer();
	shared_ptr<char> get_data(const int &len);
	bool push_data(const char *data, const int &len);
	char *buf_;
	int len_;
	int point_;
};
class AudioViewer {
public:
	AudioViewer(const int &w, const int &h, const int &channel,
		    const int &frame_len, const int &bit_deep);
	~AudioViewer();

	bool copy_data(const char *data, int len);
	void draw();
protected:
	void init();
private:
	int kW_, kH_;
	int kH_half_;
	const int len_ = 50 * 1024;
	buffer bufs_;
	int channel_;
	int frame_len_;
	int ch[10];
	int bit_deep_;
	const int scale_ = 300;
	mutex mtx;
	bool quit_;
	const int catch_ = 4; //隔多少帧绘制一次
	ns_windowx::windowx wind_;
};
void load_audio();
void audio_clear();
};
