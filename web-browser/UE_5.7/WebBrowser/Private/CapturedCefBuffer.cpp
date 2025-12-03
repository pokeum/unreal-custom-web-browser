// Copyright Epic Games, Inc. All Rights Reserved.


#include "CapturedCefBuffer.h"


#if UE_WITH_CAPTURED_CEF_BUFFER
#include "Runtime/Engine/Public/TextureResource.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/StyleColors.h"
#endif


bool FCapturedCefBuffer::SetBufferAsB8G8R8A8(const void* InBufferB8G8R8A8, const int32 InBufferWidth, const int32 InBufferHeight, const float InViewportDPIScaleFactor, const bool bDoSkipBadBufferTest)
{
#if UE_WITH_CAPTURED_CEF_BUFFER
	
	
	if (!InBufferB8G8R8A8 || InBufferWidth <= 0 || InBufferHeight <= 0)
	{
		ClearBuffer();
		
		return false;
	}


	// This works depending on whether the web browser content has a transparent background.
	// If it does, then the web browser will show its content, centered and stable.
	// If it doesn't, then the web browser will occasionally adapt so that its content fills the window, depending on this -
	//
	// CEF can sometimes send us a buffer that's apparently a good size. But, the actual image pixels of the web browser content are actually
	// somewhere in the middle of that image, smaller than the entire image actually is, stretched or compressed, and all surrounded with a margin of
	// transparent pixels. To test if this is the case at all, we really only need to test the alpha channel of a single pixel in one of the corners,
	// and the very first pixel will be one such pixel. If this is the case, it's a dead giveaway that the buffer is not good, and so we don't want to
	// capture this CEF buffer, because the web browser image will appear to be the wrong size and stretched or compressed. We'll wait until a buffer
	// comes along where the web browser image is actually completely filled and not stretched or compressed, which is the case when there's no
	// transparent margin.
	//
	// We also don't capture the buffer if the dimensions have not changed, to avoid captures on every frame.
	//
	// But we allow the caller to skip this test and force capture, especially if the caller knows they have a correct buffer.

	if (InBufferWidth == BufferDimensions.X && InBufferHeight == BufferDimensions.Y && InViewportDPIScaleFactor == ViewportDPIScaleFactor)
	{
		return false;
	}
	
	if (const bool bDoesBufferHaveTransparentMargin = (static_cast<const uint8*>(InBufferB8G8R8A8)[3] == 0x0); 
		bDoSkipBadBufferTest || (!bDoSkipBadBufferTest && !bDoesBufferHaveTransparentMargin))
	{
		BufferDimensions = FIntPoint(InBufferWidth, InBufferHeight);
		
		const TArray<uint8>::SizeType NumBytes = CalculateBufferNumBytes();
		if (BufferData.Num() != NumBytes)
		{
			BufferData.SetNum(NumBytes);
		}
		FMemory::Memcpy(BufferData.GetData(), InBufferB8G8R8A8, NumBytes);


		ViewportDPIScaleFactor = InViewportDPIScaleFactor;


		bDoesNeedNewPaintObjects = true;

		
		return true;
	}

	
#endif

	
	return false;
}


bool FCapturedCefBuffer::ClearBuffer()
{
#if UE_WITH_CAPTURED_CEF_BUFFER

	
	if (!BufferData.IsEmpty())
	{
		BufferData.Empty();
	}
	BufferDimensions = FIntPoint::ZeroValue;
	ViewportDPIScaleFactor = 1.0f;
	
	return true;


#else


	return false;

	
#endif
}


bool FCapturedCefBuffer::MaybeUpdatePaintObjects() 
{
#if UE_WITH_CAPTURED_CEF_BUFFER
	
	
	if (!FSlateApplication::IsInitialized()) // ..safety check
	{
		return false;
	}
	
	if (!IsBufferValid())
	{
		ClearPaintObjects();
		
		return false;
	}

	
	// Copy buffer into texture, and into Slate brush.

	if (bDoesNeedNewPaintObjects)
	{
		if (PaintTexture.Get() && (PaintTexture->GetSizeX() != BufferDimensions.X || PaintTexture->GetSizeY() != BufferDimensions.Y))
		{
			PaintTexture.Reset(); // ..uses GC
		}

		if (!PaintTexture.IsValid())
		{
			PaintTexture = TStrongObjectPtr(UTexture2D::CreateTransient(BufferDimensions.X, BufferDimensions.Y, PF_B8G8R8A8));
			PaintTexture->SRGB = true;
			PaintTexture->LODGroup = TEXTUREGROUP_UI;
		}
		
		void* TextureData = PaintTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(TextureData, BufferData.GetData(), CalculateBufferNumBytes());
		PaintTexture->GetPlatformData()->Mips[0].BulkData.Unlock();
		PaintTexture->UpdateResource();
	
	
		PaintSlateBrush.SetResourceObject(PaintTexture.Get());
		PaintSlateBrush.ImageSize = FVector2D(BufferDimensions.X, BufferDimensions.Y);
		PaintSlateBrush.DrawAs = ESlateBrushDrawType::Type::Image;

		
		bDoesNeedNewPaintObjects = false;
		bHasPaintObjects = true;
	}

	
	return true;

	
#else

	
	return false;
	
	
#endif
}


bool FCapturedCefBuffer::ClearPaintObjects()
{
#if UE_WITH_CAPTURED_CEF_BUFFER
	
	
	if (!bHasPaintObjects)
	{
		return false;
	}
	
	if (!FSlateApplication::IsInitialized()) // ..safety check
	{
		return false;
	}

	
	bHasPaintObjects = false;
	
	if (FSlateApplication::Get().GetRenderer()/*becomes null on shutdown*/ && PaintSlateBrush.GetResourceObject())
	{
		FSlateApplication::Get().GetRenderer()->ReleaseDynamicResource(PaintSlateBrush);
	}

	if (PaintTexture.Get())
	{
		PaintTexture.Reset(); // ..uses GC
	}


	return true;


#else

	
	return false;


#endif
}


int32 FCapturedCefBuffer::PaintCentered(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle) const
{
#if UE_WITH_CAPTURED_CEF_BUFFER
	
	
	if (!bHasPaintObjects || ViewportDPIScaleFactor <= 0.0)
	{
		return LayerId;
	}


	const float Zoom = 1.0f / ViewportDPIScaleFactor; // ..was checked for divide by zero

	const FVector2D& SlateBrushSize = PaintSlateBrush.ImageSize;
	const FVector2D WidgetSize = AllottedGeometry.GetLocalSize();
	const FVector2D CenteringOffset = (WidgetSize - SlateBrushSize) * 0.5f;
	const FSlateLayoutTransform CenteringTransform(Zoom, Zoom * CenteringOffset);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId++,
		AllottedGeometry.ToPaintGeometry(SlateBrushSize, CenteringTransform),
		&PaintSlateBrush,
		ESlateDrawEffect::None,
		InWidgetStyle.GetColorAndOpacityTint()
	);

	
#endif
	
	
	return LayerId;
}


bool FCapturedCefBuffer::HasPaintObjects() const
{
#if UE_WITH_CAPTURED_CEF_BUFFER
	
	return bHasPaintObjects;
	
#else
	
	return false;
	
#endif
}


#if UE_WITH_CAPTURED_CEF_BUFFER
bool FCapturedCefBuffer::IsBufferValid() const
{
	return (!BufferData.IsEmpty() && BufferDimensions.X > 0 && BufferDimensions.Y > 0);
}
#endif


#if UE_WITH_CAPTURED_CEF_BUFFER
uint32 FCapturedCefBuffer::CalculateBufferNumBytes() const
{
	return (BufferDimensions.X * BufferDimensions.Y * 4/*PF_B8G8R8A8*/);
}
#endif

