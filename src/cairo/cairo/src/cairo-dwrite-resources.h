// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//
//----------------------------------------------------------------------------

#pragma once

#include <Windows.h>
#include "cairo-resources-common.h"

class CairoDWriteFontContext
{
public:
	CairoDWriteFontContext();
	~CairoDWriteFontContext();

	HRESULT Initialize();

	HRESULT CreateFontCollection(
		UINT const* fontCollectionKey,
		UINT32 keySize,
		OUT IDWriteFontCollection** result
		);

private:
	CairoDWriteFontContext(CairoDWriteFontContext const&);
	void operator=(CairoDWriteFontContext const&);

	HRESULT InitializeInternal();

	HRESULT hr_;
};

class CairoDWriteFontCollectionLoader : public IDWriteFontCollectionLoader
{
public:
	CairoDWriteFontCollectionLoader() : refCount_(0)
	{
	}

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** ppvObject);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();

	virtual HRESULT STDMETHODCALLTYPE CreateEnumeratorFromKey(
		IDWriteFactory* factory,
		void const* collectionKey,
		UINT32 collectionKeySize,
		OUT IDWriteFontFileEnumerator** fontFileEnumerator
		);

	static IDWriteFontCollectionLoader* GetLoader()
	{
		return instance_;
	}

	static bool IsLoaderInitialized()
	{
		return instance_ != NULL;
	}

private:
	ULONG refCount_;

	static IDWriteFontCollectionLoader* instance_;
};

class CairoDWriteFontFileEnumerator : public IDWriteFontFileEnumerator
{
public:
	CairoDWriteFontFileEnumerator(
		IDWriteFactory* factory
		);

	HRESULT Initialize(
		UINT const* resourceIDs,    // [resourceCount]
		UINT32 resourceCount
		);

	~CairoDWriteFontFileEnumerator()
	{
		SafeRelease(&currentFile_);
		SafeRelease(&factory_);
	}

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** ppvObject);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();

	virtual HRESULT STDMETHODCALLTYPE MoveNext(OUT BOOL* hasCurrentFile);
	virtual HRESULT STDMETHODCALLTYPE GetCurrentFontFile(OUT IDWriteFontFile** fontFile);

private:
	ULONG refCount_;

	IDWriteFactory* factory_;
	IDWriteFontFile* currentFile_;
	std::vector<cairo_font_data> resourceIDs_;
	size_t nextIndex_;
};

class CairoDWriteFontFileLoader : public IDWriteFontFileLoader
{
public:
	CairoDWriteFontFileLoader() : refCount_(0)
	{
	}

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** ppvObject);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();

	virtual HRESULT STDMETHODCALLTYPE CreateStreamFromKey(
		void const* fontFileReferenceKey,       // [fontFileReferenceKeySize] in bytes
		UINT32 fontFileReferenceKeySize,
		OUT IDWriteFontFileStream** fontFileStream
		);

	static IDWriteFontFileLoader* GetLoader()
	{
		return instance_;
	}

	static bool IsLoaderInitialized()
	{
		return instance_ != NULL;
	}

private:
	ULONG refCount_;

	static IDWriteFontFileLoader* instance_;
};

class CairoDWriteFontFileStream : public IDWriteFontFileStream
{
public:
	explicit CairoDWriteFontFileStream(const cairo_font_data resourceData);

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** ppvObject);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();

	virtual HRESULT STDMETHODCALLTYPE ReadFileFragment(
		void const** fragmentStart, // [fragmentSize] in bytes
		UINT64 fileOffset,
		UINT64 fragmentSize,
		OUT void** fragmentContext
		);

	virtual void STDMETHODCALLTYPE ReleaseFileFragment(
		void* fragmentContext
		);

	virtual HRESULT STDMETHODCALLTYPE GetFileSize(
		OUT UINT64* fileSize
		);

	virtual HRESULT STDMETHODCALLTYPE GetLastWriteTime(
		OUT UINT64* lastWriteTime
		);

	bool IsInitialized()
	{
		return resourcePtr_ != NULL;
	}

private:
	ULONG refCount_;
	void const* resourcePtr_;       // [resourceSize_] in bytes
	DWORD resourceSize_;

	static HMODULE const moduleHandle_;
	static HMODULE GetCurrentModule();
};
