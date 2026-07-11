// Wilson Karl 2025

#include "TikStudioErrorLibrary.h"

bool UTikStudioErrorLibrary::IsError(const FTikStudioErrorStatus& Error)
{
	return Error.bIsError;
}

FString UTikStudioErrorLibrary::GetErrorMessage(const FTikStudioErrorStatus& Error)
{
	return Error.ErrorMessage;
}

FTikStudioErrorStatus UTikStudioErrorLibrary::CreateSuccess()
{
	return FTikStudioErrorStatus(false, TEXT(""));
}

FTikStudioErrorStatus UTikStudioErrorLibrary::CreateError(const FString& ErrorMessage)
{
	return FTikStudioErrorStatus(true, ErrorMessage);
}