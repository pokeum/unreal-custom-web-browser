// Copyright Epic Games, Inc. All Rights Reserved.


#pragma once


#define UE_WITH_CAPTURED_CEF_BUFFER (WITH_ENGINE && WITH_CEF3)


#include "Layout/Geometry.h"
#include "Rendering/DrawElements.h"

#if UE_WITH_CAPTURED_CEF_BUFFER
#include "Engine/Texture2D.h"
#include "UObject/StrongObjectPtrTemplates.h"
#include "Styling/SlateBrush.h"
#endif

#include "CapturedCefBuffer.generated.h"


USTRUCT()
struct FCapturedCefBuffer
{
	GENERATED_BODY()


public:


	bool SetBufferAsB8G8R8A8(const void* InBufferB8G8R8A8, const int32 InBufferWidth, const int32 InBufferHeight, const float InViewportDPIScaleFactor, const bool bDoSkipBadBufferTest);
	bool ClearBuffer();
	
	bool MaybeUpdatePaintObjects();
	bool ClearPaintObjects();
	int32 PaintCentered(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle) const;

	bool HasPaintObjects() const;

	
private:


#if UE_WITH_CAPTURED_CEF_BUFFER

	
	bool IsBufferValid() const; 
	uint32 CalculateBufferNumBytes() const;
	

	TArray<uint8> BufferData;
	FIntPoint BufferDimensions = FIntPoint::ZeroValue;

	float ViewportDPIScaleFactor = 1.0f;
	
	bool bDoesNeedNewPaintObjects = false;
	bool bHasPaintObjects = false;
	TStrongObjectPtr<UTexture2D> PaintTexture;
	FSlateBrush PaintSlateBrush;

	
#endif
};
