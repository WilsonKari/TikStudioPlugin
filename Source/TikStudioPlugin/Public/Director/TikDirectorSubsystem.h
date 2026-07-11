// Wilson Karl 2025

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/Engine.h"
#include "Director/TikDirectorTypes.h"
#include "TikStudioErrorLibrary.h"
#include "TikDirectorSubsystem.generated.h"

// Delegates para notificar cambios a Blueprints
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDirectorStateChanged, ETikDirectorState, NewState, float, CurrentActivity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClimaxReached, float, ClimaxThreshold);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFlavorChanged, ETikFlavorType, NewFlavor, float, Confidence);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActivityChanged, float, NewActivity, float, PreviousActivity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHypeChanged, float, NewHypeMultiplier, float, PreviousHypeMultiplier);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCooldownStarted, float, CooldownDuration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCooldownEnded, ETikDirectorState, NewState);

/**
 * Main subsystem for the TikStudio AI Director
 * Migrated from useDirectorLogic.js - handles all Director logic and state management
 */
UCLASS(BlueprintType, Blueprintable)
class TIKSTUDIOPLUGIN_API UTikDirectorSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Process a TikTok event through the AI Director
	 * Main entry point for events from TikFinityPlugin
	 *
	 * @param EventType Type of event (comment, gift, follow, etc.)
	 * @param Username User who generated the event
	 * @param EventData Additional event data (gift value, message, etc.)
	 * @return Error status indicating success or failure
	 */
	UFUNCTION(BlueprintCallable, Category = "TikStudio|Director")
	FTikStudioErrorStatus ProcessTikTokEvent(const FString& EventType, const FString& Username, const TMap<FString, FString>& EventData);

	/**
	 * Alternative event processing using structured event data
	 *
	 * @param Event Structured event data
	 * @return Error status indicating success or failure
	 */
	UFUNCTION(BlueprintCallable, Category = "TikStudio|Director")
	FTikStudioErrorStatus ProcessTikEvent(const FTikEvent& Event);

	/**
	 * Start the AI Director simulation
	 *
	 * @return Error status indicating success or failure
	 */
	UFUNCTION(BlueprintCallable, Category = "TikStudio|Director")
	FTikStudioErrorStatus StartDirector();

	/**
	 * Stop the AI Director simulation
	 *
	 * @return Error status indicating success or failure
	 */
	UFUNCTION(BlueprintCallable, Category = "TikStudio|Director")
	FTikStudioErrorStatus StopDirector();

	/**
	 * Reset the AI Director to initial state
	 *
	 * @return Error status indicating success or failure
	 */
	UFUNCTION(BlueprintCallable, Category = "TikStudio|Director")
	FTikStudioErrorStatus ResetDirector();

	// Getters for current state (Blueprint accessible)
	
	/**
	 * Gets the current AI Director state
	 * @return Current Director state (DORMANT, ACTIVE, INTENSIVE, COOLDOWN, LURKING)
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Director")
	ETikDirectorState GetCurrentState() const { return CurrentStatus.CurrentState; }

	/**
	 * Gets the current activity level of the Director
	 * @return Current activity level as a float value (0.0 and above)
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Director")
	float GetCurrentActivity() const { return CurrentStatus.CurrentActivity; }

	/**
	 * Gets the current hype multiplier affecting event processing
	 * @return Current hype multiplier (1.0 or higher, where 1.0 = no bonus)
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Director")
	float GetHypeMultiplier() const { return CurrentStatus.HypeMultiplier; }

	/**
	 * Gets the current dominant flavor type based on recent event analysis
	 * @return Current flavor type (IDLE, COMMUNITY, GENEROSITY, HYPE, ENGAGEMENT)
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Director")
	ETikFlavorType GetCurrentFlavor() const { return CurrentStatus.CurrentFlavor; }

	/**
	 * Gets the current dynamic climax threshold that adapts to activity levels
	 * @return Current climax threshold value (activity level needed to trigger climax)
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Director")
	float GetCurrentClimaxThreshold() const { return CurrentStatus.CurrentClimaxThreshold; }

	/**
	 * Gets the remaining time in the current cooldown period
	 * @return Remaining cooldown time in seconds (0.0 if not in cooldown)
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Director")
	float GetCooldownRemainingTime() const { return CurrentStatus.CooldownRemainingTime; }

	/**
	 * Gets the remaining time before threshold decay activation
	 * @return Remaining time in seconds before decay starts (0.0 if decay is active)
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Director")
	float GetDecayThresholdRemainingTime() const { return FMath::Max(0.0f, Config.DecayDelayThreshold - TimeInDecayZone); }

	/**
	 * Gets the remaining time in the current lurking state
	 * @return Remaining lurking time in seconds (0.0 if not in lurking state)
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Director")
	float GetLurkingRemainingTime() const 
	{ 
		if (!bIsInLurking || LurkingStartTime <= 0.0f) return 0.0f;
		float Elapsed = FPlatformTime::Seconds() - LurkingStartTime;
		return FMath::Max(0.0f, Config.Lurking.LurkingDuration - Elapsed);
	}

	/**
	 * Gets the remaining time until natural plateau is reached (time since last event)
	 * @return Remaining time in seconds until plateau (0.0 if plateau reached or no events yet)
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Director")
	float GetNaturalPlateauRemainingTime() const 
	{ 
		// If no events have occurred yet, plateau should be 0
		if (LastEventTime < 0.0f) return 0.0f;
		
		float TimeSinceLastEvent = FPlatformTime::Seconds() - LastEventTime;
		return FMath::Max(0.0f, Config.NaturalPlateauTime - TimeSinceLastEvent);
	}

	/**
	 * Gets the configured natural plateau time threshold
	 * @return Natural plateau time configuration value in seconds
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Director")
	float GetNaturalPlateauTime() const { return Config.NaturalPlateauTime; }

	/**
	 * Checks if the Director is currently in lurking state
	 * @return True if currently in lurking state, false otherwise
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Director")
	bool IsInLurking() const { return bIsInLurking; }

	/**
	 * Gets the total number of events processed since Director started
	 * @return Total event count as integer
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Director")
	int32 GetTotalEvents() const { return CurrentStatus.TotalEvents; }

	// Flavor system getters
	
	/**
	 * Gets the configured flavor analysis window duration (static configuration value)
	 * @return Analysis window time in seconds (default: 30.0) - used for debugging/configuration
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Flavor")
	float GetFlavorAnalysisWindow() const { return Config.Flavor.AnalysisWindow; }

	/**
	 * Gets the configured flavor dominance threshold (static configuration value)
	 * @return Dominance threshold as percentage (default: 0.6 = 60%) - used for debugging/configuration
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Flavor")
	float GetFlavorDominanceThreshold() const { return Config.Flavor.DominanceThreshold; }

	/**
	 * Gets the configured flavor analysis frequency (static configuration value)
	 * @return Analysis frequency in seconds (default: 5.0) - used for debugging/configuration
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Flavor")
	float GetFlavorAnalysisFrequency() const { return Config.Flavor.AnalysisFrequency; }

	/**
	 * Gets the configured flavor idle timeout duration (static configuration value)
	 * @return Idle timeout in seconds (default: 60.0) - used for debugging/configuration
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Flavor")
	float GetFlavorIdleTimeout() const { return Config.Flavor.IdleTimeout; }

	/**
	 * Gets the time elapsed since the last event was processed (dynamic value)
	 * @return Time in seconds since last event (-1.0 if no events have occurred yet)
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Flavor")
	float GetTimeSinceLastEvent() const { return FPlatformTime::Seconds() - LastEventTime; }

	/**
	 * Gets the current number of recent events in the analysis window (dynamic value)
	 * @return Number of recent events being tracked for flavor analysis
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Flavor")
	int32 GetRecentEventsCount() const { return RecentEventHistory.Num(); }

	/**
	 * Checks if the current conditions should force a HYPE flavor state (dynamic evaluation)
	 * @return True if conditions warrant forcing HYPE flavor, false otherwise
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Flavor")
	bool ShouldForceHypeCondition() const { return ShouldForceHype(); }

	/**
	 * Checks if the AI Director system is currently running and processing events
	 * @return True if Director is active and running, false if stopped or paused
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Director")
	bool IsRunning() const { return CurrentStatus.bIsRunning; }

	/**
	 * Gets the complete current status structure of the AI Director
	 * @return Full Director status including state, activity, flavor, thresholds, and counters
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Director")
	FTikDirectorStatus GetDirectorStatus() const { return CurrentStatus; }

	// Configuration getters and setters
	/**
	 * Gets the complete configuration structure of the AI Director
	 * @return Full Director configuration including all thresholds, timers, and system settings
	 */
	UFUNCTION(BlueprintPure, Category = "TikStudio|Director")
	FTikDirectorConfig GetDirectorConfig() const { return Config; }

	UFUNCTION(BlueprintCallable, Category = "TikStudio|Director")
	void SetDirectorConfig(const FTikDirectorConfig& NewConfig) { Config = NewConfig; }

	// Blueprint events
	UPROPERTY(BlueprintAssignable, Category = "TikStudio|Director")
	FOnDirectorStateChanged OnDirectorStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "TikStudio|Director")
	FOnClimaxReached OnClimaxReached;

	UPROPERTY(BlueprintAssignable, Category = "TikStudio|Director")
	FOnFlavorChanged OnFlavorChanged;

	UPROPERTY(BlueprintAssignable, Category = "TikStudio|Director")
	FOnActivityChanged OnActivityChanged;

	UPROPERTY(BlueprintAssignable, Category = "TikStudio|Director")
	FOnHypeChanged OnHypeChanged;

	UPROPERTY(BlueprintAssignable, Category = "TikStudio|Director")
	FOnCooldownStarted OnCooldownStarted;

	UPROPERTY(BlueprintAssignable, Category = "TikStudio|Director")
	FOnCooldownEnded OnCooldownEnded;

protected:
	// Current configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TikStudio|Config")
	FTikDirectorConfig Config;

	// Current status
	UPROPERTY(BlueprintReadOnly, Category = "TikStudio|Status")
	FTikDirectorStatus CurrentStatus;

	// Timer handles for various systems
	FTimerHandle DecayTimerHandle;
	FTimerHandle HypeDecayTimerHandle;
	FTimerHandle FlavorAnalysisTimerHandle;
	FTimerHandle CooldownTimerHandle;
	FTimerHandle LurkingTimerHandle;

	// Internal state tracking
	float LastEventTime;
	float CooldownStartTime;
	float LurkingStartTime;
	float TimeInDormant;
	float TimeInDecayZone; // Time spent below decay trigger point
	bool bJustExitedCooldown;
	bool bIsInCooldown;
	bool bIsInLurking;

	// Recent events for flavor analysis
	TArray<FTikEvent> RecentEventHistory;

private:
	// Internal methods
	void InitializeConfig();
	void StartTimers();
	void StopTimers();
	void ProcessDecay();
	void ProcessHypeDecay();
	void ProcessFlavorAnalysis();
	void ProcessCooldown();
	void ProcessLurking();
	
	ETikDirectorState DetermineState(float Activity) const;
	void TransitionToState(ETikDirectorState NewState);
	void UpdateActivity(float ActivityDelta);
	void UpdateHype(float HypeDelta);
	void UpdateClimaxThreshold();
	bool ShouldActivateLurking() const;
	bool ShouldForceHype() const;
	void NotifyStateChange(ETikDirectorState NewState);
	void NotifyActivityChange(float NewActivity, float PreviousActivity);
	void NotifyHypeChange(float NewHype, float PreviousHype);
	void NotifyFlavorChange(ETikFlavorType NewFlavor);
	void CleanupRecentEvents();
};