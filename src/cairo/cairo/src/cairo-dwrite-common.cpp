#pragma once

#include "cairo-dwrite-common.h"
#include "cairo-dwrite-private.h"
#include "cairo-win32.h"

static std::unordered_map<cairo_font_face_handle_t*, cairo_font_data>* _handles = new std::unordered_map<cairo_font_face_handle_t*, cairo_font_data>();
static unsigned long _handleId;
static std::vector<cairo_font_data>* _fonts = new std::vector<cairo_font_data>();

UINT* getFontsList() {
	return reinterpret_cast<UINT*>(_fonts);
}

static CRITICAL_SECTION critSec;
static const bool intialized = [] {
	InitializeCriticalSection(&critSec);
	return true;
}();

void cairo_begin_custom_fonts_update() {
	if (!intialized)
		return;

	EnterCriticalSection(&critSec);
}

void cairo_end_custom_fonts_update() {
	if (!intialized)
		return;

	LeaveCriticalSection(&critSec);
}

cairo_font_face_handle_t*
cairo_dwrite_register_font_face(_In_reads_bytes_(cjSize) PVOID data, _In_ DWORD cjSize)
{
	EnterCriticalSection(&critSec);

	char* nData = new char[cjSize];
	memcpy(nData, data, cjSize);

	cairo_font_data holder = { nData, cjSize };
	_fonts->push_back(holder);
	auto id = reinterpret_cast<cairo_font_face_handle_t*>(InterlockedIncrement(&_handleId));
	(*_handles)[id] = holder;

	LeaveCriticalSection(&critSec);

	return (id);
}

cairo_status_t
cairo_dwrite_unregister_font_face(cairo_font_face_handle_t* handle)
{
	EnterCriticalSection(&critSec);

	auto it = _handles->find(handle);
	if (it == _handles->end()) {
		return CAIRO_STATUS_INVALID_FORMAT;
	}
	auto item = it->second;
	_fonts->erase(std::remove(_fonts->begin(), _fonts->end(), item), _fonts->end());
	_handles->erase(it);

	delete[] item.data;

	LeaveCriticalSection(&critSec);

	return CAIRO_STATUS_SUCCESS;
}
