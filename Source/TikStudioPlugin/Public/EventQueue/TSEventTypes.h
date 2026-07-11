// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TSEventTypes.generated.h"

// ========== COMBO DEBT TYPE (SB + COPIAS MODEL) ==========

/**
 * Type of debt (pending snapshot emission) for a GiftCombo Snapshot Base (SB).
 * Used in the SB + Copias model to track what kind of Snapshot Copy (SC) 
 * needs to be paid when the lock becomes available.
 */
UENUM(BlueprintType)
enum class EComboDebtType : uint8
{
	None         UMETA(DisplayName = "No Debt"),
	Initial      UMETA(DisplayName = "Initial Snapshot"),
	Intermediate UMETA(DisplayName = "Intermediate Snapshot"),
	Final        UMETA(DisplayName = "Final Snapshot")
};

// ========== MILESTONE DIRECTION (FASE 1) ==========

/**
 * Direction of milestone crossing for RoomUserMilestone events.
 * Indicates whether audience grew (Ascending) or shrank (Descending).
 */
UENUM(BlueprintType)
enum class EMilestoneDirection : uint8
{
	None       UMETA(DisplayName = "None"),
	Ascending  UMETA(DisplayName = "Ascending"),
	Descending UMETA(DisplayName = "Descending")
};

// ========== NESTED STRUCTS ==========

/**
 * Emote information for chat events.
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTSE_EmoteInfo
{
	GENERATED_BODY()

	// Convenience constructors for easy creation from generic sources
	FTSE_EmoteInfo() = default;
	FTSE_EmoteInfo(const FString& InEmoteId, const FString& InEmoteImageUrl)
		: EmoteId(InEmoteId), EmoteImageUrl(InEmoteImageUrl) {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emote")
	FString EmoteId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emote")
	FString EmoteImageUrl;
};

/**
 * Container for emotes in a single message (wrapper for TArray<FTSE_EmoteInfo>).
 * Required because UE doesn't support TArray<TArray<T>> directly in UPROPERTY.
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTSE_MessageEmotes
{
	GENERATED_BODY()

	// Convenience constructors
	FTSE_MessageEmotes() = default;
	FTSE_MessageEmotes(const TArray<FTSE_EmoteInfo>& InEmotes) : Emotes(InEmotes) {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotes")
	TArray<FTSE_EmoteInfo> Emotes;
};

/**
 * Top viewer information for room user events.
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTSE_TopViewerInfo
{
	GENERATED_BODY()

	// Convenience constructors for easy creation from generic sources
	FTSE_TopViewerInfo() = default;
	FTSE_TopViewerInfo(const FString& InUniqueId,
		const FString& InNickname,
		const FString& InProfilePictureUrl,
		int32 InCoinCount,
		bool bInIsModerator,
		bool bInIsSubscriber,
		int32 InGifterLevel,
		int32 InTeamMemberLevel)
		: UniqueId(InUniqueId)
		, Nickname(InNickname)
		, ProfilePictureUrl(InProfilePictureUrl)
		, CoinCount(InCoinCount)
		, bIsModerator(bInIsModerator)
		, bIsSubscriber(bInIsSubscriber)
		, GifterLevel(InGifterLevel)
		, TeamMemberLevel(InTeamMemberLevel) {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewer")
	FString UniqueId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewer")
	FString Nickname;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewer")
	FString ProfilePictureUrl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewer")
	int32 CoinCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewer")
	bool bIsModerator = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewer")
	bool bIsSubscriber = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewer")
	int32 GifterLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Viewer")
	int32 TeamMemberLevel = 0;
};

// ========== INPUT EVENT TYPES (FLAT STRUCTS) ==========

/**
 * Chat event input structure - flat struct with all fields.
 * Field order matches TikFinityPlugin FTikTokChatEvent (event-specific fields first, then base user info).
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTSE_ChatIn
{
	GENERATED_BODY()

	// Chat-specific fields first (matches TikFinityPlugin order)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chat Data")
	FString Comment;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chat Data")
	TArray<FTSE_EmoteInfo> Emotes;

	// Base user info fields (matches FTikTokBaseUserInfo order)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString UniqueId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString Nickname;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString ProfilePictureUrl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 FollowRole = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsModerator = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsSubscriber = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsNewGifter = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 TopGifterRank = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 GifterLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 TeamMemberLevel = 0;
};

/**
 * Follow event input structure - flat struct with all fields.
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTSE_FollowIn
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString UniqueId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString Nickname;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString ProfilePictureUrl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 FollowRole = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsModerator = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsSubscriber = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsNewGifter = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 TopGifterRank = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 GifterLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 TeamMemberLevel = 0;
};

/**
 * Gift event input structure - flat struct with all fields.
 * Field order matches TikFinityPlugin FTikTokGiftEvent (event-specific fields first, then base user info).
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTSE_GiftIn
{
	GENERATED_BODY()

	// Gift-specific fields first (matches TikFinityPlugin order)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gift Data")
	int32 GiftId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gift Data")
	FString GiftName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gift Data")
	FString GiftPictureUrl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gift Data")
	int32 DiamondCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gift Data")
	int32 RepeatCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gift Data")
	int32 GiftType = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gift Data")
	FString Describe;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gift Data")
	bool bRepeatEnd = false;

	// NEW: GroupId used for combo grouping (only for GiftType==1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gift Data")
	FString GroupId;

	// Base user info fields (matches FTikTokBaseUserInfo order)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString UniqueId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString Nickname;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString ProfilePictureUrl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 FollowRole = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsModerator = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsSubscriber = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsNewGifter = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 TopGifterRank = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 GifterLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 TeamMemberLevel = 0;
};

/**
 * Like event input structure - flat struct with all fields.
 * Field order matches TikFinityPlugin FTikTokLikeEvent (event-specific fields first, then base user info).
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTSE_LikeIn
{
	GENERATED_BODY()

	// Like-specific fields first (matches TikFinityPlugin order)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Like Data")
	int32 LikeCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Like Data")
	int32 TotalLikeCount = 0;

	// Base user info fields (matches FTikTokBaseUserInfo order)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString UniqueId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString Nickname;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString ProfilePictureUrl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 FollowRole = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsModerator = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsSubscriber = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsNewGifter = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 TopGifterRank = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 GifterLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 TeamMemberLevel = 0;
};

/**
 * Member event input structure - flat struct with all fields.
 * Field order matches TikFinityPlugin FTikTokMemberEvent (event-specific fields first, then base user info).
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTSE_MemberIn
{
	GENERATED_BODY()

	// Member-specific fields first (matches TikFinityPlugin order)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Member Data")
	int32 ActionId = 0;

	// Base user info fields (matches FTikTokBaseUserInfo order)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString UniqueId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString Nickname;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString ProfilePictureUrl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 FollowRole = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsModerator = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsSubscriber = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsNewGifter = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 TopGifterRank = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 GifterLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 TeamMemberLevel = 0; // 1 join, 2 upgrade, 3 renewal
};

/**
 * RoomUser event input structure - flat struct with all fields.
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTSE_RoomUserIn
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Data")
	int32 ViewerCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Data")
	int32 TopGifterRank = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Data")
	TArray<FTSE_TopViewerInfo> TopViewers;
};

/**
 * Share event input structure - flat struct with all fields.
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTSE_ShareIn
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString UniqueId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString Nickname;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString ProfilePictureUrl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 FollowRole = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsModerator = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsSubscriber = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsNewGifter = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 TopGifterRank = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 GifterLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 TeamMemberLevel = 0;
};

// ========== OUTPUT EVENT TYPES ==========

/**
 * Chat event output structure - complete fields for dispatch.
 * Field order matches TikFinityPlugin FTikTokChatEvent (event-specific fields first, then base user info).
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTSE_ChatOut
{
	GENERATED_BODY()

	// Chat-specific fields first (matches TikFinityPlugin order)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chat Data")
	TArray<FString> Comments;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chat Data")
	TArray<FTSE_MessageEmotes> EmotesByMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chat Data")
	TArray<FDateTime> MessageTimestamps;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chat Data")
	int32 MergedCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chat Data")
	bool bHasCommand = false;

	// Base user info fields (matches FTikTokBaseUserInfo order)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString UniqueId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString Nickname;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString ProfilePictureUrl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 FollowRole = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsModerator = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsSubscriber = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsNewGifter = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 TopGifterRank = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 GifterLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 TeamMemberLevel = 0;
};

/**
 * Follow event output structure - complete fields for dispatch.
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTSE_FollowOut
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString UniqueId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString Nickname;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString ProfilePictureUrl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 FollowRole = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsModerator = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsSubscriber = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsNewGifter = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 TopGifterRank = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 GifterLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 TeamMemberLevel = 0;
};

/**
 * Gift event output structure - complete fields for dispatch.
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTSE_GiftOut
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString UniqueId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString Nickname;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString ProfilePictureUrl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 FollowRole = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsModerator = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsSubscriber = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsNewGifter = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 TopGifterRank = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 GifterLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 TeamMemberLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gift Data")
	int32 GiftId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gift Data")
	FString GiftName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gift Data")
	FString GiftPictureUrl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gift Data")
	int32 DiamondCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gift Data")
	int32 RepeatCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gift Data")
	int32 GiftType = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gift Data")
	FString Describe;
};

// NEW: Gift combo output (flat)
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTSE_GiftComboOut
{
	GENERATED_BODY()

	// Usuario (comunes)
	UPROPERTY(BlueprintReadOnly) FString UniqueId;
	UPROPERTY(BlueprintReadOnly) FString Nickname;
	UPROPERTY(BlueprintReadOnly) FString ProfilePictureUrl;
	UPROPERTY(BlueprintReadOnly) int32 FollowRole = 0;
	UPROPERTY(BlueprintReadOnly) bool bIsModerator = false;
	UPROPERTY(BlueprintReadOnly) bool bIsSubscriber = false;
	UPROPERTY(BlueprintReadOnly) bool bIsNewGifter = false;
	UPROPERTY(BlueprintReadOnly) int32 TopGifterRank = 0;
	UPROPERTY(BlueprintReadOnly) int32 GifterLevel = 0;
	UPROPERTY(BlueprintReadOnly) int32 TeamMemberLevel = 0;

	// Regalo
	UPROPERTY(BlueprintReadOnly) int32 GiftId = 0;
	UPROPERTY(BlueprintReadOnly) FString GiftName;
	UPROPERTY(BlueprintReadOnly) FString GiftPictureUrl;
	UPROPERTY(BlueprintReadOnly) int32 GiftType = 0;
	UPROPERTY(BlueprintReadOnly) FString Describe;
	UPROPERTY(BlueprintReadOnly) FString GroupId;

	// Agregados del combo
	UPROPERTY(BlueprintReadOnly) int32 RepeatCountTotal = 0;
	UPROPERTY(BlueprintReadOnly) int32 DiamondTotal = 0;
	UPROPERTY(BlueprintReadOnly) FDateTime FirstTs;
	UPROPERTY(BlueprintReadOnly) FDateTime LastTs;
	UPROPERTY(BlueprintReadOnly) float DurationSeconds = 0.f;

	// Estado cierre
	UPROPERTY(BlueprintReadOnly) bool bIsFinal = false;
	UPROPERTY(BlueprintReadOnly) bool bComboInitial = false;
	UPROPERTY(BlueprintReadOnly) bool bClosedByInactivity = false;
	UPROPERTY(BlueprintReadOnly) bool bRepeatEnd = false;
};

/**
 * Like event output structure for milestone detection.
 * FASE 7: Agregados campos de milestone para detección en tiempo real.
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTSE_LikeBatchOut
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Like Data")
	int32 TotalLikeCount = 0;

	// FASE 7: Campos de milestone para Like
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Like Milestone")
	int32 LikeMilestone = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Like Milestone")
	int32 LikePreviousMilestone = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Like Milestone")
	int32 LikeStepsCrossed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Like Milestone")
	int32 LikeDeltaSincePrevCommit = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Like Milestone")
	float LikeElapsedSincePrevCommitSec = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Like Milestone")
	float LikesPerSecond = 0.0f;
};

// Nuevo: LikeUser event output structure - emits individual like with full user info
USTRUCT(BlueprintType)
struct FTSE_LikeUserOut
{
	GENERATED_BODY()

	// Identidad usuario
	UPROPERTY(BlueprintReadOnly, Category="User") FString UniqueId;
	UPROPERTY(BlueprintReadOnly, Category="User") FString Nickname;
	UPROPERTY(BlueprintReadOnly, Category="User") FString ProfilePictureUrl;

	// Roles y flags
	UPROPERTY(BlueprintReadOnly, Category="User") int32 FollowRole = 0;
	UPROPERTY(BlueprintReadOnly, Category="User") bool bIsModerator = false;
	UPROPERTY(BlueprintReadOnly, Category="User") bool bIsSubscriber = false;
	UPROPERTY(BlueprintReadOnly, Category="User") bool bIsNewGifter = false;
	UPROPERTY(BlueprintReadOnly, Category="User") int32 TopGifterRank = 0;
	UPROPERTY(BlueprintReadOnly, Category="User") int32 GifterLevel = 0;
	UPROPERTY(BlueprintReadOnly, Category="User") int32 TeamMemberLevel = 0;

	// Likes
	UPROPERTY(BlueprintReadOnly, Category="Like") int32 LikeCount = 0;
	UPROPERTY(BlueprintReadOnly, Category="Like") int64 TotalLikeCount = 0;
};
/**
 * Member event output structure - complete fields for dispatch.
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTSE_MemberOut
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString UniqueId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString Nickname;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString ProfilePictureUrl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 FollowRole = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsModerator = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsSubscriber = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsNewGifter = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 TopGifterRank = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 GifterLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 TeamMemberLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Member Data")
	int32 ActionId = 0;
};

/**
 * RoomUser event output structure - milestone data.
 * FASE 1: Añadido campo Direction para indicar si fue subida/bajada de audiencia.
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTSE_RoomUserMilestoneOut
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Data")
	int32 ViewerCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Data")
	int32 Milestone = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Data", meta=(ToolTip="Milestone anterior emitido (0 si es el primero). Útil para determinar si la audiencia subió o bajó."))
	int32 PreviousMilestone = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Data", meta=(ToolTip="Número de veces que se ha emitido este milestone reportado (1=primera vez, 2=segunda vez...). Incrementa con cada emisión del mismo milestone."))
	int32 EmissionCount = 0;

	/** Algoritmo Prototipo: Dirección del milestone (true=ascendente, false=descendente) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Data", meta=(ToolTip="Dirección del milestone: true=Ascendente (audiencia subió), false=Descendente (audiencia bajó)."))
	bool bIsAscending = true;
};

/**
 * Share event output structure - complete fields for dispatch.
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTSE_ShareOut
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString UniqueId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString Nickname;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	FString ProfilePictureUrl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 FollowRole = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsModerator = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsSubscriber = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	bool bIsNewGifter = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 TopGifterRank = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 GifterLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "User Info")
	int32 TeamMemberLevel = 0;
};

namespace TikStudioCompat
{
	// Generic converter for Emotes: maps any array-like of elements using provided accessors
	template <typename SrcType, typename ContainerType, typename GetIdFunc, typename GetUrlFunc>
	static FORCEINLINE TArray<FTSE_EmoteInfo> ConvertEmotes(const ContainerType& Src, GetIdFunc GetId, GetUrlFunc GetUrl)
	{
		TArray<FTSE_EmoteInfo> Out;
		Out.Reserve(Src.Num());
		for (const SrcType& Item : Src)
		{
			Out.Emplace(GetId(Item), GetUrl(Item));
		}
		return Out;
	}

	// Generic converter for TopViewers: maps any array-like of elements using provided accessors
	template <typename SrcType,
		typename ContainerType,
		typename GetUniqueIdFunc,
		typename GetNicknameFunc,
		typename GetProfilePicFunc,
		typename GetCoinCountFunc,
		typename GetIsModeratorFunc,
		typename GetIsSubscriberFunc,
		typename GetGifterLevelFunc,
		typename GetTeamMemberLevelFunc>
	static FORCEINLINE TArray<FTSE_TopViewerInfo> ConvertTopViewers(
		const ContainerType& Src,
		GetUniqueIdFunc GetUniqueId,
		GetNicknameFunc GetNickname,
		GetProfilePicFunc GetProfilePic,
		GetCoinCountFunc GetCoinCount,
		GetIsModeratorFunc GetIsModerator,
		GetIsSubscriberFunc GetIsSubscriber,
		GetGifterLevelFunc GetGifterLevel,
		GetTeamMemberLevelFunc GetTeamMemberLevel)
	{
		TArray<FTSE_TopViewerInfo> Out;
		Out.Reserve(Src.Num());
		for (const SrcType& Item : Src)
		{
			Out.Emplace(
				GetUniqueId(Item),
				GetNickname(Item),
				GetProfilePic(Item),
				GetCoinCount(Item),
				GetIsModerator(Item),
				GetIsSubscriber(Item),
				GetGifterLevel(Item),
				GetTeamMemberLevel(Item));
		}
		return Out;
	}
}

// RoomUser: Evento periódico completo con datos de audiencia (ViewerCount + TopViewers[])
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTSE_RoomUserOut
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Data")
	int32 ViewerCount = 0;

	// Lista de top viewers en el momento del snapshot
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Data")
	TArray<FTSE_TopViewerInfo> TopViewers;
};

// Nuevo: Evento por cambio de Top 1 (incluye sólo el Top1 actual)
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTSE_RoomUserTop1ChangeOut
{
	GENERATED_BODY()

	// Conteo de espectadores en el momento del cambio
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Data")
	int32 ViewerCount = 0;

	// Información del Top1 actual (posición 0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Data")
	FTSE_TopViewerInfo Top1;
};
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTSE_MemberNormalizedOut
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "MemberNormalized")
	int32 JoinCount = 0;
	UPROPERTY(BlueprintReadOnly, Category = "MemberNormalized")
	int32 GoalJoinCount = 0;
	
	// New fields for burst aggregation and milestone tracking
	UPROPERTY(BlueprintReadOnly, Category = "MemberNormalized")
	int32 CurrentMilestone = 0;
	UPROPERTY(BlueprintReadOnly, Category = "MemberNormalized")
	int32 PreviousMilestone = 0;
	UPROPERTY(BlueprintReadOnly, Category = "MemberNormalized")
	int32 StepsCrossed = 0;
	UPROPERTY(BlueprintReadOnly, Category = "MemberNormalized")
	int32 DeltaJoins = 0;
};

/**
 * ShareMilestone event output structure - milestone data for share events.
 */
USTRUCT(BlueprintType)
struct TIKSTUDIOPLUGIN_API FTSE_ShareMilestoneOut
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category = "ShareMilestone")
	int32 ShareTotalCount = 0;
	
	UPROPERTY(BlueprintReadOnly, Category = "ShareMilestone")
	int32 GoalShareCount = 0;
	
	UPROPERTY(BlueprintReadOnly, Category = "ShareMilestone")
	int32 CurrentMilestone = 0;
	
	UPROPERTY(BlueprintReadOnly, Category = "ShareMilestone")
	int32 PreviousMilestone = 0;

	UPROPERTY(BlueprintReadOnly, Category = "ShareMilestone")
	int32 StepsCrossed = 0;

	UPROPERTY(BlueprintReadOnly, Category = "ShareMilestone")
	int32 DeltaShares = 0;
};