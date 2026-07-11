// TikStudioEventQueue_Like.cpp
// Part of UTikStudioEventQueue — Like event handling
// Split from TikStudioEventQueue.cpp following Epic's multi-cpp pattern

#include "EventQueue/TikStudioEventQueue.h"
#include "Engine/Engine.h"

using namespace TikStudioEventTypes;

void UTikStudioEventQueue::EnqueueLikeEvent(const FTSE_LikeIn& Data)
{
    // Early return if both Like and LikeUser are disabled
    if (!Settings.EventToggles.Like.bEnableLike && !Settings.EventToggles.Like.bEnableLikeUser)
    {
        return;
    }

    // ===== FASE 3: DETECCIÓN DE HITOS EN TIEMPO REAL =====
    bool bShouldEmitLikeMilestone = false;
    int32 CurrentMilestone = 0;
    int32 PreviousMilestone = 0;
    int32 StepsCrossed = 0;
    int32 DeltaLikesSincePrevCommit = 0;
    double ElapsedSincePrevCommitSec = 0.0;
    double LikesPerSecond = 0.0;
    int32 EffectiveTotalForMilestone = 0;
    
    if (Settings.EventToggles.Like.bEnableLike)
    {
        FScopeLock Lock(&CriticalSection);
        
        const int32 IncomingTotal = FMath::Max(0, Data.TotalLikeCount); // Validación: no permitir valores negativos
        const int32 Step = Settings.Like.LikeMilestoneStep;
        const double MonotonicNow = FPlatformTime::Seconds();
        
        // === VALIDACIÓN DE ENTRADA ===
        if (Step <= 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("[Like Milestone] Invalid step size: %d. Skipping milestone processing."), Step);
        }
        // FIX (Reset fantasma): TotalLikeCount=0 no es dato usable (paquete stale/vacío de TikFinity).
        // Ignorar solo la lógica de milestone; LikeUser sigue más abajo sin return global.
        else if (IncomingTotal == 0)
        {
            UE_LOG(LogTemp, Verbose, TEXT("[Like Milestone] Ignoring packet with TotalLikeCount=0 (LastTotal=%d, initialized=%s)"),
                LikeState.Like_LastTotal, LikeState.bMilestoneInitialized ? TEXT("true") : TEXT("false"));
        }
        else
        {
            EffectiveTotalForMilestone = FMath::Max(IncomingTotal, LikeState.Like_LastTotal);

            bool bAbortMilestoneThisPacket = false;

            // === DETECCIÓN DE RESET (robustez) ===
            // Solo evaluar con datos válidos (IncomingTotal > 0): un 0 nunca puede significar reinicio real de sala.
            if (IncomingTotal < LikeState.Like_LastTotal && (LikeState.Like_LastTotal - IncomingTotal) > Step)
            {
                UE_LOG(LogTemp, Warning, TEXT("[Like Milestone] Reset detected: TotalLikeCount dropped from %d to %d (diff=%d > step=%d). Resetting state."),
                    LikeState.Like_LastTotal, IncomingTotal, LikeState.Like_LastTotal - IncomingTotal, Step);

                LikeState.Like_LastTotal = IncomingTotal;
                LikeState.Like_LastCommittedTotal = 0;
                LikeState.Like_LastCommittedMilestone = 0;
                LikeState.bMilestoneInitialized = true;
                bAbortMilestoneThisPacket = true;
            }

            if (!bAbortMilestoneThisPacket)
            {
                // === INICIALIZACIÓN ===
                if (!LikeState.bMilestoneInitialized)
                {
                    LikeState.Like_LastTotal = IncomingTotal;
                    LikeState.Like_LastCommittedTotal = IncomingTotal;
                    LikeState.Like_LastCommittedMilestone = LikeState.CalculateCurrentMilestone(IncomingTotal, Step);
                    LikeState.Like_LastKnownStep = Step;
                    LikeState.bMilestoneInitialized = true;

                    UE_LOG(LogTemp, Log, TEXT("[Like Milestone] Initialized: TotalLikeCount=%d, CommittedMilestone=%d, Step=%d"),
                        IncomingTotal, LikeState.Like_LastCommittedMilestone, Step);
                }
                else
                {
                    // === DETECCIÓN DE CAMBIO DE STEP EN RUNTIME ===
                    if (Step != LikeState.Like_LastKnownStep)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[Like Milestone] Step changed from %d to %d. Recalculating state."),
                            LikeState.Like_LastKnownStep, Step);

                        LikeState.Like_LastKnownStep = Step;
                        LikeState.Like_LastCommittedMilestone = LikeState.CalculateCurrentMilestone(LikeState.Like_LastCommittedTotal, Step);
                    }

                    // === DETECCIÓN DE HITO ===
                    const int32 PrevMilestone = LikeState.Like_LastCommittedMilestone;
                    const int32 CurrMilestone = LikeState.CalculateCurrentMilestone(EffectiveTotalForMilestone, Step);

                    if (CurrMilestone > PrevMilestone)
                    {
                        bShouldEmitLikeMilestone = true;
                        CurrentMilestone = CurrMilestone;
                        PreviousMilestone = PrevMilestone;
                        StepsCrossed = FMath::Max(1, (CurrMilestone - PrevMilestone) / Step);
                        DeltaLikesSincePrevCommit = EffectiveTotalForMilestone - LikeState.Like_LastCommittedTotal;

                        // Calcular elapsed y LikesPerSecond solo tras al menos un dispatch real.
                        // Like_LastEmitMono se escribe en commit (DispatchLockedEvent); en init queda 0.0.
                        if (LikeState.Like_LastEmitMono > 0.0)
                        {
                            ElapsedSincePrevCommitSec = MonotonicNow - LikeState.Like_LastEmitMono;
                            if (ElapsedSincePrevCommitSec > 0.0)
                            {
                                LikesPerSecond = DeltaLikesSincePrevCommit / ElapsedSincePrevCommitSec;
                            }
                        }

                        UE_LOG(LogTemp, Log, TEXT("[Like Milestone] Milestone detected: %d → %d (StepsCrossed=%d, DeltaLikes=%d, Elapsed=%.2fs, LPS=%.2f)"),
                            PrevMilestone, CurrMilestone, StepsCrossed, DeltaLikesSincePrevCommit, ElapsedSincePrevCommitSec, LikesPerSecond);
                    }

                    // Actualizar Like_LastTotal con el total efectivo (nunca retrocede salvo reset explícito)
                    LikeState.Like_LastTotal = EffectiveTotalForMilestone;
                }
            }
        }
    }

    // ===== EMISIÓN DE LIKE MILESTONE =====
    if (bShouldEmitLikeMilestone)
    {
        FScopeLock Lock(&CriticalSection);
        
        // Crear evento Like milestone
        FQueuedTikTokEvent LikeMilestone;
        LikeMilestone.Id = FGuid::NewGuid();
        LikeMilestone.EventType = Like;
        LikeMilestone.Timestamp = FDateTime::UtcNow();
        
        // Campos de milestone
        LikeMilestone.LikeMilestone = CurrentMilestone;
        LikeMilestone.LikePreviousMilestone = PreviousMilestone;
        LikeMilestone.LikeCount = DeltaLikesSincePrevCommit;
        LikeMilestone.TotalLikeCount = EffectiveTotalForMilestone;
        LikeMilestone.LikeStepsCrossed = StepsCrossed;
        LikeMilestone.LikeDeltaSincePrevCommit = DeltaLikesSincePrevCommit;
        LikeMilestone.LikeElapsedSincePrevCommitSec = ElapsedSincePrevCommitSec;
        LikeMilestone.LikesPerSecond = LikesPerSecond;
        
        LikeMilestone.TTLSeconds = GetTTLForType(LikeMilestone.EventType);
        LikeMilestone.PriorityScore = ComputePriority(LikeMilestone);
        
        // === REEMPLAZO IN-PLACE ===
        if (LikeState.bHasLikeMilestoneInQueue)
        {
            // Buscar y reemplazar el evento existente
            bool bReplaced = false;
            for (int32 i = 0; i < Queue.Num(); ++i)
            {
                if (Queue[i].Id == LikeState.LastLikeMilestoneId)
                {
                    // FIX (Bug A): no sobrescribir con snapshot stale (jitter de red).
                    if (LikeMilestone.TotalLikeCount < Queue[i].TotalLikeCount)
                    {
                        UE_LOG(LogTemp, Verbose, TEXT("[Like Milestone] Skipped stale replace (incoming=%d < queued=%d, milestone=%d)"),
                            LikeMilestone.TotalLikeCount, Queue[i].TotalLikeCount, CurrentMilestone);
                        bReplaced = true; // El evento en cola sigue siendo el canónico
                        break;
                    }

                    UE_LOG(LogTemp, Log, TEXT("[Like Milestone] Replacing existing Like milestone in queue (pos=%d)"), i);
                    Queue[i] = LikeMilestone;
                    LikeState.LastLikeMilestoneId = LikeMilestone.Id;

                    // FIX (Bug B): paridad con Chat — reordenar tras cambiar PriorityScore/TTL.
                    Queue.Sort([](const FQueuedTikTokEvent& A, const FQueuedTikTokEvent& B)
                    {
                        if (A.PriorityScore != B.PriorityScore)
                        {
                            return A.PriorityScore > B.PriorityScore;
                        }
                        return A.Timestamp < B.Timestamp;
                    });

                    bReplaced = true;
                    break;
                }
            }
            
            if (!bReplaced)
            {
                UE_LOG(LogTemp, Warning, TEXT("[Like Milestone] Could not find existing Like milestone to replace. Adding new one."));
                LikeState.bHasLikeMilestoneInQueue = false;
            }
        }
        
        if (!LikeState.bHasLikeMilestoneInQueue)
        {
            // Agregar nuevo evento
            EnqueueTypedEvent(LikeMilestone.EventType, LikeMilestone);
            LikeState.LastLikeMilestoneId = LikeMilestone.Id;
            LikeState.bHasLikeMilestoneInQueue = true;
            
            UE_LOG(LogTemp, Log, TEXT("[Like Milestone] Enqueued new Like milestone: Milestone=%d, Priority=%d"), 
                CurrentMilestone, LikeMilestone.PriorityScore);
        }
    }

    // === LikeUser Event Processing (Independent) ===
    // Check LikeUser toggle first
    const bool bEnableLikeUser = Settings.EventToggles.Like.bEnableLikeUser;
    if (!bEnableLikeUser) 
    {
        return; // LikeUser disabled, exit early
    }

    // Read gate settings (static, no lock needed)
    const bool bEnableGate = Settings.Like.bEnableLikeUserViewerGate;
    const int32 gateThreshold = Settings.Like.LikeUserViewerGateThreshold;

    // Apply gate only if enabled
    if (bEnableGate)
    {
        // Use independent cache for gate decision (protected by lock)
        bool bHasVC = false;
        int32 vc = 0;
        {
            FScopeLock Lock(&CriticalSection);
            bHasVC = LikeUserGateCache.bHasViewerCount;
            vc = LikeUserGateCache.LastViewerCount;
        }

        if (!bHasVC)
        {
            // ViewerCount not available - fail-closed (block until first RoomUser)
            UE_LOG(LogTemp, Verbose, TEXT("[LikeUser] ViewerGate: VC unavailable -> fail-closed (BLOCK)"));
            return; // Block LikeUser enqueue
        }
        else if (vc > gateThreshold)
        {
            UE_LOG(LogTemp, Verbose, TEXT("[LikeUser] ViewerGate: BLOCK (VC=%d > Th=%d)"), vc, gateThreshold);
            return; // Block LikeUser enqueue
        }
        else
        {
            UE_LOG(LogTemp, Verbose, TEXT("[LikeUser] ViewerGate: ALLOW (VC=%d ≤ Th=%d)"), vc, gateThreshold);
        }
    }

    // Create and enqueue LikeUser event
    FQueuedTikTokEvent EUser;
    EUser.Id = FGuid::NewGuid();
    EUser.EventType = LikeUser;
    EUser.Timestamp = FDateTime::UtcNow();

    // Copy user identity / roles
    EUser.UniqueId = Data.UniqueId;
    EUser.Nickname = Data.Nickname;
    EUser.ProfilePictureUrl = Data.ProfilePictureUrl;
    EUser.FollowRole = Data.FollowRole;
    EUser.bIsModerator = Data.bIsModerator;
    EUser.bIsSubscriber = Data.bIsSubscriber;
    EUser.bIsNewGifter = Data.bIsNewGifter;
    EUser.TopGifterRank = Data.TopGifterRank;
    EUser.GifterLevel = Data.GifterLevel;
    EUser.TeamMemberLevel = Data.TeamMemberLevel;

    // Likes
    EUser.LikeCount = Data.LikeCount;
    EUser.TotalLikeCount = Data.TotalLikeCount;

    EUser.TTLSeconds = GetTTLForType(EUser.EventType);
    EUser.PriorityScore = ComputePriority(EUser);

    EnqueueTypedEvent(EUser.EventType, EUser);
}
