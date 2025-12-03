#include "MainMenuWidget.h"

#include "Components/Button.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Widget/CustomWebBrowser.h"

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	this->BindButtons();

	if (CustomWebBrowser != nullptr)
	{
		CustomWebBrowser->OnMessageReceived.AddDynamic(this, &UMainMenuWidget::OnMessageReceived);
	}
}

void UMainMenuWidget::NativeDestruct()
{
	Super::NativeDestruct();
}

void UMainMenuWidget::BindButtons()
{
	if (!ReloadButton->OnClicked.IsBound())
		ReloadButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnClickReloadButton);

	if (!ClearLogButton->OnClicked.IsBound())
		ClearLogButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnClickClearLogButton);
}

void UMainMenuWidget::OnClickReloadButton()
{
	CustomWebBrowser->LoadURL("https://test-github-organization-pokeum.github.io/uewebbrowser/");
}

void UMainMenuWidget::OnClickClearLogButton()
{
	Logger->SetText(FText::FromString(TEXT("")));
}

void UMainMenuWidget::OnMessageReceived(FString Message)
{
	Logger->SetText(FText::FromString(Message));
}
