#include <obs.h>
#include <atlbase.h>
#include <atlwin.h>
#include <atltypes.h>

#include <thread>
#include <mutex>
#include <condition_variable>

#include <string>

class windowx : public CWindowImpl<windowx, CWindow> {
public:
	BEGIN_MSG_MAP(windowx)
	END_MSG_MAP()
	windowx() {}
	~windowx() {}

protected:
private:
};

struct wc_data {
	obs_source_t *source;
	HWND hw;
};

void init();
void windows_msg_loop(std::condition_variable *cv, wc_data *data);
obs_source_t *create_source();
std::string select_source(obs_source_t *source);
void modify_source(obs_source_t *source, const char *window);
obs_scene_t *create_scene();
void add_source_to_scene(obs_source_t *s, obs_scene_t *sc);
void set_output_source(obs_source_t *source);
