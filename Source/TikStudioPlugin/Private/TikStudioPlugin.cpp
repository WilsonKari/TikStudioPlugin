// Wilson Karl 2025

#include "TikStudioPlugin.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FTikStudioPluginModule"

void FTikStudioPluginModule::StartupModule()
{
	// This code will execute after your module is loaded into memory
	UE_LOG(LogTemp, Log, TEXT("TikStudioPlugin module started"));
}

void FTikStudioPluginModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module
	UE_LOG(LogTemp, Log, TEXT("TikStudioPlugin module shutdown"));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTikStudioPluginModule, TikStudioPlugin)