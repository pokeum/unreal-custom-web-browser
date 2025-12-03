#include "CustomWebBrowserModule.h"

#define LOCTEXT_NAMESPACE "FCustomWebBrowserModule"

void FCustomWebBrowserModule::StartupModule()
{
}

void FCustomWebBrowserModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCustomWebBrowserModule, CustomWebBrowser)
