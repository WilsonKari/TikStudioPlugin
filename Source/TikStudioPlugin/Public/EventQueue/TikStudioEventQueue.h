// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"
#include <atomic>
#include "TikStudioEventQueueSettings.h"
#include "TikStudioEventDispatcher.h"
#include "TSEventTypes.h"
#include "TikStudioEventQueue.generated.h"

// Event type constants
namespace TikStudioEventTypes
{
	static const FName Chat = TEXT("Chat");
	static const FName Gift = TEXT("Gift");
	static const FName GiftCombo = TEXT("GiftCombo");
	static const FName Follow = TEXT("Follow");
	static const FName Like = TEXT("Like");
	static const FName LikeUser = TEXT("LikeUser");
	static const FName MemberIdentity = TEXT("MemberIdentity");
	static const FName MemberNormalized = TEXT("MemberNormalized");
	static const FName RoomUser = TEXT("RoomUser");
	static const FName RoomUserMilestone = TEXT("RoomUserMilestone");
	static const FName RoomUserTop1Change = TEXT("RoomUserTop1Change");
	static const FName Share = TEXT("Share");
	static const FName ShareMilestone = TEXT("ShareMilestone");
}

/**
 * Structure for a queued TikTok event.
 * Represents an event in the queue with metadata and cached fields for prioritization.
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FQueuedTikTokEvent
{
	GENERATED_BODY()

	/** Unique identifier for the event */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FGuid Id;

	/** Type of the event (Chat, Gift, etc.) */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FName EventType;

	/** Calculated priority score (0-150) */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	int32 PriorityScore;

	/** Timestamp when the event was enqueued */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FDateTime Timestamp;

	/** Time to live in seconds */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	float TTLSeconds;

	/** Whether the event is currently locked for processing */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	bool bLocked;

	/** Optional group key for consolidation (e.g., Likes batch) */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FName GroupKey;

	// ========== ALL EVENT FIELDS (FOR COMPLETE RECONSTRUCTION) ==========
	
	/** [CACHED] User unique identifier */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|User")
	FString UniqueId;

	/** [CACHED] User's nickname */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|User")
	FString Nickname;

	/** [CACHED] User's profile picture URL */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|User")
	FString ProfilePictureUrl;

	/** [CACHED] User's follow role */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|User")
	int32 FollowRole;

	/** [CACHED] Whether user is moderator */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|User")
	bool bIsModerator;

	/** [CACHED] Whether user is subscriber */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|User")
	bool bIsSubscriber;

	/** [CACHED] Whether user is new gifter */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|User")
	bool bIsNewGifter;

	/** [CACHED] User's top gifter rank */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|User")
	int32 TopGifterRank;

	/** [CACHED] User's gifter level */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|User")
	int32 GifterLevel;

	/** [CACHED] User's team member level */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|User")
	int32 TeamMemberLevel;

	// ========== CHAT SPECIFIC ==========
	
	/** [CACHED] Chat comment */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|Chat")
	FString Comment;

	/** [CACHED] Has command flag for Chat */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|Chat")
	bool bHasCommand;

	/** [CACHED] Chat emotes */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|Chat")
	TArray<FTSE_EmoteInfo> Emotes;

	// ========== CHAT MERGE SPECIFIC ==========
	
	/** [CACHED] Array of merged chat comments */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|Chat")
	TArray<FString> Comments;

	/** [CACHED] Array of emotes per merged message */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|Chat")
	TArray<FTSE_MessageEmotes> EmotesByMessage;

	/** [CACHED] Array of timestamps for merged messages */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|Chat")
	TArray<FDateTime> MessageTimestamps;

	/** [CACHED] Number of merged messages */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|Chat")
	int32 MergedCount;

	// ========== GIFT SPECIFIC ==========
	
	/** [CACHED] Gift ID */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|Gift")
	int32 GiftId;

	/** [CACHED] Gift name */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|Gift")
	FString GiftName;

	/** [CACHED] Gift picture URL */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|Gift")
	FString GiftPictureUrl;

	/** [CACHED] Diamond count (for Gifts) */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|Gift")
	int32 DiamondCount;

	/** [CACHED] Repeat count for Gift */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|Gift")
	int32 RepeatCount;

	/** [CACHED] Gift type */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|Gift")
	int32 GiftType;

	/** [CACHED] Gift description */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|Gift")
	FString Describe;

	/** [CACHED] Repeat end flag for Gift */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|Gift")
	bool bRepeatEnd;

	/** [CACHED] Gift group id for combos (wiring only) */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|Gift")
	FString GroupId;

	// ========== COMBO SNAPSHOT ==========
	/** Whether this queued item is a combo snapshot (no change to dispatch yet) */
	UPROPERTY(BlueprintReadOnly, Category = "Combo") bool bIsComboSnapshot = false;
	/** Whether the combo is final (closed) */
	UPROPERTY(BlueprintReadOnly, Category = "Combo") bool bComboFinal = false;
	/** Whether this is the initial snapshot (rc=1) */
	UPROPERTY(BlueprintReadOnly, Category = "Combo") bool bComboInitial = false;
	/** Whether combo was closed due to inactivity */
	UPROPERTY(BlueprintReadOnly, Category = "Combo") bool bClosedByInactivity = false;
	/** Aggregated total repeats in the combo */
	UPROPERTY(BlueprintReadOnly, Category = "Combo") int32 ComboRepeatCountTotal = 0;
	/** Aggregated total diamonds in the combo */
	UPROPERTY(BlueprintReadOnly, Category = "Combo") int32 ComboDiamondTotal = 0;
	/** Combo first seen timestamp */
	UPROPERTY(BlueprintReadOnly, Category = "Combo") FDateTime ComboFirstTs;
	/** Combo last update timestamp */
	UPROPERTY(BlueprintReadOnly, Category = "Combo") FDateTime ComboLastTs;
	
	// === SB + COPIAS MODEL ===
	/** Combo key for matching against FGiftComboState (UniqueId|GiftId|GroupId) */
	UPROPERTY(BlueprintReadOnly, Category = "Combo") FString ComboKey;
	/** Whether this is a Snapshot Base (SB) living in Queue (vs Snapshot Copy in lock) */
	UPROPERTY(BlueprintReadOnly, Category = "Combo") bool bIsSnapshotBase = false;

	// ========== LIKE SPECIFIC ==========
	
	/** [CACHED] Like count (for LikeUser) */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|Like")
	int32 LikeCount;

	/** [CACHED] Total like count (for Likes) */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|Like")
	int32 TotalLikeCount;

	// ========== LIKE MILESTONE SPECIFIC ==========
	
	/** [CACHED] Like milestone value (for Like milestones) */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|LikeMilestone")
	int32 LikeMilestone;

	/** [CACHED] Previous Like milestone value */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|LikeMilestone")
	int32 LikePreviousMilestone;

	/** [CACHED] Number of milestone steps crossed in this event */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|LikeMilestone")
	int32 LikeStepsCrossed;

	/** [CACHED] Delta likes since previous committed milestone */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|LikeMilestone")
	int32 LikeDeltaSincePrevCommit;

	/** [CACHED] Elapsed seconds since previous committed milestone */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|LikeMilestone")
	float LikeElapsedSincePrevCommitSec;

	/** [CACHED] Likes per second rate */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|LikeMilestone")
	float LikesPerSecond;

	// ========== MEMBER SPECIFIC ==========
	
	/** [CACHED] Action ID for Member */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|Member")
	int32 ActionId;

	// ========== MEMBER NORMALIZED ==========
	UPROPERTY(BlueprintReadOnly, Category = "Cached|MemberNormalized")
	int32 MemberJoinCount;
	UPROPERTY(BlueprintReadOnly, Category = "Cached|MemberNormalized")
	int32 MemberJoinGoal;
	
	// New fields for burst aggregation and milestone tracking
	UPROPERTY(BlueprintReadOnly, Category = "Cached|MemberNormalized")
	int32 MemberCurrentMilestone;
	UPROPERTY(BlueprintReadOnly, Category = "Cached|MemberNormalized")
	int32 MemberPreviousMilestone;
	UPROPERTY(BlueprintReadOnly, Category = "Cached|MemberNormalized")
	int32 MemberStepsCrossed;
	UPROPERTY(BlueprintReadOnly, Category = "Cached|MemberNormalized")
	int32 MemberDeltaJoins;

	// ========== ROOMUSER SPECIFIC ==========
	
	/** [CACHED] Viewer count (for RoomUser) */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|RoomUser")
	int32 ViewerCount;

	/** [CACHED] Milestone value (for RoomUser milestones) */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|RoomUser")
	int32 Milestone;

	/** [CACHED] Previous milestone value (clamped ≥Step, para contexto) */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|RoomUser")
	int32 PreviousMilestone = 0;

	/** [CACHED] Emission count for this milestone reportado (cuántas veces se emitió) */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|RoomUser")
	int32 EmissionCount = 0;

	/** [CACHED] Milestone direction (true=Ascending, false=Descending) */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|RoomUser")
	bool bIsAscending = true;

	/** [CACHED] Top viewers (for RoomUser) */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|RoomUser")
	TArray<FTSE_TopViewerInfo> TopViewers;

	// ========== SHARE MILESTONE ==========
	
	/** [CACHED] Total share count for ShareMilestone */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|ShareMilestone")
	int32 ShareTotalCount;

	/** [CACHED] Goal share count for ShareMilestone */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|ShareMilestone")
	int32 GoalShareCount;

	/** [CACHED] Current milestone for ShareMilestone */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|ShareMilestone")
	int32 ShareCurrentMilestone;

	/** [CACHED] Previous milestone for ShareMilestone */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|ShareMilestone")
	int32 SharePreviousMilestone;

	/** [CACHED] Steps crossed in this ShareMilestone burst (for telemetry parity with Member/Like) */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|ShareMilestone")
	int32 ShareStepsCrossed;

	/** [CACHED] Delta shares since previous commit (for telemetry parity with Member/Like) */
	UPROPERTY(BlueprintReadOnly, Category = "Cached|ShareMilestone")
	int32 ShareDeltaShares;

	/** Default constructor */
	FQueuedTikTokEvent()
		: Id(FGuid())
		, EventType(NAME_None)
		, PriorityScore(0)
		, Timestamp(FDateTime())
		, TTLSeconds(0.0f)
		, bLocked(false)
		, GroupKey(NAME_None)
		, UniqueId(TEXT(""))
		, Nickname(TEXT(""))
		, ProfilePictureUrl(TEXT(""))
		, FollowRole(0)
		, bIsModerator(false)
		, bIsSubscriber(false)
		, bIsNewGifter(false)
		, TopGifterRank(0)
		, GifterLevel(0)
		, TeamMemberLevel(0)
		, Comment(TEXT(""))
		, bHasCommand(false)
		, MergedCount(1)
		, GiftId(0)
		, GiftName(TEXT(""))
		, GiftPictureUrl(TEXT(""))
		, DiamondCount(0)
		, RepeatCount(0)
		, GiftType(0)
		, Describe(TEXT(""))
		, bRepeatEnd(false)
		, GroupId(TEXT(""))
		, bIsComboSnapshot(false)
		, bComboFinal(false)
		, bClosedByInactivity(false)
		, ComboRepeatCountTotal(0)
		, ComboDiamondTotal(0)
		, ComboFirstTs(FDateTime())
		, ComboLastTs(FDateTime())
		, LikeCount(0)
		, TotalLikeCount(0)
		, LikeMilestone(0)
		, LikePreviousMilestone(0)
		, LikeStepsCrossed(0)
		, LikeDeltaSincePrevCommit(0)
		, LikeElapsedSincePrevCommitSec(0.0f)
		, LikesPerSecond(0.0f)
		, ActionId(0)
		, MemberJoinCount(0)
		, MemberJoinGoal(0)
		, MemberCurrentMilestone(0)
		, MemberPreviousMilestone(0)
		, MemberStepsCrossed(0)
		, MemberDeltaJoins(0)
		, ViewerCount(0)
		, Milestone(0)
		, ShareTotalCount(0)
		, GoalShareCount(0)
		, ShareCurrentMilestone(0)
		, SharePreviousMilestone(0)
		, ShareStepsCrossed(0)
		, ShareDeltaShares(0)
	{
	}
};

/**
 * Class for managing the TikTok Event Queue.
 * Handles enqueuing, prioritization, expiration, and dequeue operations.
 */
UCLASS(BlueprintType)
class TIKSTUDIOPLUGIN_API UTikStudioEventQueue : public UObject
{
	GENERATED_BODY()

public:
	/** Settings for the queue */
	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	FTikStudioEventQueueSettings Settings;

	/** Event dispatcher for emitting processed events */
	UPROPERTY(BlueprintReadOnly, Category = "Dispatcher")
	UTikStudioEventDispatcher* Dispatcher = nullptr;

	// --- Config API ---
	/** Configure Event Queue */
	UFUNCTION(BlueprintCallable, Category = "TikStudio|EventQueue|Config", meta=(DisplayName="Configure Event Queue", Keywords="setup init configure queue settings"))
	void SetQueueSettings(const FTikStudioEventQueueSettings& InSettings, bool bRecomputeNow = true);

	/** Get current Queue Settings */
	UFUNCTION(BlueprintPure, Category = "TikStudio|EventQueue|Config", meta=(DisplayName="Get Queue Settings"))
	FTikStudioEventQueueSettings GetQueueSettings() const { return Settings; }

	/** Convenience: reset queue state (does not modify Settings) */
	UFUNCTION(BlueprintCallable, Category = "TikStudio|EventQueue|Config", meta=(DisplayName="Reset Event Queue"))
	void ResetQueue(bool bClearCombos = true, bool bClearLikes = true);

	/** Typed event input functions - flat structs */
	UFUNCTION(BlueprintCallable, Category = "EventQueue|Input")
	void EnqueueChatEvent(const FTSE_ChatIn& Data);

	UFUNCTION(BlueprintCallable, Category = "EventQueue|Input")
	void EnqueueGiftEvent(const FTSE_GiftIn& Data);

	UFUNCTION(BlueprintCallable, Category = "EventQueue|Input")
	void EnqueueFollowEvent(const FTSE_FollowIn& Data);

	UFUNCTION(BlueprintCallable, Category = "EventQueue|Input")
	void EnqueueLikeEvent(const FTSE_LikeIn& Data);

	UFUNCTION(BlueprintCallable, Category = "EventQueue|Input")
	void EnqueueMemberEvent(const FTSE_MemberIn& Data);

	UFUNCTION(BlueprintCallable, Category = "EventQueue|Input")
	void EnqueueRoomUserEvent(const FTSE_RoomUserIn& Data);

	UFUNCTION(BlueprintCallable, Category = "EventQueue|Input")
	void EnqueueShareEvent(const FTSE_ShareIn& Data);

	/** Dequeue the next highest priority event */
	UFUNCTION(BlueprintCallable, Category = "EventQueue")
	bool DequeueNextEvent(FQueuedTikTokEvent& OutEvent);

	/** Confirm processing of an event */
	UFUNCTION(BlueprintCallable, Category = "EventQueue")
	void ConfirmEventProcessed(const FGuid& EventId);

	/** Peek at the next event without dequeuing */
	UFUNCTION(BlueprintCallable, Category = "EventQueue")
	bool PeekNextEvent(FQueuedTikTokEvent& OutEvent);

	/** Sweep expired events */
	UFUNCTION(BlueprintCallable, Category = "EventQueue")
	void SweepExpired();

	/** Get queue size */
	UFUNCTION(BlueprintCallable, Category = "EventQueue")
	int32 GetQueueSize();

	/** Get count by event type */
	UFUNCTION(BlueprintCallable, Category = "EventQueue")
	int32 GetCountByType(FName Type);

	/** Recompute priorities (optional) */
	UFUNCTION(BlueprintCallable, Category = "EventQueue")
	void RecomputePriorities();

	/** Check if there's a locked event */
	UFUNCTION(BlueprintPure, Category = "EventQueue")
	bool HasLockedEvent() const;

	/** Get the locked event ID */
	UFUNCTION(BlueprintPure, Category = "EventQueue")
	FGuid GetLockedEventId() const;

	/** Get the event dispatcher */
	UFUNCTION(BlueprintCallable, Category = "Dispatcher")
	UTikStudioEventDispatcher* GetDispatcher();

	/** Dispatch the locked event via dispatcher */
	UFUNCTION(BlueprintCallable, Category = "EventQueue")
	bool DispatchLockedEvent();

	/**
	 * Pump once if free: Dequeue and Dispatch in one step if no locked event.
	 * 
	 * Usage cycle:
	 * 1. Call PumpOnceIfFree() when TTS is idle.
	 * 2. Narration/LLM/TTS in BP (event arrives via dispatcher bind).
	 * 3. When TTS finishes -> ConfirmEventProcessed(EventId).
	 * 4. Repeat.
	 */
	UFUNCTION(BlueprintCallable, Category = "EventQueue")
	bool PumpOnceIfFree();

	/** Get queued count for inspection */
	UFUNCTION(BlueprintPure, Category = "EventQueue")
	int32 GetQueuedCount() const;

	/** Check if queue is free (no locked event) */
	UFUNCTION(BlueprintPure, Category = "EventQueue")
	bool IsFree() const;

private:
	/** Helper to get base weight for event type */
	int32 GetBaseWeight(FName EventType) const;

	/** Helper to get TTL for event type */
	float GetTTLForType(FName EventType) const;

	/** Helper to get max slots for event type */
	int32 GetMaxSlotsForType(FName EventType) const;

	/** Helper to get expire policy for event type */
	EEventExpirePolicy GetExpirePolicyForType(FName EventType) const;

	/** Apply common modifiers to priority */
	int32 ApplyCommonModifiers(const FQueuedTikTokEvent& E) const;

	/** Apply specific modifiers per event type */
	int32 ApplySpecificModifiers(const FQueuedTikTokEvent& E) const;

	/** Compute total priority score */
	int32 ComputePriority(const FQueuedTikTokEvent& E) const;

	/** Helper to enqueue typed event data.
	 *  @return true si el evento entró físicamente a la cola (por espacio libre o por ganar
	 *          la evicción competitiva); false si fue descartado (slots llenos o perdió la competencia). */
	bool EnqueueTypedEvent(FName EventType, const FQueuedTikTokEvent& EventData);

	/** Helper (locked): intenta desplazar competitivamente el evento más débil de un tipo e insertar el candidato */
	bool TryCompetitiveEvict_Locked(FName Type, const FQueuedTikTokEvent& Candidate);

	/** The event queue */
	TArray<FQueuedTikTokEvent> Queue;

	/** Critical section for thread safety */
	mutable FCriticalSection CriticalSection;

	// NEW: marca que alguien pidió pump mientras el lock estaba ocupado
	std::atomic<bool> bPumpRequestedWhileBusy{false};

	// FASE 6: FLikeBatchAccumulator eliminado - Like milestones se procesan en tiempo real

	/** State for Like milestones (new milestone-based system) */
	struct FLikeState {
		// === MILESTONE TRACKING ===
		bool bMilestoneInitialized = false;                  // Flag de inicialización correcta (no confiar en Like_LastTotal==0)
		int32 Like_LastTotal = 0;                           // Último TotalLikeCount procesado
		int32 Like_LastCommittedTotal = 0;                  // Último TotalLikeCount realmente despachado (commit real)
		int32 Like_LastCommittedMilestone = 0;              // Último milestone realmente despachado
		int32 Like_LastKnownStep = 0;                       // Step usado para crear el estado actual (detecta cambios de configuración)
		double Like_LastEmitMono = 0.0;                     // FPlatformTime::Seconds() del último Like milestone despachado
		
		// === LIKE QUEUE MANAGEMENT ===
		FGuid LastLikeMilestoneId;                          // Handle del milestone Like en Queue (para reemplazo O(1))
		bool bHasLikeMilestoneInQueue = false;              // Flag de validación del handle
		
		// === LIKE MILESTONE CALCULATION ===
		int32 CalculateNextMilestone(int32 CurrentTotal, int32 Step) const {
			if (Step <= 0) return CurrentTotal;
			return ((CurrentTotal / Step) + 1) * Step;
		}
		
		int32 CalculateCurrentMilestone(int32 CurrentTotal, int32 Step) const {
			if (Step <= 0) return 0;
			return (CurrentTotal / Step) * Step;
		}
	};
	FLikeState LikeState;

	/** State for RoomUser milestones (algoritmo prototipo con cooldown temporal) */
	struct FRoomUserState {
		// === MILESTONE TRACKING (algoritmo prototipo) ===
		bool bMilestoneInitialized = false;                  // Flag de inicialización correcta (no confiar en v_prev==0)
		int32 v_prev = 0;                                    // Último ViewerCount procesado (v_prev del prototipo)
		int32 M_last = 0;                                    // Último milestone reportado
		int32 M_last_committed = 0;                         // Último milestone realmente despachado (commit real)
		int32 LastKnownStep = 0;                             // Step usado para crear cooldowns actuales (detecta cambios de configuración)
		TMap<int32, double> CooldownsMilestones;             // Cooldowns activos: {milestone_reportado: monotonic_seconds} (immune to NTP/DST)
		TMap<int32, int32> EmisionCountByMilestone;         // Contador de emisiones: {milestone_reportado: count}
		
		// === MILESTONE QUEUE MANAGEMENT ===
		FGuid LastMilestoneId;                               // Handle del milestone en Queue (para reemplazo O(1))
		bool bHasMilestoneInQueue = false;                   // Flag de validación del handle
		
		// === ROOMUSER SNAPSHOT DEDICATED STATE (INDEPENDENT FROM MILESTONES) ===
		int32 RUS_LastRawVC = 0;                            // Último ViewerCount crudo recibido (independiente de v_prev)
		TArray<FTSE_TopViewerInfo> RUS_LastRawTopViewers;   // Últimos TopViewers crudos recibidos
		double RUS_LastRawMonoSec = 0.0;                    // Timestamp monotónico de última entrada cruda
		double RUS_LastSnapshotMonoSec = 0.0;               // Timestamp monotónico del último snapshot emitido
		
		// === ROOMUSER SNAPSHOT QUEUE MANAGEMENT (UNIQUE REPLACEMENT) ===
		FGuid LastRoomUserSnapshotId;                       // Handle del snapshot RoomUser en Queue (para reemplazo único)
		bool bHasRoomUserSnapshotInQueue = false;           // Flag de validación del handle (máximo 1 RoomUser en cola)
		
		// === ROOMUSER SHARED DATA (LEGACY - USED BY TOP1CHANGE) ===
		TArray<FTSE_TopViewerInfo> LastTopViewers;          // Cache de últimos TopViewers (usado por Top1Change)
		FDateTime LastSnapshotTs;                            // Timestamp del último snapshot periódico (legacy)
		FString LastTop1UniqueId;                            // UniqueId del último Top1
		FDateTime LastTop1EmitTs;                            // Timestamp de último Top1Change emitido
		
		// === FORCE SNAPSHOT FLAG ===
		bool bForceRoomUserSnapshotOnNextPump = false;      // Flag para forzar snapshot tras bloqueo
	};
	FRoomUserState RoomUserState;

	/** Independent cache for LikeUser gate (isolated from other RoomUser flows) */
	struct FLikeUserGateCache {
		// === INDEPENDENT CACHE STATE ===
		int32 LastViewerCount = 0;                          // Last ViewerCount seen from base RoomUser
		bool bHasViewerCount = false;                       // Flag indicating if ViewerCount is available
		double LastUpdateMonoSec = 0.0;                    // Monotonic timestamp of last update
		
		// Reset cache state
		void Reset() {
			LastViewerCount = 0;
			bHasViewerCount = false;
			LastUpdateMonoSec = 0.0;
		}
	};
	FLikeUserGateCache LikeUserGateCache;

	// ===== Combos (Stage C) =====
	struct FGiftComboState
	{
		
		// Key (combo identifier)
		FString ComboKey;  // Cached key: UniqueId|GiftId|GroupId
		FString UniqueId;
		int32 GiftId = 0;
		FString GroupId;

		// User meta (for snapshot)
		FString Nickname;
		FString ProfilePictureUrl;
		int32 FollowRole = 0;
		bool bIsModerator = false;
		bool bIsSubscriber = false;
		bool bIsNewGifter = false;
		int32 TopGifterRank = 0;
		int32 GifterLevel = 0;
		int32 TeamMemberLevel = 0;
		
		// Gift meta (for snapshot)
		FString GiftName;
		FString GiftPictureUrl;
		int32 GiftType = 0;
		FString Describe;
		
		// Aggregates
		int32 LastKnownRepeatCount = 0;
		int32 RepeatCountTotal = 0;
		int32 DiamondTotal = 0;
		
		// Timestamps
		FDateTime CreationTimestamp = FDateTime();  // Renamed from FirstTs for clarity
		FDateTime FirstTs = FDateTime();            // First gift timestamp (kept for compatibility)
		FDateTime LastTs = FDateTime();             // Last gift timestamp (also used for TTL refresh)
		
		// === SB + COPIAS MODEL FIELDS ===
		
		// Deuda única coalescente (None -> Initial -> Intermediate -> Final)
		EComboDebtType PendingDebt = EComboDebtType::None;
		
		// Último umbral procesado (evita retriggering de deuda intermedia)
		// Renombrado de NextSnapshotThreshold para claridad semántica
		int32 LastProcessedThreshold = 0;
		
		// Flags
		bool bClosed = false;
		bool bClosedByInactivity = false;
		bool bRepeatEndSeen = false;
		
		// KEPT for transitional logging/debugging (can be removed later)
		bool bHasEmittedInitialSnapshot = false;
		
		// GUID del SB en la Queue (para lookup rápido)
		FGuid SnapshotBaseId;
	};

	/** Map of live combos, keyed by UniqueId|GiftId|GroupId */
	TMap<FString, FGiftComboState> LiveCombos;

	/** Helpers for combos (private, locked by CriticalSection) */
	static FString MakeComboKey(const FString& UniqueId, int32 GiftId, const FString& GroupId);
	void OpenOrUpdateCombo_Locked(const FTSE_GiftIn& Data, const FDateTime& Now);
	void CloseCombo_Locked(FGiftComboState& C, bool bInactivity, const FDateTime& Now);
	void SweepCombosInactivity_Locked(const FDateTime& Now);
	void PruneClosedCombos_Locked(const FDateTime& Now);

	/** NEW: SB + Copias model helpers */
	FQueuedTikTokEvent CreateSnapshotCopy_Locked(const FGiftComboState& C) const;
	void UpdateSnapshotBasePriority_Locked(FGiftComboState& C);
	FString DebtTypeToString(EComboDebtType Debt) const;
	void RemoveComboTracking_Locked(const FString& ComboKey, const FString& Reason);

	/** Lock state */
	bool bHasLockedEvent = false;
	FGuid LockedEventId;
	FName LockedEventType;
	FString LockedComboKey; // Tracks which combo has SC in lock (for threshold coalescence)

	/** Dispatch state */
	bool bLockedEventDispatched = false;
	// Reentrancy guard: prevents nested PumpOnceIfFree calls
	bool bIsPumping = false;
	// NEW: request a one-off immediate pump outside lock when snapshots are enqueued
	bool bWantsImmediatePump = false;
	
	// NEW: cache of the currently locked event (copied at dequeue), to ensure Dispatch has consistent data
	FQueuedTikTokEvent LockedEventCache;
	bool bLockedEventCacheValid = false;

	/** Internal helper: count by type without taking the lock (expects caller holds CriticalSection) */
	int32 GetCountByType_NoLock(FName Type) const;

	// === Auto-pump support ===
	FTimerHandle AutoPumpHandle;
	bool bAutoPumpActive = false;
	void AutoPumpTick();

	// --- Immediate pump request state ---
	bool bPumpRequestPending = false;
	void RequestImmediatePump(const TCHAR* Reason);
	
	// --- Recompute flag for priority updates after sweep operations ---
	bool bNeedsRecomputeAfterSweep = false;
// NOTE: keep class open; do not close here

private:
    struct FMemberNormalizedAccumulator 
    { 
        int32 JoinCount = 0; 
        
        // New fields for in-place replacement logic
        bool MN_bHasInQueue = false;           // Whether there's a MemberNormalized event in queue
        FGuid MN_LastId;                       // ID of the last enqueued MemberNormalized event
        int32 MN_last_committed = 0;          // Last milestone actually dispatched
        int32 MN_AccumRemainder = 0;          // Remainder joins not yet emitted
    };
    FMemberNormalizedAccumulator MemberNormalizedState;
    
    // ShareMilestone accumulator state (replicating MemberNormalized pattern)
    struct FShareMilestoneAccumulator
    {
        int32 ShareCount = 0;                  // Legacy count for backward compatibility
        
        // New fields for in-place replacement logic
        bool SM_bHasInQueue = false;           // Whether there's a ShareMilestone event in queue
        FGuid SM_LastId;                       // ID of the last enqueued ShareMilestone event
        int32 SM_last_committed = 0;          // Last milestone actually dispatched
        int32 SM_AccumRemainder = 0;          // Remainder shares not yet emitted
    };
    FShareMilestoneAccumulator ShareMilestoneState;
    
    // Gate de identidad por viewer count con histéresis
    bool bMemberIdentityGateOpen = false;
    // Memoria anti-spam: últimos 20 usuarios y su última vez vista
    TMap<FString, FDateTime> MemberIdentityLastSeen;
    TArray<FString> MemberIdentityLRU;
    int32 MemberIdentityLRUCapacity = 20;

    // === CHAT SHADOW TRACKING ===
    /** Map to track shadow Chat events by UniqueId for O(1) lookup during merge operations */
    TMap<FString, FGuid> ChatShadowByUser;

	// === MILESTONE HELPER FUNCTIONS (algoritmo prototipo) ===
	/** Calcula el milestone reportado (nunca retorna 0, mínimo es Step) */
	int32 MilestoneDestinoReportable(int32 ValorDestino, int32 Step) const;
	
	/** Limpia cooldowns expirados según τ (MilestoneCooldownDuration) - uses monotonic time */
	void LimpiarCooldownsExpirados(double MonotonicNow);
	
	/** Verifica si un valor está dentro de una banda protegida por cooldown activo - uses monotonic time */
	bool EstaEnBandaProtegida(int32 Valor, double MonotonicNow) const;
	
	/** Aplica política de adyacencia: conserva cooldowns ≤N pasos, elimina lejanos */
	void AplicarPoliticaAdyacencia(int32 MilestoneRef);

	/** Capa 1 (histéresis estructural): suprime DESC en colchón geométrico de frontera (no depende de τ). */
	bool ShouldSuppressDescentHysteresis(int32 Valor, int32 M_current, int32 M_last) const;

	// --- Métricas de evicción competitiva (opcional)
	TMap<FName, int32> EvictionsByType;
	// --- Métricas: evicciones/reemplazos evitados por protección de snapshots
	TMap<FName, int32> EvictionsSkippedByProtection;
	// --- Métricas SB + Copias: evicciones evitadas por deuda pendiente
	TMap<FName, int32> EvictionsSkippedByDebt;
	// --- Métricas SB + Copias: resurrecciones de SB
	int32 SBResurrectedCount = 0;
	// --- Métricas SB + Copias: SBs evictados (abiertos, sin deuda) con estado preservado
	int32 SBEvictedOpenButKeptState = 0;
	// --- Métricas SB + Copias: SBs re-encolados en cierre por inactividad (deuda Final sin SB en Queue)
	int32 SBReenqueuedOnInactivityClose = 0;
	// --- Métricas SB + Copias: SBs re-encolados en cierre por repeatEnd (deuda Final sin SB en Queue)
	UPROPERTY()
	int32 SBReenqueuedOnRepeatEndClose = 0;
	// --- Métricas SB + Copias: SBs huérfanos Final recuperados (tracking perdido pero SC Final sintetizada)
	int32 SBRecoveredFromOrphan = 0;
	// --- Métricas SB + Copias: SBs expirados por TTL con tracking preservado pero sin re-encolar (abiertos sin deuda)
	int32 SBExpiredButTrackingPreserved = 0;
	// --- Métricas SB + Copias: SBs re-encolados en expiración TTL con deuda Final
	UPROPERTY()
	int32 SBReenqueuedOnTTLExpiryFinal = 0;
	// --- Métricas SB + Copias: SBs re-encolados en expiración TTL con deuda Initial/Intermediate
	UPROPERTY()
	int32 SBReenqueuedOnTTLExpiryDebt = 0;
};