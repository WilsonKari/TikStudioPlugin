// Wilson Karl 2025

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "TikDirectorTypes.generated.h"

/**
 * Possible AI Director states
 * Migrated from DIRECTOR_STATES in constants.js
 */
UENUM(BlueprintType)
enum class ETikDirectorState : uint8
{
	DORMANT     UMETA(DisplayName = "Dormant"),
	ACTIVE      UMETA(DisplayName = "Active"),
	INTENSIVE   UMETA(DisplayName = "Intensive"),
	COOLDOWN    UMETA(DisplayName = "Cooldown"),
	LURKING     UMETA(DisplayName = "Lurking")
};

/**
 * Activity-based flavor/context types
 * Migrated from FLAVOR_TYPES in constants.js
 */
UENUM(BlueprintType)
enum class ETikFlavorType : uint8
{
	IDLE        UMETA(DisplayName = "Idle"),
	COMMUNITY   UMETA(DisplayName = "Community"),
	GENEROSITY  UMETA(DisplayName = "Generosity"),
	HYPE        UMETA(DisplayName = "Hype"),
	ENGAGEMENT  UMETA(DisplayName = "Engagement")
};

/**
 * Supported event types
 * Migrated from EVENT_TYPES in constants.js
 */
UENUM(BlueprintType)
enum class ETikEventType : uint8
{
	COMMENT     UMETA(DisplayName = "Comment"),
	FOLLOW      UMETA(DisplayName = "Follow"),
	GIFT        UMETA(DisplayName = "Gift"),
	LIKE        UMETA(DisplayName = "Like"),
	SHARE       UMETA(DisplayName = "Share")
};

/**
 * Basic event structure for tracking
 * Migrated from event logic in useDirectorLogic.js
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTikEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Event")
	ETikEventType EventType = ETikEventType::COMMENT;

	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Event")
	float Timestamp = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Event")
	float Score = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Event")
	FString Username;

	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Event")
	TMap<FString, FString> EventData;
};

/**
 * Weight configuration for different event types
 * Migrated from weights in DEFAULT_CONFIG from constants.js
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTikEventWeights
{
	GENERATED_BODY()

	/** Weight for comment events - Higher values make comments increase director activity more */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	float Comment = 2.0f;

	/** Weight for follow events - Higher values make follows increase director activity more */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	float Follow = 3.0f;

	/** Weight for gift events - Higher values make gifts increase director activity more */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	float Gift = 1.0f;

	/** Weight for like events - Higher values make likes increase director activity more */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	float Like = 1.0f;

	/** Weight for share events - Higher values make shares increase director activity more */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	float Share = 4.0f;
};

/**
 * Threshold configuration for state transitions
 * Migrated from thresholds in DEFAULT_CONFIG from constants.js
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTikStateThresholds
{
	GENERATED_BODY()

	/** Activity threshold to transition from DORMANT to ACTIVE - Lower values make it activate more easily */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	float DormantToActive = 10.0f;

	/** Activity threshold to transition from ACTIVE to INTENSIVE - Lower values make it intensify more easily */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	float ActiveToIntensive = 30.0f;

	/** Activity threshold to reach climax - Lower values make climax occur more frequently */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	float Climax = 50.0f;
};

/**
 * Decay rate configuration per state
 * Migrated from decayRates in DEFAULT_CONFIG from constants.js
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTikDecayRates
{
	GENERATED_BODY()

	/** Decay rate in DORMANT state - Multiplier (0-1) where lower values make activity decay faster */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	float Dormant = 0.90f;

	/** Decay rate in ACTIVE state - Multiplier (0-1) where lower values make activity decay faster */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	float Active = 0.95f;

	/** Decay rate in INTENSIVE state - Multiplier (0-1) where lower values make activity decay faster */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	float Intensive = 0.98f;
};

/**
 * Hype system configuration
 * Migrated from hype in DEFAULT_CONFIG from constants.js
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTikHypeConfig
{
	GENERATED_BODY()

	/** Hype multiplier increment per event - Higher values create hype more quickly */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	float Increment = 0.05f;

	/** Hype decay rate - Multiplier (0-1) where lower values make hype decay faster */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	float Decay = 0.90f;

	/** Maximum hype multiplier value - Higher values allow greater event amplification */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	float Max = 3.0f;
};

/**
 * Flavor/context analysis configuration
 * Migrated from flavor in DEFAULT_CONFIG from constants.js
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTikFlavorConfig
{
	GENERATED_BODY()

	/** Time window for flavor analysis in seconds - Longer windows give more stable analysis */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	float AnalysisWindow = 10.0f;

	/** Dominance threshold for flavor change (0-1) - Higher values require greater dominance */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	float DominanceThreshold = 0.6f;

	/** Flavor analysis frequency in seconds - Lower values give faster response but use more resources */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	float AnalysisFrequency = 2.0f;

	/** Time without events to consider IDLE in seconds - Lower values change to IDLE faster */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	float IdleTimeout = 15.0f;
};

/**
 * LURKING state configuration
 * Director's suspense and tense calm system
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTikLurkingConfig
{
	GENERATED_BODY()

	/** Probability of activating LURKING after cooldown (0.0-1.0) - Higher values create more suspenseful moments */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	float LurkingChance = 0.5f;

	/** Duration of LURKING state in seconds - Longer periods create more tension before reactivation */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	float LurkingDuration = 15.0f;
};

/**
 * Complete AI Director configuration
 * Migrated from DEFAULT_CONFIG from constants.js
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTikDirectorConfig
{
	GENERATED_BODY()

	/** Weights for different event types */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	FTikEventWeights Weights;

	/** Thresholds for state transitions */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	FTikStateThresholds Thresholds;

	/** Decay rates per state */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	FTikDecayRates DecayRates;

	/** Hype system configuration */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	FTikHypeConfig Hype;

	/** Flavor analysis configuration */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	FTikFlavorConfig Flavor;

	/** LURKING state configuration */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	FTikLurkingConfig Lurking;

	/** Cooldown duration in seconds - Longer periods give more rest between activity peaks */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	float CooldownDuration = 15.0f;

	/** Natural plateau time in seconds - Time to maintain at climax level before starting natural decay */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	float NaturalPlateauTime = 5.0f;

	/** Minimum time to confirm climax decay in seconds - Prevents rapid fluctuations */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	float DecayDelayThreshold = 20.0f;

	/** Percentage of base climax to activate decay (0.1 = 10%) - Lower values maintain climax longer */
	UPROPERTY(BlueprintReadWrite, Category = "TikStudio|Config")
	float ClimaxThresholdPercent = 0.1f;
};

/**
 * Current AI Director status
 * Contains all metrics and current system state
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTikDirectorStatus
{
	GENERATED_BODY()

	/** Current director state */
	UPROPERTY(BlueprintReadOnly, Category = "TikStudio|Status")
	ETikDirectorState CurrentState = ETikDirectorState::DORMANT;

	/** Current activity level */
	UPROPERTY(BlueprintReadOnly, Category = "TikStudio|Status")
	float CurrentActivity = 0.0f;

	/** Current hype multiplier */
	UPROPERTY(BlueprintReadOnly, Category = "TikStudio|Status")
	float HypeMultiplier = 1.0f;

	/** Current activity flavor/context */
	UPROPERTY(BlueprintReadOnly, Category = "TikStudio|Status")
	ETikFlavorType CurrentFlavor = ETikFlavorType::IDLE;

	/** Current climax threshold (dynamic) */
	UPROPERTY(BlueprintReadOnly, Category = "TikStudio|Status")
	float CurrentClimaxThreshold = 50.0f;

	/** Remaining cooldown time (0 if not in cooldown) */
	UPROPERTY(BlueprintReadOnly, Category = "TikStudio|Status")
	float CooldownRemainingTime = 0.0f;

	/** Total events processed */
	UPROPERTY(BlueprintReadOnly, Category = "TikStudio|Status")
	int32 TotalEvents = 0;

	/** Whether the director is running */
	UPROPERTY(BlueprintReadOnly, Category = "TikStudio|Status")
	bool bIsRunning = false;
};