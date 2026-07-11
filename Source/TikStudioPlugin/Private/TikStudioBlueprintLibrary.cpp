// Wilson Karl 2025

#include "TikStudioBlueprintLibrary.h"
#include "Engine/World.h"
#include "Utils/TikHelpers.h"

UTikDirectorSubsystem* UTikStudioBlueprintLibrary::GetTikDirectorSubsystem(const UObject* WorldContext)
{
	if (!WorldContext)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<UTikDirectorSubsystem>();
}

FTikStudioErrorStatus UTikStudioBlueprintLibrary::ProcessCommentEvent(const UObject* WorldContext)
{
	UTikDirectorSubsystem* DirectorSubsystem = GetTikDirectorSubsystem(WorldContext);
	if (!DirectorSubsystem)
	{
		return UTikStudioErrorLibrary::CreateError(TEXT("TikDirector subsystem not available"));
	}

	// For AI Director, we only need to count the comment occurrence
	// Username and message content don't affect the engagement scoring
	TMap<FString, FString> EventData;
	EventData.Add(TEXT("type"), TEXT("comment"));

	return DirectorSubsystem->ProcessTikTokEvent(TEXT("comment"), TEXT(""), EventData);
}

FTikStudioErrorStatus UTikStudioBlueprintLibrary::ProcessGiftEvent(const UObject* WorldContext, int32 DiamondCount, bool bRepeatEnd)
{
	UTikDirectorSubsystem* DirectorSubsystem = GetTikDirectorSubsystem(WorldContext);
	if (!DirectorSubsystem)
	{
		return UTikStudioErrorLibrary::CreateError(TEXT("TikDirector subsystem not available"));
	}

	// Ignore closure events - they don't represent new gifts
	if (bRepeatEnd)
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("Ignoring gift closure event (RepeatEnd=true)"));
		return UTikStudioErrorLibrary::CreateSuccess(); // Not an error, just skipping
	}

	// Process only the actual gift value from real events
	TMap<FString, FString> EventData;
	EventData.Add(TEXT("giftValue"), FString::FromInt(DiamondCount)); // Individual gift value
	EventData.Add(TEXT("diamondCount"), FString::FromInt(DiamondCount));

	UE_LOG(LogTemp, VeryVerbose, TEXT("Processing gift event (DiamondCount: %d)"), DiamondCount);

	return DirectorSubsystem->ProcessTikTokEvent(TEXT("gift"), TEXT(""), EventData);
}

FTikStudioErrorStatus UTikStudioBlueprintLibrary::ProcessFollowEvent(const UObject* WorldContext)
{
	UTikDirectorSubsystem* DirectorSubsystem = GetTikDirectorSubsystem(WorldContext);
	if (!DirectorSubsystem)
	{
		return UTikStudioErrorLibrary::CreateError(TEXT("TikDirector subsystem not available"));
	}

	// For AI Director, we only need to count the follow occurrence
	// Username doesn't affect the engagement scoring
	TMap<FString, FString> EventData;
	EventData.Add(TEXT("type"), TEXT("follow"));

	return DirectorSubsystem->ProcessTikTokEvent(TEXT("follow"), TEXT(""), EventData);
}

FTikStudioErrorStatus UTikStudioBlueprintLibrary::ProcessLikeEvent(const UObject* WorldContext, int32 LikeCount)
{
	UTikDirectorSubsystem* DirectorSubsystem = GetTikDirectorSubsystem(WorldContext);
	if (!DirectorSubsystem)
	{
		return UTikStudioErrorLibrary::CreateError(TEXT("TikDirector subsystem not available"));
	}

	// Process multiple likes as separate events to maintain consistency with the AI Director logic
	// The original prototype assumed 1 like = 1 event, so we simulate that behavior
	for (int32 i = 0; i < LikeCount; i++)
	{
		TMap<FString, FString> EventData;
		EventData.Add(TEXT("likeIndex"), FString::FromInt(i + 1));
		EventData.Add(TEXT("totalLikesInBatch"), FString::FromInt(LikeCount));
		
		FTikStudioErrorStatus Result = DirectorSubsystem->ProcessTikTokEvent(TEXT("like"), TEXT(""), EventData);
		if (UTikStudioErrorLibrary::IsError(Result))
		{
			return Result; // Return first error encountered
		}
	}

	UE_LOG(LogTemp, VeryVerbose, TEXT("Processed %d likes"), LikeCount);
	return UTikStudioErrorLibrary::CreateSuccess();
}

FTikStudioErrorStatus UTikStudioBlueprintLibrary::ProcessShareEvent(const UObject* WorldContext)
{
	UTikDirectorSubsystem* DirectorSubsystem = GetTikDirectorSubsystem(WorldContext);
	if (!DirectorSubsystem)
	{
		return UTikStudioErrorLibrary::CreateError(TEXT("TikDirector subsystem not available"));
	}

	// For AI Director, we only need to count the share occurrence
	// Username doesn't affect the engagement scoring
	TMap<FString, FString> EventData;
	EventData.Add(TEXT("type"), TEXT("share"));

	return DirectorSubsystem->ProcessTikTokEvent(TEXT("share"), TEXT(""), EventData);
}

FString UTikStudioBlueprintLibrary::GetDirectorStatusText(const UObject* WorldContext)
{
	UTikDirectorSubsystem* DirectorSubsystem = GetTikDirectorSubsystem(WorldContext);
	if (!DirectorSubsystem)
	{
		return TEXT("Director not available");
	}

	FTikDirectorStatus Status = DirectorSubsystem->GetDirectorStatus();
	
	FString StatusText = FString::Printf(TEXT("State: %s | Activity: %.1f | Hype: %.2fx | Flavor: %s"),
		*UTikHelpers::DirectorStateToString(Status.CurrentState),
		Status.CurrentActivity,
		Status.HypeMultiplier,
		*UTikHelpers::FlavorTypeToString(Status.CurrentFlavor)
	);

	if (Status.CurrentState == ETikDirectorState::COOLDOWN)
	{
		StatusText += FString::Printf(TEXT(" | Cooldown: %.0fs"), Status.CooldownRemainingTime);
	}

	return StatusText;
}

bool UTikStudioBlueprintLibrary::IsInHighActivityState(const UObject* WorldContext)
{
	UTikDirectorSubsystem* DirectorSubsystem = GetTikDirectorSubsystem(WorldContext);
	if (!DirectorSubsystem)
	{
		return false;
	}

	ETikDirectorState CurrentState = DirectorSubsystem->GetCurrentState();
	return CurrentState == ETikDirectorState::INTENSIVE || CurrentState == ETikDirectorState::COOLDOWN;
}

bool UTikStudioBlueprintLibrary::IsInCooldown(const UObject* WorldContext)
{
	UTikDirectorSubsystem* DirectorSubsystem = GetTikDirectorSubsystem(WorldContext);
	if (!DirectorSubsystem)
	{
		return false;
	}

	return DirectorSubsystem->GetCurrentState() == ETikDirectorState::COOLDOWN;
}

float UTikStudioBlueprintLibrary::GetActivityPercentage(const UObject* WorldContext)
{
	UTikDirectorSubsystem* DirectorSubsystem = GetTikDirectorSubsystem(WorldContext);
	if (!DirectorSubsystem)
	{
		return 0.0f;
	}

	float CurrentActivity = DirectorSubsystem->GetCurrentActivity();
	float CurrentThreshold = DirectorSubsystem->GetCurrentClimaxThreshold();

	if (CurrentThreshold <= 0.0f)
	{
		return 0.0f;
	}

	return FMath::Clamp((CurrentActivity / CurrentThreshold) * 100.0f, 0.0f, 100.0f);
}

FTikDirectorConfig UTikStudioBlueprintLibrary::CreateDefaultConfig()
{
	return FTikDirectorConfig();
}