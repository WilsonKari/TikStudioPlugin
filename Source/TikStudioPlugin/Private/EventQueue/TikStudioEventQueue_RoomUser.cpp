// TikStudioEventQueue_RoomUser.cpp
// Part of UTikStudioEventQueue — RoomUser event handling + Milestone helpers
// Split from TikStudioEventQueue.cpp following Epic's multi-cpp pattern

#include "EventQueue/TikStudioEventQueue.h"
#include "Engine/Engine.h"

using namespace TikStudioEventTypes;

// ═══════════════════════════════════════════════════════════════════════════════════
// MILESTONE HELPER FUNCTIONS (Algoritmo Prototipo)
// ═══════════════════════════════════════════════════════════════════════════════════

int32 UTikStudioEventQueue::MilestoneDestinoReportable(int32 ValorDestino, int32 Step) const
{
	// Milestone de destino reportable (mínimo = Step, nunca retorna 0)
	// Equivalente JS: return Math.max(Math.floor(Math.max(0, valorDestino) / stepVal) * stepVal, stepVal);
	const int32 mDest = (FMath::Max(0, ValorDestino) / Step) * Step;
	return FMath::Max(mDest, Step);
}

void UTikStudioEventQueue::LimpiarCooldownsExpirados(double MonotonicNow)
{
	// Limpieza de cooldowns expirados según τ (MilestoneCooldownDuration)
	// Equivalente JS: for (const m in cooldownsMilestones) { if ((t_n - t_c) < tau) { C_limpio[m] = t_c; } }
	// Uses monotonic time (immune to NTP/DST adjustments)
	const float Tau = Settings.RoomUserConfig.MilestoneCooldownDuration;
	
	TMap<int32, double> C_limpio;
	for (const auto& Pair : RoomUserState.CooldownsMilestones)
	{
		const int32 Milestone = Pair.Key;
		const double t_c = Pair.Value; // Monotonic cooldown start time
		const double Elapsed = MonotonicNow - t_c;
		
		if (Elapsed < Tau)
		{
			C_limpio.Add(Milestone, t_c);
		}
	}
	
	RoomUserState.CooldownsMilestones = C_limpio;
}

bool UTikStudioEventQueue::EstaEnBandaProtegida(int32 Valor, double MonotonicNow) const
{
	// Bloqueo SOLO sobre la banda del milestone con cooldown activo (sin ±k)
	// Equivalente JS: verificar si valor está en [bandaInferior, bandaSuperior] de algún cooldown activo
	// Uses monotonic time (immune to NTP/DST adjustments)
	const int32 Step = Settings.RoomUserConfig.RoomUserMilestoneStep;
	const float Tau = Settings.RoomUserConfig.MilestoneCooldownDuration;
	
	for (const auto& Pair : RoomUserState.CooldownsMilestones)
	{
		const int32 m = Pair.Key;
		const double t_c = Pair.Value; // Monotonic cooldown start time
		const double Elapsed = MonotonicNow - t_c;
		
		// Robustez: skipear cooldowns ya expirados
		if (Elapsed >= Tau) continue;
		
		int32 BandaInferior, BandaSuperior;
		
		if (m == 0)
		{
			BandaInferior = 1;
			BandaSuperior = Step - 1;
		}
		else
		{
			BandaInferior = FMath::Max(1, m - Step + 1);
			BandaSuperior = m + Step - 1;
		}
		
		if (Valor >= BandaInferior && Valor <= BandaSuperior)
		{
			return true; // Bloquea emisión
		}
	}
	
	return false;
}

void UTikStudioEventQueue::AplicarPoliticaAdyacencia(int32 MilestoneRef)
{
	// Política de adyacencia: conserva cooldowns a ≤N pasos del milestone de referencia
	// Equivalente JS: const distSteps = Math.abs((m - milestoneRef) / stepVal); if (distSteps > k) { delete cooldownsMilestones[key]; }
	const int32 k = Settings.RoomUserConfig.MilestoneAdjacencyRadius;
	const int32 Step = Settings.RoomUserConfig.RoomUserMilestoneStep;
	
	TArray<int32> KeysToRemove;
	
	for (const auto& Pair : RoomUserState.CooldownsMilestones)
	{
		const int32 m = Pair.Key;
		const int32 DistSteps = FMath::Abs((m - MilestoneRef) / Step);
		
		if (DistSteps > k)
		{
			KeysToRemove.Add(m);
		}
	}
	
	// Eliminar en batch
	for (int32 Key : KeysToRemove)
	{
		RoomUserState.CooldownsMilestones.Remove(Key);
	}
}

void UTikStudioEventQueue::EnqueueRoomUserEvent(const FTSE_RoomUserIn& Data)
{
	const FDateTime Now = FDateTime::UtcNow(); // Wall-clock time for logs/UI only
	const double MonotonicNow = FPlatformTime::Seconds(); // Monotonic time for cooldowns (immune to NTP/DST)
	
	FScopeLock Lock(&CriticalSection);
	
	// ═══════════════════════════════════════════════════════════════════════════════════
	// PASO 1A: Actualizar estado DEDICADO de RoomUser (independiente de milestones)
	// ═══════════════════════════════════════════════════════════════════════════════════
	RoomUserState.RUS_LastRawVC = FMath::Max(0, Data.ViewerCount);
	RoomUserState.RUS_LastRawTopViewers = Data.TopViewers; // Conservar vacía si viene vacía
	RoomUserState.RUS_LastRawMonoSec = MonotonicNow;
	
	// ═══════════════════════════════════════════════════════════════════════════════════
	// PASO 1A-BIS: Actualizar caché independiente para LikeUser gate (SIEMPRE, antes de early returns)
	// ═══════════════════════════════════════════════════════════════════════════════════
	LikeUserGateCache.LastViewerCount = FMath::Max(0, Data.ViewerCount);
	LikeUserGateCache.bHasViewerCount = true;
	LikeUserGateCache.LastUpdateMonoSec = MonotonicNow;
	
	// ═══════════════════════════════════════════════════════════════════════════════════
	// PASO 1B: Actualizar estado compartido legacy (usado por Top1Change, etc.)
	// ═══════════════════════════════════════════════════════════════════════════════════
	if (Data.TopViewers.Num() > 0)
	{
		RoomUserState.LastTopViewers = Data.TopViewers;
	}
	
	// Early return if all RoomUser flows are disabled
	if (!Settings.EventToggles.RoomUser.bEnableRoomUserMilestone && 
		!Settings.EventToggles.RoomUser.bEnableRoomUser && 
		!Settings.EventToggles.RoomUser.bEnableRoomUserTop1Change)
	{
		return;
	}
	
	// ═══════════════════════════════════════════════════════════════════════════════════
	// PASO 2: ALGORITMO PROTOTIPO - RoomUserMilestone Detection
	// ═══════════════════════════════════════════════════════════════════════════════════
	if (Settings.EventToggles.RoomUser.bEnableRoomUserMilestone)
	{
		const int32 v_n = Data.ViewerCount;
		const int32 Step = Settings.RoomUserConfig.RoomUserMilestoneStep;
		
		// ═══════════════════════════════════════════════════════════════════════════════════
		// PROTECCIÓN: Detectar cambio de Step (invalida cooldowns/contadores existentes)
		// ═══════════════════════════════════════════════════════════════════════════════════
		if (RoomUserState.bMilestoneInitialized && RoomUserState.LastKnownStep != Step)
		{
			UE_LOG(LogTemp, Warning, TEXT("[EventQueue] RoomUserMilestone STEP_CHANGE: %d → %d | Flushing cooldowns and state"), 
				RoomUserState.LastKnownStep, Step);
			
			// Resetear estado completo (cooldowns creados con Step anterior son inválidos)
			RoomUserState.CooldownsMilestones.Empty();
			RoomUserState.EmisionCountByMilestone.Empty();
			RoomUserState.bMilestoneInitialized = false;
			RoomUserState.v_prev = 0;
			RoomUserState.M_last = 0;
			RoomUserState.LastKnownStep = 0;
			
			// El siguiente bloque de código reinicializará con el nuevo Step
		}
		
		// Limpieza de cooldowns expirados según τ (uses monotonic time)
		LimpiarCooldownsExpirados(MonotonicNow);
		
		// CASO 1: Inicialización (primer evento o post-reset por cambio de Step)
		if (!RoomUserState.bMilestoneInitialized)
		{
			RoomUserState.M_last = (FMath::Max(0, v_n) / Step) * Step;
			RoomUserState.v_prev = FMath::Max(0, v_n);
			RoomUserState.LastKnownStep = Step; // Fijar Step usado para esta sesión
			RoomUserState.bMilestoneInitialized = true;
			UE_LOG(LogTemp, Log, TEXT("[EventQueue] RoomUserMilestone INITIALIZED: M_last=%d, v_prev=%d, Step=%d"), 
				RoomUserState.M_last, RoomUserState.v_prev, Step);
		// NO emitir evento, solo inicializar baseline
		goto SkipMilestoneEmission;
	}
	
	// Bloque aislado para evitar error C2362 (goto saltando inicializaciones)
	{
		// Normalizar y calcular milestone actual
		const int32 v_clamped = FMath::Max(0, v_n);
		const int32 M_current = (v_clamped / Step) * Step;
		
		// CASO 2: Sigue en la misma banda (mismo milestone)
		if (M_current == RoomUserState.M_last)
		{
			RoomUserState.v_prev = v_clamped;
			UE_LOG(LogTemp, Verbose, TEXT("[EventQueue] RoomUserMilestone NO_EVENTO: Same band (M=%d, VC=%d)"), 
				M_current, v_clamped);
			goto SkipMilestoneEmission;
		}
		
		// CASO 3: Cambió de banda → evaluar si está en banda protegida
		const bool bDireccionAsc = (M_current > RoomUserState.M_last);		if (EstaEnBandaProtegida(v_clamped, MonotonicNow))
		{
			RoomUserState.v_prev = v_clamped;
			UE_LOG(LogTemp, Verbose, TEXT("[EventQueue] RoomUserMilestone BLOCKED: Cooldown active (M_curr=%d, M_last=%d, VC=%d)"), 
				M_current, RoomUserState.M_last, v_clamped);
			goto SkipMilestoneEmission;
		}
		
		// ✅ EMITIR EVENTO: Crear milestone con reemplazo
		{
			const int32 MilestoneAReportar = MilestoneDestinoReportable(v_clamped, Step);
			
			// Incrementar contador de emisiones para el milestone reportado
			int32& CountRef = RoomUserState.EmisionCountByMilestone.FindOrAdd(MilestoneAReportar, 0);
			CountRef++;
			const int32 NuevoCount = CountRef;
			
			// Crear evento milestone
			FQueuedTikTokEvent MilestoneEvent;
			MilestoneEvent.Id = FGuid::NewGuid();
			MilestoneEvent.EventType = RoomUserMilestone;
			MilestoneEvent.GroupKey = RoomUserMilestone;
			MilestoneEvent.Timestamp = Now;
			MilestoneEvent.Milestone = MilestoneAReportar;
			// ✅ FIX: Calcular PreviousMilestone siempre desde M_last_committed actual
			const int32 PreviousMilestoneRaw = RoomUserState.M_last_committed;
			MilestoneEvent.PreviousMilestone = FMath::Max(PreviousMilestoneRaw, Step); // Clamp para no mostrar 0
			MilestoneEvent.EmissionCount = NuevoCount;
			MilestoneEvent.ViewerCount = v_n; // VC raw original
			MilestoneEvent.bIsAscending = bDireccionAsc;
			MilestoneEvent.TTLSeconds = GetTTLForType(RoomUserMilestone);
			MilestoneEvent.PriorityScore = ComputePriority(MilestoneEvent);
			
			// REEMPLAZO O(1): Si hay milestone previo en Queue, reemplazarlo
			if (RoomUserState.bHasMilestoneInQueue && RoomUserState.LastMilestoneId.IsValid())
			{
				bool bReplaced = false;
				for (int32 i = 0; i < Queue.Num(); ++i)
				{
					if (Queue[i].Id == RoomUserState.LastMilestoneId)
					{
						Queue[i] = MilestoneEvent;
						bReplaced = true;
						UE_LOG(LogTemp, Log, TEXT("[EventQueue] RoomUserMilestone REPLACED: M=%d %s (prev=%d, count=%d) VC=%d [COOLDOWN_BAND]"), 
							MilestoneAReportar, bDireccionAsc ? TEXT("ASC") : TEXT("DESC"), MilestoneEvent.PreviousMilestone, NuevoCount, v_n);
						break;
					}
				}
				
				if (!bReplaced)
				{
					// Handle inválido, encolar como nuevo
					UE_LOG(LogTemp, Warning, TEXT("[EventQueue] RoomUserMilestone handle invalid, enqueuing as new"));
					Queue.Add(MilestoneEvent);
				}
			}
			else
			{
				// Primera emisión, encolar nuevo
				Queue.Add(MilestoneEvent);
				UE_LOG(LogTemp, Log, TEXT("[EventQueue] RoomUserMilestone EMIT: M=%d %s (prev=%d, count=%d) VC=%d [COOLDOWN_BAND]"), 
					MilestoneAReportar, bDireccionAsc ? TEXT("ASC") : TEXT("DESC"), MilestoneEvent.PreviousMilestone, NuevoCount, v_n);
			}
			
			// Actualizar handle
			RoomUserState.LastMilestoneId = MilestoneEvent.Id;
			RoomUserState.bHasMilestoneInQueue = true;
			
			// Activar cooldown en el milestone alcanzado (banda actual) - uses monotonic time
			RoomUserState.CooldownsMilestones.Add(M_current, MonotonicNow);
			
			// Actualizar estado y aplicar política de adyacencia
			RoomUserState.M_last = M_current;
			AplicarPoliticaAdyacencia(M_current);
			
			// Actualizar v_prev después de emisión
			RoomUserState.v_prev = v_clamped;
			
		// Solicitar pump inmediato
		RequestImmediatePump(TEXT("RoomUserMilestone"));
	}
	}
	
SkipMilestoneEmission:
	; // Label para saltos (NO_EVENTO cases)
}	// ═══════════════════════════════════════════════════════════════════════════════════
	// PASO 3: Derivados (Snapshot periódico, Top1Change) - procesados en SweepExpired
	// ═══════════════════════════════════════════════════════════════════════════════════
	if (Settings.EventToggles.RoomUser.bEnableRoomUser || Settings.EventToggles.RoomUser.bEnableRoomUserTop1Change)
	{
		RequestImmediatePump(TEXT("RoomUserDerived"));
	}
}
