// Wilson Karl 2025

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TikStudioErrorLibrary.generated.h"

/**
 * Structure representing an error status from TikStudio operations
 * Used to report errors from TikTok event processing and Director operations
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTikStudioErrorStatus
{
	GENERATED_BODY()

	/**
	 * Default constructor initializing with no error
	 */
	FTikStudioErrorStatus()
		: bIsError(false)
	{
	}

	/**
	 * Constructor with error state and message
	 *
	 * @param bIsError Whether an error occurred
	 * @param ErrorMessage Description of the error
	 */
	FTikStudioErrorStatus(bool bInIsError, const FString& InErrorMessage)
		: bIsError(bInIsError)
		, ErrorMessage(InErrorMessage)
	{
	}

	/**
	 * Boolean conversion operator to check error state directly
	 *
	 * @return Whether an error occurred
	 */
	operator bool() const
	{
		return bIsError;
	}

	/** Flag indicating whether an error occurred */
	UPROPERTY(BlueprintReadOnly, Category = "TikStudio|Error")
	bool bIsError;

	/** Descriptive message about the error */
	UPROPERTY(BlueprintReadOnly, Category = "TikStudio|Error")
	FString ErrorMessage;
};

/**
 * Library of utility functions for working with TikStudio errors
 * Provides Blueprint-accessible functions to check and extract error information
 */
UCLASS()
class TIKSTUDIOPLUGIN_API UTikStudioErrorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Checks if an error status contains an error
	 *
	 * @param Error The error status to check
	 * @return True if the error status indicates an error occurred
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Error", meta = (CompactNodeTitle = "Error", BlueprintAutocast))
	static bool IsError(const FTikStudioErrorStatus& Error);

	/**
	 * Gets the textual representation of the error
	 *
	 * @param Error The error status to get the message from
	 * @return String message describing the error
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Error")
	static FString GetErrorMessage(const FTikStudioErrorStatus& Error);

	/**
	 * Creates a success status (no error)
	 *
	 * @return Error status indicating success
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Error")
	static FTikStudioErrorStatus CreateSuccess();

	/**
	 * Creates an error status with a message
	 *
	 * @param ErrorMessage Description of the error that occurred
	 * @return Error status indicating failure with message
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Error")
	static FTikStudioErrorStatus CreateError(const FString& ErrorMessage);
};