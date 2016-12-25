#define UNICODE 1

#include <stdio.h>
#include <wchar.h>
#include <windows.h>

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#define LOG(...) fprintf(stderr, __VA_ARGS__)
#define LOGW(...) fwprintf(stderr, __VA_ARGS__)

struct myy_implementation_specifics {
	HINSTANCE hInstance;
	int nCmdShow;
};

struct _escontext {
	/// Native System informations
	EGLNativeDisplayType native_display;
	EGLNativeWindowType native_window;
	uint16_t window_width, window_height;
	/// EGL display
	EGLDisplay  display;
	/// EGL context
	EGLContext  context;
	/// EGL surface
	EGLSurface  surface;

};

struct _escontext ESContext = {
	.native_display = NULL,
	.window_width = 0,
	.window_height = 0,
	.native_window  = 0,
	.display = NULL,
	.context = NULL,
	.surface = NULL
};

// Step 4: the Window Procedure
LRESULT CALLBACK WndProc
(HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam)	{
	wchar_t msg[32];
	switch(umsg)	{
	case WM_ACTIVATE:
	case WM_SETFOCUS:
		  return 0;
	case WM_INPUT: {
		UINT dwSize = 64;
		BYTE lpb[64];
		if (GetRawInputData((HRAWINPUT) lParam, RID_INPUT, lpb, &dwSize,
		                    sizeof(RAWINPUTHEADER)) <= dwSize) {
			RAWINPUT *raw = (RAWINPUT *) lpb;
			if (raw->header.dwType == RIM_TYPEKEYBOARD &&
			    (raw->data.keyboard.Flags & RI_KEY_BREAK))
				LOGW(L"Keycode : %04x\n", raw->data.keyboard.MakeCode);
		}
		else LOGW(L"%s", L"Haaan !");
		break;
	}
	case WM_SIZE:
		  LOGW(L"w: %d, h: %d\n", LOWORD(lParam), HIWORD(lParam));
		  return 0;

	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_CREATE: {
		RAWINPUTDEVICE Rid[1];
		Rid[0].usUsagePage = 0x01;
		Rid[0].usUsage     = 0x06;
		Rid[0].dwFlags     = RIDEV_NOLEGACY;
		Rid[0].hwndTarget  = hwnd;
		RegisterRawInputDevices(Rid, 1, sizeof(Rid[0]));
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
		
	}
	return DefWindowProc(hwnd, umsg, wParam, lParam);
}

static int InitialiseEGL(EGLNativeWindowType native_window) {
	EGLint numConfigs;
	EGLint majorVersion;
	EGLint minorVersion;
	EGLContext context;
	EGLSurface surface;
	EGLConfig config;
	EGLint fbAttribs[] =  {
	  EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	  EGL_CONFORMANT, EGL_OPENGL_ES3_BIT,
	  EGL_SAMPLES,         4,
	  EGL_RED_SIZE,        8,
	  EGL_GREEN_SIZE,      8,
	  EGL_BLUE_SIZE,       8,
	  EGL_ALPHA_SIZE,      8,
	  EGL_DEPTH_SIZE,     16,
	  EGL_NONE
	};
	/* The system can clearly provide you a OpenGL ES x.y compliant
		 configuration without OpenGL ES x.y enabled ! */
	EGLint contextAttribs[] =
	  { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE, EGL_NONE };

	EGLNativeDisplayType native_display = (EGLNativeDisplayType) NULL;
	LOG("[InitialiseEGL]\n");
	LOG("  Display...\n");
	EGLDisplay display = eglGetDisplay( native_display );
	if ( display == EGL_NO_DISPLAY ) {
		LOG("No Display :C\n");
		return EGL_FALSE;
	}

	LOG("  Init...\n");
	// Initialize EGL
	if ( !eglInitialize(display, &majorVersion, &minorVersion) ) {
		LOG("Could not Initailize EGL for unknown reasons...\n");
		return EGL_FALSE;
	}

	LOG(" Getting configurations\n");
	// Get configs
	if (   (eglGetConfigs(display, NULL, 0, &numConfigs) != EGL_TRUE)
	    || (numConfigs == 0)) {
		LOG("Could not get available EGL configurations...\n");
		return EGL_FALSE;
	}

	LOG(" Choosing configurations\n");
	// Choose config
	if ( (eglChooseConfig(display, fbAttribs,
	                      &config, 1, &numConfigs) != EGL_TRUE)
	    || (numConfigs != 1)) {
		LOG("Learn to select an EGL config...\n");
		return EGL_FALSE;
	}

	LOG(" Creating a surface...\n");
	// Create a surface
	surface =
	  eglCreateWindowSurface(display, config,
	                         native_window, NULL);

	if ( surface == EGL_NO_SURFACE ) {
		LOG("No Surface ! No Surface ! Hands Up ! Hands Up !\n");
		return EGL_FALSE;
	}

	LOG(" Creating a context...\n");
	// Create a GL context
	context = eglCreateContext(display, config,
	                           EGL_NO_CONTEXT, contextAttribs );

	if ( context == EGL_NO_CONTEXT ) {
		LOG("An EGL configuration with no context...\n");
		return EGL_FALSE;
	}

	// Make the context current
	if ( !eglMakeCurrent(display, surface, surface, context) ) {
		LOG("eglMakeCurrent failed miserably !\n");
		return EGL_FALSE;
	}

	ESContext.display = display;
	ESContext.surface = surface;
	ESContext.context = context;
	return EGL_TRUE;
}

HWND myy_CreateWindow
(unsigned int const width, unsigned int const height,
 wchar_t const * const title,
 struct myy_implementation_specifics const impl) {
	HWND hwnd;
	WNDCLASSEX wc;

	const wchar_t g_szClassName[] = L"myWindowClass";
	// Step 1: Registering the Window Class
	wc.cbSize          = sizeof(WNDCLASSEX);
	wc.style           = 0;
	wc.lpfnWndProc     = WndProc;
	wc.cbClsExtra      = 0;
	wc.cbWndExtra      = 0;
	wc.hInstance       = impl.hInstance;
	wc.hIcon           = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor         = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground   = (HBRUSH)(COLOR_WINDOW);
	wc.lpszMenuName    = NULL;
	wc.lpszClassName   = g_szClassName;
	wc.hIconSm         = LoadIcon(NULL, IDI_APPLICATION);

	if(!RegisterClassEx(&wc))	{
		MessageBox(NULL, L"Window Registration Failed !?", L"Critical Error !",
		           MB_ICONEXCLAMATION | MB_OK);
		ExitProcess(1);
	}

	// Step 2: Creating the Window
	hwnd = CreateWindowEx(WS_EX_CLIENTEDGE,
	                      g_szClassName,
	                      title,
	                      WS_OVERLAPPEDWINDOW,
	                      CW_USEDEFAULT, CW_USEDEFAULT, width, height,
	                      NULL, NULL, impl.hInstance, NULL);

	if(hwnd == NULL) {
		MessageBox(NULL, L"Window Creation Failed !?", L"Critical Error !",
		           MB_ICONEXCLAMATION | MB_OK);
		ExitProcess(1);
	}

	ShowWindow(hwnd, impl.nCmdShow);
	UpdateWindow(hwnd);

	InitialiseEGL(hwnd);
	ESContext.native_window = hwnd;
	return hwnd;
}

int WINAPI WinMain
(HINSTANCE hInstance, HINSTANCE hPrevInstance,
 LPSTR lpCmdLine, int nCmdShow) {
	MSG Msg;
	
	struct myy_implementation_specifics const impl = {
		.hInstance = hInstance,
		.nCmdShow  = nCmdShow
	};

	RECT rcClient;
	GetClientRect(myy_CreateWindow(1280, 720, L"ニャンバンダ", impl), &rcClient);

	LOGW(L"Client Width : %d, Height : %d\n", rcClient.right, rcClient.bottom);

	// Step 3: The Message Loop
	while(GetMessage(&Msg, NULL, 0, 0) > 0) {
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	return Msg.wParam;
}
