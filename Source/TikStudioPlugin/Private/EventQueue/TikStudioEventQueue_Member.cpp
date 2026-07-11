// TikStudioEventQueue_Member.cpp
// Part of UTikStudioEventQueue — Member event handling
// Split from TikStudioEventQueue.cpp following Epic's multi-cpp pattern

#include "EventQueue/TikStudioEventQueue.h"
#include "Engine/Engine.h"

using namespace TikStudioEventTypes;

void UTikStudioEventQueue::EnqueueMemberEvent(const FTSE_MemberIn& Data)
{
    // Early return if both Member flows are disabled
    if (!Settings.EventToggles.Member.bEnableMemberIdentity && !Settings.EventToggles.Member.bEnableMemberNormalized)
    {
        return;
    }

    // Flujo por identidad con anti-spam (LRU + cooldown 20s)
    if (Settings.EventToggles.Member.bEnableMemberIdentity)
    {
        // === SMART SWITCH: Aplicar gate de ViewerCount lo más temprano posible ===
        // Leer settings fuera del lock (son estáticos)
        const bool bEnableSmartSwitch = Settings.MemberConfig.bEnableMemberIdentityViewerGate;
        const int32 Threshold = FMath::Max(0, Settings.MemberConfig.MemberIdentityViewerGateThreshold); // Clamp a 0+
        
        if (bEnableSmartSwitch)
        {
            // Leer RoomUserState.RUS_LastRawVC exclusivamente bajo lock
            int32 CurrentVC = 0;
            {
                FScopeLock Lock(&CriticalSection);
                CurrentVC = RoomUserState.RUS_LastRawVC;
            }
            
            if (CurrentVC <= 0)
            {
                // ViewerCount no disponible - fail-closed (no fallback a v_prev)
                UE_LOG(LogTemp, Verbose, TEXT("[MemberIdentity] ViewerGate: VC unavailable -> fail-closed"));
                return; // Bloquear MemberIdentity
            }
            else if (CurrentVC > Threshold)
            {
                // VC > Threshold bloquea (VC == Threshold permite)
                UE_LOG(LogTemp, Verbose, TEXT("[MemberIdentity] ViewerGate: BLOCK (VC=%d > Th=%d)"), CurrentVC, Threshold);
                return; // Bloquear MemberIdentity
            }
            else
            {
                // VC <= Threshold permite
                UE_LOG(LogTemp, Verbose, TEXT("[MemberIdentity] ViewerGate: ALLOW (VC=%d ≤ Th=%d)"), CurrentVC, Threshold);
            }
        }
        else
        {
            // === GATE LEGACY: Aplicar histéresis si smart switch está desactivado ===
            // Gate de identidad por viewer count con histéresis (comportamiento legacy)
            int32 CurrentVC = 0;
            {
                FScopeLock Lock(&CriticalSection);
                CurrentVC = RoomUserState.v_prev; // Usar v_prev (último VC procesado por milestone)
            }
            const int32 EnterThreshold = Settings.MemberConfig.MemberIdentityViewerGateThreshold;
            const int32 ExitThreshold = Settings.MemberConfig.MemberIdentityViewerGateThreshold + Settings.MemberConfig.MemberIdentityGateHysteresisDelta;
            if (!bMemberIdentityGateOpen && CurrentVC <= EnterThreshold)
            {
                bMemberIdentityGateOpen = true;
            }
            else if (bMemberIdentityGateOpen && CurrentVC >= ExitThreshold)
            {
                bMemberIdentityGateOpen = false;
            }
            
            if (!bMemberIdentityGateOpen)
            {
                return; // Gate legacy cerrado - bloquear
            }
        }

        // === ANTI-SPAM: LRU + cooldown 20s ===
        const FDateTime Now = FDateTime::UtcNow();
        bool bCanEmit = true;
        const FString& Uid = Data.UniqueId;
        const FDateTime* Last = MemberIdentityLastSeen.Find(Uid);
        const FTimespan Cooldown = FTimespan::FromSeconds(20);
        if (Last && (Now - *Last < Cooldown))
        {
            bCanEmit = false;
        }
        if (bCanEmit)
        {
            // Actualizar memoria LRU
            MemberIdentityLastSeen.Add(Uid, Now);
            MemberIdentityLRU.Remove(Uid);
            MemberIdentityLRU.Insert(Uid, 0);
            while (MemberIdentityLRU.Num() > MemberIdentityLRUCapacity)
            {
                const FString Evicted = MemberIdentityLRU.Pop();
                MemberIdentityLastSeen.Remove(Evicted);
            }

            FQueuedTikTokEvent EId;
            EId.Id = FGuid::NewGuid();
            EId.EventType = MemberIdentity;
            EId.Timestamp = Now;
            // Map user fields
            EId.UniqueId = Data.UniqueId;
            EId.Nickname = Data.Nickname;
            EId.ProfilePictureUrl = Data.ProfilePictureUrl;
                EId.FollowRole = Data.FollowRole;
                EId.bIsModerator = Data.bIsModerator;
                EId.bIsSubscriber = Data.bIsSubscriber;
                EId.bIsNewGifter = Data.bIsNewGifter;
                EId.TopGifterRank = Data.TopGifterRank;
                EId.GifterLevel = Data.GifterLevel;
                EId.TeamMemberLevel = Data.TeamMemberLevel;
            EId.ActionId = Data.ActionId;
            EId.TTLSeconds = GetTTLForType(EId.EventType);
            EId.PriorityScore = ComputePriority(EId);
            EnqueueTypedEvent(EId.EventType, EId);
        }
    }

    // Flujo normalizado (acumula todas las entradas, incluidas las de identidad)
    if (Settings.EventToggles.Member.bEnableMemberNormalized)
    {
        FScopeLock Lock(&CriticalSection);
        const FDateTime Now = FDateTime::UtcNow();
        
        // 1. BURST AGGREGATION: Calculate potential steps crossed
        const int32 NewJoins = 1; // Each member entry counts as 1 join
        const int32 Step = FMath::Max(1, Settings.MemberConfig.MemberNormalizedJoinMilestone);
        const int32 TotalAccum = MemberNormalizedState.MN_AccumRemainder + NewJoins;
        const int32 StepsCrossed = TotalAccum / Step;
        
        // If no steps crossed, just accumulate and return
        if (StepsCrossed == 0)
        {
            MemberNormalizedState.MN_AccumRemainder = TotalAccum;
            UE_LOG(LogTemp, VeryVerbose, TEXT("[MemberNormalized] ACCUMULATE: TotalAccum=%d, no steps crossed"), TotalAccum);
            return;
        }
        
        // 2. UNIQUENESS & LOCKED CHECK: Determine if we can enqueue/replace
        bool bShouldEnqueue = true;
        bool bShouldReplace = false;
        bool bIsLocked = false;
        FGuid EventId = FGuid::NewGuid();
        int32 ReplaceIndex = -1;
        
        if (MemberNormalizedState.MN_bHasInQueue && MemberNormalizedState.MN_LastId.IsValid())
        {
            // Check if existing event is locked (CORRECT detection: type + ID)
            bIsLocked = bHasLockedEvent && 
                       LockedEventType == MemberNormalized && 
                       LockedEventId == MemberNormalizedState.MN_LastId;
            
            if (bIsLocked)
            {
                // Event is locked, cannot replace - defer until next dispatch.
                // FIX (fuga de datos): acumular TotalAccum en el residuo para que siga creciendo
                // mientras el TTS está ocupado. Antes el residuo se congelaba en su valor anterior,
                // perdiendo todos los joins que llegaban durante el bloqueo. MN_AccumRemainder es
                // memoria persistente (sobrevive a TTL/evicción del evento en cola).
                bShouldEnqueue = false;
                MemberNormalizedState.MN_AccumRemainder = TotalAccum;
                UE_LOG(LogTemp, Log, TEXT("[MemberNormalized] LOCKED→defer: StepsCrossed=%d, MN_AccumRemainder=%d (accumulating)"),
                       StepsCrossed, MemberNormalizedState.MN_AccumRemainder);
            }
            else
            {
                // Find existing event for in-place replacement
                for (int32 i = 0; i < Queue.Num(); ++i)
                {
                    if (Queue[i].Id == MemberNormalizedState.MN_LastId)
                    {
                        bShouldReplace = true;
                        bShouldEnqueue = false;
                        ReplaceIndex = i;
                        break;
                    }
                }
            }
        }
        
        // 3. ONLY CONSUME STEPS IF WE CAN ACTUALLY ENQUEUE/REPLACE
        if (bShouldEnqueue || bShouldReplace)
        {
            // NOW it's safe to consume the steps and update accumulator
            const int32 DeltaJoins = StepsCrossed * Step;
            const int32 CurrentMilestone = MemberNormalizedState.MN_last_committed + DeltaJoins;
            const int32 PreviousMilestone = MemberNormalizedState.MN_last_committed;
            MemberNormalizedState.MN_AccumRemainder = TotalAccum % Step; // Safe to consume now
            
            if (bShouldReplace)
            {
                // IN-PLACE REPLACEMENT: Update all fields completely
                Queue[ReplaceIndex].Timestamp = Now;
                Queue[ReplaceIndex].TTLSeconds = GetTTLForType(MemberNormalized);
                Queue[ReplaceIndex].MemberJoinCount = DeltaJoins;
                Queue[ReplaceIndex].MemberJoinGoal = CurrentMilestone + Step;
                Queue[ReplaceIndex].MemberCurrentMilestone = CurrentMilestone;
                Queue[ReplaceIndex].MemberPreviousMilestone = PreviousMilestone;
                Queue[ReplaceIndex].MemberStepsCrossed = StepsCrossed;
                Queue[ReplaceIndex].MemberDeltaJoins = DeltaJoins;
                Queue[ReplaceIndex].PriorityScore = ComputePriority(Queue[ReplaceIndex]); // FIXED: Call ComputePriority
                
                // Mark for recompute after sweep
                bNeedsRecomputeAfterSweep = true; // FIXED: Set recompute flag
                
                UE_LOG(LogTemp, Log, TEXT("[MemberNormalized] REPLACE: Id=%s, CurrentMilestone=%d, StepsCrossed=%d, DeltaJoins=%d"), 
                       *MemberNormalizedState.MN_LastId.ToString(), CurrentMilestone, StepsCrossed, DeltaJoins);
            }
            else if (bShouldEnqueue)
            {
                // CREATE NEW EVENT
                FQueuedTikTokEvent ENorm;
                ENorm.Id = EventId;
                ENorm.EventType = MemberNormalized;
                ENorm.Timestamp = Now;
                ENorm.MemberJoinCount = DeltaJoins;
                ENorm.MemberJoinGoal = CurrentMilestone + Step;
                ENorm.MemberCurrentMilestone = CurrentMilestone;
                ENorm.MemberPreviousMilestone = PreviousMilestone;
                ENorm.MemberStepsCrossed = StepsCrossed;
                ENorm.MemberDeltaJoins = DeltaJoins;
                ENorm.TTLSeconds = GetTTLForType(ENorm.EventType);
                ENorm.PriorityScore = ComputePriority(ENorm);
                
                // Update handle tracking
                MemberNormalizedState.MN_LastId = EventId;
                MemberNormalizedState.MN_bHasInQueue = true;
                
                // Enqueue without going through EnqueueTypedEvent to avoid generic path
                Queue.Add(ENorm);
                Queue.Sort([](const FQueuedTikTokEvent& A, const FQueuedTikTokEvent& B) {
                    return A.PriorityScore > B.PriorityScore;
                });
                
                UE_LOG(LogTemp, Log, TEXT("[MemberNormalized] INSERT: Id=%s, CurrentMilestone=%d, StepsCrossed=%d, DeltaJoins=%d"), 
                       *EventId.ToString(), CurrentMilestone, StepsCrossed, DeltaJoins);
                
                // Request immediate pump for first insert or significant milestone
                RequestImmediatePump(TEXT("MemberNormalized milestone reached"));
            }
            
            // Update legacy JoinCount for backward compatibility (optional)
            MemberNormalizedState.JoinCount = CurrentMilestone;
        }
        // If locked, we don't consume steps - they remain in MN_AccumRemainder for next time
    }
}
