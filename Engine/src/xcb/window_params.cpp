#ifdef USE_PLATFORM_XCB_KHR
#include "window.h"
#include "window_params.h"
#include <string.h>

namespace HelloEngine
{
	Window::~Window() {
		xcb_destroy_window(m_Parameters->connection, m_Parameters->handle);
		xcb_disconnect(m_Parameters->connection);
		delete m_Parameters->deleteReply;
	}

	bool Window::Create(const char *title) {
		int screen_index;
		m_Parameters->connection = xcb_connect(nullptr, &screen_index);

		if (!m_Parameters->connection) {
			return false;
		}

		const xcb_setup_t *setup = xcb_get_setup(m_Parameters->connection);
		xcb_screen_iterator_t screen_iterator = xcb_setup_roots_iterator(setup);

		while (screen_index-- > 0) {
			xcb_screen_next(&screen_iterator);
		}

		xcb_screen_t *screen = screen_iterator.data;

		m_Parameters->handle = xcb_generate_id(m_Parameters->connection);

		uint32_t value_list[] = {
			screen->white_pixel,
			XCB_EVENT_MASK_KEY_RELEASE |
			XCB_EVENT_MASK_KEY_PRESS |
			XCB_EVENT_MASK_EXPOSURE |
			XCB_EVENT_MASK_STRUCTURE_NOTIFY |
			XCB_EVENT_MASK_POINTER_MOTION |
			XCB_EVENT_MASK_BUTTON_PRESS |
			XCB_EVENT_MASK_BUTTON_RELEASE
		};

		xcb_create_window(
			m_Parameters->connection,
			XCB_COPY_FROM_PARENT,
			m_Parameters->handle,
			screen->root,
			20,
			20,
			500,
			500,
			0,
			XCB_WINDOW_CLASS_INPUT_OUTPUT,
			screen->root_visual,
			XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
			value_list);

		xcb_change_property(
			m_Parameters->connection,
			XCB_PROP_MODE_REPLACE,
			m_Parameters->handle,
			XCB_ATOM_WM_NAME,
			XCB_ATOM_STRING,
			8,
			strlen(title),
			title);
		xcb_flush(m_Parameters->connection);
		xcb_intern_atom_cookie_t  protocols_cookie = xcb_intern_atom(m_Parameters->connection, 1, 12, "WM_PROTOCOLS");
		xcb_intern_atom_reply_t  *protocols_reply = xcb_intern_atom_reply(m_Parameters->connection, protocols_cookie, 0);
		xcb_intern_atom_cookie_t  delete_cookie = xcb_intern_atom(m_Parameters->connection, 0, 16, "WM_DELETE_WINDOW");
		m_Parameters->deleteReply = xcb_intern_atom_reply(m_Parameters->connection, delete_cookie, 0);
		xcb_change_property(m_Parameters->connection, XCB_PROP_MODE_REPLACE, m_Parameters->handle, (*protocols_reply).atom, 4, 32, 1, &(m_Parameters->deleteReply->atom));
		free(protocols_reply);
		xcb_map_window(m_Parameters->connection, m_Parameters->handle);
		xcb_flush(m_Parameters->connection);
		return true;
	}

	bool Window::ProcessInput() {
		xcb_generic_event_t *event = xcb_poll_for_event(m_Parameters->connection);
		if (event) {			
			switch (event->response_type & 0x7f) {		
			case XCB_CONFIGURE_NOTIFY: {
					xcb_configure_notify_event_t *configure_event = (xcb_configure_notify_event_t*)event;
					static uint16_t width = configure_event->width;
					static uint16_t height = configure_event->height;

					if (((configure_event->width > 0) && (width != configure_event->width)) ||
						((configure_event->height > 0) && (height != configure_event->height))) {
						m_ResizeRequested = true;
						width = configure_event->width;
						height = configure_event->height;
					}
				}
				break;			
			case XCB_CLIENT_MESSAGE:
				if ((*(xcb_client_message_event_t*)event).data.data32[0] == m_Parameters->deleteReply->atom) {
					m_CloseRequested = true;
					free(m_Parameters->deleteReply);
				}
				break;
			case XCB_BUTTON_PRESS:
			{
				xcb_button_press_event_t *press = (xcb_button_press_event_t *)event;
				for (auto eventHandler : m_EventHandlers) {
					if (press->detail == XCB_BUTTON_INDEX_1)
						eventHandler->OnLButtonDown(press->event_x, press->event_y);
					if (press->detail == XCB_BUTTON_INDEX_3)
						eventHandler->OnRButtonDown(press->event_x, press->event_y);
				}					
			}
			break;
			case XCB_BUTTON_RELEASE:
			{
				xcb_button_press_event_t *press = (xcb_button_press_event_t *)event;
				for (auto eventHandler : m_EventHandlers) {
					if (press->detail == XCB_BUTTON_INDEX_1)
						eventHandler->OnLButtonUp(press->event_x, press->event_y);
					if (press->detail == XCB_BUTTON_INDEX_3)
						eventHandler->OnRButtonUp(press->event_x, press->event_y);
				}				
			}
			break;			
			}
			free(event);
			return true;
		}
		return false;
	}	

	void Window::HandleEvent(WindowEventParameters message)
	{
		
	}
}
#endif
