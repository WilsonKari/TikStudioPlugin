// Wilson Karl 2025

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Director/TikDirectorTypes.h"
#include "Director/TikDirectorSubsystem.h"
#include "TikStudioErrorLibrary.h"
#include "TikStudioBlueprintLibrary.generated.h"

/**
 * Blueprint function library for easy access to TikStudio functionality
 * Provides convenient wrapper functions for common operations
 */
UCLASS()
class TIKSTUDIOPLUGIN_API UTikStudioBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Get the TikDirector subsystem instance
	 *
	 * @param WorldContext World context for subsystem lookup
	 * @return TikDirector subsystem instance or nullptr if not available
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Director", CallInEditor, meta = (WorldContext = "WorldContext"))
	static UTikDirectorSubsystem* GetTikDirectorSubsystem(const UObject* WorldContext);

	/**
	 * Quick function to process a simple comment event
	 * Only counts the occurrence of the comment for AI Director
	 *
	 * @param WorldContext World context for subsystem lookup
	 * @return Error status indicating success or failure
	 */
	UFUNCTION(BlueprintCallable, Category = "TikStudio|Events", meta = (WorldContext = "WorldContext"))
	static FTikStudioErrorStatus ProcessCommentEvent(const UObject* WorldContext);

	/**
	 * Quick function to process a gift event with proper streak handling
	 * Handles TikTok gift streaks by ignoring closure events (RepeatEnd=true)
	 * Only processes the actual gift value from real events
	 *
	 * @param WorldContext World context for subsystem lookup
	 * @param DiamondCount Diamond cost of the individual gift
	 * @param bRepeatEnd Whether this is a streak closure event (ignored if true)
	 * @return Error status indicating success or failure
	 */
	UFUNCTION(BlueprintCallable, Category = "TikStudio|Events", meta = (WorldContext = "WorldContext"))
	static FTikStudioErrorStatus ProcessGiftEvent(const UObject* WorldContext, int32 DiamondCount, bool bRepeatEnd);

	/**
	 * Quick function to process a follow event
	 * Only counts the occurrence of the follow for AI Director
	 *
	 * @param WorldContext World context for subsystem lookup
	 * @return Error status indicating success or failure
	 */
	UFUNCTION(BlueprintCallable, Category = "TikStudio|Events", meta = (WorldContext = "WorldContext"))
	static FTikStudioErrorStatus ProcessFollowEvent(const UObject* WorldContext);

	/**
	 * Quick function to process a like event with proper count handling
	 * Handles multiple likes in a single event correctly
	 *
	 * @param WorldContext World context for subsystem lookup
	 * @param LikeCount Number of likes in this event
	 * @return Error status indicating success or failure
	 */
	UFUNCTION(BlueprintCallable, Category = "TikStudio|Events", meta = (WorldContext = "WorldContext"))
	static FTikStudioErrorStatus ProcessLikeEvent(const UObject* WorldContext, int32 LikeCount);

	/**
	 * Quick function to process a share event
	 * Only counts the occurrence of the share for AI Director
	 *
	 * @param WorldContext World context for subsystem lookup
	 * @return Error status indicating success or failure
	 */
	UFUNCTION(BlueprintCallable, Category = "TikStudio|Events", meta = (WorldContext = "WorldContext"))
	static FTikStudioErrorStatus ProcessShareEvent(const UObject* WorldContext);

	/**
	 * Get current director status as readable text
	 *
	 * @param WorldContext World context for subsystem lookup
	 * @return Formatted status string
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Director", meta = (WorldContext = "WorldContext"))
	static FString GetDirectorStatusText(const UObject* WorldContext);

	/**
	 * Check if the director is in a high activity state (INTENSIVE or COOLDOWN)
	 *
	 * @param WorldContext World context for subsystem lookup
	 * @return True if in high activity state
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Director", meta = (WorldContext = "WorldContext"))
	static bool IsInHighActivityState(const UObject* WorldContext);

	/**
	 * Check if the director is in cooldown
	 *
	 * @param WorldContext World context for subsystem lookup
	 * @return True if in cooldown state
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Director", meta = (WorldContext = "WorldContext"))
	static bool IsInCooldown(const UObject* WorldContext);

	/**
	 * Get activity level as a percentage (0-100) based on current climax threshold
	 *
	 * @param WorldContext World context for subsystem lookup
	 * @return Activity percentage
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Director", meta = (WorldContext = "WorldContext"))
	static float GetActivityPercentage(const UObject* WorldContext);

	/**
	 * Create a default TikDirector configuration
	 *
	 * @return Default configuration struct
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Config")
	static FTikDirectorConfig CreateDefaultConfig();
};