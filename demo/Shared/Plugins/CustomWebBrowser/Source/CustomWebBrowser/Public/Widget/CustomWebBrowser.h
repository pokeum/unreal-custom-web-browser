#pragma once

#include "IWebBrowserWindow.h"
#include "Components/Widget.h"

#include "CustomWebBrowser.generated.h"

#define BIND_UOBJECT_NAME	TEXT("uewebbrowser")
#define MESSAGING_SCHEME	TEXT("uewebbrowser://")

DECLARE_DELEGATE_RetVal_ThreeParams(bool, FOnLoadUrl,
                                    const FString& /*Method*/, const FString& /*Url*/, FString& /* Response */)

DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnBeforeBrowse,
                                  const FString& /*Url*/, const FWebNavigationRequest& /*Request*/)

/**
 * UMG Widget Class
 */
UCLASS()
class CUSTOMWEBBROWSER_API UCustomWebBrowser : public UWidget
{
	GENERATED_UCLASS_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUrlChanged, const FText&, Text);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBeforePopup, FString, URL, FString, Frame);
	
	/**
	 * Load the specified URL
	 *
	 * @param NewURL New URL to load
	 */
	UFUNCTION(BlueprintCallable, Category="Custom Web Browser")
	void LoadURL(FString NewURL);

	/**
	 * Load a string as data to create a web page
	 *
	 * @param Contents String to load
	 * @param DummyURL Dummy URL for the page
	 */
	UFUNCTION(BlueprintCallable, Category="Custom Web Browser")
	void LoadString(FString Contents, FString DummyURL);

	/**
	* Executes a JavaScript string in the context of the web page
	*
	* @param ScriptText JavaScript string to execute
	*/
	UFUNCTION(BlueprintCallable, Category = "Custom Web Browser")
	void ExecuteJavascript(const FString& ScriptText);

	/**
	 * Get the current title of the web page
	 */
	UFUNCTION(BlueprintCallable, Category="Custom Web Browser")
	FText GetTitleText() const;

	/**
	* Gets the currently loaded URL.
	*
	* @return The URL, or empty string if no document is loaded.
	*/
	UFUNCTION(BlueprintCallable, Category = "Custom Web Browser")
	FString GetUrl() const;

	/** Called when the Url changes. */
	UPROPERTY(BlueprintAssignable, Category = "Custom Web Browser|Event")
	FOnUrlChanged OnUrlChanged;

	/** Called when a popup is about to spawn. */
	UPROPERTY(BlueprintAssignable, Category = "Custom Web Browser|Event")
	FOnBeforePopup OnBeforePopup;
	
public:
	//~ Begin UWidget interface
	virtual void SynchronizeProperties() override;
	// End UWidget interface

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:
	/** URL that the browser will initially navigate to. The URL should include the protocol, eg http:// */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FString InitialURL;

	/** Should the browser window support transparency. */
	UPROPERTY(EditAnywhere, Category=Appearance)
	bool bSupportsTransparency;

protected:
	// https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/WebBrowser/SWebBrowser
	TSharedPtr<class SWebBrowser> WebBrowserWidget;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface
	
	void HandleOnUrlChanged(const FText& Text);
	bool HandleOnBeforePopup(FString URL, FString Frame);

	//////////////////////////////////////////////////////////////////////////

protected:
	/** Called to allow user code to override the contents of a Url. */
	bool HandleOnLoadUrl(const FString& Method, const FString& Url, FString& Response);

	/** Called when document loading started. */
	void HandleOnLoadStarted();

	/** Called when document loading completed. */
	void HandleOnLoadCompleted();

	//////////////////////////////////////////////////////////////////////////
	/// Web Browser to Unreal Engine - Messaging System

public:
	void BindUObject(const FString& Name, UObject* Object, bool bIsPermanent = true) const;

	UFUNCTION()
	void SendMessage(const FString& Message);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMessageReceived, FString, Message);

	/** Called when a message is received. */
	UPROPERTY(BlueprintAssignable, Category = "Custom Web Browser|Event")
	FOnMessageReceived OnMessageReceived;

protected:
	/** Called prior to browser navigation. */
	bool HandleOnBeforeBrowse(const FString& Url, const FWebNavigationRequest& WebNavigationRequest);

private:
	void Logging(const FString& Tag, const FString& Msg) const;
};
