// TikStudioEventQueue_Follow.cpp
// Part of UTikStudioEventQueue — Follow event handling
// Split from TikStudioEventQueue.cpp following Epic's multi-cpp pattern

#include "EventQueue/TikStudioEventQueue.h"
#include "Engine/Engine.h"

using namespace TikStudioEventTypes;

void UTikStudioEventQueue::EnqueueFollowEvent(const FTSE_FollowIn& Data)
{
    // Early return if Follow is disabled
    if (!Settings.EventToggles.Follow.bEnableFollow)
    {
        return;
    }

    FQueuedTikTokEvent E;
    E.Id = FGuid::NewGuid();
    E.EventType = Follow;
    E.Timestamp = FDateTime::UtcNow();

    // Map user fields if present
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
