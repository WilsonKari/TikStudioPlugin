// Wilson Karl 2025
// Continuation of TikDirectorSubsystem.cpp - Core Logic Methods
// This file contains the core processing methods migrated from useDirectorLogic.js

#include "Director/TikDirectorSubsystem.h"
#include "Utils/TikHelpers.h"
#include "TikStudioErrorLibrary.h"

void UTikDirectorSubsystem::ProcessDecay()
{
	if (!CurrentStatus.bIsRunning)
	{
		return;
	}

	// NO permitir decay durante COOLDOWN y LURKING - mantener actividad estable
	if (CurrentStatus.CurrentState == ETikDirectorState::LURKING || CurrentStatus.CurrentState == ETikDirectorState::COOLDOWN)
	{
		return;
	}

	// Handle intelligent climax threshold decay (floor-based system)
	// Process climax decay based ONLY on activity, independent of state
	{
		// Process intelligent climax decay by floors - NUEVO SISTEMA DE PISOS
		float CurrentThreshold = CurrentStatus.CurrentClimaxThreshold;
		float ClimaxBase = Config.Thresholds.Climax; // 50
		
		// Are we at base floor? → NO DECAY POSSIBLE
		if (CurrentThreshold <= ClimaxBase) 
		{
			TimeInDecayZone = 0.0f; // Reset tracking
		}
		else
		{
			// Calculate decay trigger point - BAJAR UN PISO COMPLETO + BUFFER
			// Si Threshold = 150 (piso 3), decaer cuando Activity < 55 (piso 1 + buffer)
			// Si Threshold = 200 (piso 4), decaer cuando Activity < 105 (piso 2 + buffer) 
			// Si Threshold = 250 (piso 5), decaer cuando Activity < 155 (piso 3 + buffer)
			
			float PreviousFloorBase = CurrentThreshold - ClimaxBase; // 150-50=100, 200-50=150, etc.
			float DecayTriggerPoint = PreviousFloorBase + (ClimaxBase * Config.ClimaxThresholdPercent);
			// Ejemplos:
			// Threshold=150: 100 + (50*0.1) = 105 → pero queremos 55
			// Threshold=200: 150 + (50*0.1) = 155 → pero queremos 105  
			// Threshold=250: 200 + (50*0.1) = 205 → pero queremos 155
			
			// CORRECCIÓN: Bajar un piso completo antes de aplicar buffer
			float OneFloorDown = PreviousFloorBase - ClimaxBase; // 100-50=50, 150-50=100, 200-50=150
			// Si estamos en el segundo piso (Threshold=100), OneFloorDown sería 0, así que usamos ClimaxBase
			float TargetFloorBase = FMath::Max(ClimaxBase, OneFloorDown);
			DecayTriggerPoint = TargetFloorBase + (ClimaxBase * Config.ClimaxThresholdPercent);
			
			// Ejemplos corregidos:
			// Threshold=150 (piso 3): TargetFloor=50, DecayTrigger=50+(50*0.1)=55 ✓
			// Threshold=200 (piso 4): TargetFloor=100, DecayTrigger=100+(50*0.1)=105 ✓  
			// Threshold=250 (piso 5): TargetFloor=150, DecayTrigger=150+(50*0.1)=155 ✓
			// Threshold=100 (piso 2): TargetFloor=50, DecayTrigger=50+(50*0.1)=55 ✓
			
			UE_LOG(LogTemp, Log, TEXT("Floor-based decay check - Threshold: %f, TargetFloor: %f, Activity: %f, DecayTrigger: %f, Time: %f/%f"), 
				CurrentThreshold, TargetFloorBase, CurrentStatus.CurrentActivity, DecayTriggerPoint, TimeInDecayZone, Config.DecayDelayThreshold);
			
			// Is Activity in decay zone? (Independent of state)
			if (CurrentStatus.CurrentActivity < DecayTriggerPoint)
			{
				TimeInDecayZone += 1.0f;
				
				// Confirm decay after delay threshold
				if (TimeInDecayZone >= Config.DecayDelayThreshold)
				{
					// Decay one complete floor
					float NewThreshold = FMath::Max(ClimaxBase, CurrentThreshold - ClimaxBase);
					CurrentStatus.CurrentClimaxThreshold = NewThreshold;
					TimeInDecayZone = 0.0f;
					
					UE_LOG(LogTemp, Log, TEXT("Floor-based decay executed: %f → %f (Activity %f below trigger %f for %fs)"), 
						CurrentThreshold, NewThreshold, CurrentStatus.CurrentActivity, DecayTriggerPoint, Config.DecayDelayThreshold);
				}
			}
			else
			{
				TimeInDecayZone = 0.0f; // Reset if activity rises
			}
		}
	}

	// Skip decay if just exited cooldown (migrado de useDirectorLogic.js)
	if (bJustExitedCooldown)
	{
		bJustExitedCooldown = false;
		UE_LOG(LogTemp, Log, TEXT("Post-cooldown: Natural decay paused temporarily"));
		return;
	}

	// Check natural plateau time
	// If no events have occurred yet (LastEventTime = -1), allow normal decay
	if (LastEventTime >= 0.0f)
	{
		float TimeSinceLastEvent = FPlatformTime::Seconds() - LastEventTime;
		if (TimeSinceLastEvent <= Config.NaturalPlateauTime)
		{
			return; // Plateau active, skip decay
		}
	}

	// Apply decay based on current state
	if (CurrentStatus.CurrentState == ETikDirectorState::INTENSIVE || 
		CurrentStatus.CurrentState == ETikDirectorState::ACTIVE || 
		CurrentStatus.CurrentState == ETikDirectorState::DORMANT)
	{
		float DecayRate = UTikHelpers::GetDecayRate(CurrentStatus.CurrentState, Config.DecayRates);
		float NewActivity = FMath::Max(0.0f, CurrentStatus.CurrentActivity * DecayRate);
		
		if (NewActivity != CurrentStatus.CurrentActivity)
		{
			float PreviousActivity = CurrentStatus.CurrentActivity;
			CurrentStatus.CurrentActivity = NewActivity;
			NotifyActivityChange(NewActivity, PreviousActivity);
			
			// Check for state transitions after decay
			if (!bIsInCooldown)
			{
				ETikDirectorState NewState = DetermineState(CurrentStatus.CurrentActivity);
				if (NewState != CurrentStatus.CurrentState)
				{
					TransitionToState(NewState);
				}
			}
		}
	}
}

void UTikDirectorSubsystem::ProcessHypeDecay()
{
	if (!CurrentStatus.bIsRunning)
	{
		return;
	}

	float NewHype = FMath::Max(1.0f, CurrentStatus.HypeMultiplier * Config.Hype.Decay);
	if (NewHype != CurrentStatus.HypeMultiplier)
	{
		float PreviousHype = CurrentStatus.HypeMultiplier;
		CurrentStatus.HypeMultiplier = NewHype;
		NotifyHypeChange(NewHype, PreviousHype);
	}
}

void UTikDirectorSubsystem::ProcessFlavorAnalysis()
{
	if (!CurrentStatus.bIsRunning)
	{
		return;
	}

	// Clean up old events first
	CleanupRecentEvents();

	// Check for IDLE timeout - sin eventos durante IdleTimeout segundos
	// If no events have occurred yet (LastEventTime = -1), don't trigger IDLE timeout
	if (LastEventTime >= 0.0f)
	{
		float TimeSinceLastEvent = FPlatformTime::Seconds() - LastEventTime;
		if (TimeSinceLastEvent > Config.Flavor.IdleTimeout)
		{
			if (CurrentStatus.CurrentFlavor != ETikFlavorType::IDLE)
			{
				CurrentStatus.CurrentFlavor = ETikFlavorType::IDLE;
				NotifyFlavorChange(ETikFlavorType::IDLE);
				UE_LOG(LogTemp, Log, TEXT("Flavor changed to IDLE due to timeout (%f seconds without events)"), 
					Config.Flavor.IdleTimeout);
			}
			return;
		}
	}

	// Si no hay eventos recientes pero no timeout, mantener flavor actual
	if (RecentEventHistory.Num() == 0)
	{
		// No cambiar flavor, mantener el último
		return;
	}

	// Determine dominant flavor using improved logic
	ETikFlavorType DominantFlavor = UTikHelpers::DetermineDominantFlavor(RecentEventHistory, Config.Flavor.DominanceThreshold);

	// Check special conditions for HYPE
	if (ShouldForceHype())
	{
		DominantFlavor = ETikFlavorType::HYPE;
	}

	if (DominantFlavor != CurrentStatus.CurrentFlavor)
	{
		ETikFlavorType PreviousFlavor = CurrentStatus.CurrentFlavor;
		CurrentStatus.CurrentFlavor = DominantFlavor;
		NotifyFlavorChange(DominantFlavor);
		UE_LOG(LogTemp, Log, TEXT("Flavor changed from %s to %s"), 
			*UTikHelpers::FlavorTypeToString(PreviousFlavor), 
			*UTikHelpers::FlavorTypeToString(DominantFlavor));
	}
}

bool UTikDirectorSubsystem::ShouldForceHype() const
{
	// HYPE si: alta actividad + múltiples tipos + hype multiplier alto
	bool HighActivity = CurrentStatus.CurrentActivity > Config.Thresholds.ActiveToIntensive;
	bool HighHype = CurrentStatus.HypeMultiplier > 1.8f;
	
	// Verificar si hay múltiples tipos de eventos recientes
	TSet<ETikEventType> UniqueEventTypes;
	for (const FTikEvent& Event : RecentEventHistory)
	{
		UniqueEventTypes.Add(Event.EventType);
	}
	bool MixedEventTypes = UniqueEventTypes.Num() >= 3;

	return HighActivity && HighHype && MixedEventTypes;
}

void UTikDirectorSubsystem::ProcessCooldown()
{
	// Migrado de useDirectorLogic.js líneas 232-250
	// Solo activar timer si estamos en COOLDOWN y la simulación está corriendo
	if (!CurrentStatus.bIsRunning || CurrentStatus.CurrentState != ETikDirectorState::COOLDOWN)
	{
		// Si no estamos en cooldown, limpiar el timer visual (migrado del prototipo web)
		if (CurrentStatus.CooldownRemainingTime > 0.0f)
		{
			CurrentStatus.CooldownRemainingTime = 0.0f;
			UE_LOG(LogTemp, VeryVerbose, TEXT("Cooldown timer cleared - not in cooldown state"));
		}
		return;
	}

	// Asegurar que tenemos tiempo de inicio válido
	if (CooldownStartTime <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("ProcessCooldown called but CooldownStartTime is invalid"));
		return;
	}

	// Calcular tiempo transcurrido y restante (migrado del prototipo web)
	float CooldownElapsed = FPlatformTime::Seconds() - CooldownStartTime;
	float Remaining = Config.CooldownDuration - CooldownElapsed;
	float RemainingTime = FMath::Max(0.0f, FMath::CeilToFloat(Remaining));
	
	// Actualizar timer visual (equivalente a setCooldownRemainingTime del prototipo web)
	CurrentStatus.CooldownRemainingTime = RemainingTime;
	
	UE_LOG(LogTemp, VeryVerbose, TEXT("Cooldown update - Elapsed: %f, Remaining: %f"), 
		CooldownElapsed, RemainingTime);

	// Verificar si el cooldown ha terminado (migrado de useDirectorLogic.js línea 245)
	if (CooldownElapsed >= Config.CooldownDuration)
	{
		UE_LOG(LogTemp, Log, TEXT("Cooldown duration finished. Re-evaluating state."));
		CooldownStartTime = 0.0f;
		bIsInCooldown = false;
		bJustExitedCooldown = true;
		CurrentStatus.CooldownRemainingTime = 0.0f;

		// APLICAR DECAY INMEDIATO al salir del cooldown (migrado de useDirectorLogic.js)
		float DecayedActivity = CurrentStatus.CurrentActivity * Config.DecayRates.Active;
		float PreviousActivity = CurrentStatus.CurrentActivity;
		CurrentStatus.CurrentActivity = DecayedActivity;
		NotifyActivityChange(DecayedActivity, PreviousActivity);

		UE_LOG(LogTemp, Log, TEXT("Applied post-cooldown decay: %f → %f"), PreviousActivity, DecayedActivity);

		ETikDirectorState NewState;

		// Después del cooldown, el próximo clímax requiere SUPERAR el threshold actual
		if (DecayedActivity > CurrentStatus.CurrentClimaxThreshold)
		{
			NewState = ETikDirectorState::COOLDOWN;
			bIsInCooldown = true; // Re-entering cooldown
			CooldownStartTime = FPlatformTime::Seconds();
			CurrentStatus.CooldownRemainingTime = Config.CooldownDuration;
			OnClimaxReached.Broadcast(CurrentStatus.CurrentClimaxThreshold);
			OnCooldownStarted.Broadcast(Config.CooldownDuration);
			UE_LOG(LogTemp, Log, TEXT("Activity still exceeds threshold (%f > %f), re-entering cooldown"), 
				DecayedActivity, CurrentStatus.CurrentClimaxThreshold);
		}
		else
		{
			// Aplicar lógica probabilística de LURKING
			if (ShouldActivateLurking())
			{
				// Activar LURKING - calma tensa
				NewState = ETikDirectorState::LURKING;
				bIsInLurking = true;
				LurkingStartTime = FPlatformTime::Seconds();
				UE_LOG(LogTemp, Log, TEXT("Exiting cooldown to LURKING state (probability triggered)"));
			}
			else
			{
				// Usar lógica normal para estados no-cooldown
				NewState = DetermineState(DecayedActivity);
				UE_LOG(LogTemp, Log, TEXT("Exiting cooldown to %s state (activity: %f)"), 
					*UTikHelpers::DirectorStateToString(NewState), DecayedActivity);
			}
			OnCooldownEnded.Broadcast(NewState);
		}

		TransitionToState(NewState);
	}
}

void UTikDirectorSubsystem::ProcessLurking()
{
	// Solo procesar si estamos en LURKING y la simulación está corriendo
	if (!CurrentStatus.bIsRunning || CurrentStatus.CurrentState != ETikDirectorState::LURKING)
	{
		return;
	}

	// Verificar si alguien ha roto el suspense con actividad
	if (CurrentStatus.CurrentActivity >= Config.Thresholds.DormantToActive)
	{
		UE_LOG(LogTemp, Log, TEXT("LURKING interrupted by audience activity (%f >= %f)"), 
			CurrentStatus.CurrentActivity, Config.Thresholds.DormantToActive);
		
		bIsInLurking = false;
		LurkingStartTime = 0.0f;
		
		// Transicionar a ACTIVE por interrupción de audiencia
		ETikDirectorState NewState = DetermineState(CurrentStatus.CurrentActivity);
		TransitionToState(NewState);
		return;
	}

	// Verificar si el tiempo de LURKING ha terminado
	if (LurkingStartTime <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("ProcessLurking called but LurkingStartTime is invalid"));
		return;
	}

	float LurkingElapsed = FPlatformTime::Seconds() - LurkingStartTime;
	
	if (LurkingElapsed >= Config.Lurking.LurkingDuration)
	{
		UE_LOG(LogTemp, Log, TEXT("LURKING duration finished (%f seconds). Transitioning to normal state."), 
			Config.Lurking.LurkingDuration);
		
		bIsInLurking = false;
		LurkingStartTime = 0.0f;
		
		// Salir de LURKING - evaluar estado normal
		ETikDirectorState NewState = DetermineState(CurrentStatus.CurrentActivity);
		TransitionToState(NewState);
	}
}

bool UTikDirectorSubsystem::ShouldActivateLurking() const
{
	// Generar número aleatorio entre 0.0 y 1.0
	float RandomValue = FMath::RandRange(0.0f, 1.0f);
	
	bool ShouldActivate = RandomValue <= Config.Lurking.LurkingChance;
	
	UE_LOG(LogTemp, Log, TEXT("LURKING probability check: %f <= %f = %s"), 
		RandomValue, Config.Lurking.LurkingChance, ShouldActivate ? TEXT("TRUE") : TEXT("FALSE"));
	
	return ShouldActivate;
}

ETikDirectorState UTikDirectorSubsystem::DetermineState(float Activity) const
{
	// Migrado de determineState en useDirectorLogic.js
	float EffectiveClimaxThreshold = CurrentStatus.CurrentClimaxThreshold;

	UE_LOG(LogTemp, Log, TEXT("DetermineState: Activity=%f, EffectiveClimaxThreshold=%f, ActiveToIntensive=%f, DormantToActive=%f"), 
		Activity, EffectiveClimaxThreshold, Config.Thresholds.ActiveToIntensive, Config.Thresholds.DormantToActive);

	if (Activity >= EffectiveClimaxThreshold)
	{
		UE_LOG(LogTemp, Log, TEXT("Activity >= effective climax threshold → COOLDOWN"));
		return ETikDirectorState::COOLDOWN;
	}
	if (Activity >= Config.Thresholds.ActiveToIntensive)
	{
		UE_LOG(LogTemp, Log, TEXT("Activity >= activeToIntensive → INTENSIVE"));
		return ETikDirectorState::INTENSIVE;
	}
	if (Activity >= Config.Thresholds.DormantToActive)
	{
		UE_LOG(LogTemp, Log, TEXT("Activity >= dormantToActive → ACTIVE"));
		return ETikDirectorState::ACTIVE;
	}

	UE_LOG(LogTemp, Log, TEXT("Activity below all thresholds → DORMANT"));
	return ETikDirectorState::DORMANT;
}

void UTikDirectorSubsystem::TransitionToState(ETikDirectorState NewState)
{
	if (NewState != CurrentStatus.CurrentState)
	{
		ETikDirectorState PreviousState = CurrentStatus.CurrentState;
		CurrentStatus.CurrentState = NewState;
		NotifyStateChange(NewState);
		
		UE_LOG(LogTemp, Log, TEXT("State transition from %s to %s (Activity: %f)"), 
			*UTikHelpers::DirectorStateToString(PreviousState),
			*UTikHelpers::DirectorStateToString(NewState), 
			CurrentStatus.CurrentActivity);
	}
}

void UTikDirectorSubsystem::UpdateActivity(float ActivityDelta)
{
	CurrentStatus.CurrentActivity = FMath::Max(0.0f, CurrentStatus.CurrentActivity + ActivityDelta);
}

void UTikDirectorSubsystem::UpdateHype(float HypeDelta)
{
	CurrentStatus.HypeMultiplier = FMath::Clamp(
		CurrentStatus.HypeMultiplier + HypeDelta, 
		1.0f, 
		Config.Hype.Max
	);
}

void UTikDirectorSubsystem::UpdateClimaxThreshold()
{
	// SIEMPRE calcular y actualizar el Next Climax Target (migrado de useDirectorLogic.js)
	float ClimaxBase = Config.Thresholds.Climax;
	int32 FloorsReached = FMath::FloorToInt(CurrentStatus.CurrentActivity / ClimaxBase);
	float CalculatedNextThreshold = (FloorsReached * ClimaxBase) + ClimaxBase;

	UE_LOG(LogTemp, VeryVerbose, TEXT("Threshold calculation - CurrentActivity: %f, FloorsReached: %d, CalculatedNextThreshold: %f"), 
		CurrentStatus.CurrentActivity, FloorsReached, CalculatedNextThreshold);

	// Actualizar threshold si es mayor (independiente del estado)
	if (CalculatedNextThreshold > CurrentStatus.CurrentClimaxThreshold)
	{
		float OldThreshold = CurrentStatus.CurrentClimaxThreshold;
		CurrentStatus.CurrentClimaxThreshold = CalculatedNextThreshold;
		UE_LOG(LogTemp, Log, TEXT("Updating climax threshold from %f to %f (CurrentActivity: %f)"), 
			OldThreshold, CalculatedNextThreshold, CurrentStatus.CurrentActivity);
	}
}

void UTikDirectorSubsystem::NotifyStateChange(ETikDirectorState NewState)
{
	OnDirectorStateChanged.Broadcast(NewState, CurrentStatus.CurrentActivity);
}

void UTikDirectorSubsystem::NotifyActivityChange(float NewActivity, float PreviousActivity)
{
	OnActivityChanged.Broadcast(NewActivity, PreviousActivity);
}

void UTikDirectorSubsystem::NotifyHypeChange(float NewHype, float PreviousHype)
{
	OnHypeChanged.Broadcast(NewHype, PreviousHype);
}

void UTikDirectorSubsystem::NotifyFlavorChange(ETikFlavorType NewFlavor)
{
	OnFlavorChanged.Broadcast(NewFlavor, 1.0f); // Confidence = 1.0 for now
}

void UTikDirectorSubsystem::CleanupRecentEvents()
{
	// Remove events older than analysis window (migrado de useDirectorLogic.js)
	float CurrentTime = FPlatformTime::Seconds();
	RecentEventHistory.RemoveAll([this, CurrentTime](const FTikEvent& Event)
	{
		return (CurrentTime - Event.Timestamp) > Config.Flavor.AnalysisWindow;
	});
}