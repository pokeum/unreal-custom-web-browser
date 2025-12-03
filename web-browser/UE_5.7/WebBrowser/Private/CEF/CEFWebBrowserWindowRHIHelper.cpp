// Copyright Epic Games, Inc. All Rights Reserved.

#include "CEFWebBrowserWindowRHIHelper.h"
#include "WebBrowserLog.h"

#if WITH_CEF3

#include "CEF/CEFWebBrowserWindow.h"
#if WITH_ENGINE
#include "RHI.h"
#if PLATFORM_WINDOWS
#include "Slate/SlateTextures.h"
#include "RenderingThread.h"
#endif
#endif

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "d3d11_1.h"
#include "d3d12.h"
#include "dxgi1_4.h"
#include "Windows/HideWindowsPlatformTypes.h"

static TComPtr<IDXGIAdapter> GetDXGIAdapterFromRHI()
{
	TComPtr<IDXGIAdapter> DXGIAdapter;

	const ERHIInterfaceType RHIType = RHIGetInterfaceType();
	if (RHIType == ERHIInterfaceType::D3D11)
	{
		TComPtr<IDXGIDevice> DXGIDevice;
		if (HRESULT Hr = static_cast<ID3D11Device*>(GDynamicRHI->RHIGetNativeDevice())->QueryInterface(IID_PPV_ARGS(&DXGIDevice)); FAILED(Hr))
		{
			UE_LOG(LogWebBrowser, Error, TEXT("GetDXGIAdapter() - - ID3D11Device::QueryInterface 0x%x"), Hr);
			return DXGIAdapter;
		}
		if (HRESULT Hr = DXGIDevice->GetAdapter(&DXGIAdapter); FAILED(Hr))
		{
			UE_LOG(LogWebBrowser, Error, TEXT("GetDXGIAdapter() - - IDXGIDevice::GetAdapter 0x%x"), Hr);
			return DXGIAdapter;
		}
	}
	else if (RHIType == ERHIInterfaceType::D3D12)
	{
		const LUID Luid = static_cast<ID3D12Device*>(GDynamicRHI->RHIGetNativeDevice())->GetAdapterLuid();

		TComPtr<IDXGIFactory4> DXGIFactory;
		if (HRESULT Hr = CreateDXGIFactory1(IID_PPV_ARGS(&DXGIFactory)); FAILED(Hr))
		{
			UE_LOG(LogWebBrowser, Error, TEXT("GetDXGIAdapter() - - CreateDXGIFactory1 0x%x"), Hr);
			return DXGIAdapter;
		}
		if (HRESULT Hr = DXGIFactory->EnumAdapterByLuid(Luid, IID_PPV_ARGS(&DXGIAdapter)); FAILED(Hr))
		{
			UE_LOG(LogWebBrowser, Error, TEXT("GetDXGIAdapter() - - IDXGIFactory4::EnumAdapterByLuid 0x%x"), Hr);
			return DXGIAdapter;
		}
	}

	return DXGIAdapter;
}
#endif // PLATFORM_WINDOWS


FCEFWebBrowserWindowRHIHelper::FCEFWebBrowserWindowRHIHelper()
{
#if WITH_ENGINE
#if PLATFORM_WINDOWS
	// We need to get access to the underlying D3D device from RHI to create a new device with the same adapter
	TComPtr<IDXGIAdapter> DXGIAdapter = GetDXGIAdapterFromRHI();

	const D3D_FEATURE_LEVEL FeatureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	if (HRESULT Hr = D3D11CreateDevice(DXGIAdapter.Get(), DXGIAdapter.Get() ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, FeatureLevels, UE_ARRAY_COUNT(FeatureLevels), D3D11_SDK_VERSION,
		&D3D11Device, nullptr, &D3D11DeviceContext); FAILED(Hr))
	{
		UE_LOG(LogWebBrowser, Error, TEXT("FCEFWebBrowserWindowRHIHelper::FCEFWebBrowserWindowRHIHelper() - - D3D11CreateDevice 0x%x"), Hr);
		return;
	}
#endif // PLATFORM_WINDOWS
#endif // WITH_ENGINE
}

bool FCEFWebBrowserWindowRHIHelper::BUseSupportedRHIRenderer()
{
#if WITH_ENGINE
	const ERHIInterfaceType RHIType = RHIGetInterfaceType();
	// We only support D3D RHIs as we rely on the OpenSharedResource1 interface
	return (RHIType == ERHIInterfaceType::D3D11 || RHIType == ERHIInterfaceType::D3D12)
		// Disable during automation as the GPU process often fails to initialize on build machines
		&& !GIsAutomationTesting && !GIsBuildMachine;
#else
	return false;
#endif // WITH_ENGINE
}

uint64_t FCEFWebBrowserWindowRHIHelper::GetRHIAdapterLuid()
{
	uint64_t AdapterLuid = 0;

#if WITH_ENGINE
#if PLATFORM_WINDOWS
	LUID Luid = {};

	const ERHIInterfaceType RHIType = RHIGetInterfaceType();
	if (RHIType == ERHIInterfaceType::D3D11)
	{
		TComPtr<IDXGIAdapter> DXGIAdapter = GetDXGIAdapterFromRHI();
		DXGI_ADAPTER_DESC Desc = {};
		if (SUCCEEDED(DXGIAdapter->GetDesc(&Desc)))
		{
			Luid = Desc.AdapterLuid;
		}
	}
	else if (RHIType == ERHIInterfaceType::D3D12)
	{
		Luid = static_cast<ID3D12Device*>(GDynamicRHI->RHIGetNativeDevice())->GetAdapterLuid();
	}

	AdapterLuid = (static_cast<uint64_t>(Luid.HighPart) << 32) | static_cast<uint64_t>(Luid.LowPart);
#endif // PLATFORM_WINDOWS
#endif // WITH_ENGINE

	return AdapterLuid;
}

FSlateUpdatableTexture* FCEFWebBrowserWindowRHIHelper::CreateSlateUpdatableTexture(const FUintPoint& TextureSize)
{
#if WITH_ENGINE
	check(BUseSupportedRHIRenderer());
#if PLATFORM_WINDOWS
	// On Windows we need the slate texture to be shared so we can access it from the D3D device that lives in the OnAcceleratedPaint thread
	FSlateTexture2DRHIRef* SlateTexture = new FSlateTexture2DRHIRef(TextureSize.X, TextureSize.Y, PF_B8G8R8A8, nullptr, ETextureCreateFlags::Shared | ETextureCreateFlags::RenderTargetable, true);
	BeginInitResource(SlateTexture);
	return SlateTexture;
#else
	return nullptr;
#endif // PLATFORM_WINDOWS
#else
	return nullptr;
#endif // WITH_ENGINE
}

void FCEFWebBrowserWindowRHIHelper::ReleaseSlateUpdatableTexture(FSlateUpdatableTexture* SlateTexture)
{
#if WITH_ENGINE
	check(BUseSupportedRHIRenderer());
#if PLATFORM_WINDOWS
	if (!SlateTexture)
	{
		return;
	}

	SlateTextureHandles.Remove(SlateTexture);

	BeginReleaseResource(static_cast<FSlateTexture2DRHIRef*>(SlateTexture));
	FlushRenderingCommands();

	delete SlateTexture;
#endif // PLATFORM_WINDOWS
#endif // WITH_ENGINE
}

bool FCEFWebBrowserWindowRHIHelper::EnsureShareable(FSlateUpdatableTexture* SlateTexture)
{
#if WITH_ENGINE
	if (!SlateTexture)
	{
		UE_LOG(LogWebBrowser, Error, TEXT("FCEFWebBrowserWindowRHIHelper::EnsureShareable() - Invalid SlateTexture"));
		return false;
	}

	if (TSharedPtr<void>* HandlePtr = SlateTextureHandles.Find(SlateTexture); HandlePtr)
	{
		// Null Handle means that extraction/initialization failed
		return HandlePtr->Get() != nullptr;
	}

#if PLATFORM_WINDOWS
	FTextureRHIRef Slate2DRef = static_cast<FSlateTexture2DRHIRef*>(SlateTexture)->GetRHIRef();
	if (!Slate2DRef.IsValid() || !Slate2DRef->GetNativeResource())
	{
		// If this happens frequently then we need to make the resource creation sync in CreateSlateUpdatableTexture
		UE_LOG(LogWebBrowser, Error, TEXT("FCEFWebBrowserWindowRHIHelper::EnsureShareable() - SlateTexture is not ready!"));
		return false;
	}

	HANDLE SharedHandle = nullptr;

	// We add SharedHandle to the map even if we couldn't extract it, so we don't continuously try and report failures
	const ERHIInterfaceType RHIType = RHIGetInterfaceType();
	if (RHIType == ERHIInterfaceType::D3D11)
	{
		TComPtr<IDXGIResource> DXGIResource;
		if (HRESULT Hr = static_cast<ID3D11Texture2D*>(Slate2DRef->GetNativeResource())->QueryInterface(IID_PPV_ARGS(&DXGIResource)); FAILED(Hr))
		{
			UE_LOG(LogWebBrowser, Error, TEXT("FCEFWebBrowserWindowRHIHelper::EnsureShareable() - - ID3D11Texture2D::QueryInterface 0x%x"), Hr);
		}
		else if (Hr = DXGIResource->GetSharedHandle(&SharedHandle); FAILED(Hr))
		{
			UE_LOG(LogWebBrowser, Error, TEXT("FCEFWebBrowserWindowRHIHelper::EnsureShareable() - - IDXGIResource::GetSharedHandle 0x%x"), Hr);
		}

		// GetSharedHandle returns a legacy non-NT handle, which must NOT be closed when we're done with it
		SlateTextureHandles.Add(SlateTexture, MakeShareable(SharedHandle, [](HANDLE _LegacySharedHandle) {}));
	}
	else if (RHIType == ERHIInterfaceType::D3D12)
	{
		if (HRESULT Hr = static_cast<ID3D12Device*>(GDynamicRHI->RHIGetNativeDevice())->CreateSharedHandle(static_cast<ID3D12Resource*>(Slate2DRef->GetNativeResource()), NULL, GENERIC_ALL, NULL, &SharedHandle); FAILED(Hr))
		{
			UE_LOG(LogWebBrowser, Error, TEXT("FCEFWebBrowserWindowRHIHelper::EnsureShareable() - - ID3D12Device::CreateSharedHandle 0x%x"), Hr);
		}

		// CreateSharedHandle returns a NT handle,. which must be closed when we're done with it
		SlateTextureHandles.Add(SlateTexture, MakeShareable(SharedHandle, [](HANDLE _NTSharedHandle)
			{
				if (_NTSharedHandle)
				{
					CloseHandle(_NTSharedHandle);
				}
			}));
	}

	return SharedHandle != nullptr;
#else
	return false;
#endif // PLATFORM_WINDOWS
#else
	return false;
#endif // WITH_ENGINE

}

bool FCEFWebBrowserWindowRHIHelper::CopySharedTextureSync(FSlateUpdatableTexture* SlateTexture, void* SharedHandle, const FIntRect& DirtyIn)
{
#if WITH_ENGINE
	check(BUseSupportedRHIRenderer());
	FIntRect Dirty = DirtyIn;

	if (!EnsureShareable(SlateTexture))
	{
		return false;
	}

#if PLATFORM_WINDOWS
	TComPtr<ID3D11Device1> D3D11Device1;
	if (HRESULT Hr = D3D11Device->QueryInterface(IID_PPV_ARGS(&D3D11Device1)); FAILED(Hr))
	{
		UE_LOG(LogWebBrowser, Error, TEXT("FCEFWebBrowserWindowRHIHelper::CopySharedTextureSync() - - ID3D11Device::QueryInterface 0x%x"), Hr);
		return false;
	}

	TComPtr<ID3D11Texture2D> SourceTexture;
	if (HRESULT Hr = D3D11Device1->OpenSharedResource1((HANDLE)SharedHandle, IID_PPV_ARGS(&SourceTexture)); FAILED(Hr))
	{
		UE_LOG(LogWebBrowser, Error, TEXT("FCEFWebBrowserWindowRHIHelper::CopySharedTextureSync() - - ID3D11Device1::OpenSharedResource1 0x%x"), Hr);
		return false;
	}

	D3D11_TEXTURE2D_DESC SourceTextureDesc = {};
	SourceTexture->GetDesc(&SourceTextureDesc);
	if (SlateTexture->GetSlateResource()->GetWidth() != SourceTextureDesc.Width || SlateTexture->GetSlateResource()->GetHeight() != SourceTextureDesc.Height)
	{
		SlateTexture->ResizeTexture(SourceTextureDesc.Width, SourceTextureDesc.Height);
		FlushRenderingCommands();

		// The native resource has changed, we need to update the shared handle
		SlateTextureHandles.Remove(SlateTexture);
		EnsureShareable(SlateTexture);

		Dirty = FIntRect();
	}

	TComPtr<ID3D11Texture2D> DestTexture;
	const ERHIInterfaceType RHIType = RHIGetInterfaceType();
	if (RHIType == ERHIInterfaceType::D3D11)
	{
		if (HRESULT Hr = D3D11Device->OpenSharedResource((HANDLE)SlateTextureHandles.FindRef(SlateTexture).Get(), IID_PPV_ARGS(&DestTexture)); FAILED(Hr))
		{
			UE_LOG(LogWebBrowser, Error, TEXT("FCEFWebBrowserWindowRHIHelper::CopySharedTextureSync() - - ID3D11Device::OpenSharedResource 0x%x"), Hr);
			return false;
		}
	}
	else if (RHIType == ERHIInterfaceType::D3D12)
	{
		if (HRESULT Hr = D3D11Device1->OpenSharedResource1((HANDLE)SlateTextureHandles.FindRef(SlateTexture).Get(), IID_PPV_ARGS(&DestTexture)); FAILED(Hr))
		{
			UE_LOG(LogWebBrowser, Error, TEXT("FCEFWebBrowserWindowRHIHelper::CopySharedTextureSync() - - ID3D11Device1::OpenSharedResource1 0x%x"), Hr);
			return false;
		}
	}

	// Copy the textures on the GPU
	// Note that as of CEF v128 the Dirty rect sent by CEF always matches the whole SourceTexture no matter what part has actually changed,
	// so we can just do full copies. If that were to change in the future we can start using CopySubresourceRegion instead
	D3D11DeviceContext->CopyResource(DestTexture.Get(), SourceTexture.Get());

	// The CopyResource call is asynchronous by default, so we need to force a sync flush to ensure it completes before we exit this method
	D3D11DeviceContext->Flush();

	// Now wait on the GPU to complete the copy and flush
	D3D11_QUERY_DESC QueryDesc = {};
	QueryDesc.Query = D3D11_QUERY_EVENT;
	TComPtr<ID3D11Query> Query;
	if (HRESULT Hr = D3D11Device1->CreateQuery(&QueryDesc, &Query); FAILED(Hr))
	{
		UE_LOG(LogWebBrowser, Error, TEXT("FCEFWebBrowserWindowRHIHelper::CopySharedTextureSync() - ID3D11Device1::CreateQuery 0x%x"), Hr);
		return false;
	}
	D3D11DeviceContext->End(Query.Get());
	BOOL bIsDone = false;
	HRESULT Hr = S_OK;
	for (;;)
	{
		Hr = D3D11DeviceContext->GetData(Query.Get(), &bIsDone, sizeof(bIsDone), 0);
		if (FAILED(Hr))
		{
			UE_LOG(LogWebBrowser, Error, TEXT("FCEFWebBrowserWindowRHIHelper::CopySharedTextureSync() - ID3D11DeviceContext::GetData  0x%x"), Hr);
			break;
		}

		// We need to check for S_OK specifically as S_FALSE is also considered a success return code
		if (Hr == S_OK && bIsDone)
		{
			break;
		}

		// Idle wait before retrying
		if (!SwitchToThread())
		{
			Sleep(1); // 1ms
		}
	}

	return true;
#else
	UE_LOG(LogWebBrowser, Error, TEXT("FCEFWebBrowserWindowRHIHelper::CopySharedTextureSync() - missing implementation"));
	return false;
#endif // PLATFORM_WINDOWS
#else
	UE_LOG(LogWebBrowser, Error, TEXT("FCEFWebBrowserWindowRHIHelper::CopySharedTextureSync() - unsupported usage, RHI renderer but missing engine"));
	return false;
#endif // WITH_ENGINE
}

#endif
