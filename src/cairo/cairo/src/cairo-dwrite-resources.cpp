// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//
//----------------------------------------------------------------------------

#pragma once

#include "cairo-dwrite-resources.h"
#include "cairo-dwrite-private.h"
#include <vector>
#include "cairo-dwrite-common.h"

CairoDWriteFontContext::CairoDWriteFontContext() : hr_(S_FALSE)
{
}

CairoDWriteFontContext::~CairoDWriteFontContext()
{
	DWriteFactory::Instance()->UnregisterFontCollectionLoader(CairoDWriteFontCollectionLoader::GetLoader());
	DWriteFactory::Instance()->UnregisterFontFileLoader(CairoDWriteFontFileLoader::GetLoader());
}

HRESULT CairoDWriteFontContext::Initialize()
{
	if (hr_ == S_FALSE)
	{
		hr_ = InitializeInternal();
	}
	return hr_;
}

HRESULT CairoDWriteFontContext::InitializeInternal()
{
	HRESULT hr = S_OK;

	if (!CairoDWriteFontFileLoader::IsLoaderInitialized()
		|| !CairoDWriteFontCollectionLoader::IsLoaderInitialized())
	{
		return E_FAIL;
	}

	if (FAILED(hr = DWriteFactory::Instance()->RegisterFontFileLoader(CairoDWriteFontFileLoader::GetLoader())))
		return hr;

	hr = DWriteFactory::Instance()->RegisterFontCollectionLoader(CairoDWriteFontCollectionLoader::GetLoader());

	return hr;
}

HRESULT CairoDWriteFontContext::CreateFontCollection(
	UINT const* fontCollectionKey, // [keySize] in bytes
	UINT32 keySize,
	OUT IDWriteFontCollection** result
	)
{
	*result = NULL;

	HRESULT hr = S_OK;

	hr = Initialize();
	if (FAILED(hr))
		return hr;

	hr = DWriteFactory::Instance()->CreateCustomFontCollection(
		CairoDWriteFontCollectionLoader::GetLoader(),
		fontCollectionKey,
		keySize,
		result
		);

	return hr;
}

IDWriteFontCollectionLoader* CairoDWriteFontCollectionLoader::instance_(
	new(std::nothrow) CairoDWriteFontCollectionLoader()
	);

HRESULT STDMETHODCALLTYPE CairoDWriteFontCollectionLoader::QueryInterface(REFIID iid, void** ppvObject)
{
	if (iid == IID_IUnknown || iid == __uuidof(IDWriteFontCollectionLoader))
	{
		*ppvObject = this;
		AddRef();
		return S_OK;
	}
	else
	{
		*ppvObject = NULL;
		return E_NOINTERFACE;
	}
}

ULONG STDMETHODCALLTYPE CairoDWriteFontCollectionLoader::AddRef()
{
	return InterlockedIncrement(&refCount_);
}

ULONG STDMETHODCALLTYPE CairoDWriteFontCollectionLoader::Release()
{
	ULONG newCount = InterlockedDecrement(&refCount_);
	if (newCount == 0)
		delete this;

	return newCount;
}

HRESULT STDMETHODCALLTYPE CairoDWriteFontCollectionLoader::CreateEnumeratorFromKey(
	IDWriteFactory* factory,
	void const* collectionKey, // [collectionKeySize] in bytes
	UINT32 collectionKeySize,
	OUT IDWriteFontFileEnumerator** fontFileEnumerator
	)
{
	*fontFileEnumerator = NULL;

	HRESULT hr = S_OK;

	if (collectionKeySize % sizeof(UINT) != 0)
		return E_INVALIDARG;

	CairoDWriteFontFileEnumerator* enumerator = new(std::nothrow) CairoDWriteFontFileEnumerator(
		factory
		);
	if (enumerator == NULL)
		return E_OUTOFMEMORY;

	auto vec = reinterpret_cast<std::vector<cairo_font_data>*>((void*)collectionKey);
	UINT32 const resourceCount = vec->size();

	hr = enumerator->Initialize(
		static_cast<UINT const*>(collectionKey),
		resourceCount
		);

	if (FAILED(hr))
	{
		delete enumerator;
		return hr;
	}

	*fontFileEnumerator = SafeAcquire(enumerator);

	return hr;
}

CairoDWriteFontFileEnumerator::CairoDWriteFontFileEnumerator(
	IDWriteFactory* factory
	) :
	refCount_(0),
	factory_(SafeAcquire(factory)),
	currentFile_(),
	nextIndex_(0)
{
}

HRESULT CairoDWriteFontFileEnumerator::Initialize(
	UINT const* resourceIDs, // [resourceCount]
	UINT32 resourceCount
	)
{
	try
	{
		auto vec = reinterpret_cast<std::vector<cairo_font_data>*>((void*)resourceIDs);
		resourceIDs_.swap(*vec);
		//resourceIDs_.assign(resourceIDs, resourceIDs + resourceCount);
	}
	catch (...)
	{
		return ExceptionToHResult();
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CairoDWriteFontFileEnumerator::QueryInterface(REFIID iid, OUT void** ppvObject)
{
	if (iid == IID_IUnknown || iid == __uuidof(IDWriteFontFileEnumerator))
	{
		*ppvObject = this;
		AddRef();
		return S_OK;
	}
	else
	{
		*ppvObject = NULL;
		return E_NOINTERFACE;
	}
}

ULONG STDMETHODCALLTYPE CairoDWriteFontFileEnumerator::AddRef()
{
	return InterlockedIncrement(&refCount_);
}

ULONG STDMETHODCALLTYPE CairoDWriteFontFileEnumerator::Release()
{
	ULONG newCount = InterlockedDecrement(&refCount_);
	if (newCount == 0)
		delete this;

	return newCount;
}

HRESULT STDMETHODCALLTYPE CairoDWriteFontFileEnumerator::MoveNext(OUT BOOL* hasCurrentFile)
{
	HRESULT hr = S_OK;

	*hasCurrentFile = FALSE;
	SafeRelease(&currentFile_);

	if (nextIndex_ < resourceIDs_.size())
	{
		cairo_font_data data = resourceIDs_[nextIndex_];
		hr = factory_->CreateCustomFontFileReference(
			&data,
			sizeof(cairo_font_data),
			CairoDWriteFontFileLoader::GetLoader(),
			&currentFile_
			);

		if (SUCCEEDED(hr))
		{
			*hasCurrentFile = TRUE;

			++nextIndex_;
		}
	}

	return hr;
}

HRESULT STDMETHODCALLTYPE CairoDWriteFontFileEnumerator::GetCurrentFontFile(OUT IDWriteFontFile** fontFile)
{
	*fontFile = SafeAcquire(currentFile_);

	return (currentFile_ != NULL) ? S_OK : E_FAIL;
}

IDWriteFontFileLoader* CairoDWriteFontFileLoader::instance_(
	new(std::nothrow) CairoDWriteFontFileLoader()
	);

HRESULT STDMETHODCALLTYPE CairoDWriteFontFileLoader::QueryInterface(REFIID iid, void** ppvObject)
{
	if (iid == IID_IUnknown || iid == __uuidof(IDWriteFontFileLoader))
	{
		*ppvObject = this;
		AddRef();
		return S_OK;
	}
	else
	{
		*ppvObject = NULL;
		return E_NOINTERFACE;
	}
}

// AddRef
ULONG STDMETHODCALLTYPE CairoDWriteFontFileLoader::AddRef()
{
	return InterlockedIncrement(&refCount_);
}

// Release
ULONG STDMETHODCALLTYPE CairoDWriteFontFileLoader::Release()
{
	ULONG newCount = InterlockedDecrement(&refCount_);
	if (newCount == 0)
		delete this;

	return newCount;
}

HRESULT STDMETHODCALLTYPE CairoDWriteFontFileLoader::CreateStreamFromKey(
	void const* fontFileReferenceKey,       // [fontFileReferenceKeySize] in bytes
	UINT32 fontFileReferenceKeySize,
	OUT IDWriteFontFileStream** fontFileStream
	)
{
	*fontFileStream = NULL;

	if (fontFileReferenceKeySize != sizeof(cairo_font_data))
		return E_INVALIDARG;

	CairoDWriteFontFileStream* stream = new(std::nothrow) CairoDWriteFontFileStream(*static_cast<cairo_font_data*>((void*)fontFileReferenceKey));
	if (stream == NULL)
		return E_OUTOFMEMORY;

	if (!stream->IsInitialized())
	{
		delete stream;
		return E_FAIL;
	}

	*fontFileStream = SafeAcquire(stream);

	return S_OK;
}

CairoDWriteFontFileStream::CairoDWriteFontFileStream(const cairo_font_data& resourceData) :
refCount_(0),
resourcePtr_(resourceData.data),
resourceSize_(resourceData.size)
{

}

// IUnknown methods
HRESULT STDMETHODCALLTYPE CairoDWriteFontFileStream::QueryInterface(REFIID iid, void** ppvObject)
{
	if (iid == IID_IUnknown || iid == __uuidof(IDWriteFontFileStream))
	{
		*ppvObject = this;
		AddRef();
		return S_OK;
	}
	else
	{
		*ppvObject = NULL;
		return E_NOINTERFACE;
	}
}

ULONG STDMETHODCALLTYPE CairoDWriteFontFileStream::AddRef()
{
	return InterlockedIncrement(&refCount_);
}

ULONG STDMETHODCALLTYPE CairoDWriteFontFileStream::Release()
{
	ULONG newCount = InterlockedDecrement(&refCount_);
	if (newCount == 0)
		delete this;

	return newCount;
}

HRESULT STDMETHODCALLTYPE CairoDWriteFontFileStream::ReadFileFragment(
	void const** fragmentStart, // [fragmentSize] in bytes
	UINT64 fileOffset,
	UINT64 fragmentSize,
	OUT void** fragmentContext
	)
{
	// bounds check.
	if (fileOffset <= resourceSize_ &&
		fragmentSize <= resourceSize_ - fileOffset)
	{
		*fragmentStart = static_cast<BYTE const*>(resourcePtr_)+static_cast<size_t>(fileOffset);
		*fragmentContext = NULL;
		return S_OK;
	}
	else
	{
		*fragmentStart = NULL;
		*fragmentContext = NULL;
		return E_FAIL;
	}
}

void STDMETHODCALLTYPE CairoDWriteFontFileStream::ReleaseFileFragment(
	void* fragmentContext
	)
{
}

HRESULT STDMETHODCALLTYPE CairoDWriteFontFileStream::GetFileSize(
	OUT UINT64* fileSize
	)
{
	*fileSize = resourceSize_;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CairoDWriteFontFileStream::GetLastWriteTime(
	OUT UINT64* lastWriteTime
	)
{
	*lastWriteTime = 0;
	return E_NOTIMPL;
}
