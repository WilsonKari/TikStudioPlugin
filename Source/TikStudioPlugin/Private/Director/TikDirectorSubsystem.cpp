// Wilson Karl 2025

#include "Director/TikDirectorSubsystem.h"
#include "Utils/TikHelpers.h"
#include "TikStudioErrorLibrary.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UTikDirectorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	// Initialize configuration with default values
	InitializeConfig();
	
	// Initialize status
	CurrentStatus = FTikDirectorStatus();
	
	// Initialize internal state
	LastEventTime = -1.0f; // -1 indicates no events have occurred yet
	CooldownStartTime = 0.0f;
	LurkingStartTime = 0.0f;
	TimeInDormant = 0.0f;
	TimeInDecayZone = 0.0f;
	bJustExitedCooldown = false;
	bIsInCooldown = false;
	bIsInLurking = false;
	
	UE_LOG(LogTemp, Log, TEXT("TikDirectorSubsystem initialized"));
}

void UTikDirectorSubsystem::Deinitialize()
{
	StopDirector();
	Super::Deinitialize();
	
	UE_LOG(LogTemp, Log, TEXT("TikDirectorSubsystem deinitialized"));
}

void UTikDirectorSubsystem::InitializeConfig()
{
	// Initialize with default values from constants.js
	Config = FTikDirectorConfig();
	
	// Set current climax threshold to base threshold
	CurrentStatus.CurrentClimaxThreshold = Config.Thresholds.Climax;
}

FTikStudioErrorStatus UTikDirectorSubsystem::ProcessTikTokEvent(const FString& EventType, const FString& Username, const TMap<FString, FString>& EventData)
{
	if (!CurrentStatus.bIsRunning)
	{
		return UTikStudioErrorLibrary::CreateError(TEXT("Director is not running. Call StartDirector() first."));
	}

	// Convert string to event type
	ETikEventType EventTypeEnum = UTikHelpers::StringToEventType(EventType);
	
	// Create structured event
	FTikEvent Event;
	Event.EventType = EventTypeEnum;
	Event.Username = Username;
	Event.EventData = EventData;
	Event.Timestamp = FPlatformTime::Seconds();
	Event.Score = 0.0f; // Se calculará en ProcessTikEvent
	
	return ProcessTikEvent(Event);
}

FTikStudioErrorStatus UTikDirectorSubsystem::ProcessTikEvent(const FTikEvent& Event)
{
	if (!CurrentStatus.bIsRunning)
	{
		return UTikStudioErrorLibrary::CreateError(TEXT("Director is not running. Call StartDirector() first."));
	}

	// Calculate event score
	float EventScore = UTikHelpers::CalculateEventScore(Event, Config.Weights, CurrentStatus.HypeMultiplier);
	
	// Update activity
	float PreviousActivity = CurrentStatus.CurrentActivity;
	UpdateActivity(EventScore);
	
	// Update hype
	float PreviousHype = CurrentStatus.HypeMultiplier;
	UpdateHype(Config.Hype.Increment);
	
	// Update last event time
	LastEventTime = FPlatformTime::Seconds();
	
	// Increment total events
	CurrentStatus.TotalEvents++;
	
	// Add to recent events for flavor analysis
	// CRITICAL FIX: Preserve the calculated EventScore for proper flavor analysis
	// Previously, the calculated score was lost and events stored Score = 0.0f
	FTikEvent EventWithScore = Event;
	EventWithScore.Timestamp = LastEventTime;
	EventWithScore.Score = EventScore;  // Preserve calculated score with weights and hype multiplier
	RecentEventHistory.Add(EventWithScore);
	
	UE_LOG(LogTemp, Log, TEXT("Event processed - Activity: %f, Threshold: %f, State: %s"), 
		CurrentStatus.CurrentActivity, 
		CurrentStatus.CurrentClimaxThreshold,
		*UTikHelpers::DirectorStateToString(CurrentStatus.CurrentState));
	
	// Check for state transitions - Migrado de useDirectorLogic.js líneas 178-201
	// Solo SKIP evaluación si YA estamos EN cooldown, pero SÍ evaluar para ENTRAR a cooldown
	if (CurrentStatus.CurrentState == ETikDirectorState::COOLDOWN || bIsInCooldown)
	{
		// Durante cooldown, solo actualizar gráfico, no evaluar estados
		UE_LOG(LogTemp, VeryVerbose, TEXT("Skipping state evaluation - in cooldown"));
	}
	else
	{
		// CRITICAL: Use CURRENT threshold for state evaluation (BEFORE updating it)
		ETikDirectorState NewState = DetermineState(CurrentStatus.CurrentActivity);
		
		// Transición normal a un nuevo estado (no Cooldown)
		if (NewState != CurrentStatus.CurrentState && NewState != ETikDirectorState::COOLDOWN)
		{
			UE_LOG(LogTemp, Log, TEXT("State transition from %s to %s"), 
				*UTikHelpers::DirectorStateToString(CurrentStatus.CurrentState),
				*UTikHelpers::DirectorStateToString(NewState));
			TransitionToState(NewState);
		}
		// Se alcanza el umbral de clímax - ENTRADA A COOLDOWN
		else if (NewState == ETikDirectorState::COOLDOWN)
		{
			UE_LOG(LogTemp, Log, TEXT("Climax reached! Entering cooldown (Activity: %f, Threshold: %f)"), 
				CurrentStatus.CurrentActivity, CurrentStatus.CurrentClimaxThreshold);
			
			// CRITICAL: Set state FIRST, then flags (matching prototipo web pattern)
			TransitionToState(ETikDirectorState::COOLDOWN);
			
			// Mark flags AFTER state transition (siguiendo patrón del prototipo web)
			bIsInCooldown = true;
			CooldownStartTime = FPlatformTime::Seconds();
			CurrentStatus.CooldownRemainingTime = Config.CooldownDuration;
			
			// Broadcast events
			OnClimaxReached.Broadcast(CurrentStatus.CurrentClimaxThreshold);
			OnCooldownStarted.Broadcast(Config.CooldownDuration);
		}
	}
	
	// CRITICAL: Update climax threshold AFTER evaluating states (migrado de useDirectorLogic.js)
	UpdateClimaxThreshold();
	
	// Notify activity and hype changes
	if (PreviousActivity != CurrentStatus.CurrentActivity)
	{
		NotifyActivityChange(CurrentStatus.CurrentActivity, PreviousActivity);
	}
	
	if (PreviousHype != CurrentStatus.HypeMultiplier)
	{
		NotifyHypeChange(CurrentStatus.HypeMultiplier, PreviousHype);
	}
	
	UE_LOG(LogTemp, VeryVerbose, TEXT("Processed TikTok event: %s, Score: %f, Activity: %f"), 
		*UTikHelpers::EventTypeToString(Event.EventType), EventScore, CurrentStatus.CurrentActivity);
	
	return UTikStudioErrorLibrary::CreateSuccess();
}

FTikStudioErrorStatus UTikDirectorSubsystem::StartDirector()
{
	if (CurrentStatus.bIsRunning)
	{
		return UTikStudioErrorLibrary::CreateError(TEXT("Director is already running"));
	}

	// Reset to initial state
	CurrentStatus = FTikDirectorStatus();
	CurrentStatus.CurrentClimaxThreshold = Config.Thresholds.Climax;
	CurrentStatus.bIsRunning = true;
	
	// Reset internal state
	LastEventTime = -1.0f; // Reset to no events state
	CooldownStartTime = 0.0f;
	TimeInDormant = 0.0f;
	TimeInDecayZone = 0.0f;
	bJustExitedCooldown = false;
	bIsInCooldown = false;
	
	// Clear recent events
	RecentEventHistory.Empty();
	
	// Start timers
	StartTimers();
	
	UE_LOG(LogTemp, Log, TEXT("TikDirector started"));
	
	return UTikStudioErrorLibrary::CreateSuccess();
}

FTikStudioErrorStatus UTikDirectorSubsystem::StopDirector()
{
	if (!CurrentStatus.bIsRunning)
	{
		return UTikStudioErrorLibrary::CreateError(TEXT("Director is not running"));
	}

	CurrentStatus.bIsRunning = false;
	
	// Stop all timers
	StopTimers();
	
	UE_LOG(LogTemp, Log, TEXT("TikDirector stopped"));
	
	return UTikStudioErrorLibrary::CreateSuccess();
}

FTikStudioErrorStatus UTikDirectorSubsystem::ResetDirector()
{
	// Stop if running
	if (CurrentStatus.bIsRunning)
	{
		StopDirector();
	}
	
	// Reset to default configuration
	InitializeConfig();
	
	UE_LOG(LogTemp, Log, TEXT("TikDirector reset"));
	
	return UTikStudioErrorLibrary::CreateSuccess();
}

void UTikDirectorSubsystem::StartTimers()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FTimerManager& TimerManager = World->GetTimerManager();
	
	// Decay timer (every second)
	TimerManager.SetTimer(DecayTimerHandle, this, &UTikDirectorSubsystem::ProcessDecay, 1.0f, true);
	
	// Hype decay timer (every second)
	TimerManager.SetTimer(HypeDecayTimerHandle, this, &UTikDirectorSubsystem::ProcessHypeDecay, 1.0f, true);
	
	// Flavor analysis timer (configurable frequency)
	TimerManager.SetTimer(FlavorAnalysisTimerHandle, this, &UTikDirectorSubsystem::ProcessFlavorAnalysis, Config.Flavor.AnalysisFrequency, true);
	
	// Cooldown timer (every 0.25 seconds for smooth updates)
	TimerManager.SetTimer(CooldownTimerHandle, this, &UTikDirectorSubsystem::ProcessCooldown, 0.25f, true);
	
	// Lurking timer (every 0.25 seconds for responsive interruption detection)
	TimerManager.SetTimer(LurkingTimerHandle, this, &UTikDirectorSubsystem::ProcessLurking, 0.25f, true);
}

void UTikDirectorSubsystem::StopTimers()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FTimerManager& TimerManager = World->GetTimerManager();
	
	TimerManager.ClearTimer(DecayTimerHandle);
	TimerManager.ClearTimer(HypeDecayTimerHandle);
	TimerManager.ClearTimer(FlavorAnalysisTimerHandle);
	TimerManager.ClearTimer(CooldownTimerHandle);
	TimerManager.ClearTimer(LurkingTimerHandle);
}