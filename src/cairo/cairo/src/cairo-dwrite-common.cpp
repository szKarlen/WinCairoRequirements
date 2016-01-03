#pragma once

#include "cairo-dwrite-common.h"
#include "cairo-dwrite-private.h"
#include "cairo-win32.h"

static std::unordered_map<cairo_font_face_handle_t*, void*>* _handles = new std::unordered_map<cairo_font_face_handle_t*, void*>();
static unsigned long _handleId;
static std::vector<void*>* _fonts = new std::vector<void*>();

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

	_fonts->push_back(nData);
	auto id = reinterpret_cast<cairo_font_face_handle_t*>(InterlockedIncrement(&_handleId));
	(*_handles)[id] = data;

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

	delete[] item;

	LeaveCriticalSection(&critSec);

	return CAIRO_STATUS_SUCCESS;
}
