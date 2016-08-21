#ifdef USE_PLATFORM_XCB_KHR
#pragma once
#include <xcb/xcb.h>
#include <dlfcn.h>
#include <cstdlib>

namespace HelloEngine
{
	struct WindowParameters {
		xcb_connection_t		 *connection;
		xcb_window_t			  handle;
		xcb_intern_atom_reply_t  *deleteReply;

		WindowParameters() :
			connection(),
			handle() {}
	};

	struct WindowEventParameters {
		
	};
}
#endif