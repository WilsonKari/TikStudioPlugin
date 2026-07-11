// Copyright Epic Games, Inc. All Rights Reserved.
//
// ═══════════════════════════════════════════════════════════════════════════
// UTikStudioEventQueue — Implementation split across multiple .cpp files
// following Epic's multi-cpp pattern (same as AActor).
//
// FILE MAP:
//   TikStudioEventQueue.cpp          ← Core: EnqueueTypedEvent, SweepExpired,
//                                      DispatchLockedEvent, DequeueNextEvent,
//                                      ConfirmEventProcessed, Priority, Pump,
//                                      Config, TryCompetitiveEvict (THIS FILE)
//   TikStudioEventQueue_Chat.cpp     ← EnqueueChatEvent (shadow merge)
//   TikStudioEventQueue_Gift.cpp     ← EnqueueGiftEvent + GiftCombo system
//   TikStudioEventQueue_Follow.cpp   ← EnqueueFollowEvent
//   TikStudioEventQueue_Like.cpp     ← EnqueueLikeEvent (milestone + LikeUser)
//   TikStudioEventQueue_Member.cpp   ← EnqueueMemberEvent (identity + normalized)
//   TikStudioEventQueue_RoomUser.cpp ← EnqueueRoomUserEvent + milestone helpers
//   TikStudioEventQueue_Share.cpp    ← EnqueueShareEvent (+ ShareMilestone)
// ═══════════════════════════════════════════════════════════════════════════

#include "EventQueue/TikStudioEventQueue.h"
#include "Engine/Engine.h" // For UE_LOG
#include "Async/Async.h" // For AsyncTask
#include "Templates/UnrealTemplate.h" // For TGuardValue
#include "Engine/World.h" // For GetWorld
#include "TimerManager.h" // For SetTimerForNextTick
#include "Containers/Ticker.h" // For FTSTicker

namespace TikStudioModifierKeys
{
	const FName FollowRole(TEXT("FollowRole"));
	const FName Moderator(TEXT("bIsModerator"));
	const FName Subscriber(TEXT("bIsSubscriber"));
	const FName NewGifter(TEXT("bIsNewGifter"));
	const FName TopGifterRank(TEXT("TopGifterRank"));
	const FName GifterLevel(TEXT("GifterLevel"));
	const FName TeamMemberLevel(TEXT("TeamMemberLevel"));

	const FName DiamondCount(TEXT("DiamondCount"));
	const FName RepeatCount(TEXT("RepeatCount"));
	const FName RepeatEnd(TEXT("bRepeatEnd"));

	const FName TotalDiamondCount(TEXT("TotalDiamondCount"));
	const FName TotalRepeatCount(TEXT("TotalRepeatCount"));
	const FName FinalSnapshot(TEXT("bIsFinalSnapshot"));

	const FName HasCommand(TEXT("HasCommand"));

	const FName LikeCount(TEXT("LikeCount"));
	const FName TotalLikeCount(TEXT("TotalLikeCount"));

	const FName ViewerCount(TEXT("ViewerCount"));
	const FName Milestone(TEXT("Milestone"));
}

using namespace TikStudioEventTypes;

bool UTikStudioEventQueue::EnqueueTypedEvent(FName EventType, const FQueuedTikTokEvent& EventData)
{
    bool bShouldPumpFirst = false;
    {
        FScopeLock Lock(&CriticalSection);

        // Check slots (policy: discard new if exceeds)
        int32 CurrentCount = GetCountByType_NoLock(EventType);
        int32 MaxSlots = GetMaxSlotsForType(EventType);
        if (CurrentCount >= MaxSlots)
        {
            if (Settings.Eviction.bEnableCompetitiveEviction && !Settings.Eviction.ExemptFromEviction.Contains(EventType))
            {
                // Try competitive eviction (locked variant, we already hold lock)
                if (TryCompetitiveEvict_Locked(EventType, EventData))
                {
                    UE_LOG(LogTemp, Log, TEXT("[EventQueue] Competitive eviction: inserted new Type=%s Score=%d, replaced weakest (count=%d / max=%d)"), *EventType.ToString(), EventData.PriorityScore, CurrentCount, MaxSlots);
                    // El candidato SÍ entró físicamente a la cola (reemplazó al más débil)
                    return true;
                }

                UE_LOG(LogTemp, Warning, TEXT("[EventQueue] Discard new event Type=%s Score=%d (count=%d / max=%d) — competitive eviction failed/no win"), *EventType.ToString(), EventData.PriorityScore, CurrentCount, MaxSlots);
                return false;
            }
            UE_LOG(LogTemp, Warning, TEXT("[EventQueue] Discard new event Type=%s Score=%d (count=%d / max=%d) — slot limit reached"), *EventType.ToString(), EventData.PriorityScore, CurrentCount, MaxSlots);
            return false;
        }

		// Track queue count before inserting
		int32 CountBefore = Queue.Num();

		// Insert in order (descending PriorityScore)
		int32 InsertIndex = 0;
		for (int32 i = 0; i < Queue.Num(); ++i)
		{
			if (Queue[i].PriorityScore < EventData.PriorityScore)
			{
				break;
			}
			InsertIndex = i + 1;
		}
		Queue.Insert(EventData, InsertIndex);

		// Log
		UE_LOG(LogTemp, Log, TEXT("[EventQueue] Enqueue Type=%s Nick=%s Score=%d TTL=%.1fs"), *EventType.ToString(), *EventData.Nickname, EventData.PriorityScore, EventData.TTLSeconds);

        // Decide pump on first enqueue (no lock held outside)
        bShouldPumpFirst = Settings.bPumpOnFirstEnqueue && (CountBefore == 0) && !bHasLockedEvent;
    }

	if (bShouldPumpFirst)
	{
		RequestImmediatePump(TEXT("FirstEnqueue"));
	}

	// Camino exitoso: el evento fue insertado en la cola
	return true;
}

UTikStudioEventDispatcher* UTikStudioEventQueue::GetDispatcher()
{
	if (!Dispatcher)
	{
		Dispatcher = NewObject<UTikStudioEventDispatcher>(this);
	}
	return Dispatcher;
}


// ═══════════════════════════════════════════════════════════════════════════════════
// EVENT HANDLERS — Extracted to separate .cpp files (Epic multi-cpp pattern)
// See: TikStudioEventQueue_Chat.cpp      → EnqueueChatEvent
//      TikStudioEventQueue_Gift.cpp      → EnqueueGiftEvent + Combo system
// ═══════════════════════════════════════════════════════════════════════════════════


void UTikStudioEventQueue::SweepExpired()
{
	bool WantsPumpAfter = false;
	{
		FScopeLock Lock(&CriticalSection);

		const FDateTime Now = FDateTime::UtcNow();

		// Stage C: sweep combo inactivity and repeatEnd closures first
		SweepCombosInactivity_Locked(Now);
		PruneClosedCombos_Locked(Now);

		TArray<FQueuedTikTokEvent> NewQueue;
		// Cachear ViewerCount y TopViewers para Snapshot (DEDICADO) y Top1Change (legacy)
		int32 LatestRoomUserVC = RoomUserState.RUS_LastRawVC; // Usar estado dedicado (independiente de milestone)
		TArray<FTSE_TopViewerInfo> LatestTopViewers = RoomUserState.RUS_LastRawTopViewers; // Usar estado dedicado
		TArray<FTSE_TopViewerInfo> LatestTopViewersLegacy = RoomUserState.LastTopViewers; // Para Top1Change (legacy)
		
		// Point 8: Track re-enqueued SBs to force priority recompute and prevent duplicates
		TSet<FString> ReenqueuedComboKeys;
		bNeedsRecomputeAfterSweep = false;

		for (int32 i = Queue.Num() - 1; i >= 0; --i)
		{
			FQueuedTikTokEvent& E = Queue[i];
			float Elapsed = (Now - E.Timestamp).GetTotalSeconds();
			bool bExpired = Elapsed > E.TTLSeconds;

			// Never touch the locked event
			if (E.Id == LockedEventId)
			{
				NewQueue.Insert(E, 0);
				continue;
			}

			if (E.EventType == Like)
		{
			// FASE 6: Like milestones ya no se normalizan por ventana - se procesan directamente
			// Mantener el Like en la cola para procesamiento normal
			NewQueue.Insert(E, 0);
		}
			else if (E.EventType == RoomUser)
			{
				// Update latest VC and discard all raw
				LatestRoomUserVC = FMath::Max(LatestRoomUserVC, E.ViewerCount);
				// Actualizar últimos TopViewers si presentes
				if (E.TopViewers.Num() > 0)
				{
					LatestTopViewers = E.TopViewers;
				}
				if (bExpired)
				{
					UE_LOG(LogTemp, Log, TEXT("[EventQueue] RoomUser (input raw) discarded during sweep (VC=%d, TopViewers=%d)"), E.ViewerCount, E.TopViewers.Num());
				}
				// Don't add to NewQueue
			}
			else if (bExpired)
			{
				EEventExpirePolicy Policy = GetExpirePolicyForType(E.EventType);
				if (Policy == EEventExpirePolicy::Discard)
				{
				UE_LOG(LogTemp, Log, TEXT("[EventQueue] TTL expired Type=%s Id=%s"), *E.EventType.ToString(), *E.Id.ToString());
				
				// ========== FASE 1: Invalidar handles al expirar por TTL ==========
				if (E.EventType == RoomUserMilestone && E.Id == RoomUserState.LastMilestoneId)
				{
					RoomUserState.LastMilestoneId.Invalidate();
					RoomUserState.bHasMilestoneInQueue = false;
					UE_LOG(LogTemp, Verbose, TEXT("[EventQueue] Milestone handle invalidated on TTL expiry (M=%d)"), E.Milestone);
				}
				
				// Invalidar handle de RoomUser snapshot al expirar por TTL
				if (E.EventType == RoomUser && E.Id == RoomUserState.LastRoomUserSnapshotId)
				{
					RoomUserState.LastRoomUserSnapshotId.Invalidate();
					RoomUserState.bHasRoomUserSnapshotInQueue = false;
					UE_LOG(LogTemp, Verbose, TEXT("[EventQueue] RoomUser snapshot handle invalidated on TTL expiry (VC=%d)"), E.ViewerCount);
				}
				
				// FASE 1: Invalidar handle de Like milestone al expirar por TTL
				if (E.EventType == Like && E.Id == LikeState.LastLikeMilestoneId)
				{
					LikeState.LastLikeMilestoneId.Invalidate();
					LikeState.bHasLikeMilestoneInQueue = false;
					UE_LOG(LogTemp, Verbose, TEXT("[EventQueue] Like milestone handle invalidated on TTL expiry (M=%d)"), E.Milestone);
				}
				
				// Invalidar handle de MemberNormalized al expirar por TTL
				if (E.EventType == MemberNormalized && E.Id == MemberNormalizedState.MN_LastId)
				{
					MemberNormalizedState.MN_LastId.Invalidate();
					MemberNormalizedState.MN_bHasInQueue = false;
					UE_LOG(LogTemp, Log, TEXT("[MemberNormalized] TTL INVALIDATE: handle invalidated on TTL expiry (CurrentMilestone=%d)"), E.MemberCurrentMilestone);
				}
				
				// Invalidar handle de ShareMilestone al expirar por TTL
				if (E.EventType == ShareMilestone && E.Id == ShareMilestoneState.SM_LastId)
				{
					ShareMilestoneState.SM_LastId.Invalidate();
					ShareMilestoneState.SM_bHasInQueue = false;
					UE_LOG(LogTemp, Verbose, TEXT("[ShareMilestone] TTL INVALIDATE"));
				}
				
			// ========== SB + COPIAS: HARDENING - TTL expiry con re-encolado según deuda/estado ==========
			if (E.EventType == GiftCombo && E.bIsSnapshotBase)
			{
			// 1) Guard anti-lock: preservar el SB si hay un SC locked del MISMO combo
			const bool bThisComboInLock = bHasLockedEvent && (LockedEventType == GiftCombo) && (LockedComboKey == E.ComboKey);
			if (bThisComboInLock)
			{
				// Mantener el SB tal cual, evitando duplicados/inconsistencias
				UE_LOG(LogTemp, Warning, TEXT("[SB] TTL expired but PRESERVED (guard anti-lock: SC locked for same combo) Key=%s"), *E.ComboKey);
				NewQueue.Insert(E, 0);
				continue;
			}				// 2) Prevenir duplicados durante re-encolado
				if (ReenqueuedComboKeys.Contains(E.ComboKey))
				{
					UE_LOG(LogTemp, Warning, TEXT("[SB] TTL sweep: duplicate SB prevented for Key=%s (already re-enqueued)"), *E.ComboKey);
					continue; // Skip duplicate SB
				}					FGiftComboState* ComboState = LiveCombos.Find(E.ComboKey);						if (!ComboState)
						{
							// SB sin tracking (ya inconsistente/huérfano) - solo log y descartar
							UE_LOG(LogTemp, Warning, TEXT("[SB] TTL expired for SB without tracking Key=%s (already orphaned)"), *E.ComboKey);
							// SB removed from Queue (not added to NewQueue)
						}
						else if (ComboState->PendingDebt == EComboDebtType::Final)
						{
							// ========== BRANCH 1: FINAL DEBT → RE-ENCOLAR con TTL extendido + pump ==========
							// Invariante: "Si hay deuda, debe existir SB en Queue"
							// Re-crear SB minimal (idéntico a repeatEnd/inactividad)
							FQueuedTikTokEvent NewSB;
							NewSB.Id = ComboState->SnapshotBaseId;
							NewSB.EventType = GiftCombo;
							NewSB.ComboKey = E.ComboKey;
							NewSB.bIsSnapshotBase = true;
							NewSB.bIsComboSnapshot = true;
							NewSB.ComboRepeatCountTotal = ComboState->RepeatCountTotal;
							NewSB.ComboDiamondTotal = ComboState->DiamondTotal;
							NewSB.Timestamp = ComboState->LastTs; // Aging correcto
							NewSB.ComboFirstTs = ComboState->FirstTs;
							NewSB.ComboLastTs = ComboState->LastTs;
							NewSB.UniqueId = ComboState->UniqueId;
							NewSB.Nickname = ComboState->Nickname;
							NewSB.ProfilePictureUrl = ComboState->ProfilePictureUrl;
							NewSB.FollowRole = ComboState->FollowRole;
							NewSB.bIsModerator = ComboState->bIsModerator;
							NewSB.bIsSubscriber = ComboState->bIsSubscriber;
							NewSB.bIsNewGifter = ComboState->bIsNewGifter;
							NewSB.TopGifterRank = ComboState->TopGifterRank;
							NewSB.GifterLevel = ComboState->GifterLevel;
							NewSB.TeamMemberLevel = ComboState->TeamMemberLevel;
							NewSB.GiftId = ComboState->GiftId;
							NewSB.GiftName = ComboState->GiftName;
							NewSB.GiftPictureUrl = ComboState->GiftPictureUrl;
							NewSB.GiftType = ComboState->GiftType;
							NewSB.Describe = ComboState->Describe;
							NewSB.GroupId = ComboState->GroupId;
							NewSB.bComboFinal = true;
							NewSB.bComboInitial = false;
							NewSB.PriorityScore = ComputePriority(NewSB); // +15 bonus por Final
							
							// Point 3: TTL extendido para FINAL desde configuración
							float NormalTTL = GetTTLForType(GiftCombo);
							float TTLFloorFinal = Settings.GiftConfig.GiftComboFinalTTLMinSeconds;
							NewSB.TTLSeconds = FMath::Max(NormalTTL, TTLFloorFinal);
							
							// Re-encolar en NewQueue (insertion at beginning for priority)
							NewQueue.Insert(NewSB, 0);
							
							// Point 8: Marcar como re-encolado para prevenir duplicados
							ReenqueuedComboKeys.Add(E.ComboKey);
							// Point 2: Marcar flag para forzar RecomputePriorities después del sweep
							bNeedsRecomputeAfterSweep = true;
							
							// Point 5: Métrica con guard consistente
							if (Settings.Eviction.bTrackEvictionMetrics)
							{
								++SBReenqueuedOnTTLExpiryFinal;
							}
							
							UE_LOG(LogTemp, Warning, TEXT("[SB] TTL expired with FINAL debt → SB re-enqueued (extended TTL=%.1fs) Key=%s rc=%d diamonds=%d score=%d"),
								NewSB.TTLSeconds, *E.ComboKey, ComboState->RepeatCountTotal, ComboState->DiamondTotal, NewSB.PriorityScore);
							
							// Pedir pump inmediato (fuera del lock)
							WantsPumpAfter = true;
							// Old SB no se añade a NewQueue (reemplazado por NewSB)
						}
						else if (ComboState->PendingDebt == EComboDebtType::Initial || ComboState->PendingDebt == EComboDebtType::Intermediate)
						{
							// ========== BRANCH 2: INITIAL/INTERMEDIATE DEBT → RE-ENCOLAR con TTL normal ==========
							FQueuedTikTokEvent NewSB;
							NewSB.Id = ComboState->SnapshotBaseId;
							NewSB.EventType = GiftCombo;
							NewSB.ComboKey = E.ComboKey;
							NewSB.bIsSnapshotBase = true;
							NewSB.bIsComboSnapshot = true;
							NewSB.ComboRepeatCountTotal = ComboState->RepeatCountTotal;
							NewSB.ComboDiamondTotal = ComboState->DiamondTotal;
							// Point 1 CRÍTICO: Timestamp = Now (reinicia ventana TTL, evita re-expiración inmediata)
							NewSB.Timestamp = Now;
							NewSB.ComboFirstTs = ComboState->FirstTs;
							NewSB.ComboLastTs = ComboState->LastTs;
							NewSB.UniqueId = ComboState->UniqueId;
							NewSB.Nickname = ComboState->Nickname;
							NewSB.ProfilePictureUrl = ComboState->ProfilePictureUrl;
							NewSB.FollowRole = ComboState->FollowRole;
							NewSB.bIsModerator = ComboState->bIsModerator;
							NewSB.bIsSubscriber = ComboState->bIsSubscriber;
							NewSB.bIsNewGifter = ComboState->bIsNewGifter;
							NewSB.TopGifterRank = ComboState->TopGifterRank;
							NewSB.GifterLevel = ComboState->GifterLevel;
							NewSB.TeamMemberLevel = ComboState->TeamMemberLevel;
							NewSB.GiftId = ComboState->GiftId;
							NewSB.GiftName = ComboState->GiftName;
							NewSB.GiftPictureUrl = ComboState->GiftPictureUrl;
							NewSB.GiftType = ComboState->GiftType;
							NewSB.Describe = ComboState->Describe;
							NewSB.GroupId = ComboState->GroupId;
							NewSB.bComboFinal = false; // NO es Final
							NewSB.bComboInitial = false;
							NewSB.PriorityScore = ComputePriority(NewSB);
							NewSB.TTLSeconds = GetTTLForType(GiftCombo); // TTL normal
							
							NewQueue.Insert(NewSB, 0);
							
							// Point 8: Marcar como re-encolado para prevenir duplicados
							ReenqueuedComboKeys.Add(E.ComboKey);
							// Point 2: Marcar flag para forzar RecomputePriorities después del sweep
							bNeedsRecomputeAfterSweep = true;
							
							// Point 5: Métrica con guard consistente
							if (Settings.Eviction.bTrackEvictionMetrics)
							{
								++SBReenqueuedOnTTLExpiryDebt;
							}
							
							UE_LOG(LogTemp, Warning, TEXT("[SB] TTL expired with pending debt → SB re-enqueued Key=%s debt=%s rc=%d score=%d"),
								*E.ComboKey, *DebtTypeToString(ComboState->PendingDebt), ComboState->RepeatCountTotal, NewSB.PriorityScore);
							// Point 4 CRÍTICO: Pump coalescado para Initial/Intermediate también (guard anti-reentrancia protege)
							WantsPumpAfter = true;
						}
						else if (!ComboState->bClosed)
						{
							// ========== BRANCH 3: ABIERTO SIN DEUDA → PRESERVAR tracking, NO re-encolar ==========
							// Permite "resurrección" con próximo gift
							if (Settings.Eviction.bTrackEvictionMetrics)
							{
								++SBExpiredButTrackingPreserved;
							}
							
							UE_LOG(LogTemp, Log, TEXT("[SB] TTL expired (open, no debt) → tracking preserved Key=%s rc=%d"),
								*E.ComboKey, ComboState->RepeatCountTotal);
							// SB removed from Queue, tracking remains for resurrection
						}
						else
						{
							// ========== BRANCH 4: CERRADO SIN DEUDA → SAFE PRUNE ==========
							RemoveComboTracking_Locked(E.ComboKey, TEXT("TTLExpiry"));
							// SB removed from Queue, tracking removed
						}
					}
				// Remove old SB from Queue (not added to NewQueue unless re-enqueued above)
			}
			else if (Policy == EEventExpirePolicy::Consolidate)
			{
				// FASE 6: Like ya no usa Consolidate - solo otros tipos si los hubiera
				// Check defensivo: si aparece algún tipo Consolidate, no perderlo silenciosamente
				ensureMsgf(false, TEXT("[SweepExpired] Unexpected Consolidate type: %s"), *E.EventType.ToString());
				NewQueue.Insert(E, 0); // Conservar por seguridad
			}
		}
		else
		{
			NewQueue.Insert(E, 0);
		}
	}

		// ═══════════════════════════════════════════════════════════════════════════════════
		// RoomUserMilestone: LÓGICA MOVIDA A EnqueueRoomUserEvent (algoritmo prototipo)
		// ═══════════════════════════════════════════════════════════════════════════════════
		// NOTA: La detección de milestone ahora ocurre en EnqueueRoomUserEvent con cooldown temporal
		// SweepExpired solo maneja Snapshot periódico y Top1Change
		
		// Snapshot periódico RoomUser (INDEPENDIENTE de milestones)
		// CRÍTICO: Hacer reemplazo en NewQueue ANTES de Queue = NewQueue
	if (Settings.EventToggles.RoomUser.bEnableRoomUser)
	{
		const double MonotonicNow = FPlatformTime::Seconds();
		const double SinceLastSnapshot = RoomUserState.RUS_LastSnapshotMonoSec > 0.0 ? (MonotonicNow - RoomUserState.RUS_LastSnapshotMonoSec) : TNumericLimits<double>::Max();
		
		// Chequeo del período usando reloj monotónico
		const bool bPeriodOk = SinceLastSnapshot >= Settings.RoomUserConfig.RoomUserSnapshotPeriodSeconds;
		
		// ***NEW***: bypass del período si venimos de un busy
		if (bPeriodOk || RoomUserState.bForceRoomUserSnapshotOnNextPump)
		{
			// ***COALESCENCIA PERFECTA***: No insertar RoomUser snapshots si RoomUser está locked
			if (bHasLockedEvent && LockedEventType == RoomUser)
			{
				// Solo actualizar estado raw y marcar flags, NO insertar
				// El estado raw ya se actualiza arriba en el loop principal
				bPumpRequestedWhileBusy.store(true, std::memory_order_relaxed);
				RoomUserState.bForceRoomUserSnapshotOnNextPump = true;
				UE_LOG(LogTemp, Log, TEXT("[RoomUser] LOCKED→defer snapshot (VC=%d, TopViewers=%d) - will emit after unlock"), 
					LatestRoomUserVC, LatestTopViewers.Num());
				// NO insertar, NO actualizar handles, NO WantsPumpAfter
				// El snapshot se creará en el siguiente pump cuando se libere el lock
			}
			else
			{
				// Proceder normalmente: Smart Switch y emisión de snapshot
			// Smart Switch: Bloquear snapshot si VC > MaxVC (cuando está habilitado)
			bool bSmartSwitchBlocked = false;
			if (Settings.RoomUserConfig.bEnableRoomUserSmartSwitch)
			{
				if (LatestRoomUserVC > Settings.RoomUserConfig.RoomUserSmartSwitchMaxVC)
				{
					bSmartSwitchBlocked = true;
					UE_LOG(LogTemp, Log, TEXT("[RoomUser] SmartSwitch: BLOCKED (VC=%d > MaxVC=%d)"), 
						LatestRoomUserVC, Settings.RoomUserConfig.RoomUserSmartSwitchMaxVC);
				}
				else
				{
					UE_LOG(LogTemp, VeryVerbose, TEXT("[RoomUser] SmartSwitch: ALLOW (VC=%d ≤ MaxVC=%d) — period OK"), 
						LatestRoomUserVC, Settings.RoomUserConfig.RoomUserSmartSwitchMaxVC);
				}
			}
			
			// Solo proceder si el Smart Switch no está bloqueando
			const bool bCanEmitSnapshot = !bSmartSwitchBlocked;
			
			if (bCanEmitSnapshot)
			{
				// Construir nuevo snapshot (siempre con nuevo Id, Timestamp, TTL, PriorityScore)
				FQueuedTikTokEvent Snapshot;
				Snapshot.Id = FGuid::NewGuid();
				Snapshot.EventType = RoomUser;
				Snapshot.GroupKey = RoomUser;
				Snapshot.ViewerCount = LatestRoomUserVC;
				Snapshot.TopViewers = LatestTopViewers;
				Snapshot.TTLSeconds = GetTTLForType(RoomUser);
				Snapshot.Timestamp = Now;
				Snapshot.PriorityScore = ComputePriority(Snapshot);

				// REEMPLAZO ÚNICO: Si hay snapshot previo en NewQueue, reemplazarlo in-place
				if (RoomUserState.bHasRoomUserSnapshotInQueue && RoomUserState.LastRoomUserSnapshotId.IsValid())
				{
					bool bReplaced = false;
					for (int32 i = 0; i < NewQueue.Num(); ++i)
					{
						if (NewQueue[i].Id == RoomUserState.LastRoomUserSnapshotId)
						{
							// REEMPLAZO IN-PLACE: sobrescribir TODO el struct con el nuevo evento
							NewQueue[i] = Snapshot;
							bReplaced = true;
							bNeedsRecomputeAfterSweep = true; // Recompute para mantener orden por prioridad
							UE_LOG(LogTemp, Log, TEXT("[EventQueue] REPLACE RoomUser (snapshot ÚNICO) VC=%d TopViewers=%d Score=%d SmartSwitch=%s"), 
								Snapshot.ViewerCount, Snapshot.TopViewers.Num(), Snapshot.PriorityScore, 
								Settings.RoomUserConfig.bEnableRoomUserSmartSwitch ? TEXT("enabled") : TEXT("disabled"));
							break;
						}
					}
					
					if (!bReplaced)
					{
						// Handle inválido, tratar como nuevo (comportamiento normal cuando el evento anterior está siendo procesado)
						UE_LOG(LogTemp, Verbose, TEXT("[EventQueue] RoomUser snapshot handle no longer valid (previous event likely being processed), inserting new event"));
						int32 InsertIndexSnapshot = 0;
						for (int32 i = 0; i < NewQueue.Num(); ++i)
						{
							if (NewQueue[i].PriorityScore < Snapshot.PriorityScore)
							{
								break;
							}
							InsertIndexSnapshot = i + 1;
						}
						NewQueue.Insert(Snapshot, InsertIndexSnapshot);
						bNeedsRecomputeAfterSweep = true;
						UE_LOG(LogTemp, Log, TEXT("[EventQueue] INSERT RoomUser (snapshot NUEVO tras handle inválido) VC=%d TopViewers=%d Score=%d"), 
							Snapshot.ViewerCount, Snapshot.TopViewers.Num(), Snapshot.PriorityScore);
					}
				}
				else
				{
					// No había handle activo, insertar nuevo ordenado por prioridad
					int32 InsertIndexSnapshot = 0;
					for (int32 i = 0; i < NewQueue.Num(); ++i)
					{
						if (NewQueue[i].PriorityScore < Snapshot.PriorityScore)
						{
							break;
						}
						InsertIndexSnapshot = i + 1;
					}
					NewQueue.Insert(Snapshot, InsertIndexSnapshot);
					bNeedsRecomputeAfterSweep = true;
					UE_LOG(LogTemp, Log, TEXT("[EventQueue] INSERT RoomUser (snapshot NUEVO) VC=%d TopViewers=%d Score=%d SmartSwitch=%s"), 
						Snapshot.ViewerCount, Snapshot.TopViewers.Num(), Snapshot.PriorityScore, 
						Settings.RoomUserConfig.bEnableRoomUserSmartSwitch ? TEXT("enabled") : TEXT("disabled"));
				}
				
				// Actualizar handle (siempre, tanto para reemplazo como para nuevo)
				RoomUserState.LastRoomUserSnapshotId = Snapshot.Id;
				RoomUserState.bHasRoomUserSnapshotInQueue = true;
				
				// Actualizar timestamp monotónico del último snapshot emitido
				RoomUserState.RUS_LastSnapshotMonoSec = MonotonicNow;
				RoomUserState.LastSnapshotTs = Now; // Mantener legacy para compatibilidad
				
				// Consumir el flag de bypass después de emitir el snapshot
				RoomUserState.bForceRoomUserSnapshotOnNextPump = false;
				
				WantsPumpAfter = true;
			}
			} // Cerrar el bloque else de coalescencia perfecta
		}
	}

		Queue = NewQueue;
		
		// Point 2: Si hubo re-encolados TTL, forzar RecomputePriorities para mantener orden correcto
		if (bNeedsRecomputeAfterSweep)
		{
			// Recalcular prioridades y re-ordenar Queue
			for (FQueuedTikTokEvent& Ev : Queue)
			{
				Ev.PriorityScore = ComputePriority(Ev);
			}
			Queue.Sort([](const FQueuedTikTokEvent& A, const FQueuedTikTokEvent& B)
			{
				if (A.PriorityScore != B.PriorityScore)
					return A.PriorityScore > B.PriorityScore;
				return A.Timestamp < B.Timestamp;
			});
			UE_LOG(LogTemp, Log, TEXT("[EventQueue] Recomputed priorities after TTL re-enqueue (count=%d)"), ReenqueuedComboKeys.Num());
		}

		// FASE 6: Eliminada lógica de LikeBatch window - Like milestones se procesan en tiempo real

		// Evento por cambio de Top1 (cooldown y validación mínima) - USA LEGACY
	if (Settings.EventToggles.RoomUser.bEnableRoomUserTop1Change && LatestTopViewersLegacy.Num() >= Settings.RoomUserConfig.RoomUserTop1MinTopViewers)
	{
		const FString CurrentTop1Id = LatestTopViewersLegacy[0].UniqueId;
		const bool bChanged = Settings.RoomUserConfig.bRoomUserTop1StrictEquality ? (CurrentTop1Id != RoomUserState.LastTop1UniqueId) : (!CurrentTop1Id.Equals(RoomUserState.LastTop1UniqueId, ESearchCase::IgnoreCase));
		const double SinceLastTop1 = RoomUserState.LastTop1EmitTs.GetTicks() != 0 ? (Now - RoomUserState.LastTop1EmitTs).GetTotalSeconds() : TNumericLimits<double>::Max();
		if (bChanged && SinceLastTop1 >= Settings.RoomUserConfig.RoomUserTop1CooldownSeconds)
		{
			FQueuedTikTokEvent Top1;
				Top1.Id = FGuid::NewGuid();
				Top1.EventType = RoomUserTop1Change;
				Top1.GroupKey = RoomUserTop1Change;
				Top1.ViewerCount = LatestRoomUserVC;
				Top1.TopViewers.Reset();
				Top1.TopViewers.Add(LatestTopViewersLegacy[0]);
				Top1.TTLSeconds = GetTTLForType(RoomUserTop1Change);
				Top1.Timestamp = Now;
				Top1.PriorityScore = ComputePriority(Top1);

				int32 InsertIndexTop1 = 0;
				for (int32 i = 0; i < Queue.Num(); ++i)
				{
					if (Queue[i].PriorityScore < Top1.PriorityScore)
					{
						break;
					}
					InsertIndexTop1 = i + 1;
				}
				Queue.Insert(Top1, InsertIndexTop1);
				bNeedsRecomputeAfterSweep = true; // Recompute tras derivado
				UE_LOG(LogTemp, Log, TEXT("[EventQueue] Emit RoomUserTop1Change Top1=%s VC=%d Score=%d"), *CurrentTop1Id, LatestRoomUserVC, Top1.PriorityScore);
				RoomUserState.LastTop1UniqueId = CurrentTop1Id;
				RoomUserState.LastTop1EmitTs = Now;
				WantsPumpAfter = true;
			}
		}

		// Write-back: mantener compatibilidad legacy (NO tocar estado dedicado RUS_*)
		// NOTA: v_prev ya se actualiza en EnqueueRoomUserEvent (milestone logic)
		// NOTA: RUS_* se actualiza en EnqueueRoomUserEvent (snapshot logic)
		RoomUserState.LastTopViewers = LatestTopViewersLegacy;

		// Clean up expired Chat shadows from ChatShadowByUser
		TArray<FString> ExpiredShadowKeys;
		for (auto& Pair : ChatShadowByUser)
		{
			const FString& UniqueId = Pair.Key;
			const FGuid& ShadowEventId = Pair.Value;
			
			// Check if the shadow event still exists in the queue
			bool bShadowStillExists = false;
			for (const FQueuedTikTokEvent& QueuedEvent : Queue)
			{
				if (QueuedEvent.Id == ShadowEventId && QueuedEvent.EventType == Chat)
				{
					bShadowStillExists = true;
					break;
				}
			}
			
			// If shadow event no longer exists (expired or processed), mark for removal
			if (!bShadowStillExists)
			{
				ExpiredShadowKeys.Add(UniqueId);
			}
		}
		
		// Remove expired shadows
		for (const FString& ExpiredKey : ExpiredShadowKeys)
		{
			ChatShadowByUser.Remove(ExpiredKey);
			UE_LOG(LogTemp, Log, TEXT("[EventQueue] Cleaned expired Chat shadow for UniqueId=%s"), *ExpiredKey);
		}

		// Preparar bombeo fuera del lock
		WantsPumpAfter = WantsPumpAfter || bWantsImmediatePump;
		bWantsImmediatePump = false;
	}

	// Pump outside lock if requested
	if (WantsPumpAfter)
	{
		RequestImmediatePump(TEXT("AfterSweep"));
	}
}

bool UTikStudioEventQueue::DispatchLockedEvent()
{
	// IMPORTANT: never hold CriticalSection durante Broadcast
	// Guard against double dispatch
	if (bLockedEventDispatched)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EventQueue] Double dispatch attempt for locked event Id=%s"), *LockedEventId.ToString());
		return false;
	}

	if (!bHasLockedEvent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EventQueue] Dispatch called but no locked event"));
		return false;
	}

	if (!Dispatcher)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EventQueue] Dispatch called but no dispatcher"));
		return false;
	}

	// Copy data under lock
	FGuid EventId;
	FName EventType;
	FQueuedTikTokEvent CachedEvent;
	bool bIsRoomUserMilestone = false;
	bool bIsLikeMilestone = false;

	{
		FScopeLock Lock(&CriticalSection);
		EventId = LockedEventId;
		EventType = LockedEventType;

		if (!bLockedEventCacheValid || !EventId.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[EventQueue] Dispatch: no cached locked event for Id=%s"), *EventId.ToString());
			return false;
		}
		CachedEvent = LockedEventCache;
		
		// Update M_last_committed INSIDE lock for thread safety
		if (EventType == RoomUserMilestone)
		{
			RoomUserState.M_last_committed = CachedEvent.Milestone;
			bIsRoomUserMilestone = true;
		}
		// Update Like milestone state INSIDE lock for thread safety
		else if (EventType == Like && CachedEvent.LikeMilestone > 0)
		{
			LikeState.Like_LastCommittedTotal = CachedEvent.TotalLikeCount;
			LikeState.Like_LastCommittedMilestone = CachedEvent.LikeMilestone;
			LikeState.Like_LastEmitMono = FPlatformTime::Seconds();
			LikeState.LastLikeMilestoneId.Invalidate(); // Invalidate handle
			LikeState.bHasLikeMilestoneInQueue = false; // Clear queue flag
			bIsLikeMilestone = true;
			
			UE_LOG(LogTemp, Log, TEXT("[Like Milestone] Committed: Milestone=%d, TotalLikes=%d"), 
				CachedEvent.LikeMilestone, CachedEvent.TotalLikeCount);
		}
	} // Unlock here

	// Now outside lock, broadcast on Game Thread with typed structs
	if (EventType == Chat)
	{
		FTSE_ChatOut OutData;
		OutData.UniqueId = CachedEvent.UniqueId;
		OutData.Nickname = CachedEvent.Nickname;
		OutData.ProfilePictureUrl = CachedEvent.ProfilePictureUrl;
		OutData.FollowRole = CachedEvent.FollowRole;
		OutData.bIsModerator = CachedEvent.bIsModerator;
		OutData.bIsSubscriber = CachedEvent.bIsSubscriber;
		OutData.bIsNewGifter = CachedEvent.bIsNewGifter;
		OutData.TopGifterRank = CachedEvent.TopGifterRank;
		OutData.GifterLevel = CachedEvent.GifterLevel;
		OutData.TeamMemberLevel = CachedEvent.TeamMemberLevel;
		
		// Map arrays from FQueuedTikTokEvent to FTSE_ChatOut
		OutData.Comments = CachedEvent.Comments;
		OutData.EmotesByMessage = CachedEvent.EmotesByMessage;
		OutData.MessageTimestamps = CachedEvent.MessageTimestamps;
		OutData.MergedCount = CachedEvent.MergedCount;
		
		// Apply last-wins logic for bHasCommand (last message in Comments array determines this)
		OutData.bHasCommand = CachedEvent.bHasCommand;

		// Defensive validation: ensure array consistency before broadcast
		ensureMsgf(OutData.Comments.Num() == OutData.EmotesByMessage.Num() && 
		           OutData.Comments.Num() == OutData.MessageTimestamps.Num(),
		           TEXT("[EventQueue] Array size mismatch in Chat dispatch: Comments=%d, Emotes=%d, Timestamps=%d, EventId=%s"),
		           OutData.Comments.Num(), OutData.EmotesByMessage.Num(), OutData.MessageTimestamps.Num(), *EventId.ToString());

		AsyncTask(ENamedThreads::GameThread, [this, OutData, EventId]() {
			Dispatcher->OnChatEventProcessed.Broadcast(OutData, EventId);
		});
		UE_LOG(LogTemp, Log, TEXT("[EventQueue] DISPATCH Chat Id=%s MergedCount=%d"), *EventId.ToString(), OutData.MergedCount);
	}
	else if (EventType == Follow)
	{
		FTSE_FollowOut OutData;
		OutData.UniqueId = CachedEvent.UniqueId;
		OutData.Nickname = CachedEvent.Nickname;
		OutData.ProfilePictureUrl = CachedEvent.ProfilePictureUrl;
		OutData.FollowRole = CachedEvent.FollowRole;
		OutData.bIsModerator = CachedEvent.bIsModerator;
		OutData.bIsSubscriber = CachedEvent.bIsSubscriber;
		OutData.bIsNewGifter = CachedEvent.bIsNewGifter;
		OutData.TopGifterRank = CachedEvent.TopGifterRank;
		OutData.GifterLevel = CachedEvent.GifterLevel;
		OutData.TeamMemberLevel = CachedEvent.TeamMemberLevel;

		AsyncTask(ENamedThreads::GameThread, [this, OutData, EventId]() {
			Dispatcher->OnFollowEventProcessed.Broadcast(OutData, EventId);
		});
		UE_LOG(LogTemp, Log, TEXT("[EventQueue] Dispatched %s Id=%s UniqueId=%s"), *Follow.ToString(), *EventId.ToString(), *OutData.UniqueId);
	}
	else if (EventType == Gift)
	{
		FTSE_GiftOut OutData;
		OutData.UniqueId = CachedEvent.UniqueId;
		OutData.Nickname = CachedEvent.Nickname;
		OutData.ProfilePictureUrl = CachedEvent.ProfilePictureUrl;
		OutData.FollowRole = CachedEvent.FollowRole;
		OutData.bIsModerator = CachedEvent.bIsModerator;
		OutData.bIsSubscriber = CachedEvent.bIsSubscriber;
		OutData.bIsNewGifter = CachedEvent.bIsNewGifter;
		OutData.TopGifterRank = CachedEvent.TopGifterRank;
		OutData.GifterLevel = CachedEvent.GifterLevel;
		OutData.TeamMemberLevel = CachedEvent.TeamMemberLevel;
		OutData.GiftId = CachedEvent.GiftId;
		OutData.GiftName = CachedEvent.GiftName;
		OutData.GiftPictureUrl = CachedEvent.GiftPictureUrl;
		OutData.DiamondCount = CachedEvent.DiamondCount;
		OutData.GiftType = CachedEvent.GiftType;
		OutData.Describe = CachedEvent.Describe;

		AsyncTask(ENamedThreads::GameThread, [this, OutData, EventId]() {
			Dispatcher->OnGiftEventProcessed.Broadcast(OutData, EventId);
		});
		UE_LOG(LogTemp, Log, TEXT("[EventQueue] Dispatched %s Id=%s"), *Gift.ToString(), *EventId.ToString());
	}
	else if (EventType == GiftCombo)
	{
		FTSE_GiftComboOut OutData;
		// User
		OutData.UniqueId = CachedEvent.UniqueId;
		OutData.Nickname = CachedEvent.Nickname;
		OutData.ProfilePictureUrl = CachedEvent.ProfilePictureUrl;
		OutData.FollowRole = CachedEvent.FollowRole;
		OutData.bIsModerator = CachedEvent.bIsModerator;
		OutData.bIsSubscriber = CachedEvent.bIsSubscriber;
		OutData.bIsNewGifter = CachedEvent.bIsNewGifter;
		OutData.TopGifterRank = CachedEvent.TopGifterRank;
		OutData.GifterLevel = CachedEvent.GifterLevel;
		OutData.TeamMemberLevel = CachedEvent.TeamMemberLevel;
		// Gift
		OutData.GiftId = CachedEvent.GiftId;
		OutData.GiftName = CachedEvent.GiftName;
		OutData.GiftPictureUrl = CachedEvent.GiftPictureUrl;
		OutData.GiftType = CachedEvent.GiftType;
		OutData.Describe = CachedEvent.Describe;
		OutData.GroupId = CachedEvent.GroupId;
		// Aggregates
		OutData.RepeatCountTotal = CachedEvent.ComboRepeatCountTotal;
		OutData.DiamondTotal = CachedEvent.ComboDiamondTotal;
		OutData.FirstTs = CachedEvent.ComboFirstTs;
		OutData.LastTs = CachedEvent.ComboLastTs;
		OutData.DurationSeconds = static_cast<float>((CachedEvent.ComboLastTs - CachedEvent.ComboFirstTs).GetTotalSeconds());
		// Final state
		OutData.bIsFinal = CachedEvent.bComboFinal;
		OutData.bComboInitial = CachedEvent.bComboInitial;
		OutData.bClosedByInactivity = CachedEvent.bClosedByInactivity;
		OutData.bRepeatEnd = (CachedEvent.bComboFinal && !CachedEvent.bClosedByInactivity);

		AsyncTask(ENamedThreads::GameThread, [this, OutData, EventId]() {
			Dispatcher->OnGiftComboProcessed.Broadcast(OutData, EventId);
		});
		UE_LOG(LogTemp, Log, TEXT("[EventQueue] Dispatched %s Id=%s GroupId=%s (ComboSnapshot final=%d)"), *GiftCombo.ToString(), *EventId.ToString(), *OutData.GroupId, OutData.bIsFinal ? 1 : 0);
	}
	else if (EventType == Like)
	{
		FTSE_LikeBatchOut OutData;
		OutData.TotalLikeCount = CachedEvent.TotalLikeCount;
		
		// Include milestone fields if this is a milestone event
		if (bIsLikeMilestone)
		{
			OutData.LikeMilestone = CachedEvent.LikeMilestone;
			OutData.LikePreviousMilestone = CachedEvent.LikePreviousMilestone;
			OutData.LikeStepsCrossed = CachedEvent.LikeStepsCrossed;
			OutData.LikeDeltaSincePrevCommit = CachedEvent.LikeDeltaSincePrevCommit;
			OutData.LikeElapsedSincePrevCommitSec = CachedEvent.LikeElapsedSincePrevCommitSec;
			OutData.LikesPerSecond = CachedEvent.LikesPerSecond;
		}

		AsyncTask(ENamedThreads::GameThread, [this, OutData, EventId, bIsLikeMilestone]() {
			Dispatcher->OnLikeEventProcessed.Broadcast(OutData, EventId);
		});
		
		if (bIsLikeMilestone)
		{
			UE_LOG(LogTemp, Log, TEXT("[EventQueue] Dispatched %s Milestone Id=%s (Milestone=%d)"), 
				*Like.ToString(), *EventId.ToString(), CachedEvent.LikeMilestone);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[EventQueue] Dispatched %s Id=%s"), *Like.ToString(), *EventId.ToString());
		}
	}
	else if (EventType == LikeUser)
	{
		FTSE_LikeUserOut OutData;
		// User identity
		OutData.UniqueId = CachedEvent.UniqueId;
		OutData.Nickname = CachedEvent.Nickname;
		OutData.ProfilePictureUrl = CachedEvent.ProfilePictureUrl;
		// Roles/flags
		OutData.FollowRole = CachedEvent.FollowRole;
		OutData.bIsModerator = CachedEvent.bIsModerator;
		OutData.bIsSubscriber = CachedEvent.bIsSubscriber;
		OutData.bIsNewGifter = CachedEvent.bIsNewGifter;
		OutData.TopGifterRank = CachedEvent.TopGifterRank;
		OutData.GifterLevel = CachedEvent.GifterLevel;
		OutData.TeamMemberLevel = CachedEvent.TeamMemberLevel;
		// Likes
		OutData.LikeCount = CachedEvent.LikeCount;
		OutData.TotalLikeCount = static_cast<int64>(CachedEvent.TotalLikeCount);

		AsyncTask(ENamedThreads::GameThread, [this, OutData, EventId]() {
			Dispatcher->OnLikeUserEventProcessed.Broadcast(OutData, EventId);
		});
		UE_LOG(LogTemp, Log, TEXT("[EventQueue] Dispatched %s Id=%s (user=%s)"), *LikeUser.ToString(), *EventId.ToString(), *OutData.Nickname);
	}
	else if (EventType == MemberNormalized)
	{
		FTSE_MemberNormalizedOut OutData;
		OutData.JoinCount = CachedEvent.MemberJoinCount;
		OutData.GoalJoinCount = CachedEvent.MemberJoinGoal;
		
		// Populate the new milestone tracking fields
		OutData.CurrentMilestone = CachedEvent.MemberCurrentMilestone;
		OutData.PreviousMilestone = CachedEvent.MemberPreviousMilestone;
		OutData.StepsCrossed = CachedEvent.MemberStepsCrossed;
		OutData.DeltaJoins = CachedEvent.MemberDeltaJoins;
		
		// Log all payload fields before broadcast to verify they're not zero
		UE_LOG(LogTemp, Log, TEXT("[MemberNormalized] DISPATCH PAYLOAD: CurrentMilestone=%d, PreviousMilestone=%d, StepsCrossed=%d, DeltaJoins=%d, JoinCount=%d, GoalJoinCount=%d"), 
			OutData.CurrentMilestone, OutData.PreviousMilestone, OutData.StepsCrossed, OutData.DeltaJoins, OutData.JoinCount, OutData.GoalJoinCount);
		
		AsyncTask(ENamedThreads::GameThread, [this, OutData, EventId]() {
			Dispatcher->OnMemberNormalizedProcessed.Broadcast(OutData, EventId);
		});
		UE_LOG(LogTemp, Log, TEXT("[EventQueue] Dispatched %s Id=%s (Join=%d/%d, Milestone=%d)"), *MemberNormalized.ToString(), *EventId.ToString(), OutData.JoinCount, OutData.GoalJoinCount, OutData.CurrentMilestone);
	}
	else if (EventType == MemberIdentity)
	{
		FTSE_MemberOut OutData;
		OutData.UniqueId = CachedEvent.UniqueId;
		OutData.Nickname = CachedEvent.Nickname;
		OutData.ProfilePictureUrl = CachedEvent.ProfilePictureUrl;
		OutData.FollowRole = CachedEvent.FollowRole;
		OutData.bIsModerator = CachedEvent.bIsModerator;
		OutData.bIsSubscriber = CachedEvent.bIsSubscriber;
		OutData.bIsNewGifter = CachedEvent.bIsNewGifter;
		OutData.TopGifterRank = CachedEvent.TopGifterRank;
		OutData.GifterLevel = CachedEvent.GifterLevel;
		OutData.TeamMemberLevel = CachedEvent.TeamMemberLevel;
		OutData.ActionId = CachedEvent.ActionId;

		AsyncTask(ENamedThreads::GameThread, [this, OutData, EventId]() {
			Dispatcher->OnMemberIdentityProcessed.Broadcast(OutData, EventId);
		});
		UE_LOG(LogTemp, Log, TEXT("[EventQueue] Dispatched %s Id=%s (user=%s)"), *MemberIdentity.ToString(), *EventId.ToString(), *OutData.Nickname);
	}
	else if (EventType == RoomUserMilestone)
	{
		FTSE_RoomUserMilestoneOut OutData;
		OutData.ViewerCount = CachedEvent.ViewerCount;
		OutData.Milestone = CachedEvent.Milestone;
		OutData.PreviousMilestone = CachedEvent.PreviousMilestone;
		OutData.EmissionCount = CachedEvent.EmissionCount;
		OutData.bIsAscending = CachedEvent.bIsAscending;

		AsyncTask(ENamedThreads::GameThread, [this, OutData, EventId]() {
			Dispatcher->OnRoomUserMilestoneProcessed.Broadcast(OutData, EventId);
		});
		
		const FString DirectionStr = OutData.bIsAscending ? TEXT("ASC") : TEXT("DESC");
		
		UE_LOG(LogTemp, Log, TEXT("[EventQueue] Dispatched %s Id=%s (M=%d, prev=%d, count=%d, dir=%s, VC=%d) - M_last_committed updated to %d (thread-safe)"), 
			*RoomUserMilestone.ToString(), *EventId.ToString(), OutData.Milestone, OutData.PreviousMilestone, 
			OutData.EmissionCount, *DirectionStr, OutData.ViewerCount, OutData.Milestone);
	}
	// Dispatch RoomUser (snapshot completo)
	else if (EventType == RoomUser)
	{
		FTSE_RoomUserOut OutData;
		OutData.ViewerCount = CachedEvent.ViewerCount;
		OutData.TopViewers = CachedEvent.TopViewers;
		AsyncTask(ENamedThreads::GameThread, [this, OutData, EventId]() {
			Dispatcher->OnRoomUserProcessed.Broadcast(OutData, EventId);
		});
		UE_LOG(LogTemp, Log, TEXT("[EventQueue] Dispatched %s Id=%s (TopViewers=%d)"), *RoomUser.ToString(), *EventId.ToString(), OutData.TopViewers.Num());
	}
	// Nuevo derived: RoomUserTop1Change
	else if (EventType == RoomUserTop1Change)
	{
		FTSE_RoomUserTop1ChangeOut OutData;
		OutData.ViewerCount = CachedEvent.ViewerCount;
		if (CachedEvent.TopViewers.Num() > 0)
		{
			OutData.Top1 = CachedEvent.TopViewers[0];
		}
		AsyncTask(ENamedThreads::GameThread, [this, OutData, EventId]() {
			Dispatcher->OnRoomUserTop1ChangeProcessed.Broadcast(OutData, EventId);
		});
		UE_LOG(LogTemp, Log, TEXT("[EventQueue] Dispatched %s Id=%s (Top1=%s)"), *RoomUserTop1Change.ToString(), *EventId.ToString(), *OutData.Top1.UniqueId);
	}
	else if (EventType == Share)
	{
		FTSE_ShareOut OutData;
		OutData.UniqueId = CachedEvent.UniqueId;
		OutData.Nickname = CachedEvent.Nickname;
		OutData.ProfilePictureUrl = CachedEvent.ProfilePictureUrl;
		OutData.FollowRole = CachedEvent.FollowRole;
		OutData.bIsModerator = CachedEvent.bIsModerator;
		OutData.bIsSubscriber = CachedEvent.bIsSubscriber;
		OutData.bIsNewGifter = CachedEvent.bIsNewGifter;
		OutData.TopGifterRank = CachedEvent.TopGifterRank;
		OutData.GifterLevel = CachedEvent.GifterLevel;
		OutData.TeamMemberLevel = CachedEvent.TeamMemberLevel;

		AsyncTask(ENamedThreads::GameThread, [this, OutData, EventId]() {
			Dispatcher->OnShareEventProcessed.Broadcast(OutData, EventId);
		});
		UE_LOG(LogTemp, Log, TEXT("[EventQueue] Dispatched %s Id=%s"), *Share.ToString(), *EventId.ToString());
	}
	else if (EventType == ShareMilestone)
	{
		FTSE_ShareMilestoneOut OutData;
		OutData.ShareTotalCount = CachedEvent.ShareTotalCount;
		OutData.GoalShareCount = CachedEvent.GoalShareCount;
		OutData.CurrentMilestone = CachedEvent.ShareCurrentMilestone;
		OutData.PreviousMilestone = CachedEvent.SharePreviousMilestone;
		OutData.StepsCrossed = CachedEvent.ShareStepsCrossed;
		OutData.DeltaShares = CachedEvent.ShareDeltaShares;

		// Consistency: defer COMMIT/handle invalidation to ConfirmEventProcessed (like MemberNormalized)
		AsyncTask(ENamedThreads::GameThread, [this, OutData, EventId]() {
			Dispatcher->OnShareMilestoneProcessed.Broadcast(OutData, EventId);
		});
		
		// Compact log with all 6 fields for debugging
		UE_LOG(LogTemp, Log, TEXT("[ShareMilestone] DISPATCH: Curr=%d Prev=%d Total=%d Goal=%d Steps=%d Delta=%d"), 
			OutData.CurrentMilestone, OutData.PreviousMilestone, OutData.ShareTotalCount, OutData.GoalShareCount, OutData.StepsCrossed, OutData.DeltaShares);
		UE_LOG(LogTemp, Log, TEXT("[EventQueue] Dispatched %s Id=%s (Current=%d, Prev=%d)"), 
			*ShareMilestone.ToString(), *EventId.ToString(), OutData.CurrentMilestone, OutData.PreviousMilestone);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[EventQueue] Unknown event type for dispatch: %s"), *EventType.ToString());
		return false;
	}

	// Mark as dispatched
	bLockedEventDispatched = true;
	return true;
}

bool UTikStudioEventQueue::PumpOnceIfFree()
{
	// Clear any pending request at the start of the pump
	bPumpRequestPending = false;

	if (bIsPumping)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EventQueue] PumpOnceIfFree: re-entrant call ignored"));
		return false;
	}
	TGuardValue<bool> ReentryGuard(bIsPumping, true);

	// Ensure we sweep expired items before attempting to pump
	SweepExpired();

	// Recompute priorities just before pumping if enabled
	if (Settings.bRecomputeOnPump)
	{
		RecomputePriorities();
	}

	if (HasLockedEvent())
	{
		UE_LOG(LogTemp, Log, TEXT("[EventQueue] PumpOnceIfFree: busy (locked)"));
		bPumpRequestedWhileBusy.store(true, std::memory_order_relaxed); // NEW
		RoomUserState.bForceRoomUserSnapshotOnNextPump = true; // NEW
		return false;
	}

	FQueuedTikTokEvent OutEvent;
	if (!DequeueNextEvent(OutEvent))
	{
		UE_LOG(LogTemp, Log, TEXT("[EventQueue] PumpOnceIfFree: empty"));
		return false;
	}

	if (!DispatchLockedEvent())
	{
		UE_LOG(LogTemp, Warning, TEXT("[EventQueue] PumpOnceIfFree: dispatch failed"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[EventQueue] PumpOnceIfFree: dispatched Type=%s Id=%s"), *OutEvent.EventType.ToString(), *OutEvent.Id.ToString());
	return true;
}

int32 UTikStudioEventQueue::GetQueuedCount() const
{
	FScopeLock Lock(&CriticalSection);
	return Queue.Num();
}

bool UTikStudioEventQueue::IsFree() const
{
	return !HasLockedEvent();
}

int32 UTikStudioEventQueue::GetCountByType(FName Type)
{
	FScopeLock Lock(&CriticalSection);
	int32 Count = 0;
	for (const FQueuedTikTokEvent& E : Queue)
	{
		if (E.EventType == Type)
		{
			Count++;
		}
	}
	return Count;
}

int32 UTikStudioEventQueue::GetCountByType_NoLock(FName Type) const
{
	int32 Count = 0;
	for (const FQueuedTikTokEvent& E : Queue)
	{
		if (E.EventType == Type)
		{
			++Count;
		}
	}
	return Count;
}

void UTikStudioEventQueue::RecomputePriorities()
{
	FScopeLock Lock(&CriticalSection);

	// Recalculate priority for each queued item (locked event is not in Queue)
	for (FQueuedTikTokEvent& E : Queue)
	{
		E.PriorityScore = ComputePriority(E);
	}

	// Sort descending by PriorityScore; tie-breaker by Timestamp (older first)
	Queue.Sort([](const FQueuedTikTokEvent& A, const FQueuedTikTokEvent& B)
	{
		if (A.PriorityScore != B.PriorityScore)
		{
			return A.PriorityScore > B.PriorityScore;
		}
		return A.Timestamp < B.Timestamp;
	});
}

bool UTikStudioEventQueue::HasLockedEvent() const
{
	return bHasLockedEvent;
}

FGuid UTikStudioEventQueue::GetLockedEventId() const
{
	return LockedEventId;
}

int32 UTikStudioEventQueue::GetBaseWeight(FName EventType) const
{
	if (const int32* Weight = Settings.BaseWeights.Find(EventType))
	{
		return *Weight;
	}
	return 0;
}

float UTikStudioEventQueue::GetTTLForType(FName EventType) const
{
	if (const float* TTL = Settings.TTLSeconds.Find(EventType))
	{
		return *TTL;
	}
	return 0.0f;
}

int32 UTikStudioEventQueue::GetMaxSlotsForType(FName EventType) const
{
	if (const int32* Slots = Settings.MaxSlots.Find(EventType))
	{
		return *Slots;
	}

	// Fallback seguro
#if WITH_EDITOR || UE_BUILD_DEVELOPMENT
	UE_LOG(LogTemp, Verbose, TEXT("[Settings] MaxSlots fallback for %s"), *EventType.ToString());
#endif
	if (EventType == Gift)
	{
		return 10; // fallback para Gift normal
	}
	if (EventType == GiftCombo)
	{
		return 1000; // fallback para GiftCombo (alta capacidad)
	}
	return 0; // resto se mantiene en 0 si falta configuración
}

EEventExpirePolicy UTikStudioEventQueue::GetExpirePolicyForType(FName EventType) const
{
	if (const EEventExpirePolicy* Policy = Settings.ExpirePolicies.Find(EventType))
	{
		return *Policy;
	}
	return EEventExpirePolicy::Discard;
}

int32 UTikStudioEventQueue::ApplyCommonModifiers(const FQueuedTikTokEvent& E) const
{
	int32 Score = 0;

	auto GetCommonModifier = [this](const FName& Key, int32 LegacyFallback)
	{
		if (const int32* Modifier = Settings.CommonModifiers.Find(Key))
		{
			return *Modifier;
		}
		return LegacyFallback;
	};

	// FollowRole: configurable points per role level (lineal: puntos × nivel de rol).
	// Usa el mismo helper que el resto de modificadores comunes para que el fallback
	// (cuando no hay clave en CommonModifiers) sea COHERENTE con la ruta configurada:
	// fallback=5 → rol1=+5, rol2=+10 (antes el fallback era no-lineal 5/12, lo que daba
	// resultados distintos según si la clave existía o no).
	if (E.FollowRole > 0)
	{
		Score += GetCommonModifier(TikStudioModifierKeys::FollowRole, 5) * E.FollowRole;
	}

	// bIsModerator: configurable via Settings.CommonModifiers, fallback keeps legacy behavior.
	if (E.bIsModerator)
	{
		Score += GetCommonModifier(TikStudioModifierKeys::Moderator, 15);
	}

	// bIsSubscriber: configurable via Settings.CommonModifiers.
	if (E.bIsSubscriber)
	{
		Score += GetCommonModifier(TikStudioModifierKeys::Subscriber, 10);
	}

	// bIsNewGifter: configurable via Settings.CommonModifiers.
	if (E.bIsNewGifter)
	{
		Score += GetCommonModifier(TikStudioModifierKeys::NewGifter, 15);
	}

	// TopGifterRank: rank 1 receives the largest configurable bonus.
	{
		const int32 TopRank = FMath::Clamp(E.TopGifterRank, 0, 100);
		if (TopRank >= 1 && TopRank <= 5)
		{
			const int32 TopGifterRankBonus = GetCommonModifier(TikStudioModifierKeys::TopGifterRank, 4);
			Score += (6 - TopRank) * TopGifterRankBonus;
		}
	}

	// GifterLevel: configurable points per level, cap alto para Chat/LikeUser/Gift.
	{
		int32 GifterBonus = static_cast<int32>(1.5f * E.GifterLevel);
		if (const int32* GifterLevelBonus = Settings.CommonModifiers.Find(TikStudioModifierKeys::GifterLevel))
		{
			GifterBonus = (*GifterLevelBonus) * E.GifterLevel;
		}
		const bool bHighCapType = (E.EventType == Chat) || (E.EventType == LikeUser) || (E.EventType == Gift) || (E.EventType == GiftCombo);
		// Cap calibrado al nivel máximo (50): 75 = legacy 1.5 * 50. Con el modificador por defecto (1) el aporte llega solo a 50 y el cap no se alcanza.
		const int32 GifterMax = bHighCapType ? 75 : 20;
		Score += FMath::Clamp(GifterBonus, 0, GifterMax);
	}

	// TeamMemberLevel: configurable points per level, cap alto para Chat/LikeUser/Gift.
	{
		int32 TeamBonus = 2 * E.TeamMemberLevel;
		if (const int32* TeamMemberLevelBonus = Settings.CommonModifiers.Find(TikStudioModifierKeys::TeamMemberLevel))
		{
			TeamBonus = (*TeamMemberLevelBonus) * E.TeamMemberLevel;
		}
		const bool bHighCapType = (E.EventType == Chat) || (E.EventType == LikeUser) || (E.EventType == Gift) || (E.EventType == GiftCombo);
		// Cap calibrado al nivel máximo (50): 100 = legacy 2 * 50. Con el modificador por defecto (1) el aporte llega solo a 50 y el cap no se alcanza.
		const int32 TeamMax = bHighCapType ? 100 : 20;
		Score += FMath::Clamp(TeamBonus, 0, TeamMax);
	}

	return Score;
}

int32 UTikStudioEventQueue::ApplySpecificModifiers(const FQueuedTikTokEvent& E) const
{
	int32 Score = 0;

	auto GetSpecificModifier = [this](FName EventType, const FName& Key, int32 LegacyFallback)
	{
		if (const FEventTypeModifiers* TypeModifiers = Settings.SpecificModifiers.Find(EventType))
		{
			if (const int32* Modifier = TypeModifiers->Modifiers.Find(Key))
			{
				return *Modifier;
			}
		}
		return LegacyFallback;
	};

	if (E.EventType == Gift)
	{
		// Componente lineal de diamantes: configurable points per 10 diamonds.
		const int32 DiamondPoints = (E.DiamondCount / 10) * GetSpecificModifier(Gift, TikStudioModifierKeys::DiamondCount, 1);
		Score += DiamondPoints;
		
		// bRepeatEnd bonus: configurable via Settings.SpecificModifiers[Gift].
		if (E.bRepeatEnd)
		{
			Score += GetSpecificModifier(Gift, TikStudioModifierKeys::RepeatEnd, 10);
		}
		
		// Componente lineal de repeticiones: configurable points per repeat.
		const int32 RepeatPoints = E.RepeatCount * GetSpecificModifier(Gift, TikStudioModifierKeys::RepeatCount, 3);
		Score += RepeatPoints;
	}
	else if (E.EventType == GiftCombo)
	{
		// Componente lineal de diamantes acumulados: configurable points per 10 diamonds.
		const int32 DiamondPoints = (E.ComboDiamondTotal / 10) * GetSpecificModifier(GiftCombo, TikStudioModifierKeys::TotalDiamondCount, 1);
		Score += DiamondPoints;
		
		// Componente lineal de repeticiones acumuladas: configurable points per repeat.
		const int32 RepeatPoints = E.ComboRepeatCountTotal * GetSpecificModifier(GiftCombo, TikStudioModifierKeys::TotalRepeatCount, 3);
		Score += RepeatPoints;
		
		// bIsFinalSnapshot bonus: configurable via Settings.SpecificModifiers[GiftCombo].
		if (E.bComboFinal)
		{
			Score += GetSpecificModifier(GiftCombo, TikStudioModifierKeys::FinalSnapshot, 15);
		}
	}
	else if (E.EventType == Chat)
	{
		// HasCommand bonus: configurable via Settings.SpecificModifiers[Chat].
		if (E.bHasCommand)
		{
			Score += GetSpecificModifier(Chat, TikStudioModifierKeys::HasCommand, 20);
		}
	}
	else if (E.EventType == Like)
	{
		// TotalLikeCount: configurable points per 1000 total likes.
		Score += GetSpecificModifier(Like, TikStudioModifierKeys::TotalLikeCount, 5) * (E.TotalLikeCount / 1000);
		
		// Bonus específico para Like milestones.
		// NOTA: antes el guard era 'E.Milestone > 0', pero E.Milestone es el campo de RoomUser
		// y en los eventos Like siempre vale 0, por lo que este bloque nunca se ejecutaba.
		// Se corrige a E.LikeMilestone (el campo que sí setea el evento Like milestone).
		if (E.LikeMilestone > 0)
		{
			// Bonus por steps cruzados: +10 por cada step adicional
			// Esto hace que saltos grandes (ej: 3 steps) tengan mayor prioridad
			const int32 StepsCrossedBonus = 10 * FMath::Max(1, E.LikeStepsCrossed - 1);
			Score += StepsCrossedBonus;
			
			// Bonus por likes desde el último commit: configurable points per 100 likes.
			const int32 LikeCountForScore = E.LikeCount > 0 ? E.LikeCount : E.LikeDeltaSincePrevCommit;
			const int32 DeltaLikesBonus = GetSpecificModifier(Like, TikStudioModifierKeys::LikeCount, 1) * (LikeCountForScore / 100);
			Score += DeltaLikesBonus;
		}
	}
	else if (E.EventType == LikeUser)
	{
		// LikeCount: configurable points per like count reported by this user event.
		Score += GetSpecificModifier(LikeUser, TikStudioModifierKeys::LikeCount, 0) * E.LikeCount;

		// TotalLikeCount: configurable points per 1000 total likes.
		Score += GetSpecificModifier(LikeUser, TikStudioModifierKeys::TotalLikeCount, 0) * (E.TotalLikeCount / 1000);
	}
	else if (E.EventType == RoomUserMilestone)
	{
		// Milestone: configurable points por cada nivel de hito de audiencia cruzado.
		// (E.Milestone / Step) = número de hito alcanzado (1.º, 2.º...). Guard Step>0 evita división por cero.
		if (Settings.RoomUserConfig.RoomUserMilestoneStep > 0)
		{
			Score += GetSpecificModifier(RoomUserMilestone, TikStudioModifierKeys::Milestone, 15) * (E.Milestone / Settings.RoomUserConfig.RoomUserMilestoneStep);
		}
	}
	else if (E.EventType == RoomUser)
	{
		// Snapshot completo: escala por ViewerCount
		if (Settings.RoomUserConfig.RoomUserMilestoneStep > 0)
		{
			Score += GetSpecificModifier(RoomUser, TikStudioModifierKeys::ViewerCount, 15) * (E.ViewerCount / Settings.RoomUserConfig.RoomUserMilestoneStep);
		}
	}
	// Follow and Share: 0 (hooks ready)

	return Score;
}

int32 UTikStudioEventQueue::ComputePriority(const FQueuedTikTokEvent& E) const
{
	int32 Base = GetBaseWeight(E.EventType);
	int32 Common = (E.EventType == Like) ? 0 : ApplyCommonModifiers(E);
	int32 Specific = ApplySpecificModifiers(E);
	int32 Total = Base + Common + Specific;

	// Fairness: aging bonus (optional)
	if (Settings.AgingPointsPerSecond > 0.0f)
	{
		const FDateTime NowUtc = FDateTime::UtcNow();
		const double AgeSeconds = (NowUtc - E.Timestamp).GetTotalSeconds();
		const int32 AgingBonus = FMath::Clamp(int32(Settings.AgingPointsPerSecond * AgeSeconds), 0, Settings.AgingMaxBonus);
		Total += AgingBonus;
#if WITH_EDITOR || UE_BUILD_DEVELOPMENT
		UE_LOG(LogTemp, Verbose, TEXT("[Fairness] AgeBonus=%d Age=%.1fs Type=%s Id=%s"), AgingBonus, AgeSeconds, *E.EventType.ToString(), *E.Id.ToString());
#endif
	}

	// Sin clampeo - retorna valor bruto con protección de negativos
	int32 Final = FMath::Max(Total, 0);

#if WITH_EDITOR || UE_BUILD_DEVELOPMENT
	// Verbose: evita inundar la consola en O(N^2) cuando la cola se recomputa estando llena.
	// No afecta la matemática del ordenamiento; solo la verbosidad del log.
	UE_LOG(LogTemp, Verbose, TEXT("ComputePriority for %s: Base=%d, Common=%d, Specific=%d, Aging=%d, Final=%d"),
		*E.EventType.ToString(), Base, Common, Specific, (Total - Base - Common - Specific), Final);
#endif

	return Final;
}

bool UTikStudioEventQueue::DequeueNextEvent(FQueuedTikTokEvent& OutEvent)
{
	// ========== SB + COPIAS MODEL: Elegibilidad y pago de deuda ==========
	
	FScopeLock Lock(&CriticalSection);

	if (bHasLockedEvent)
	{
		// Already have a locked event; cannot dequeue new one
		return false;
	}

	if (Queue.Num() == 0)
	{
		return false;
	}

	// ========== ITERATE QUEUE TO FIND ELIGIBLE EVENT ==========
	// SBs without debt are NOT eligible (skip them)
	// SBs with debt ARE eligible (pay the debt by creating SC)
	// Non-combo events are ALWAYS eligible
	
	for (int32 i = 0; i < Queue.Num(); ++i)
	{
		FQueuedTikTokEvent& Candidate = Queue[i];
		
		// ========== CASE 1: GiftCombo (SB) ==========
		if (Candidate.EventType == GiftCombo && Candidate.bIsSnapshotBase)
		{
			// Find the combo state
			FGiftComboState* ComboState = LiveCombos.Find(Candidate.ComboKey);
			
			if (!ComboState)
			{
				// ========== FAILSAFE: SB huérfano (tracking perdido) ==========
				// Si el SB está marcado como Final, NO perder el combo: sintetizar SC Final
				if (Candidate.bComboFinal)
				{
					// ========== RECUPERACIÓN COMPLETA: SB huérfano FINAL → SC Final ==========
					OutEvent = Candidate; // Copiar todos los campos del SB
					
					// Conversión completa a SC
					OutEvent.bIsSnapshotBase = false; // YA NO es SB
					OutEvent.bIsComboSnapshot = true; // Confirmar que es snapshot
					OutEvent.Id = FGuid::NewGuid(); // Nuevo GUID para la SC
					
					// Forzar flags correctos para Final
					OutEvent.bComboFinal = true; // Mantener (ya viene del SB)
					OutEvent.bComboInitial = false; // Forzar a false (es Final, no Initial)
					
					// Preservar Timestamp y TTL originales (aging correcto)
					// OutEvent.Timestamp ya viene del SB
					// OutEvent.TTLSeconds ya viene del SB
					
					// Eliminar SB huérfano de Queue ANTES de bloquear
					Queue.RemoveAt(i);
					
					// Métrica
					if (Settings.Eviction.bTrackEvictionMetrics)
					{
						++SBRecoveredFromOrphan;
					}
					
					UE_LOG(LogTemp, Warning, TEXT("[SB] RECOVER: FINAL orphan → synthesized SC Final Key=%s rc=%d diamonds=%d (tracking lost but combo SAVED)"),
						*Candidate.ComboKey, Candidate.ComboRepeatCountTotal, Candidate.ComboDiamondTotal);
					
					// ========== BLOQUEO COMPLETO DE LA SC ==========
					LockedEventCache = OutEvent;
					bLockedEventCacheValid = true;
					bHasLockedEvent = true;
					bLockedEventDispatched = false;
					LockedEventId = OutEvent.Id;
					LockedEventType = GiftCombo; // Asegurar tipo correcto
					return true;
				}
				else
				{
					// SB huérfano NO-Final: descartar (puede ser recuperado con próximo regalo o re-encolado)
					UE_LOG(LogTemp, Warning, TEXT("[SB] Orphan non-final SB dropped Key=%s (tracking lost, not recoverable)"), *Candidate.ComboKey);
					Queue.RemoveAt(i);
					i--; // Adjust index
					continue;
				}
			}
			
			// ========== RULE: rc=1 closed → eliminate SB directly ==========
			if (ComboState->bClosed && ComboState->RepeatCountTotal == 1)
			{
				FString KeyToRemove = ComboState->ComboKey; // Copy before RemoveAt to avoid reference corruption
				UE_LOG(LogTemp, Log, TEXT("[SB] Eliminating SB for rc=1 combo Key=%s"), *KeyToRemove);
				Queue.RemoveAt(i);
				RemoveComboTracking_Locked(KeyToRemove, TEXT("Rc1Closed"));
				i--;
				continue;
			}
			
			// ========== RULE: SB without debt → NOT eligible, skip ==========
			if (ComboState->PendingDebt == EComboDebtType::None)
			{
				UE_LOG(LogTemp, Verbose, TEXT("[SB] Skipping SB without debt Key=%s"), *ComboState->ComboKey);
				continue; // Move to next candidate
			}
			
			// ========== RULE: SB with debt → ELIGIBLE, pay debt ==========
			// Create the Snapshot Copy (SC) from current state
			OutEvent = CreateSnapshotCopy_Locked(*ComboState);
			
			UE_LOG(LogTemp, Log, TEXT("[SB] Paying debt %s Key=%s rc=%d diamonds=%d"),
				*DebtTypeToString(ComboState->PendingDebt), *ComboState->ComboKey, 
				ComboState->RepeatCountTotal, ComboState->DiamondTotal);
			
			// Saldar la deuda
			EComboDebtType PaidDebt = ComboState->PendingDebt;
			ComboState->PendingDebt = EComboDebtType::None;
			
			// Mark for transitional logging
			if (PaidDebt == EComboDebtType::Initial)
			{
				ComboState->bHasEmittedInitialSnapshot = true;
			}
			
			// ========== IF FINAL DEBT: Eliminate SB and tracking ==========
			if (PaidDebt == EComboDebtType::Final)
			{
				FString KeyToRemove = ComboState->ComboKey; // Copy before RemoveAt to avoid reference corruption
				UE_LOG(LogTemp, Log, TEXT("[SB] Eliminating SB after paying Final debt Key=%s"), *KeyToRemove);
				Queue.RemoveAt(i);
				RemoveComboTracking_Locked(KeyToRemove, TEXT("FinalPaid"));
			}
			// ========== IF NOT FINAL: SB remains in Queue (may accumulate more debt later) ==========
			else
			{
				// SB stays in Queue, debt cleared, may receive more gifts
				UE_LOG(LogTemp, Verbose, TEXT("[SB] SB remains in Queue after paying %s debt Key=%s"), 
					*DebtTypeToString(PaidDebt), *ComboState->ComboKey);
			}
			
			// ========== LOCK THE SC ==========
			LockedEventCache = OutEvent;
			bLockedEventCacheValid = true;
			
			bHasLockedEvent = true;
			bLockedEventDispatched = false;
			LockedEventId = OutEvent.Id;
			LockedEventType = OutEvent.EventType;
			LockedComboKey = ComboState->ComboKey; // Track combo in lock for coalescence
			
			return true;
		}
		// ========== CASE 2: Other event types (Chat, Gift, Like, etc.) ==========
		else
		{
			// Always eligible
			OutEvent = Candidate;
			
			// ========== FASE 1: Invalidar handle de milestone al dequeue ==========
			if (Candidate.EventType == RoomUserMilestone && Candidate.Id == RoomUserState.LastMilestoneId)
			{
				RoomUserState.LastMilestoneId.Invalidate();
				RoomUserState.bHasMilestoneInQueue = false;
				UE_LOG(LogTemp, Verbose, TEXT("[EventQueue] Milestone handle invalidated on dequeue (M=%d)"), Candidate.Milestone);
			}
			
			// Fill locked cache before removing from Queue
			LockedEventCache = OutEvent;
			bLockedEventCacheValid = true;
			Queue.RemoveAt(i);
			
			// Lock it
			bHasLockedEvent = true;
			bLockedEventDispatched = false;
			LockedEventId = OutEvent.Id;
			LockedEventType = OutEvent.EventType;
			
			return true;
		}
	}
	
	// ========== NO ELIGIBLE EVENTS FOUND ==========
	// Queue may contain only SBs without debt (waiting for gifts/thresholds)
	return false;
	
	// ========== END SB + COPIAS MODEL ==========
}

void UTikStudioEventQueue::ConfirmEventProcessed(const FGuid& EventId)
{
	bool WantsPumpAfter = false;
	{
		FScopeLock Lock(&CriticalSection);
		if (!bHasLockedEvent || LockedEventId != EventId)
		{
			UE_LOG(LogTemp, Warning, TEXT("[EventQueue] ConfirmEventProcessed mismatch or no lock. Incoming=%s Current=%s HasLock=%d"), *EventId.ToString(), *LockedEventId.ToString(), bHasLockedEvent ? 1 : 0);
			return;
		}

		// Invalidar handle de RoomUser snapshot tras despachar
		if (LockedEventType == RoomUser && EventId == RoomUserState.LastRoomUserSnapshotId)
		{
			RoomUserState.LastRoomUserSnapshotId.Invalidate();
			RoomUserState.bHasRoomUserSnapshotInQueue = false;
			UE_LOG(LogTemp, Verbose, TEXT("[EventQueue] RoomUser snapshot handle invalidated after dispatch"));
		}

		// Commit MemberNormalized tras despachar exitoso
		if (LockedEventType == MemberNormalized && EventId == MemberNormalizedState.MN_LastId)
		{
			// Actualizar MN_last_committed al CurrentMilestone del evento despachado
			// Necesitamos obtener el CurrentMilestone del evento locked
			if (bLockedEventCacheValid)
			{
				MemberNormalizedState.MN_last_committed = LockedEventCache.MemberCurrentMilestone;
				UE_LOG(LogTemp, Log, TEXT("[MemberNormalized] COMMIT: milestone advanced to %d"), MemberNormalizedState.MN_last_committed);
			}
			
			// Invalidar handle
			MemberNormalizedState.MN_LastId.Invalidate();
			MemberNormalizedState.MN_bHasInQueue = false;
			UE_LOG(LogTemp, Log, TEXT("[MemberNormalized] COMMIT: handle invalidated after successful dispatch"));
		}

		// Commit ShareMilestone tras despachar exitoso
		if (LockedEventType == ShareMilestone && EventId == ShareMilestoneState.SM_LastId)
		{
			// Actualizar SM_last_committed al CurrentMilestone del evento despachado
			if (bLockedEventCacheValid)
			{
				ShareMilestoneState.SM_last_committed = LockedEventCache.ShareCurrentMilestone;
				UE_LOG(LogTemp, Verbose, TEXT("[ShareMilestone] COMMIT (milestone %d)"), ShareMilestoneState.SM_last_committed);
			}
			
			// Invalidar handle
			ShareMilestoneState.SM_LastId.Invalidate();
			ShareMilestoneState.SM_bHasInQueue = false;
			
			// Check if there's enough remainder to create next ShareMilestone immediately
		int32 ShareMilestoneStep = Settings.ShareConfig.ShareMilestoneStep;
		int32 StepsCrossed = ShareMilestoneState.SM_AccumRemainder / ShareMilestoneStep;
		
		UE_LOG(LogTemp, Verbose, TEXT("[ShareMilestone] POST-COMMIT: SM_AccumRemainder=%d, ShareMilestoneStep=%d, StepsCrossed=%d"), 
			ShareMilestoneState.SM_AccumRemainder, ShareMilestoneStep, StepsCrossed);
		
		if (StepsCrossed >= 1 && Settings.EventToggles.Share.bEnableShareMilestone)
		{
			// Create next ShareMilestone event immediately to capture accumulated shares during lock
			int32 DeltaShares = StepsCrossed * ShareMilestoneStep;
			int32 CurrentMilestone = ShareMilestoneState.SM_last_committed + DeltaShares;
			int32 PreviousMilestone = ShareMilestoneState.SM_last_committed;
			
			UE_LOG(LogTemp, Verbose, TEXT("[ShareMilestone] POST-COMMIT: DeltaShares=%d, CurrentMilestone=%d, PreviousMilestone=%d"), 
				DeltaShares, CurrentMilestone, PreviousMilestone);
			
			// Consume remainder
			ShareMilestoneState.SM_AccumRemainder -= DeltaShares;
			
			UE_LOG(LogTemp, Verbose, TEXT("[ShareMilestone] POST-COMMIT: SM_AccumRemainder after consume=%d"), ShareMilestoneState.SM_AccumRemainder);
				
				// Create ShareMilestone event
				FQueuedTikTokEvent SMEvent;
				SMEvent.Id = FGuid::NewGuid();
				SMEvent.EventType = ShareMilestone;
				SMEvent.Timestamp = FDateTime::UtcNow();
				SMEvent.TTLSeconds = GetTTLForType(ShareMilestone);
				SMEvent.PriorityScore = GetBaseWeight(ShareMilestone);
				
				// Apply common modifiers by default
				SMEvent.PriorityScore += ApplyCommonModifiers(SMEvent);
				
			// Set ShareMilestone-specific payload
			SMEvent.ShareTotalCount = CurrentMilestone;
			SMEvent.GoalShareCount = CurrentMilestone + ShareMilestoneStep;
			SMEvent.ShareCurrentMilestone = CurrentMilestone;
			SMEvent.SharePreviousMilestone = PreviousMilestone;
			SMEvent.ShareStepsCrossed = StepsCrossed;
			SMEvent.ShareDeltaShares = DeltaShares;
				
				ShareMilestoneState.SM_LastId = SMEvent.Id;
				ShareMilestoneState.SM_bHasInQueue = true;
				
				EnqueueTypedEvent(ShareMilestone, SMEvent);
				UE_LOG(LogTemp, Verbose, TEXT("[ShareMilestone] POST-COMMIT INSERT (Current=%d/Prev=%d, captured during lock)"), CurrentMilestone, PreviousMilestone);
				RequestImmediatePump(TEXT("ShareMilestone POST-COMMIT"));
			}
		}

		// Clean up ChatShadowByUser for processed Chat event
		if (LockedEventType == Chat && bLockedEventCacheValid)
		{
			const FString& ProcessedUniqueId = LockedEventCache.UniqueId;
			if (ChatShadowByUser.Contains(ProcessedUniqueId))
			{
				const FGuid& ShadowEventId = ChatShadowByUser[ProcessedUniqueId];
				if (ShadowEventId == EventId)
				{
					ChatShadowByUser.Remove(ProcessedUniqueId);
					UE_LOG(LogTemp, Log, TEXT("[EventQueue] Removed Chat shadow for processed UniqueId=%s"), *ProcessedUniqueId);
				}
			}
		}

		// Release lock state
		bHasLockedEvent = false;
		bLockedEventDispatched = false;
		LockedComboKey.Empty(); // Clear combo lock tracking
		LockedEventId.Invalidate();
		LockedEventType = NAME_None;

		// Clear locked event cache
		bLockedEventCacheValid = false;
		LockedEventCache = FQueuedTikTokEvent();

		// No pending snapshots to emit - all snapshots go directly to Queue
		// Queue already contains all necessary events, ordered by priority

		WantsPumpAfter = bWantsImmediatePump;
		bWantsImmediatePump = false;
	}

	// Schedule pump(s) outside the lock, respecting empty queue guard
	const int32 Size = GetQueueSize();
	
	// Re-solicitar pump inmediatamente si había pump pendiente mientras estaba busy
	if (bPumpRequestedWhileBusy.exchange(false, std::memory_order_acq_rel))
	{
		RequestImmediatePump(TEXT("AfterConfirm-RoomUserPending"));
	}
	
	if (Settings.bPumpAfterConfirm && Size > 0)
	{
		RequestImmediatePump(TEXT("AfterConfirm"));
	}
	if (WantsPumpAfter && Size > 0)
	{
		RequestImmediatePump(TEXT("CombosPending"));
	}
}

bool UTikStudioEventQueue::PeekNextEvent(FQueuedTikTokEvent& OutEvent)
{
	FScopeLock Lock(&CriticalSection);
	if (bHasLockedEvent || Queue.Num() == 0)
	{
		return false;
	}
	OutEvent = Queue[0];
	return true;
}

int32 UTikStudioEventQueue::GetQueueSize()
{
	FScopeLock Lock(&CriticalSection);
	return Queue.Num();
}


// ═══════════════════════════════════════════════════════════════════════════════════
// EVENT HANDLERS — Extracted to separate .cpp files (Epic multi-cpp pattern)
// See: TikStudioEventQueue_Follow.cpp    → EnqueueFollowEvent
//      TikStudioEventQueue_Like.cpp      → EnqueueLikeEvent
//      TikStudioEventQueue_Member.cpp    → EnqueueMemberEvent
//      TikStudioEventQueue_RoomUser.cpp  → EnqueueRoomUserEvent + Milestone helpers
//      TikStudioEventQueue_Share.cpp     → EnqueueShareEvent
// ═══════════════════════════════════════════════════════════════════════════════════


void UTikStudioEventQueue::AutoPumpTick()
{
	// Periodic auto pump
	PumpOnceIfFree();
}

void UTikStudioEventQueue::SetQueueSettings(const FTikStudioEventQueueSettings& InSettings, bool bRecomputeNow)
{
	// Apply settings
	Settings = InSettings;
	
	// Clamp ShareMilestoneStep to prevent division by zero
	Settings.ShareConfig.ShareMilestoneStep = FMath::Max(1, Settings.ShareConfig.ShareMilestoneStep);

	// --- Soft validation / defaults ---
	auto EnsureDefault = [](TMap<FName, int32>& MapRef, const FName& Key, int32 DefaultValue)
	{
		if (!MapRef.Contains(Key))
		{
			MapRef.Add(Key, DefaultValue);
			UE_LOG(LogTemp, Verbose, TEXT("[EventQueue][Config] Missing BaseWeights[%s] -> default %d applied"), *Key.ToString(), DefaultValue);
		}
	};
	auto EnsureDefaultFloat = [](TMap<FName, float>& MapRef, const FName& Key, float DefaultValue)
	{
		if (!MapRef.Contains(Key))
		{
			MapRef.Add(Key, DefaultValue);
			UE_LOG(LogTemp, Verbose, TEXT("[EventQueue][Config] Missing TTLSeconds[%s] -> default %.2f applied"), *Key.ToString(), DefaultValue);
		}
	};
	auto EnsureDefaultSlots = [](TMap<FName, int32>& MapRef, const FName& Key, int32 DefaultValue)
	{
		if (!MapRef.Contains(Key))
		{
			MapRef.Add(Key, DefaultValue);
			UE_LOG(LogTemp, Verbose, TEXT("[EventQueue][Config] Missing MaxSlots[%s] -> default %d applied"), *Key.ToString(), DefaultValue);
		}
	};

	EnsureDefault(Settings.BaseWeights, Chat, 40);
	EnsureDefault(Settings.BaseWeights, Gift, 70);
	EnsureDefault(Settings.BaseWeights, GiftCombo, 80);
	EnsureDefault(Settings.BaseWeights, Follow, 60);
	EnsureDefault(Settings.BaseWeights, Like, 25);
	EnsureDefault(Settings.BaseWeights, LikeUser, 10);
	EnsureDefault(Settings.BaseWeights, MemberIdentity, 5);
	EnsureDefault(Settings.BaseWeights, MemberNormalized, 20);
	EnsureDefault(Settings.BaseWeights, RoomUserMilestone, 30);
	EnsureDefault(Settings.BaseWeights, RoomUser, 35);
	EnsureDefault(Settings.BaseWeights, RoomUserTop1Change, 50);
	EnsureDefault(Settings.BaseWeights, Share, 55);

	EnsureDefaultFloat(Settings.TTLSeconds, Chat, 8.0f);
	EnsureDefaultFloat(Settings.TTLSeconds, Gift, 45.0f);
	EnsureDefaultFloat(Settings.TTLSeconds, GiftCombo, 60.0f);
	EnsureDefaultFloat(Settings.TTLSeconds, Follow, 30.0f);
	EnsureDefaultFloat(Settings.TTLSeconds, Like, 10.0f);
	EnsureDefaultFloat(Settings.TTLSeconds, LikeUser, 5.0f);
	EnsureDefaultFloat(Settings.TTLSeconds, MemberIdentity, 6.0f);
	EnsureDefaultFloat(Settings.TTLSeconds, MemberNormalized, 12.0f);
	EnsureDefaultFloat(Settings.TTLSeconds, RoomUserMilestone, 8.0f);
	EnsureDefaultFloat(Settings.TTLSeconds, Share, 25.0f);
	EnsureDefaultFloat(Settings.TTLSeconds, RoomUser, 15.0f);
	EnsureDefaultFloat(Settings.TTLSeconds, RoomUserTop1Change, 10.0f);

	EnsureDefaultSlots(Settings.MaxSlots, Chat, 30);
	EnsureDefaultSlots(Settings.MaxSlots, Gift, 1000);
	EnsureDefaultSlots(Settings.MaxSlots, GiftCombo, 1000);
	EnsureDefaultSlots(Settings.MaxSlots, Follow, 10);
	EnsureDefaultSlots(Settings.MaxSlots, Like, 1);
	EnsureDefaultSlots(Settings.MaxSlots, LikeUser, 5);
	EnsureDefaultSlots(Settings.MaxSlots, MemberIdentity, 10);
	EnsureDefaultSlots(Settings.MaxSlots, MemberNormalized, 1);
	EnsureDefaultSlots(Settings.MaxSlots, Share, 10);
	EnsureDefaultSlots(Settings.MaxSlots, RoomUserMilestone, 1);
	EnsureDefaultSlots(Settings.MaxSlots, RoomUser, 1);
	EnsureDefaultSlots(Settings.MaxSlots, RoomUserTop1Change, 2);

	// --- Auto-pump timer ---
	{
		UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(this, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
		const bool bWantsAutoPump = Settings.bEnableAutoPump && Settings.AutoPumpInterval > 0.0f;
		if (World && bWantsAutoPump)
		{
			World->GetTimerManager().ClearTimer(AutoPumpHandle);
			World->GetTimerManager().SetTimer(AutoPumpHandle, this, &UTikStudioEventQueue::AutoPumpTick, Settings.AutoPumpInterval, true);
			bAutoPumpActive = true;
			UE_LOG(LogTemp, Log, TEXT("[EventQueue][Config] AutoPump enabled @ %.2fs"), Settings.AutoPumpInterval);
		}
		else
		{
			if (World)
			{
				World->GetTimerManager().ClearTimer(AutoPumpHandle);
			}
			if (bAutoPumpActive)
			{
				UE_LOG(LogTemp, Log, TEXT("[EventQueue][Config] AutoPump disabled"));
			}
			bAutoPumpActive = false;
			if (!World && bWantsAutoPump)
			{
				UE_LOG(LogTemp, Warning, TEXT("[EventQueue][Config] AutoPump requested but no valid UWorld context. Skipping timer."));
			}
		}
	}

	// --- Combos flag change handling ---
    if (!Settings.EventToggles.Gift.bEnableGiftCombo)
    {
        // Close and clear any live combos
        LiveCombos.Empty();
        UE_LOG(LogTemp, Log, TEXT("[EventQueue][Config] GiftCombo disabled -> cleared live combos"));
    }

	// --- Immediate maintenance / recompute ---
	if (bRecomputeNow)
	{
		SweepExpired();
		RecomputePriorities();
	}

    // --- Summary logs ---
    UE_LOG(LogTemp, Log, TEXT("[EventQueue][Config] Applied. Weights(Chat=%d, Gift=%d, GiftCombo=%d, Follow=%d, Like=%d) MaxSlots(Gift=%d, GiftCombo=%d) Aging=%.2f ComboStep=%d AutoPump=%d/%.2fs"),
        Settings.BaseWeights[Chat], Settings.BaseWeights[Gift], Settings.BaseWeights[GiftCombo], Settings.BaseWeights[Follow], Settings.BaseWeights[Like],
        Settings.MaxSlots[Gift], Settings.MaxSlots[GiftCombo], Settings.AgingPointsPerSecond, Settings.GiftConfig.GiftComboRepeatSnapshotStep,
        Settings.bEnableAutoPump ? 1 : 0, Settings.AutoPumpInterval);
}

void UTikStudioEventQueue::ResetQueue(bool bClearCombos, bool bClearLikes)
{
	{
		FScopeLock Lock(&CriticalSection);
		Queue.Reset();
		bHasLockedEvent = false;
		bLockedEventDispatched = false;
		bLockedEventCacheValid = false;
		LockedComboKey.Empty(); // Clear combo lock tracking
	}

	if (bClearCombos)
	{
		LiveCombos.Empty();
	}

	if (bClearLikes)
	{
		// FASE 6: LikeBatch eliminado - Like milestones se procesan en tiempo real
		// Solo limpiar estado de Like si es necesario
		LikeState.bHasLikeMilestoneInQueue = false;
		LikeState.LastLikeMilestoneId.Invalidate();
	}

	UE_LOG(LogTemp, Log, TEXT("[EventQueue] ResetQueue done (Combos=%d Likes=%d)"), bClearCombos ? 1 : 0, bClearLikes ? 1 : 0);
}

void UTikStudioEventQueue::RequestImmediatePump(const TCHAR* Reason)
{
	if (bPumpRequestPending || bIsPumping)
	{
#if WITH_EDITOR || UE_BUILD_DEVELOPMENT
		UE_LOG(LogTemp, Verbose, TEXT("[EventQueue] RequestImmediatePump ignored (%s): pending=%d pumping=%d"), Reason, bPumpRequestPending ? 1 : 0, bIsPumping ? 1 : 0);
#endif
		return;
	}

	bPumpRequestPending = true;

	UWorld* World = GetWorld();
	if (World)
	{
		FTimerDelegate Delegate;
		Delegate.BindLambda([this, Reason]()
		{
#if WITH_EDITOR || UE_BUILD_DEVELOPMENT
			UE_LOG(LogTemp, Verbose, TEXT("[EventQueue] ImmediatePump tick (%s)"), Reason);
#endif
			PumpOnceIfFree();
		});
		World->GetTimerManager().SetTimerForNextTick(Delegate);
	}
	else
	{
		// Fallback: schedule for next tick using core ticker (no UWorld context available)
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this, Reason](float)
		{
#if WITH_EDITOR || UE_BUILD_DEVELOPMENT
			UE_LOG(LogTemp, Verbose, TEXT("[EventQueue] ImmediatePump tick (%s) [Ticker]"), Reason);
#endif
			PumpOnceIfFree();
			return false; // one-shot
		}), 0.0f);
	}

#if WITH_EDITOR || UE_BUILD_DEVELOPMENT
	UE_LOG(LogTemp, Log, TEXT("[EventQueue] RequestImmediatePump scheduled (%s)"), Reason);
#endif
}

bool UTikStudioEventQueue::TryCompetitiveEvict_Locked(FName Type, const FQueuedTikTokEvent& Candidate)
{
    // Expect caller holds CriticalSection
    // Respect global exemptions
    if (Settings.Eviction.ExemptFromEviction.Contains(Type))
    {
        return false;
    }

    // Identify weakest queued event of this type
    int32 WeakestIndex = -1;
    int32 WeakestScore = TNumericLimits<int32>::Max();
    FDateTime WeakestTs = FDateTime::MaxValue();
    for (int32 i = 0; i < Queue.Num(); ++i)
    {
        const FQueuedTikTokEvent& E = Queue[i];
        if (E.EventType != Type)
        {
            continue;
        }
        
        // ========== SB + COPIAS: Protección por deuda pendiente ==========
        // NUNCA evictar un SB con deuda pendiente (debe pagar antes de ser eliminado)
        if (E.EventType == GiftCombo && E.bIsSnapshotBase)
        {
            // Buscar el estado del combo para verificar deuda
            const FGiftComboState* ComboState = LiveCombos.Find(E.ComboKey);
            if (ComboState && ComboState->PendingDebt != EComboDebtType::None)
            {
                // SB con deuda: NO es candidato a evicción (protección absoluta)
                if (Settings.Eviction.bTrackEvictionMetrics)
                {
                    int32& Skipped = EvictionsSkippedByDebt.FindOrAdd(TEXT("GiftCombo_SB"));
                    ++Skipped;
                }
                continue;
            }
        }
        
        // Protección: no considerar snapshots de combo GiftCombo como candidatos a evicción
        if (Settings.GiftConfig.bProtectGiftComboSnapshots && E.EventType == GiftCombo && E.bIsComboSnapshot)
        {
            continue;
        }
        const int32 S = E.PriorityScore;
        if (WeakestIndex == -1 || S < WeakestScore || (S == WeakestScore && E.Timestamp < WeakestTs))
        {
            WeakestIndex = i;
            WeakestScore = S;
            WeakestTs = E.Timestamp;
        }
    }

    if (WeakestIndex == -1)
    {
        // No hay candidato válido (posiblemente todos protegidos como snapshots Gift); no evictar
        if (Settings.Eviction.bTrackEvictionMetrics)
        {
            int32& Skipped = EvictionsSkippedByProtection.FindOrAdd(Type);
            ++Skipped;
        }
#if WITH_EDITOR || UE_BUILD_DEVELOPMENT
        UE_LOG(LogTemp, Warning, TEXT("[EventQueue] CompetitiveEvict skipped by protection (Type=%s)"), *Type.ToString());
#endif
        return false;
    }

    // Decide if candidate wins: strictly higher score wins; on tie, newer replaces older
    const bool bWins = (Candidate.PriorityScore > WeakestScore) ||
                       (Candidate.PriorityScore == WeakestScore && Candidate.Timestamp > WeakestTs);
    if (!bWins)
    {
        return false;
    }

    // Evict weakest and insert candidate in order (descending PriorityScore)
    const FQueuedTikTokEvent Evicted = Queue[WeakestIndex];
    Queue.RemoveAt(WeakestIndex);
    
    // ========== FASE 1: Invalidar handles al evictar ==========
    if (Evicted.EventType == RoomUserMilestone && Evicted.Id == RoomUserState.LastMilestoneId)
    {
        RoomUserState.LastMilestoneId.Invalidate();
        RoomUserState.bHasMilestoneInQueue = false;
        UE_LOG(LogTemp, Verbose, TEXT("[EventQueue] Milestone handle invalidated on eviction (M=%d)"), Evicted.Milestone);
    }
    
    // Invalidar handle de RoomUser snapshot al evictar
    if (Evicted.EventType == RoomUser && Evicted.Id == RoomUserState.LastRoomUserSnapshotId)
    {
        RoomUserState.LastRoomUserSnapshotId.Invalidate();
        RoomUserState.bHasRoomUserSnapshotInQueue = false;
        UE_LOG(LogTemp, Verbose, TEXT("[EventQueue] RoomUser snapshot handle invalidated on eviction (VC=%d)"), Evicted.ViewerCount);
    }
    
    // FASE 1: Invalidar handle de Like milestone al evictar
    if (Evicted.EventType == Like && Evicted.Id == LikeState.LastLikeMilestoneId)
    {
        LikeState.LastLikeMilestoneId.Invalidate();
        LikeState.bHasLikeMilestoneInQueue = false;
        UE_LOG(LogTemp, Verbose, TEXT("[EventQueue] Like milestone handle invalidated on eviction (M=%d)"), Evicted.Milestone);
    }
    
    // Invalidar handle de MemberNormalized al evictar
    if (Evicted.EventType == MemberNormalized && Evicted.Id == MemberNormalizedState.MN_LastId)
    {
        MemberNormalizedState.MN_LastId.Invalidate();
        MemberNormalizedState.MN_bHasInQueue = false;
        UE_LOG(LogTemp, Log, TEXT("[MemberNormalized] EVICT INVALIDATE: handle invalidated on competitive eviction (CurrentMilestone=%d)"), Evicted.MemberCurrentMilestone);
    }
    
    // Invalidar handle de ShareMilestone al evictar
    if (Evicted.EventType == ShareMilestone && Evicted.Id == ShareMilestoneState.SM_LastId)
    {
        ShareMilestoneState.SM_LastId.Invalidate();
        ShareMilestoneState.SM_bHasInQueue = false;
        UE_LOG(LogTemp, Verbose, TEXT("[ShareMilestone] EVICT INVALIDATE"));
    }
    
    // ========== SB + COPIAS: Sincronización condicional al evictar SB ==========
    // Regla: Eliminar tracking SOLO si el combo está CERRADO
    // Si el combo está ABIERTO (sin deuda, evictado por presión), MANTENER tracking para resurrección
    if (Evicted.EventType == GiftCombo && Evicted.bIsSnapshotBase)
    {
        const FGiftComboState* ComboState = LiveCombos.Find(Evicted.ComboKey);
        if (ComboState)
        {
            if (ComboState->bClosed)
            {
                // Combo CERRADO: Eliminar tracking (no habrá más regalos)
                RemoveComboTracking_Locked(Evicted.ComboKey, TEXT("EvictClosed"));
            }
            else
            {
                // Combo ABIERTO: MANTENER tracking para resurrección
                // El siguiente regalo recreará el SB con estado acumulado
                if (Settings.Eviction.bTrackEvictionMetrics)
                {
                    ++SBEvictedOpenButKeptState;
                }
                UE_LOG(LogTemp, Log, TEXT("[Evict] SB evicted (open, no debt) but state kept for resurrection Key=%s rc=%d diamonds=%d"), 
                    *Evicted.ComboKey, ComboState->RepeatCountTotal, ComboState->DiamondTotal);
            }
        }
    }

    int32 InsertIndex = 0;
    for (int32 i = 0; i < Queue.Num(); ++i)
    {
        if (Queue[i].PriorityScore < Candidate.PriorityScore)
        {
            break;
        }
        InsertIndex = i + 1;
    }
    Queue.Insert(Candidate, InsertIndex);

    if (Settings.Eviction.bTrackEvictionMetrics)
    {
        int32& Cnt = EvictionsByType.FindOrAdd(Type);
        ++Cnt;
    }

    UE_LOG(LogTemp, Log, TEXT("[EventQueue] CompetitiveEvict Type=%s evicted Score=%d ts=%s -> inserted Score=%d"), *Type.ToString(), Evicted.PriorityScore, *Evicted.Timestamp.ToString(), Candidate.PriorityScore);
    return true;
}
