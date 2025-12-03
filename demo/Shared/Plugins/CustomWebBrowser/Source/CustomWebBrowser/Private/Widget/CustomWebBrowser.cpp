#include "Widget/CustomWebBrowser.h"

#include "SWebBrowser.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#endif

#define DEBUGGING	1
#define TAG			TEXT("CustomWebBrowser")

#define LOCTEXT_NAMESPACE "CustomWebBrowser"

/////////////////////////////////////////////////////
/// UCustomWebBrowser

UCustomWebBrowser::UCustomWebBrowser(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsVariable = true;
}

void UCustomWebBrowser::LoadURL(FString NewURL)
{
	if (WebBrowserWidget.IsValid())
	{
		return WebBrowserWidget->LoadURL(NewURL);
	}
}

void UCustomWebBrowser::LoadString(FString Contents, FString DummyURL)
{
	if (WebBrowserWidget.IsValid())
	{
		return WebBrowserWidget->LoadString(Contents, DummyURL);
	}
}

void UCustomWebBrowser::ExecuteJavascript(const FString& ScriptText)
{
	if (WebBrowserWidget.IsValid())
	{
		return WebBrowserWidget->ExecuteJavascript(ScriptText);
	}
}

FText UCustomWebBrowser::GetTitleText() const
{
	if (WebBrowserWidget.IsValid())
	{
		return WebBrowserWidget->GetTitleText();
	}

	return FText::GetEmpty();
}

FString UCustomWebBrowser::GetUrl() const
{
	if (WebBrowserWidget.IsValid())
	{
		return WebBrowserWidget->GetUrl();
	}

	return FString();
}

void UCustomWebBrowser::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	WebBrowserWidget.Reset();
}

TSharedRef<SWidget> UCustomWebBrowser::RebuildWidget()
{
	if (IsDesignTime())
	{
		return SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Web Browser", "Web Browser"))
			];
	}
	else
	{
		WebBrowserWidget = SNew(SWebBrowser)
			.InitialURL(InitialURL)
			.ShowControls(false)
			.SupportsTransparency(bSupportsTransparency)
			.OnUrlChanged(BIND_UOBJECT_DELEGATE(FOnTextChanged, HandleOnUrlChanged))
			.OnBeforePopup(BIND_UOBJECT_DELEGATE(FOnBeforePopupDelegate, HandleOnBeforePopup))
			.OnLoadUrl(BIND_UOBJECT_DELEGATE(FOnLoadUrl, HandleOnLoadUrl))
			.OnLoadStarted(BIND_UOBJECT_DELEGATE(FSimpleDelegate, HandleOnLoadStarted))
			.OnLoadCompleted(BIND_UOBJECT_DELEGATE(FSimpleDelegate, HandleOnLoadCompleted))
			.OnBeforeNavigation(BIND_UOBJECT_DELEGATE(FOnBeforeBrowse, HandleOnBeforeBrowse));

		// https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/WebBrowser/SWebBrowser/BindUObject?application_version=5.3
		WebBrowserWidget->BindUObject(BIND_UOBJECT_NAME, this, true);

		return WebBrowserWidget.ToSharedRef();
	}
}

void UCustomWebBrowser::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (WebBrowserWidget.IsValid())
	{
	}
}

void UCustomWebBrowser::HandleOnUrlChanged(const FText& InText)
{
	Logging(TAG, FString::Printf(
		        TEXT("HandleOnUrlChanged: InText={%s}"),
		        *InText.ToString()));

	OnUrlChanged.Broadcast(InText);
}

bool UCustomWebBrowser::HandleOnBeforePopup(FString URL, FString Frame)
{
	Logging(TAG, FString::Printf(
		        TEXT("HandleOnBeforePopup: URL={%s}, Frame={%s}"),
		        *URL, *Frame));

	if (OnBeforePopup.IsBound())
	{
		if (IsInGameThread())
		{
			OnBeforePopup.Broadcast(URL, Frame);
		}
		else
		{
			// Retry on the GameThread.
			TWeakObjectPtr<UCustomWebBrowser> WeakThis = this;
			FFunctionGraphTask::CreateAndDispatchWhenReady([WeakThis, URL, Frame]()
			{
				if (WeakThis.IsValid())
				{
					WeakThis->HandleOnBeforePopup(URL, Frame);
				}
			}, TStatId(), nullptr, ENamedThreads::GameThread);
		}

		return true;
	}

	return false;
}

bool UCustomWebBrowser::HandleOnLoadUrl(const FString& Method, const FString& Url, FString& Response)
{
	Logging(TAG, FString::Printf(
		        TEXT("HandleOnLoadUrl: Method={%s}, Url={%s}, Response={%s}"),
		        *Method, *Url, *Response));

	return false;
}

void UCustomWebBrowser::HandleOnLoadStarted()
{
	Logging(TAG, TEXT("HandleOnLoadStarted"));
}

void UCustomWebBrowser::HandleOnLoadCompleted()
{
	Logging(TAG, TEXT("HandleOnLoadCompleted"));
}

void UCustomWebBrowser::BindUObject(const FString& Name, UObject* Object, bool bIsPermanent) const
{
	if (WebBrowserWidget.IsValid())
	{
		WebBrowserWidget->BindUObject(Name, Object, bIsPermanent);
	}
}

void UCustomWebBrowser::SendMessage(const FString& Message)
{
	if (!WebBrowserWidget.IsValid()) return;

	Logging(TAG, FString::Printf(
		        TEXT("window.ue.uewebbrowser.sendmessage(\"%s\")"),
		        *Message));

	if (OnMessageReceived.IsBound())
	{
		if (IsInGameThread())
		{
			OnMessageReceived.Broadcast(Message);
		}
		else
		{
			// Retry on the GameThread.
			TWeakObjectPtr<UCustomWebBrowser> WeakThis = this;
			FFunctionGraphTask::CreateAndDispatchWhenReady([WeakThis, Message]()
			{
				if (WeakThis.IsValid())
				{
					WeakThis->SendMessage(Message);
				}
			}, TStatId(), nullptr, ENamedThreads::GameThread);
		}
	}
}

bool UCustomWebBrowser::HandleOnBeforeBrowse(const FString& Url, const FWebNavigationRequest& WebNavigationRequest)
{
	Logging(TAG, FString::Printf(
		        TEXT("HandleOnBeforeBrowse: Url={%s}"),
		        *Url));

	if (Url.StartsWith(MESSAGING_SCHEME))
	{
		if (OnMessageReceived.IsBound())
		{
			if (IsInGameThread())
			{
				FString Message(Url);
				Message.RemoveFromStart(MESSAGING_SCHEME);
				OnMessageReceived.Broadcast(Message);
			}
			else
			{
				// Retry on the GameThread.
				TWeakObjectPtr<UCustomWebBrowser> WeakThis = this;
				FFunctionGraphTask::CreateAndDispatchWhenReady([WeakThis, Url, WebNavigationRequest]()
				{
					if (WeakThis.IsValid())
					{
						WeakThis->HandleOnBeforeBrowse(Url, WebNavigationRequest);
					}
				}, TStatId(), nullptr, ENamedThreads::GameThread);
			}

			return true;
		}
	}

	return false;
}

#if WITH_EDITOR

const FText UCustomWebBrowser::GetPaletteCategory()
{
	return LOCTEXT("Experimental", "Experimental");
}

#endif

void UCustomWebBrowser::Logging(const FString& Tag, const FString& Msg) const
{
	if (!DEBUGGING) return;

#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		jmethodID Method = FJavaWrapper::FindMethod(
			Env,
			FJavaWrapper::GameActivityClassID,
			"cwbLogDebug",
			"(Ljava/lang/String;Ljava/lang/String;)V",
			false
		);
		jstring JStringTag = Env->NewStringUTF(TCHAR_TO_UTF8(*Tag));
		jstring JStringMsg = Env->NewStringUTF(TCHAR_TO_UTF8(*Msg));
		FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, Method, JStringTag, JStringMsg);
	}
#elif PLATFORM_IOS
	NSLog(@ "[%@] %@", Tag.GetNSString(), Msg.GetNSString());
#else
	UE_LOG(LogTemp, Display, TEXT("[%s] %s"), *Tag, *Msg);
#endif
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
