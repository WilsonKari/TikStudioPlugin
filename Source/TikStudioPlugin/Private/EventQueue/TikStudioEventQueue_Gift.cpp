// TikStudioEventQueue_Gift.cpp
// Part of UTikStudioEventQueue — Gift & GiftCombo event handling
// Split from TikStudioEventQueue.cpp following Epic's multi-cpp pattern

#include "EventQueue/TikStudioEventQueue.h"
#include "Engine/Engine.h"

using namespace TikStudioEventTypes;

void UTikStudioEventQueue::EnqueueGiftEvent(const FTSE_GiftIn& Data)
{
	// Early return if both Gift flows disabled
	if (!Settings.EventToggles.Gift.bEnableGift && !Settings.EventToggles.Gift.bEnableGiftCombo)
	{
		return;
	}

	FQueuedTikTokEvent E;
	E.Id = FGuid::NewGuid();
	E.EventType = Gift;
	E.Timestamp = FDateTime::UtcNow();

	// Map all user fields
	E.UniqueId = Data.UniqueId;
	E.Nickname = Data.Nickname;
	E.ProfilePictureUrl = Data.ProfilePictureUrl;
	E.FollowRole = Data.FollowRole;
	E.bIsModerator = Data.bIsModerator;
	E.bIsSubscriber = Data.bIsSubscriber;
	E.bIsNewGifter = Data.bIsNewGifter;
	E.TopGifterRank = Data.TopGifterRank;
	E.GifterLevel = Data.GifterLevel;
	E.TeamMemberLevel = Data.TeamMemberLevel;

	// Map gift-specific fields
	E.GiftId = Data.GiftId;
	E.GiftName = Data.GiftName;
	E.GiftPictureUrl = Data.GiftPictureUrl;
	E.DiamondCount = Data.DiamondCount;
	E.RepeatCount = Data.RepeatCount;  // Mantener para lógica interna de combos
	E.GiftType = Data.GiftType;
	E.Describe = Data.Describe;
	E.bRepeatEnd = Data.bRepeatEnd;
	// NEW wiring: GroupId from input
	E.GroupId = Data.GroupId;

	// Assign TTL and compute priority
	E.TTLSeconds = GetTTLForType(E.EventType);
	E.PriorityScore = ComputePriority(E);

	UE_LOG(LogTemp, Log, TEXT("[EventQueue] Enqueue Gift Id=%s GroupId=%s RepeatCount=%d Diamonds=%d"), *E.Id.ToString(), *E.GroupId, E.RepeatCount, E.DiamondCount);

	// Determine if this gift is a combo gift (requires tracking)
	const bool bIsComboGift = (Data.GiftType == 1 && !Data.GroupId.IsEmpty());

	bool WantsPumpAfterLocal = false;
	{
		FScopeLock Lock(&CriticalSection);
		// Stage C: combo tracking (only if GiftCombo enabled AND gift is combo type)
        if (Settings.EventToggles.Gift.bEnableGiftCombo && bIsComboGift)
        {
            const FDateTime Now = FDateTime::UtcNow();
            OpenOrUpdateCombo_Locked(Data, Now);
            // Capture immediate pump request
            if (bWantsImmediatePump)
			{
				WantsPumpAfterLocal = true;
				bWantsImmediatePump = false;
			}
		}
	}

	// Enqueue normal Gift event if Gift flow enabled AND gift is NOT a combo type
	// Combo gifts are handled exclusively by the GiftCombo flow (snapshots)
	if (Settings.EventToggles.Gift.bEnableGift && !bIsComboGift)
	{
		EnqueueTypedEvent(E.EventType, E);
	}

	// Pump outside lock if requested
	if (WantsPumpAfterLocal)
	{
		RequestImmediatePump(*GiftCombo.ToString());
	}
}

// ===== Combos (Stage C) Implementation =====
FString UTikStudioEventQueue::MakeComboKey(const FString& UniqueId, int32 GiftId, const FString& GroupId)
{
	return FString::Printf(TEXT("%s|%d|%s"), *UniqueId, GiftId, *GroupId);
}

// ========== SB + COPIAS MODEL HELPERS ==========

FString UTikStudioEventQueue::DebtTypeToString(EComboDebtType Debt) const
{
	switch (Debt)
	{
		case EComboDebtType::None: return TEXT("None");
		case EComboDebtType::Initial: return TEXT("Initial");
		case EComboDebtType::Intermediate: return TEXT("Intermediate");
		case EComboDebtType::Final: return TEXT("Final");
		default: return TEXT("Unknown");
	}
}

void UTikStudioEventQueue::RemoveComboTracking_Locked(const FString& ComboKey, const FString& Reason)
{
	// ========== FIX 3: Centralized tracking removal with traceability ==========
	// Single point for removing LiveCombos entries with logging of the reason
	// Reasons: "TTLExpiry", "FinalPaid", "EvictClosed", "Rc1Closed", "PruneNoDebtNoSB"
	
	if (LiveCombos.Remove(ComboKey) > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[Tracking] Removed combo tracking Key=%s Reason=%s"), *ComboKey, *Reason);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Tracking] Attempted to remove non-existent tracking Key=%s Reason=%s"), *ComboKey, *Reason);
	}
}

FQueuedTikTokEvent UTikStudioEventQueue::CreateSnapshotCopy_Locked(const FGiftComboState& C) const
{
	// Creates a Snapshot Copy (SC) from the current state of the Snapshot Base (SB)
	// The SC is what actually enters the lock and gets dispatched
	FQueuedTikTokEvent SC;
	
	// Identification
	SC.Id = FGuid::NewGuid(); // New SC, new ID
	SC.EventType = GiftCombo;
	SC.ComboKey = C.ComboKey;
	SC.bIsSnapshotBase = false; // This is a COPY, not the base
	SC.bIsComboSnapshot = true;
	
	// Estado actual del combo
	SC.ComboRepeatCountTotal = C.RepeatCountTotal;
	SC.ComboDiamondTotal = C.DiamondTotal;
	
	// Tipo de snapshot según la deuda que se está pagando
	SC.bComboInitial = (C.PendingDebt == EComboDebtType::Initial);
	SC.bComboFinal = (C.PendingDebt == EComboDebtType::Final);
	// Si no es inicial ni final, es intermedio (implícito)
	
	// Metadata
	SC.Timestamp = C.LastTs; // Timestamp del último regalo
	SC.ComboFirstTs = C.FirstTs;
	SC.ComboLastTs = C.LastTs;
	SC.bClosedByInactivity = C.bClosedByInactivity;
	
	// User info
	SC.UniqueId = C.UniqueId;
	SC.Nickname = C.Nickname;
	SC.ProfilePictureUrl = C.ProfilePictureUrl;
	SC.FollowRole = C.FollowRole;
	SC.bIsModerator = C.bIsModerator;
	SC.bIsSubscriber = C.bIsSubscriber;
	SC.bIsNewGifter = C.bIsNewGifter;
	SC.TopGifterRank = C.TopGifterRank;
	SC.GifterLevel = C.GifterLevel;
	SC.TeamMemberLevel = C.TeamMemberLevel;
	
	// Gift info
	SC.GiftId = C.GiftId;
	SC.GiftName = C.GiftName;
	SC.GiftPictureUrl = C.GiftPictureUrl;
	SC.GiftType = C.GiftType;
	SC.Describe = C.Describe;
	SC.GroupId = C.GroupId;
	
	// Calcular PriorityScore de la SC (uses ComputePriority which considers bComboFinal for +15)
	SC.PriorityScore = ComputePriority(SC);
	
	// TTL (inherited from settings)
	SC.TTLSeconds = GetTTLForType(GiftCombo);
	
	return SC;
}

void UTikStudioEventQueue::UpdateSnapshotBasePriority_Locked(FGiftComboState& C)
{
	// Updates the PriorityScore of the SB in the Queue
	// This is called when the combo state changes (new gift, debt elevation, closure)
	
	// Find the SB in the Queue by matching SnapshotBaseId
	for (FQueuedTikTokEvent& E : Queue)
	{
		if (E.EventType == GiftCombo && 
			E.bIsSnapshotBase && 
			E.Id == C.SnapshotBaseId)
		{
			// Update state fields
			E.ComboRepeatCountTotal = C.RepeatCountTotal;
			E.ComboDiamondTotal = C.DiamondTotal;
			E.ComboLastTs = C.LastTs;
			E.Timestamp = C.LastTs; // CRITICAL: Refresh TTL timestamp
			
			// Apply bComboFinal flag if debt is Final (for +15 bonus)
			E.bComboFinal = (C.PendingDebt == EComboDebtType::Final);
			
			// Recalculate priority (considers bComboFinal for +15)
			E.PriorityScore = ComputePriority(E);
			
			// Log update
			UE_LOG(LogTemp, Verbose, TEXT("[SB] Priority updated Key=%s rc=%d diamonds=%d score=%d debt=%s"),
				*C.ComboKey, C.RepeatCountTotal, C.DiamondTotal, E.PriorityScore, *DebtTypeToString(C.PendingDebt));
			
			break;
		}
	}
}

// ========== END SB + COPIAS MODEL HELPERS ==========

void UTikStudioEventQueue::OpenOrUpdateCombo_Locked(const FTSE_GiftIn& Data, const FDateTime& Now)
{
	// ========== SB + COPIAS MODEL IMPLEMENTATION ==========
	// Expect caller holds CriticalSection
	
	const FString Key = MakeComboKey(Data.UniqueId, Data.GiftId, Data.GroupId);
	const int32 Step = Settings.GiftConfig.GiftComboRepeatSnapshotStep;
	
	FGiftComboState* Found = LiveCombos.Find(Key);
	
	// ========== CREATION: New Combo (rc=0→1) ==========
	if (!Found)
	{
		FGiftComboState C;
		
		// Key fields
		C.ComboKey = Key;
		C.UniqueId = Data.UniqueId;
		C.GiftId = Data.GiftId;
		C.GroupId = Data.GroupId;
		
		// User metadata
		C.Nickname = Data.Nickname;
		C.ProfilePictureUrl = Data.ProfilePictureUrl;
		C.FollowRole = Data.FollowRole;
		C.bIsModerator = Data.bIsModerator;
		C.bIsSubscriber = Data.bIsSubscriber;
		C.bIsNewGifter = Data.bIsNewGifter;
		C.TopGifterRank = Data.TopGifterRank;
		C.GifterLevel = Data.GifterLevel;
		C.TeamMemberLevel = Data.TeamMemberLevel;
		
		// Gift metadata
		C.GiftName = Data.GiftName;
		C.GiftPictureUrl = Data.GiftPictureUrl;
		C.GiftType = Data.GiftType;
		C.Describe = Data.Describe;
		
		// Timestamps
		C.CreationTimestamp = Now;
		C.FirstTs = Now;
		C.LastTs = Now;
		
		// Aggregates
		C.LastKnownRepeatCount = Data.RepeatCount;
		C.RepeatCountTotal = Data.RepeatCount;
		C.DiamondTotal = Data.DiamondCount * Data.RepeatCount;
		
		// SB + Copias control
		C.PendingDebt = EComboDebtType::Initial; // Mark initial debt immediately
		C.LastProcessedThreshold = 0; // No thresholds processed yet
		C.bClosed = false;
		C.bClosedByInactivity = false;
		C.bRepeatEndSeen = Data.bRepeatEnd;
		C.bHasEmittedInitialSnapshot = false; // Transitional flag for logging
		
		// Generate unique ID for the SB
		C.SnapshotBaseId = FGuid::NewGuid();
		
		// Add to tracking
		LiveCombos.Add(Key, C);
		
		// ========== CREATE AND ENQUEUE THE SB ==========
		FQueuedTikTokEvent SB;
		SB.Id = C.SnapshotBaseId;
		SB.EventType = GiftCombo;
		SB.ComboKey = Key;
		SB.bIsSnapshotBase = true; // This is the Snapshot Base, not a copy
		SB.bIsComboSnapshot = true;
		
		// Initial state
		SB.ComboRepeatCountTotal = C.RepeatCountTotal;
		SB.ComboDiamondTotal = C.DiamondTotal;
		SB.Timestamp = C.LastTs; // For TTL and aging
		SB.ComboFirstTs = C.FirstTs;
		SB.ComboLastTs = C.LastTs;
		
		// User info
		SB.UniqueId = C.UniqueId;
		SB.Nickname = C.Nickname;
		SB.ProfilePictureUrl = C.ProfilePictureUrl;
		SB.FollowRole = C.FollowRole;
		SB.bIsModerator = C.bIsModerator;
		SB.bIsSubscriber = C.bIsSubscriber;
		SB.bIsNewGifter = C.bIsNewGifter;
		SB.TopGifterRank = C.TopGifterRank;
		SB.GifterLevel = C.GifterLevel;
		SB.TeamMemberLevel = C.TeamMemberLevel;
		
		// Gift info
		SB.GiftId = C.GiftId;
		SB.GiftName = C.GiftName;
		SB.GiftPictureUrl = C.GiftPictureUrl;
		SB.GiftType = C.GiftType;
		SB.Describe = C.Describe;
		SB.GroupId = C.GroupId;
		
		// SB does NOT have bComboFinal=true yet (debt is Initial, not Final)
		SB.bComboFinal = false;
		SB.bComboInitial = false; // The SB itself is not "initial", it OWES an initial SC
		
		// Calculate priority
		SB.PriorityScore = ComputePriority(SB);
		SB.TTLSeconds = GetTTLForType(GiftCombo);
		
		// Enqueue the SB (competitive eviction if necessary)
		EnqueueTypedEvent(GiftCombo, SB);
		
		UE_LOG(LogTemp, Log, TEXT("[SB] Created Key=%s rc=%d diamond=%d debt=%s score=%d"),
			*Key, C.RepeatCountTotal, C.DiamondTotal, *DebtTypeToString(C.PendingDebt), SB.PriorityScore);
		
		// Request immediate pump to try paying the debt
		bWantsImmediatePump = true;
		
		// Check if repeatEnd came with the first gift (rc=1 combo)
		if (Data.bRepeatEnd)
		{
			FGiftComboState* NewCombo = LiveCombos.Find(Key);
			if (NewCombo && NewCombo->RepeatCountTotal == 1)
			{
				// rc=1 with repeatEnd: cancel debt (no final emission)
				NewCombo->bClosed = true;
				NewCombo->PendingDebt = EComboDebtType::None;
				UE_LOG(LogTemp, Log, TEXT("[SB] Closed at rc=1 Key=%s (initial debt will be paid, no final)"), *Key);
			}
			else if (NewCombo)
			{
				// Shouldn't happen (rc=1 but >1?), but handle gracefully
				NewCombo->bClosed = true;
				NewCombo->PendingDebt = EComboDebtType::Final;
				UpdateSnapshotBasePriority_Locked(*NewCombo);
				UE_LOG(LogTemp, Log, TEXT("[SB] Closed Key=%s debt elevated to Final"), *Key);
			}
		}
		
		return;
	}
	
	// ========== UPDATE: Existing Combo (rc>1) ==========
	FGiftComboState& C = *Found;
	
	// Update aggregates
	int32 Delta = FMath::Max(0, Data.RepeatCount - C.LastKnownRepeatCount);
	C.RepeatCountTotal += Delta;
	C.DiamondTotal += Delta * Data.DiamondCount;
	C.LastKnownRepeatCount = Data.RepeatCount;
	C.LastTs = Now;
	
	// Update user metadata (may change between combo events)
	C.Nickname = Data.Nickname;
	C.ProfilePictureUrl = Data.ProfilePictureUrl;
	C.FollowRole = Data.FollowRole;
	C.bIsModerator = Data.bIsModerator;
	C.bIsSubscriber = Data.bIsSubscriber;
	C.bIsNewGifter = Data.bIsNewGifter;
	C.TopGifterRank = Data.TopGifterRank;
	C.GifterLevel = Data.GifterLevel;
	C.TeamMemberLevel = Data.TeamMemberLevel;
	
	// Update gift metadata
	C.GiftName = Data.GiftName;
	C.GiftPictureUrl = Data.GiftPictureUrl;
	C.GiftType = Data.GiftType;
	C.Describe = Data.Describe;
	
	// ========== SB + COPIAS: Resurrección del SB si fue evictado ==========
	// Si el SB desaparece de la Queue (por evicción bajo presión), el siguiente regalo lo resucita
	bool bSBExistsInQueue = false;
	for (const FQueuedTikTokEvent& E : Queue)
	{
		if (E.EventType == GiftCombo && 
			E.bIsSnapshotBase && 
			E.ComboKey == Key)
		{
			bSBExistsInQueue = true;
			break;
		}
	}
	
	if (!bSBExistsInQueue && C.PendingDebt == EComboDebtType::None)
	{
		// SB no existe en Queue y no tiene deuda pendiente: fue evictado → RESUCITAR
		// Crear nuevo SB con el estado actualizado
		FQueuedTikTokEvent SB;
		SB.Id = FGuid::NewGuid(); // Nuevo GUID para el SB resucitado
		C.SnapshotBaseId = SB.Id; // Actualizar referencia
		SB.EventType = GiftCombo;
		SB.ComboKey = Key;
		SB.bIsSnapshotBase = true;
		SB.bIsComboSnapshot = true;
		
		// Estado actualizado
		SB.ComboRepeatCountTotal = C.RepeatCountTotal;
		SB.ComboDiamondTotal = C.DiamondTotal;
		SB.Timestamp = C.LastTs;
		SB.ComboFirstTs = C.FirstTs;
		SB.ComboLastTs = C.LastTs;
		
		// User info
		SB.UniqueId = C.UniqueId;
		SB.Nickname = C.Nickname;
		SB.ProfilePictureUrl = C.ProfilePictureUrl;
		SB.FollowRole = C.FollowRole;
		SB.bIsModerator = C.bIsModerator;
		SB.bIsSubscriber = C.bIsSubscriber;
		SB.bIsNewGifter = C.bIsNewGifter;
		SB.TopGifterRank = C.TopGifterRank;
		SB.GifterLevel = C.GifterLevel;
		SB.TeamMemberLevel = C.TeamMemberLevel;
		
		// Gift info
		SB.GiftId = C.GiftId;
		SB.GiftName = C.GiftName;
		SB.GiftPictureUrl = C.GiftPictureUrl;
		SB.GiftType = C.GiftType;
		SB.Describe = C.Describe;
		SB.GroupId = C.GroupId;
		
		// SB no tiene deuda inicialmente (fue evictado sin deuda)
		SB.bComboFinal = false;
		SB.bComboInitial = false;
		
		// Calculate priority
		SB.PriorityScore = ComputePriority(SB);
		SB.TTLSeconds = GetTTLForType(GiftCombo);
		
		// Enqueue resurrected SB
		EnqueueTypedEvent(GiftCombo, SB);
		
		// Track metric
		if (Settings.Eviction.bTrackEvictionMetrics)
		{
			++SBResurrectedCount;
		}
		
		UE_LOG(LogTemp, Log, TEXT("[SB] Resurrected (state persisted across eviction) Key=%s rc=%d diamond=%d"),
			*Key, C.RepeatCountTotal, C.DiamondTotal);
	}
	
	UE_LOG(LogTemp, Log, TEXT("[SB] Upsert Key=%s rc=%d diamond=%d debt=%s"),
		*Key, C.RepeatCountTotal, C.DiamondTotal, *DebtTypeToString(C.PendingDebt));
	
	// ========== CHECK INTERMEDIATE THRESHOLD ==========
	// Only process if not closed and Step > 0
	if (!C.bClosed && Step > 0 && C.RepeatCountTotal >= Step)
	{
		// Calculate current threshold (multiple of Step)
		const int32 CurrentThreshold = (C.RepeatCountTotal / Step) * Step;
		
		// If this threshold hasn't been processed yet
		if (CurrentThreshold > C.LastProcessedThreshold)
		{
			// Check if this combo already has SC in lock (coalescence guard)
			const bool bThisComboInLock = bHasLockedEvent && (LockedEventType == GiftCombo) && (LockedComboKey == Key);
			
			// Elevate debt to Intermediate (unless already Final)
			if (C.PendingDebt != EComboDebtType::Final)
			{
				// COALESCENCE: if SC in lock and no debt and not closed, just advance threshold without marking debt
				if (bThisComboInLock && C.PendingDebt == EComboDebtType::None && !C.bClosed)
				{
					// Don't mark new debt; coalesce with SC in flight
					C.LastProcessedThreshold = CurrentThreshold;
					UE_LOG(LogTemp, Log, TEXT("[SB] Coalesced threshold while SC in lock Key=%s threshold=%d (no new debt)"),
						*Key, CurrentThreshold);
				}
				else
				{
					// Mark Intermediate debt
					C.PendingDebt = EComboDebtType::Intermediate;
					C.LastProcessedThreshold = CurrentThreshold;
					
					// Request immediate pump to materialize SC without latency
					bWantsImmediatePump = true;
					
					UE_LOG(LogTemp, Log, TEXT("[SB] Threshold reached Key=%s rc=%d threshold=%d debt=%s (pump requested)"),
						*Key, C.RepeatCountTotal, CurrentThreshold, *DebtTypeToString(C.PendingDebt));
				}
			}
			else
			{
				// Already Final, just advance threshold
				C.LastProcessedThreshold = CurrentThreshold;
			}
		}
	}
	
	// Update the SB in Queue with new state and recalculated priority
	UpdateSnapshotBasePriority_Locked(C);
	
	// ========== CHECK FOR CLOSURE (repeatEnd) ==========
	if (Data.bRepeatEnd && !C.bClosed)
	{
		C.bClosed = true;
		C.bRepeatEndSeen = true;
		
		// rc=1 special case: NO final emission
		if (C.RepeatCountTotal == 1)
		{
			// Cancel any debt (initial will be paid if it's the top, then SB eliminated)
			C.PendingDebt = EComboDebtType::None;
			UE_LOG(LogTemp, Log, TEXT("[SB] Closed at rc=1 Key=%s (no final, SB will be eliminated after initial paid)"), *Key);
		}
		else
		{
			// Elevate debt to Final
			C.PendingDebt = EComboDebtType::Final;
			
			// ========== CHECK: Is SB in Queue? (simetría con inactividad) ==========
			// Reutilizar variable bSBExistsInQueue ya declarada arriba
			bSBExistsInQueue = false;
			for (const FQueuedTikTokEvent& E : Queue)
			{
				if (E.EventType == GiftCombo && 
					E.bIsSnapshotBase && 
					E.ComboKey == Key)
				{
					bSBExistsInQueue = true;
					break;
				}
			}
			
			if (bSBExistsInQueue)
			{
				// SB already in Queue: just update priority with +15 bonus
				UpdateSnapshotBasePriority_Locked(C);
				UE_LOG(LogTemp, Log, TEXT("[SB] Closed by repeatEnd Key=%s rc=%d debt=Final (SB in queue)"), *Key, C.RepeatCountTotal);
			}
			else
			{
				// SB NOT in Queue (was evicted when open): RE-ENQUEUE minimal SB
				FQueuedTikTokEvent SB;
				SB.Id = C.SnapshotBaseId;
				SB.EventType = GiftCombo;
				SB.ComboKey = Key;
				SB.bIsSnapshotBase = true;
				SB.bIsComboSnapshot = true;
				SB.ComboRepeatCountTotal = C.RepeatCountTotal;
				SB.ComboDiamondTotal = C.DiamondTotal;
				SB.Timestamp = C.LastTs;
				SB.ComboFirstTs = C.FirstTs;
				SB.ComboLastTs = C.LastTs;
				SB.UniqueId = C.UniqueId;
				SB.Nickname = C.Nickname;
				SB.ProfilePictureUrl = C.ProfilePictureUrl;
				SB.FollowRole = C.FollowRole;
				SB.bIsModerator = C.bIsModerator;
				SB.bIsSubscriber = C.bIsSubscriber;
				SB.bIsNewGifter = C.bIsNewGifter;
				SB.TopGifterRank = C.TopGifterRank;
				SB.GifterLevel = C.GifterLevel;
				SB.TeamMemberLevel = C.TeamMemberLevel;
				SB.GiftId = C.GiftId;
				SB.GiftName = C.GiftName;
				SB.GiftPictureUrl = C.GiftPictureUrl;
				SB.GiftType = C.GiftType;
				SB.Describe = C.Describe;
				SB.GroupId = C.GroupId;
				SB.bComboFinal = true;
				SB.bComboInitial = false;
				SB.PriorityScore = ComputePriority(SB);
				SB.TTLSeconds = GetTTLForType(GiftCombo);
				EnqueueTypedEvent(GiftCombo, SB);
				
				if (Settings.Eviction.bTrackEvictionMetrics)
				{
					++SBReenqueuedOnRepeatEndClose; // Métrica separada para repeatEnd
				}
				
				UE_LOG(LogTemp, Log, TEXT("[SB] Re-enqueued on repeatEnd close (Final debt, no SB in queue) Key=%s rc=%d diamonds=%d score=%d"), 
					*Key, C.RepeatCountTotal, C.DiamondTotal, SB.PriorityScore);
				
				// Request immediate pump ONLY in this case
				bWantsImmediatePump = true;
				return;
			}
		}
		
		// Request pump to pay final debt (solo si SB ya estaba en Queue)
		bWantsImmediatePump = true;
	}
	
	// ========== END SB + COPIAS MODEL IMPLEMENTATION ==========
}

void UTikStudioEventQueue::CloseCombo_Locked(FGiftComboState& C, bool bInactivity, const FDateTime& Now)
{
	C.bClosed = true;
	C.bClosedByInactivity = bInactivity;
	C.LastTs = Now;
	const TCHAR* Reason = bInactivity ? TEXT("inactivity") : TEXT("repeatEnd");
	double Duration = (C.LastTs - C.FirstTs).GetTotalSeconds();
	const FString Key = MakeComboKey(C.UniqueId, C.GiftId, C.GroupId);
	UE_LOG(LogTemp, Log, TEXT("[Combo] Closed Key=%s reason=%s rc=%d diamond=%d dur=%.2fs"), *Key, Reason, C.RepeatCountTotal, C.DiamondTotal, Duration);
}

void UTikStudioEventQueue::SweepCombosInactivity_Locked(const FDateTime& Now)
{
	// ========== SB + COPIAS MODEL: Sweep inactive combos ==========
	
	for (TPair<FString, FGiftComboState>& It : LiveCombos)
	{
		FGiftComboState& C = It.Value;
		
		if (!C.bClosed)
		{
			double Idle = (Now - C.LastTs).GetTotalSeconds();
			
			// ========== CLOSURE BY INACTIVITY ==========
			if (Idle >= Settings.GiftConfig.GiftComboInactivitySeconds)
			{
				CloseCombo_Locked(C, /*bInactivity=*/true, Now);
				C.bClosedByInactivity = true;
				
				// rc=1 special case: NO final emission
				if (C.RepeatCountTotal == 1)
				{
					// Cancel any pending debt
					C.PendingDebt = EComboDebtType::None;
					UE_LOG(LogTemp, Log, TEXT("[SB] Closed by inactivity at rc=1 Key=%s (no final)"), *C.ComboKey);
				}
				else
				{
					// Elevate debt to Final
					C.PendingDebt = EComboDebtType::Final;
					
					// ========== CHECK: Is SB in Queue? ==========
					bool bSBExistsInQueue = false;
					for (const FQueuedTikTokEvent& E : Queue)
					{
						if (E.EventType == GiftCombo && 
							E.bIsSnapshotBase && 
							E.ComboKey == C.ComboKey)
						{
							bSBExistsInQueue = true;
							break;
						}
					}
					
					if (bSBExistsInQueue)
					{
						// SB already in Queue: just update priority with +15 bonus
						UpdateSnapshotBasePriority_Locked(C);
						UE_LOG(LogTemp, Log, TEXT("[SB] Closed by inactivity Key=%s rc=%d debt=Final (SB in queue)"), *C.ComboKey, C.RepeatCountTotal);
						// NO RequestImmediatePump (avoid excessive pumps in sweep)
					}
					else
					{
						// SB NOT in Queue (was evicted when open): RE-ENQUEUE minimal SB
						FQueuedTikTokEvent SB;
						SB.Id = C.SnapshotBaseId;
						SB.EventType = GiftCombo;
						SB.ComboKey = C.ComboKey;
						SB.bIsSnapshotBase = true;
						SB.bIsComboSnapshot = true;
						
						// Current state
						SB.ComboRepeatCountTotal = C.RepeatCountTotal;
						SB.ComboDiamondTotal = C.DiamondTotal;
						SB.Timestamp = C.LastTs;
						SB.ComboFirstTs = C.FirstTs;
						SB.ComboLastTs = C.LastTs;
						
						// User metadata
						SB.UniqueId = C.UniqueId;
						SB.Nickname = C.Nickname;
						SB.ProfilePictureUrl = C.ProfilePictureUrl;
						SB.FollowRole = C.FollowRole;
						SB.bIsModerator = C.bIsModerator;
						SB.bIsSubscriber = C.bIsSubscriber;
						SB.bIsNewGifter = C.bIsNewGifter;
						SB.TopGifterRank = C.TopGifterRank;
						SB.GifterLevel = C.GifterLevel;
						SB.TeamMemberLevel = C.TeamMemberLevel;
						
						// Gift metadata
						SB.GiftId = C.GiftId;
						SB.GiftName = C.GiftName;
						SB.GiftPictureUrl = C.GiftPictureUrl;
						SB.GiftType = C.GiftType;
						SB.Describe = C.Describe;
						SB.GroupId = C.GroupId;
						
						// Mark as FINAL (enables +15 priority bonus)
						SB.bComboFinal = true;
						SB.bComboInitial = false;
						
						// Calculate priority
						SB.PriorityScore = ComputePriority(SB);
						SB.TTLSeconds = GetTTLForType(GiftCombo);
						
						// Enqueue (competitive eviction if necessary)
						EnqueueTypedEvent(GiftCombo, SB);
						
						// Metrics
						if (Settings.Eviction.bTrackEvictionMetrics)
						{
							++SBReenqueuedOnInactivityClose;
						}
						
						UE_LOG(LogTemp, Log, TEXT("[SB] Re-enqueued on inactivity close (Final debt, no SB in queue) Key=%s rc=%d diamonds=%d score=%d"), 
							*C.ComboKey, C.RepeatCountTotal, C.DiamondTotal, SB.PriorityScore);
						
						// Request immediate pump ONLY in this case (avoid excessive pumps)
						bWantsImmediatePump = true;
					}
				}
			}
			// ========== FALLBACK: repeatEnd not processed ==========
			else if (C.bRepeatEndSeen)
			{
				// This should rarely happen (OpenOrUpdateCombo_Locked handles repeatEnd immediately)
				UE_LOG(LogTemp, Warning, TEXT("[SB] Sweep detected unclosed combo with repeatEnd Key=%s (fallback closure)"), *C.ComboKey);
				
				CloseCombo_Locked(C, /*bInactivity=*/false, Now);
				
				// rc=1 special case
				if (C.RepeatCountTotal == 1)
				{
					C.PendingDebt = EComboDebtType::None;
					UE_LOG(LogTemp, Log, TEXT("[SB] Fallback closure at rc=1 Key=%s (no final)"), *C.ComboKey);
				}
				else
				{
					C.PendingDebt = EComboDebtType::Final;
					
					// ========== CHECK: Is SB in Queue? ==========
					bool bSBExistsInQueue = false;
					for (const FQueuedTikTokEvent& E : Queue)
					{
						if (E.EventType == GiftCombo && 
							E.bIsSnapshotBase && 
							E.ComboKey == C.ComboKey)
						{
							bSBExistsInQueue = true;
							break;
						}
					}
					
					if (bSBExistsInQueue)
					{
						// SB already in Queue: just update priority
						UpdateSnapshotBasePriority_Locked(C);
						UE_LOG(LogTemp, Log, TEXT("[SB] Fallback closure Key=%s debt=Final (SB in queue)"), *C.ComboKey);
					}
					else
					{
						// SB NOT in Queue: RE-ENQUEUE (same logic as inactivity)
						FQueuedTikTokEvent SB;
						SB.Id = C.SnapshotBaseId;
						SB.EventType = GiftCombo;
						SB.ComboKey = C.ComboKey;
						SB.bIsSnapshotBase = true;
						SB.bIsComboSnapshot = true;
						SB.ComboRepeatCountTotal = C.RepeatCountTotal;
						SB.ComboDiamondTotal = C.DiamondTotal;
						SB.Timestamp = C.LastTs;
						SB.ComboFirstTs = C.FirstTs;
						SB.ComboLastTs = C.LastTs;
						SB.UniqueId = C.UniqueId;
						SB.Nickname = C.Nickname;
						SB.ProfilePictureUrl = C.ProfilePictureUrl;
						SB.FollowRole = C.FollowRole;
						SB.bIsModerator = C.bIsModerator;
						SB.bIsSubscriber = C.bIsSubscriber;
						SB.bIsNewGifter = C.bIsNewGifter;
						SB.TopGifterRank = C.TopGifterRank;
						SB.GifterLevel = C.GifterLevel;
						SB.TeamMemberLevel = C.TeamMemberLevel;
						SB.GiftId = C.GiftId;
						SB.GiftName = C.GiftName;
						SB.GiftPictureUrl = C.GiftPictureUrl;
						SB.GiftType = C.GiftType;
						SB.Describe = C.Describe;
						SB.GroupId = C.GroupId;
						SB.bComboFinal = true;
						SB.bComboInitial = false;
						SB.PriorityScore = ComputePriority(SB);
						SB.TTLSeconds = GetTTLForType(GiftCombo);
						EnqueueTypedEvent(GiftCombo, SB);
						
						if (Settings.Eviction.bTrackEvictionMetrics)
						{
							++SBReenqueuedOnInactivityClose;
						}
						
						UE_LOG(LogTemp, Log, TEXT("[SB] Re-enqueued on fallback closure (Final debt, no SB in queue) Key=%s rc=%d"), 
							*C.ComboKey, C.RepeatCountTotal);
						
						bWantsImmediatePump = true;
					}
				}
			}
		}
	}
	
	// ========== END SB + COPIAS MODEL SWEEP ==========
}

// ===== REMOVED: Old deprecated functions (ShouldEmitSnapshot_Locked, EmitComboSnapshot_Locked) =====
// These functions have been completely removed as they are no longer used in the SB + Copias model.
// The new model uses:
//   - CreateSnapshotCopy_Locked: Creates SC from SB state
//   - UpdateSnapshotBasePriority_Locked: Updates SB priority in Queue
//   - Debt tracking in FGiftComboState (PendingDebt field)
//   - Eligibility logic in DequeueNextEvent (pay debt when SB is top)

void UTikStudioEventQueue::PruneClosedCombos_Locked(const FDateTime& Now)
{
	// ========== SB + COPIAS: Prune cerrado condicionado ==========
	// NO eliminar estado de LiveCombos si:
	// 1. Hay deuda pendiente (el SB debe pagar antes de ser eliminado), O
	// 2. El SB aún está en Queue (mantener sincronización)
	
	for (auto It = LiveCombos.CreateIterator(); It; ++It)
	{
		const FString& Key = It->Key;
		const FGiftComboState& C = It->Value;
		if (C.bClosed)
		{
			double Age = (Now - C.LastTs).GetTotalSeconds();
            if (Age >= Settings.GiftConfig.GiftComboClosedPruneSeconds)
            {
				// ========== GUARD 1: Deuda pendiente ==========
				// Si hay deuda pendiente (Initial/Intermediate/Final), NO eliminar estado
				// El estado se eliminará atómicamente cuando se pague la deuda
				if (C.PendingDebt != EComboDebtType::None)
				{
					UE_LOG(LogTemp, Verbose, TEXT("[Combo] Prune deferred: Key=%s has pending debt=%s (will be removed when debt is paid)"),
						*Key, *DebtTypeToString(C.PendingDebt));
					continue;
			}
			
			// ========== GUARD 2: SB en Queue ==========
			// Verificar si el SB todavía está en Queue
			// Nota: Este escaneo es O(N) pero solo se ejecuta sobre combos cerrados viejos.
			// Si en el futuro esto se volviera un cuello de botella, considerar mantener
			// un flag bSBPresentInQueue en FGiftComboState actualizado en insert/remove del SB.
			bool bSBInQueue = false;
			for (const FQueuedTikTokEvent& E : Queue)
			{
				if (E.EventType == GiftCombo && 
					E.bIsSnapshotBase && 
					E.ComboKey == Key)
				{
					bSBInQueue = true;
					break;
				}
			}
			
			if (bSBInQueue)
			{
				UE_LOG(LogTemp, Verbose, TEXT("[Combo] Prune deferred: Key=%s has SB still in Queue (maintaining sync)"), *Key);
				continue;
			}
			
			// ========== SAFE TO PRUNE ==========
			// Solo si NO hay deuda Y el SB NO está en Queue, eliminar estado
			// Log de trazabilidad ANTES de eliminar
			UE_LOG(LogTemp, Log, TEXT("[Tracking] Removed combo tracking Key=%s Reason=PruneNoDebtNoSB age=%.2fs"), *Key, Age);
			// Usar It.RemoveCurrent() (iterator-safe) en lugar de LiveCombos.Remove(Key)
            It.RemoveCurrent();
            }
        }
	}
}
