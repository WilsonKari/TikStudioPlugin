// TikStudioEventQueue_Share.cpp
// Part of UTikStudioEventQueue — Share event handling
// Split from TikStudioEventQueue.cpp following Epic's multi-cpp pattern

#include "EventQueue/TikStudioEventQueue.h"
#include "Engine/Engine.h"

using namespace TikStudioEventTypes;

void UTikStudioEventQueue::EnqueueShareEvent(const FTSE_ShareIn& Data)
{
    const bool bShareOn = Settings.EventToggles.Share.bEnableShare;
    const bool bShareMsOn = Settings.EventToggles.Share.bEnableShareMilestone;

    // Early return only if BOTH Share and ShareMilestone are disabled
    if (!bShareOn && !bShareMsOn)
    {
        return;
    }

    // Debug log for toggle states
    UE_LOG(LogTemp, Verbose, TEXT("[EnqueueShareEvent] Share=%s, ShareMilestone=%s"), 
           bShareOn ? TEXT("ON") : TEXT("OFF"), 
           bShareMsOn ? TEXT("ON") : TEXT("OFF"));

    // 1) ShareMilestone logic - completely independent of Share
    if (bShareMsOn)
    {
        FScopeLock Lock(&CriticalSection);
        
        // FIX (Bug 1): bandera de control en vez de return global. Si el hito está locked,
        // deferimos sin cortar el flujo hacia el bloque de Share individual (más abajo).
        bool bAbortMilestoneThisPacket = false;
        
        // Increment accumulator (incondicional: crece incluso bajo lock → no fuga datos)
        ShareMilestoneState.SM_AccumRemainder++;
        
        // Calculate steps crossed
        int32 ShareMilestoneStep = Settings.ShareConfig.ShareMilestoneStep;
        int32 StepsCrossed = ShareMilestoneState.SM_AccumRemainder / ShareMilestoneStep;
        
        if (StepsCrossed >= 1)
        {
            // Check for existing pending event and locked state FIRST
            bool bHasPendingEvent = ShareMilestoneState.SM_bHasInQueue;
            // Consider ANY ShareMilestone in lock, not just matching SM_LastId
            bool bIsLocked = bHasLockedEvent && (LockedEventType == ShareMilestone);

            if (bIsLocked)
            {
                // LOCKED→defer: no consumir remainder, no INSERT/REPLACE. El acumulador
                // ya creció con el ++ arriba, así que retiene los shares sin perderlos.
                // NO usar return global: dejamos que el flujo continúe al bloque Share individual.
                bAbortMilestoneThisPacket = true;
                UE_LOG(LogTemp, Verbose, TEXT("[ShareMilestone] LOCKED→defer (remainder=%d, deferring milestone only)"), ShareMilestoneState.SM_AccumRemainder);
            }
            
            if (!bAbortMilestoneThisPacket)
            {
                // NOT LOCKED: Proceed with normal processing
                // Prepare payload
                int32 DeltaShares = StepsCrossed * ShareMilestoneStep;
                int32 CurrentMilestone = ShareMilestoneState.SM_last_committed + DeltaShares;
                int32 PreviousMilestone = ShareMilestoneState.SM_last_committed;
                
                // Consume remainder ONLY when not locked
                ShareMilestoneState.SM_AccumRemainder -= DeltaShares;
                // Create ShareMilestone event
                FQueuedTikTokEvent SMEvent;
                SMEvent.EventType = ShareMilestone;
                SMEvent.Timestamp = FDateTime::UtcNow();
                SMEvent.TTLSeconds = GetTTLForType(ShareMilestone);
                
                // Set ShareMilestone-specific payload
                SMEvent.ShareTotalCount = CurrentMilestone;
                SMEvent.GoalShareCount = CurrentMilestone + ShareMilestoneStep;
                SMEvent.ShareCurrentMilestone = CurrentMilestone;
                SMEvent.SharePreviousMilestone = PreviousMilestone;
                SMEvent.ShareStepsCrossed = StepsCrossed;
                SMEvent.ShareDeltaShares = DeltaShares;
                
                if (!bHasPendingEvent)
                {
                    // INSERT: New event
                    SMEvent.Id = FGuid::NewGuid();
                    SMEvent.PriorityScore = GetBaseWeight(ShareMilestone);
                    
                    // Apply common modifiers by default
                    SMEvent.PriorityScore += ApplyCommonModifiers(SMEvent);
                    
                    ShareMilestoneState.SM_LastId = SMEvent.Id;
                    ShareMilestoneState.SM_bHasInQueue = true;
                    
                    EnqueueTypedEvent(ShareMilestone, SMEvent);
                    UE_LOG(LogTemp, Verbose, TEXT("[ShareMilestone] INSERT (Current=%d/Prev=%d, Steps=%d, Delta=%d)"), CurrentMilestone, PreviousMilestone, StepsCrossed, DeltaShares);
                    RequestImmediatePump(TEXT("ShareMilestone INSERT"));
                }
                else
                {
                    // REPLACE in-place: Update existing event
                    SMEvent.Id = ShareMilestoneState.SM_LastId;
                    SMEvent.PriorityScore = GetBaseWeight(ShareMilestone);
                    
                    // Apply common modifiers by default
                    SMEvent.PriorityScore += ApplyCommonModifiers(SMEvent);
                    
                    // Find and replace the existing event
                    for (int32 i = 0; i < Queue.Num(); i++)
                    {
                        if (Queue[i].Id == ShareMilestoneState.SM_LastId)
                        {
                            Queue[i] = SMEvent; // Replace in-place
                            break;
                        }
                    }
                    
                    UE_LOG(LogTemp, Verbose, TEXT("[ShareMilestone] REPLACE (Current=%d/Prev=%d, Steps=%d, Delta=%d)"), CurrentMilestone, PreviousMilestone, StepsCrossed, DeltaShares);
                    RequestImmediatePump(TEXT("ShareMilestone REPLACE"));
                }
            }
        }
    }

    // 2) If Share is disabled, exit here after processing ShareMilestone
    if (!bShareOn)
    {
        return;
    }

    // 3) Share event processing (viewer gate, mapping 1:1, etc.)
    // Smart Viewer Gate for Share events (fail-closed)
    if (Settings.ShareConfig.bEnableShareViewerGate)
    {
        int32 CurrentVC = 0;
        {
            FScopeLock Lock(&CriticalSection);
            CurrentVC = RoomUserState.RUS_LastRawVC;
        }
        
        if (CurrentVC <= 0)
        {
            // ViewerCount unavailable - fail-closed
            UE_LOG(LogTemp, Verbose, TEXT("[Share] ViewerGate: VC unavailable -> fail-closed"));
            return;
        }
        
        if (CurrentVC > Settings.ShareConfig.ShareViewerGateThreshold)
        {
            // ViewerCount exceeds threshold - block
            UE_LOG(LogTemp, Verbose, TEXT("[Share] ViewerGate: BLOCK (VC=%d > Th=%d)"), CurrentVC, Settings.ShareConfig.ShareViewerGateThreshold);
            return;
        }
        
        // ViewerCount within threshold - allow
        UE_LOG(LogTemp, Verbose, TEXT("[Share] ViewerGate: ALLOW (VC=%d ≤ Th=%d)"), CurrentVC, Settings.ShareConfig.ShareViewerGateThreshold);
    }

    FQueuedTikTokEvent E;
    E.Id = FGuid::NewGuid();
    E.EventType = Share;
    E.Timestamp = FDateTime::UtcNow();

    // Map user fields
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

    E.TTLSeconds = GetTTLForType(E.EventType);
    E.PriorityScore = ComputePriority(E);

    EnqueueTypedEvent(E.EventType, E);
}
