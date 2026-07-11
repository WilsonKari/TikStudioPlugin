// Wilson Karl 2025

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Director/TikDirectorTypes.h"
#include "TikHelpers.generated.h"

/**
 * Library of utility functions for TikStudio calculations and helpers
 * Migrated from helpers.js in the web prototype
 */
UCLASS()
class TIKSTUDIOPLUGIN_API UTikHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Calculates the score of an event based on its type and data
	 * Migrated from calculateEventScore in helpers.js
	 *
	 * @param Event Event to process
	 * @param Weights Configured weights for each event type
	 * @param HypeMultiplier Current hype multiplier
	 * @return Calculated score
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Helpers")
	static float CalculateEventScore(const FTikEvent& Event, const FTikEventWeights& Weights, float HypeMultiplier);

	/**
	 * Determines the dominant flavor based on recent events
	 * Migrated from determineDominantFlavor in helpers.js
	 *
	 * @param RecentEvents Array of recent events with their scores
	 * @param DominanceThreshold Threshold for dominance (0-1)
	 * @return Dominant flavor type
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Helpers")
	static ETikFlavorType DetermineDominantFlavor(const TArray<FTikEvent>& RecentEvents, float DominanceThreshold);

	/**
	 * Gets the color associated with a director state
	 * Migrated from getStateColor in helpers.js
	 *
	 * @param State Director state
	 * @return Color as linear color
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Helpers")
	static FLinearColor GetStateColor(ETikDirectorState State);

	/**
	 * Gets the color associated with a flavor type
	 * Migrated from getFlavorInfo in helpers.js
	 *
	 * @param Flavor Flavor type
	 * @return Color as linear color
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Helpers")
	static FLinearColor GetFlavorColor(ETikFlavorType Flavor);

	/**
	 * Formats a timestamp for display
	 * Migrated from formatTime in helpers.js
	 *
	 * @param Timestamp Timestamp in seconds
	 * @return Formatted time string
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Helpers")
	static FString FormatTime(float Timestamp);

	/**
	 * Converts ETikEventType to string
	 *
	 * @param EventType Event type enum
	 * @return String representation
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Helpers")
	static FString EventTypeToString(ETikEventType EventType);

	/**
	 * Converts string to ETikEventType
	 *
	 * @param EventTypeString String representation
	 * @return Event type enum
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Helpers")
	static ETikEventType StringToEventType(const FString& EventTypeString);

	/**
	 * Converts ETikDirectorState to string
	 *
	 * @param State Director state enum
	 * @return String representation
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Helpers")
	static FString DirectorStateToString(ETikDirectorState State);

	/**
	 * Converts ETikFlavorType to string
	 *
	 * @param Flavor Flavor type enum
	 * @return String representation
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Helpers")
	static FString FlavorTypeToString(ETikFlavorType Flavor);

	/**
	 * Gets the weight for a specific event type from weights config
	 *
	 * @param EventType Type of event
	 * @param Weights Weight configuration
	 * @return Weight value for the event type
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Helpers")
	static float GetEventWeight(ETikEventType EventType, const FTikEventWeights& Weights);

	/**
	 * Gets the decay rate for a specific director state
	 *
	 * @param State Director state
	 * @param DecayRates Decay rates configuration
	 * @return Decay rate for the state
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Helpers")
	static float GetDecayRate(ETikDirectorState State, const FTikDecayRates& DecayRates);
};