// Copyright Epic Games, Inc. All Rights Reserved.

#include "SWebBrowser.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SThrobber.h"
#if WITH_CEF3
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Async/Async.h"
#include "CEF/CEFWebBrowserWindow.h"
#endif // WITH_CEF3
#include "WebBrowserModule.h"
#include "WebBrowserSingleton.h"

#define LOCTEXT_NAMESPACE "WebBrowser"

SWebBrowser::SWebBrowser()
{
}

SWebBrowser::~SWebBrowser()
{
}

#if WITH_CEF3
static TSharedRef<FCEFWebBrowserWindow> GetCefWebBrowserWindowChecked(const TSharedPtr<IWebBrowserWindow>& WebBrowserWindow)
{
	check(WebBrowserWindow.IsValid());
	
	const TSharedPtr<FCEFWebBrowserWindow> CefWebBrowserWindow = StaticCastSharedPtr<FCEFWebBrowserWindow>(WebBrowserWindow);
	check(CefWebBrowserWindow.IsValid());
	
	return CefWebBrowserWindow.ToSharedRef();
}
#endif // WITH_CEF3

static FWebBrowserSingleton& GetWebBrowserSingletonChecked()
{
	IWebBrowserModule& WebBrowserModule = FModuleManager::LoadModuleChecked<IWebBrowserModule>("WebBrowser");
	FWebBrowserSingleton* WebBrowserSingleton = static_cast<FWebBrowserSingleton*>(WebBrowserModule.GetSingleton());
	check(WebBrowserSingleton);
	return *WebBrowserSingleton;
}

/*virtual*/ void SWebBrowser::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

#if WITH_EDITOR
#if WITH_CEF3
	if (WebBrowserWindow.IsValid()) 
	{
		if (const TSharedRef<FCEFWebBrowserWindow> CefWebBrowserWindow = GetCefWebBrowserWindowChecked(WebBrowserWindow);
			CefWebBrowserWindow->HasResizeBug())
		{
			// NOTE - We won't end up here if the CEF version doesn't require it.
			
			const bool bIsFloatingWindow = (FSlateApplication::Get().FindWidgetWindow(AsShared()) != FGlobalTabmanager::Get()->GetRootWindow());
			

			// * This is necessary during active widget resizes, whether or not the widget is in a floating window. *
			// If web browser doesn't have the correct CEF buffer, tell it to resize.
			// NOTE - Call this before the 'forced tick' code below.
			
			if (!CefWebBrowserWindow->HasCorrectNativeCefBuffer())
			{
				if (bIsFloatingWindow)
				{
					// Seems necessary to make web browser content appear, if it's already in a floating window when the Editor starts up.
					// But this is not good to call if docked in editor!
					CefWebBrowserWindow->SetViewportSize(BrowserView->GetCachedGeometry().Size.IntPoint(), BrowserView->GetCachedGeometry().AbsolutePosition.IntPoint());
				}

				// Always seems necessary (even if viewport size was just changed above.)
				CefWebBrowserWindow->SetNeedsResize(true);
			}
	
#if PLATFORM_WINDOWS
			// * This is necessary during active widget resizes, only if the widget is in a floating window, and only when on Windows. *
			// When we are (1) running on Windows (2) not docked in the Editor (floating), and (3) our window is actively being resized - then the FTS
			// Ticker interface on the Web Browser Singleton will be suspended. But our Tick() function here will not be suspended. So under these and
			// certain other conditions (see below), we want to force Web Browser Singleton to tick, and we can do that from here. But, we also can't tick
			// it too fast or too often, or we'll overwhelm the CEF message pump, and it will start ignoring what we want it to do. We also can't tick it
			// too slow, or it can ignore the tick.
			// TODO - In this floating window case only, the web browser can still end up stuck with the wrong texture, when mouse stops moving during
			//		  an active resize, until mouse is released. Why?
			
			if (FWebBrowserSingleton& WebBrowserSingleton = GetWebBrowserSingletonChecked();
				bIsFloatingWindow && FPlatformTime::Seconds() - WebBrowserSingleton.GetPreviousTickTimeSeconds() >= WebBrowserSingleton.GetExternalTickCallWaitSeconds())
			{
				WebBrowserSingleton.Tick(InDeltaTime);
			}
#endif // PLATFORM_WINDOWS
		}
	}
#endif // WITH_CEF3
#endif // WITH_EDITOR
}

/*virtual*/ int32 SWebBrowser::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
#if WITH_EDITOR
#if WITH_CEF3
	if (WebBrowserWindow.IsValid())
	{
		if (const TSharedPtr<FCEFWebBrowserWindow> CefWebBrowserWindow = GetCefWebBrowserWindowChecked(WebBrowserWindow);
			CefWebBrowserWindow->HasResizeBug())
		{
			// NOTE - We won't end up here if the CEF version doesn't require it.
			
			if (const TSharedPtr<FCapturedCefBuffer>& CapturedCefBuffer = CefWebBrowserWindow->GetCapturedCefBuffer();
				CapturedCefBuffer.IsValid())
			{
				if (CefWebBrowserWindow->HasCorrectNativeCefBuffer())
				{
					// The CEF web browser can display the correct texture itself. So clean-up the captured CEF buffer's paint objects, if it has any.
					// But do this on the next tick to avoid a white flash.

					if (CapturedCefBuffer->HasPaintObjects())
					{
						AsyncTask(ENamedThreads::GameThread, [CapturedCefBuffer]() -> void
							{
								if (CapturedCefBuffer.IsValid())
								{
									CapturedCefBuffer->ClearPaintObjects();
								}
							});
					}
				}
				else if (CapturedCefBuffer->MaybeUpdatePaintObjects())
				{
					// The CEF web browser can't display the correct texture itself. So paint the captured CEF buffer instead.
					// NOTE - We still need to call the usual OnPaint() to make the web browser update, but we ignore what it actually paints. Instead, we
					// only properly paint the captured CEF buffer.
			
					FSlateWindowElementList IgnoredDrawElements(FSlateApplication::Get().FindWidgetWindow(AsShared()));
					SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, IgnoredDrawElements, LayerId, InWidgetStyle, bParentEnabled);

					const FWidgetStyle CompoundedWidgetStyle = FWidgetStyle(InWidgetStyle)
						.BlendColorAndOpacityTint(GetColorAndOpacity())
						.SetForegroundColor(ShouldBeEnabled(bParentEnabled) ? GetForegroundColor() : GetDisabledForegroundColor() );

					return CapturedCefBuffer->PaintCentered(AllottedGeometry, OutDrawElements, LayerId, CompoundedWidgetStyle);
				}
			}
		}
	}
#endif // WITH_CEF3
#endif // WITH_EDITOR

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}

void SWebBrowser::Construct(const FArguments& InArgs, const TSharedPtr<IWebBrowserWindow>& InWebBrowserWindow)
{
	OnLoadCompleted = InArgs._OnLoadCompleted;
	OnLoadError = InArgs._OnLoadError;
	OnLoadStarted = InArgs._OnLoadStarted;
	OnTitleChanged = InArgs._OnTitleChanged;
	OnUrlChanged = InArgs._OnUrlChanged;
	OnBeforeNavigation = InArgs._OnBeforeNavigation;
	OnLoadUrl = InArgs._OnLoadUrl;
	OnShowDialog = InArgs._OnShowDialog;
	OnDismissAllDialogs = InArgs._OnDismissAllDialogs;
	OnBeforePopup = InArgs._OnBeforePopup;
	OnConsoleMessage = InArgs._OnConsoleMessage;
	OnCreateWindow = InArgs._OnCreateWindow;
	OnCloseWindow = InArgs._OnCloseWindow;
	bShowInitialThrobber = InArgs._ShowInitialThrobber;
	
	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			.Visibility((InArgs._ShowControls || InArgs._ShowAddressBar) ? EVisibility::Visible : EVisibility::Collapsed)
			+ SHorizontalBox::Slot()
			.Padding(0, 5)
			.AutoWidth()
			[
				SNew(SHorizontalBox)
				.Visibility(InArgs._ShowControls ? EVisibility::Visible : EVisibility::Collapsed)
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("Back","Back"))
					.IsEnabled(this, &SWebBrowser::CanGoBack)
					.OnClicked(this, &SWebBrowser::OnBackClicked)
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("Forward", "Forward"))
					.IsEnabled(this, &SWebBrowser::CanGoForward)
					.OnClicked(this, &SWebBrowser::OnForwardClicked)
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(this, &SWebBrowser::GetReloadButtonText)
					.OnClicked(this, &SWebBrowser::OnReloadClicked)
				]
				+SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Right)
				.Padding(5)
				[
					SNew(STextBlock)
					.Visibility(InArgs._ShowAddressBar ? EVisibility::Collapsed : EVisibility::Visible )
					.Text(this, &SWebBrowser::GetTitleText)
					.Justification(ETextJustify::Right)
				]
			]
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Fill)
			.Padding(5.f, 5.f)
			[
				// @todo: A proper addressbar widget should go here, for now we use a simple textbox.
				SAssignNew(InputText, SEditableTextBox)
				.Visibility(InArgs._ShowAddressBar ? EVisibility::Visible : EVisibility::Collapsed)
				.OnTextCommitted(this, &SWebBrowser::OnUrlTextCommitted)
				.Text(this, &SWebBrowser::GetAddressBarUrlText)
				.SelectAllTextWhenFocused(true)
				.ClearKeyboardFocusOnCommit(true)
				.RevertTextOnEscape(true)
			]
		]
		+SVerticalBox::Slot()
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SAssignNew(BrowserView, SWebBrowserView, InWebBrowserWindow)
				.ParentWindow(InArgs._ParentWindow)
				.InitialURL(InArgs._InitialURL)
				.ContentsToLoad(InArgs._ContentsToLoad)
				.ShowErrorMessage(InArgs._ShowErrorMessage)
				.SupportsTransparency(InArgs._SupportsTransparency)
				.SupportsThumbMouseButtonNavigation(InArgs._SupportsThumbMouseButtonNavigation)
				.BackgroundColor(InArgs._BackgroundColor)
				.PopupMenuMethod(InArgs._PopupMenuMethod)
				.ViewportSize(InArgs._ViewportSize)
				.OnLoadCompleted(OnLoadCompleted)
				.OnLoadError(OnLoadError)
				.OnLoadStarted(OnLoadStarted)
				.OnTitleChanged(OnTitleChanged)
				.OnUrlChanged(OnUrlChanged)
				.OnBeforePopup(OnBeforePopup)
				.OnCreateWindow(OnCreateWindow)
				.OnCloseWindow(OnCloseWindow)
				.OnBeforeNavigation(OnBeforeNavigation)
				.OnLoadUrl(OnLoadUrl)
				.OnShowDialog(OnShowDialog)
				.OnDismissAllDialogs(OnDismissAllDialogs)
				.Visibility(this, &SWebBrowser::GetViewportVisibility)
				.OnSuppressContextMenu(InArgs._OnSuppressContextMenu)
				.OnDragWindow(InArgs._OnDragWindow)
				.OnConsoleMessage(OnConsoleMessage)
				.BrowserFrameRate(InArgs._BrowserFrameRate)
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SCircularThrobber)
				.Radius(10.0f)
				.ToolTipText(LOCTEXT("LoadingThrobberToolTip", "Loading page..."))
				.Visibility(this, &SWebBrowser::GetLoadingThrobberVisibility)
			]
		]
	];


	WebBrowserWindow = (InWebBrowserWindow.IsValid() ? InWebBrowserWindow : BrowserView->GetBrowserWindow());	


#if WITH_CEF3
	if (WebBrowserWindow.IsValid()) 
	{
		const bool bUseSmoothScrolling = 
#if WITH_EDITOR
			(GIsPlayInEditorWorld ? false : InArgs._UseSmoothResizing)
#else
			false
#endif // WITH_EDITOR
			;
		
		GetCefWebBrowserWindowChecked(WebBrowserWindow)->SetUseCapturedCefBuffer(bUseSmoothScrolling);
	}
#endif // WITH_CEF3
}

void SWebBrowser::LoadURL(FString NewURL)
{
	if (BrowserView.IsValid())
	{
		BrowserView->LoadURL(NewURL);
	}
}

void SWebBrowser::LoadString(FString Contents, FString DummyURL)
{
	if (BrowserView.IsValid())
	{
		BrowserView->LoadString(Contents, DummyURL);
	}
}

void SWebBrowser::Reload()
{
	if (BrowserView.IsValid())
	{
		BrowserView->Reload();
	}
}

void SWebBrowser::StopLoad()
{
	if (BrowserView.IsValid())
	{
		BrowserView->StopLoad();
	}
}

FText SWebBrowser::GetTitleText() const
{
	if (BrowserView.IsValid())
	{
		return BrowserView->GetTitleText();
	}
	return LOCTEXT("InvalidWindow", "Browser Window is not valid/supported");
}

FString SWebBrowser::GetUrl() const
{
	if (BrowserView.IsValid())
	{
		return BrowserView->GetUrl();
	}

	return FString();
}

FText SWebBrowser::GetAddressBarUrlText() const
{
	if(BrowserView.IsValid())
	{
		return BrowserView->GetAddressBarUrlText();
	}
	return FText::GetEmpty();
}

bool SWebBrowser::IsLoaded() const
{
	if (BrowserView.IsValid())
	{
		return BrowserView->IsLoaded();
	}

	return false;
}

bool SWebBrowser::IsLoading() const
{
	if (BrowserView.IsValid())
	{
		return BrowserView->IsLoading();
	}

	return false;
}

bool SWebBrowser::CanGoBack() const
{
	if (BrowserView.IsValid())
	{
		return BrowserView->CanGoBack();
	}
	return false;
}

void SWebBrowser::GoBack()
{
	if (BrowserView.IsValid())
	{
		BrowserView->GoBack();
	}
}

FReply SWebBrowser::OnBackClicked()
{
	GoBack();
	return FReply::Handled();
}

bool SWebBrowser::CanGoForward() const
{
	if (BrowserView.IsValid())
	{
		return BrowserView->CanGoForward();
	}
	return false;
}

void SWebBrowser::GoForward()
{
	if (BrowserView.IsValid())
	{
		BrowserView->GoForward();
	}
}

FReply SWebBrowser::OnForwardClicked()
{
	GoForward();
	return FReply::Handled();
}

FText SWebBrowser::GetReloadButtonText() const
{
	static FText ReloadText = LOCTEXT("Reload", "Reload");
	static FText StopText = LOCTEXT("StopText", "Stop");

	if (BrowserView.IsValid())
	{
		if (BrowserView->IsLoading())
		{
			return StopText;
		}
	}
	return ReloadText;
}

FReply SWebBrowser::OnReloadClicked()
{
	if (IsLoading())
	{
		StopLoad();
	}
	else
	{
		Reload();
	}
	return FReply::Handled();
}

void SWebBrowser::OnUrlTextCommitted( const FText& NewText, ETextCommit::Type CommitType )
{
	if(CommitType == ETextCommit::OnEnter)
	{
		LoadURL(NewText.ToString());
	}
}

EVisibility SWebBrowser::GetViewportVisibility() const
{
	if (!bShowInitialThrobber || BrowserView->IsInitialized())
	{
		return EVisibility::Visible;
	}
	return EVisibility::Hidden;
}

EVisibility SWebBrowser::GetLoadingThrobberVisibility() const
{
	if (bShowInitialThrobber && !BrowserView->IsInitialized())
	{
		return EVisibility::Visible;
	}
	return EVisibility::Hidden;
}


void SWebBrowser::ExecuteJavascript(const FString& ScriptText)
{
	if (BrowserView.IsValid())
	{
		BrowserView->ExecuteJavascript(ScriptText);
	}
}

void SWebBrowser::GetSource(TFunction<void (const FString&)> Callback) const
{
	if (BrowserView.IsValid())
	{
		BrowserView->GetSource(Callback);
	}
}

void SWebBrowser::BindUObject(const FString& Name, UObject* Object, bool bIsPermanent)
{
	if (BrowserView.IsValid())
	{
		BrowserView->BindUObject(Name, Object, bIsPermanent);
	}
}

void SWebBrowser::UnbindUObject(const FString& Name, UObject* Object, bool bIsPermanent)
{
	if (BrowserView.IsValid())
	{
		BrowserView->UnbindUObject(Name, Object, bIsPermanent);
	}
}

void SWebBrowser::BindAdapter(const TSharedRef<IWebBrowserAdapter>& Adapter)
{
	if (BrowserView.IsValid())
	{
		BrowserView->BindAdapter(Adapter);
	}
}

void SWebBrowser::UnbindAdapter(const TSharedRef<IWebBrowserAdapter>& Adapter)
{
	if (BrowserView.IsValid())
	{
		BrowserView->UnbindAdapter(Adapter);
	}
}

void SWebBrowser::BindInputMethodSystem(ITextInputMethodSystem* TextInputMethodSystem)
{
	if (BrowserView.IsValid())
	{
		BrowserView->BindInputMethodSystem(TextInputMethodSystem);
	}
}

void SWebBrowser::UnbindInputMethodSystem()
{
	if (BrowserView.IsValid())
	{
		BrowserView->UnbindInputMethodSystem();
	}
}

void SWebBrowser::SetParentWindow(TSharedPtr<SWindow> Window)
{
	if (BrowserView.IsValid())
	{
		BrowserView->SetParentWindow(Window);
	}
}


#undef LOCTEXT_NAMESPACE
