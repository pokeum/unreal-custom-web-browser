#pragma once

/**
 * Custom Web Browser Module
 */
class FCustomWebBrowser : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
