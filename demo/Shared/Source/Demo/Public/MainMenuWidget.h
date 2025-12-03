#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UMultiLineEditableTextBox;
class UButton;
class UCustomWebBrowser;

UCLASS()
class DEMO_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UButton* ReloadButton;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UButton* ClearLogButton;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UMultiLineEditableTextBox* Logger;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UCustomWebBrowser* CustomWebBrowser;

private:
	void BindButtons();

	UFUNCTION()
	void OnClickReloadButton();

	UFUNCTION()
	void OnClickClearLogButton();

	UFUNCTION()
	void OnMessageReceived(FString Message);
};
