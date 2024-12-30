# Custom Web Browser

Created a `Custom Web Browser` plugin by extending the `Web Browser Widget` provided by Unreal Engine.

## Integrating the Plugin

1. Copy [`demo/Plugins/CustomWebBrowser`](demo/Plugins/CustomWebBrowser) into your own app's Plugin folder.

2. Copy the `CustomWebBrowser` folder into it. You should have something like:
   
   ```
   MyProject
      ├── Plugins
            └── CustomWebBrowser
                  └── CustomWebBrowser.uplugin
   ```

3. If using a C++ Project, configure your module to reference the CustomWebBrowser module. In your *.Build.cs file, add **"CustomWebBrowser"** to your PublicDependencyModuleNames. Ex:
   
    ```csharp
    PublicDependencyModuleNames.AddRange(new string[] { ... , "CustomWebBrowser" });
    ```

4. In a Blueprint Project, enable the plugin by first clicking **Settings** > **Plugins**, scrolling down to Project section of plugins, and clicking **Widgets** > **Custom Web Browser**. From there you can tick the Enabled checkbox:

    | <img src="document/image/Integrating_Plugin_Blueprint.png"  width="800"> |
    | -- |

<br/>

## Messaging System

### Send and Receive

You can send a message from the web view to `UCustomWebBrowserWidget`. It helps you implement a way to "control" your game from the web page.

By using the **HandleOnBeforeBrowse** method, Custom Web Browser inspects all links starts from `uewebbrowser://` by default. That means if your user taps a link starting with **"uewebbrowser"**, an OnMessageReceived event will be raised with an URL string as its parameter.

If you have this on your page:

```html
<a href="uewebbrowser://action?key=value&anotherKey=anotherValue">Tap Me</a>
```

And you have this event listener in Unreal Engine:

- _WebViewWidget.h_
  ```cpp
  #pragma once
  
  #include "Blueprint/UserWidget.h"
  #include "WebViewWidget.generated.h"
  
  class UCustomWebBrowserWidget;
  
  UCLASS()
  class UWebViewWidget : public UUserWidget
  {
    GENERATED_BODY()
  	
  protected:
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UCustomWebBrowserWidget* CustomWebBrowserWidget;
  
  private:
    UFUNCTION()
    void OnMessageReceived(const FString& Message);
  };
  ```
- _WebViewWidget.cpp_
  ```cpp
  #include "CustomWebBrowser/Public/Widget/CustomWebBrowserWidget.h"

  if (CustomWebBrowserWidget != nullptr)
  {
    CustomWebBrowserWidget->OnMessageReceived.AddDynamic(this, &UWebViewWidget::OnMessageReceived);
  }

  void UWebViewWidget::OnMessageReceived(const FString& Message)
  {
    UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
  }
  ```

When the link is tapped, it prints:

> uewebbrowser://action?key=value&anotherKey=anotherValue

#### NAVIGATION
Besides of an HTML link, a `location.href` JavaScript will also send a message to UCustomWebBrowserWidget and trigger the event. The code below is identical to example above:

```js
location.href = "uewebbrowser://action?key=value&anotherKey=anotherValue";
```

## Reference

https://docs.uniwebview.com/guide/messaging-system.html
