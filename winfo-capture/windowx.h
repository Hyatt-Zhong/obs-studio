#include <atlbase.h>
#include <atlapp.h>
#include <atltypes.h>

#include <thread>
#include <mutex>
#include <condition_variable>

#define WM_LOOP (WM_USER + 1)
namespace ns_windowx {

using std::thread;
using std::mutex;
using std::unique_lock;
using std::condition_variable;

class HideWindow : public CWindowImpl<HideWindow, CWindow> {
public:
	BEGIN_MSG_MAP(HideWindow)
	END_MSG_MAP()
	HideWindow()  {}

	~HideWindow() {}
};
class Loop : public CMessageLoop, public CIdleHandler {
public:
	Loop() {}
	virtual ~Loop() { PostQuitMessage(0); }

	void Run()
	{
		thread thrd(&Loop::Start, this);
		thrd.detach();

		unique_lock<mutex> lk(mtx_);
		cv_.wait(lk);
	}

	void Push() { PostMessage(NULL, WM_LOOP, 0, 0); }

	virtual BOOL Init() = 0;
	virtual void UnInit() = 0;

private:
	void Start()
	{
		AddIdleHandler(this);
		if (!Init()) {
			return;
		}
		cv_.notify_all();
		CMessageLoop::Run();
		UnInit();
	}

private:
	condition_variable cv_;
	mutex mtx_;
};

//非Windows坐标系的，数学上常规的坐标系的窗口
class windowx : public CWindowImpl<windowx, CWindow> {
public:
	BEGIN_MSG_MAP(windowx)
	END_MSG_MAP()
	windowx() : w_(0), h_(0), ox_(0), oy_(0)
	{
	}

	~windowx()
	{
		ReleaseDC(dc_);
		DeleteDC(dc_mem_);
		DeleteObject(pen_);
		DeleteObject(bmp_);
		DeleteObject(black_);
	}
	
	void create(const int &w, const int &h)
	{
		w_ = w;
		h_ = h;
		CRect rt(0, 0, w_, h_);
		AdjustWindowRectEx(&rt, WS_CAPTION | WS_VISIBLE, FALSE, 0);
		Create(NULL, rt, L"windowx", WS_CAPTION | WS_VISIBLE);
		CenterWindow();
		ShowWindow(SW_NORMAL);

		dc_ = GetDC();
		dc_mem_ = CreateCompatibleDC(dc_);
		bmp_ = CreateCompatibleBitmap(dc_mem_, w_, h_);
		auto old = SelectObject(dc_mem_, bmp_);
		DeleteObject(old);

		black_ = CreateSolidBrush(0);
	}

	void setcolor(COLORREF color)
	{
		pen_ = CreatePen(PS_SOLID, 1, color);

		auto old = SelectObject(dc_mem_, pen_);
		DeleteObject(old);
	}

	void moveto(const int &x, const int &y)
	{
		auto a = x, b = y;
		offset(a, b);
		MoveToEx(dc_mem_, a, b, nullptr);
	}

	void lineto(const int &x, const int &y)
	{
		auto a = x, b = y;
		offset(a, b);
		LineTo(dc_mem_, a, b);
	}

	//原始Windows窗口坐标系下的点作为原点，比如0，0是左上角的点作为原点
	void setorigin(const int &x, const int &y)
	{
		ox_ = x;
		oy_ = y;
	}

	void refresh() { BitBlt(dc_, 0, 0, w_, h_, dc_mem_, 0, 0, SRCCOPY); }

	void clear() { FillRect(dc_mem_, CRect(0, 0, w_, h_), black_); }

private:
	void offset(int &x, int &y)
	{
		x = x + ox_;
		y = oy_ - y;
	}

private:
	int w_, h_;
	int ox_, oy_;
	HDC dc_mem_;
	HDC dc_;
	HBITMAP bmp_;
	HPEN pen_;
	HBRUSH black_;
};
}
