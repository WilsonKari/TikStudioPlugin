// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TSEventTypes.h"
#include "TikStudioEventDispatcher.generated.h"

// Dynamic multicast delegates for processed events (flat typed outputs)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatEventProcessed, FTSE_ChatOut, Data, FGuid, EventId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFollowEventProcessed, FTSE_FollowOut, Data, FGuid, EventId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGiftEventProcessed, FTSE_GiftOut, Data, FGuid, EventId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLikeEventProcessed, FTSE_LikeBatchOut, Data, FGuid, EventId);
// Nuevo: LikeUser
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLikeUserEventProcessed, FTSE_LikeUserOut, Data, FGuid, EventId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMemberNormalizedProcessed, FTSE_MemberNormalizedOut, Data, FGuid, EventId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMemberIdentityProcessed, FTSE_MemberOut, Data, FGuid, EventId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRoomUserMilestoneProcessed, FTSE_RoomUserMilestoneOut, Data, FGuid, EventId);
// Nuevo: delegates RoomUser derivados
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRoomUserProcessed, FTSE_RoomUserOut, Data, FGuid, EventId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRoomUserTop1ChangeProcessed, FTSE_RoomUserTop1ChangeOut, Data, FGuid, EventId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShareEventProcessed, FTSE_ShareOut, Data, FGuid, EventId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShareMilestoneProcessed, FTSE_ShareMilestoneOut, Data, FGuid, EventId);

// Nuevo: delegate para combos de regalos
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGiftComboProcessed, FTSE_GiftComboOut, Data, FGuid, EventId);

/**
 * Class for dispatching processed TikTok events to Blueprint.
 * Contains dynamic multicast delegates for each event type.
 */
UCLASS(BlueprintType)
class TIKSTUDIOPLUGIN_API UTikStudioEventDispatcher : public UObject
{
	GENERATED_BODY()

public:
	/** Delegate for processed Chat events */
	UPROPERTY(BlueprintAssignable, Category = "EventQueue")
	FOnChatEventProcessed OnChatEventProcessed;

	/** Delegate for processed Follow events */
	UPROPERTY(BlueprintAssignable, Category = "EventQueue")
	FOnFollowEventProcessed OnFollowEventProcessed;

	/** Delegate for processed Gift events */
	UPROPERTY(BlueprintAssignable, Category = "EventQueue")
	FOnGiftEventProcessed OnGiftEventProcessed;

	/** Delegate for processed Like events */
	UPROPERTY(BlueprintAssignable, Category = "EventQueue")
	FOnLikeEventProcessed OnLikeEventProcessed;

	/** Delegate for processed LikeUser events */
	UPROPERTY(BlueprintAssignable, Category = "EventQueue")
	FOnLikeUserEventProcessed OnLikeUserEventProcessed;
	
	/** Delegate for processed Member events */
	UPROPERTY(BlueprintAssignable, Category = "EventQueue")
	FOnMemberNormalizedProcessed OnMemberNormalizedProcessed;
	UPROPERTY(BlueprintAssignable, Category = "EventQueue")
	FOnMemberIdentityProcessed OnMemberIdentityProcessed;

	/** Delegate for processed RoomUser events */
	UPROPERTY(BlueprintAssignable, Category = "EventQueue")
	FOnRoomUserMilestoneProcessed OnRoomUserMilestoneProcessed;

	/** Delegate for processed RoomUser snapshot events */
	UPROPERTY(BlueprintAssignable, Category = "EventQueue")
	FOnRoomUserProcessed OnRoomUserProcessed;

	/** Delegate for processed RoomUser Top1 change events */
	UPROPERTY(BlueprintAssignable, Category = "EventQueue")
	FOnRoomUserTop1ChangeProcessed OnRoomUserTop1ChangeProcessed;

	/** Delegate for processed Share events */
	UPROPERTY(BlueprintAssignable, Category = "EventQueue")
	FOnShareEventProcessed OnShareEventProcessed;

	/** Delegate for processed ShareMilestone events */
	UPROPERTY(BlueprintAssignable, Category = "EventQueue")
	FOnShareMilestoneProcessed OnShareMilestoneProcessed;

	// Nuevo: combos de regalos
	UPROPERTY(BlueprintAssignable, Category = "EventQueue")
	FOnGiftComboProcessed OnGiftComboProcessed;
};