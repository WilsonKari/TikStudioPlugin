// TikStudioEventQueue_Chat.cpp
// Part of UTikStudioEventQueue — Chat event handling
// Split from TikStudioEventQueue.cpp following Epic's multi-cpp pattern

#include "EventQueue/TikStudioEventQueue.h"
#include "Engine/Engine.h"

using namespace TikStudioEventTypes;

void UTikStudioEventQueue::EnqueueChatEvent(const FTSE_ChatIn& Data)
{
	// Early return if Chat is disabled
	if (!Settings.EventToggles.Chat.bEnableChat)
	{
		return;
	}

	// Anti-DoS limits (hard-coded)
	static const int32 MaxMergedMessagesPerEvent = 20;
	static const int32 MaxMergedCharsPerEvent = 2000;

	FQueuedTikTokEvent E;
	E.Id = FGuid::NewGuid();
	E.EventType = Chat;
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

	// Map chat-specific fields
	E.Comment = Data.Comment;
	E.Emotes = Data.Emotes;
	FString TrimmedComment = Data.Comment.TrimStart();
	E.bHasCommand = TrimmedComment.StartsWith(TEXT("!"));

	// Initialize merge arrays with single message
	E.Comments.Add(Data.Comment);
	E.EmotesByMessage.Add(FTSE_MessageEmotes(Data.Emotes));
	E.MessageTimestamps.Add(E.Timestamp);
	E.MergedCount = 1;

	// Assign TTL and compute priority
    E.TTLSeconds = GetTTLForType(E.EventType);
    E.PriorityScore = ComputePriority(E);

	// Check if Chat is locked and if we should merge
	bool bShouldMerge = false;
	// FIX (Bug 2): copiar el FGuid POR VALOR dentro del lock, NUNCA sostener el puntero
	// que devuelve TMap::Find(). Ese puntero apunta al almacenamiento interno del TMap y
	// queda invalidado si el mapa se rehashea/realoca (Add/Remove) durante el hueco entre
	// los dos locks. FGuid es un POD de 16 bytes: la copia es inmune a esa reubicación.
	bool bHasExistingShadow = false;
	FGuid ExistingShadowId;
	
	{
		FScopeLock Lock(&CriticalSection);
		
		// Fusionar siempre que la cola esté ocupada por CUALQUIER evento en reproducción
		// (Follow, Gift, Chat, etc.), no solo cuando el evento bloqueado es Chat. Esto
		// maximiza la deduplicación del flujo de chats del mismo usuario mientras el TTS habla.
		if (bHasLockedEvent && !Data.UniqueId.IsEmpty())
		{
			// Find existing shadow for this user (copiar el valor dentro del lock, no el puntero)
			if (const FGuid* FoundShadowId = ChatShadowByUser.Find(Data.UniqueId))
			{
				ExistingShadowId = *FoundShadowId;
				bHasExistingShadow = true;
			}
			bShouldMerge = true;
		}
	}

	if (bShouldMerge && bHasExistingShadow)
	{
		// REPLACE: Update existing shadow event
		FScopeLock Lock(&CriticalSection);
		
		// Find the shadow event in queue
		for (int32 i = 0; i < Queue.Num(); ++i)
		{
			if (Queue[i].Id == ExistingShadowId && Queue[i].EventType == Chat)
			{
				FQueuedTikTokEvent& Shadow = Queue[i];
				
				// Check anti-DoS limits
				if (Shadow.MergedCount >= MaxMergedMessagesPerEvent)
				{
					UE_LOG(LogTemp, Log, TEXT("[EventQueue] Chat REPLACE shadow uid=%s: MaxMergedMessages reached (%d), enqueueing as separate event"), *Data.UniqueId, MaxMergedMessagesPerEvent);
					// Fall back to normal enqueue
					EnqueueTypedEvent(E.EventType, E);
					return;
				}
				
				// Calculate total chars after adding new message
				int32 TotalChars = 0;
				for (const FString& Comment : Shadow.Comments)
				{
					TotalChars += Comment.Len();
				}
				TotalChars += Data.Comment.Len();
				
				if (TotalChars > MaxMergedCharsPerEvent)
				{
					// Truncate the new comment to fit within limit
					int32 AvailableChars = MaxMergedCharsPerEvent - (TotalChars - Data.Comment.Len());
					FString TruncatedComment = Data.Comment.Left(FMath::Max(0, AvailableChars));
					UE_LOG(LogTemp, Log, TEXT("[EventQueue] Chat REPLACE shadow uid=%s: MaxMergedChars reached, truncating comment from %d to %d chars"), *Data.UniqueId, Data.Comment.Len(), TruncatedComment.Len());
					
					// Add truncated message
					Shadow.Comments.Add(TruncatedComment);
				}
				else
				{
					// Add full message
					Shadow.Comments.Add(Data.Comment);
				}
				
				// Add emotes and timestamp
				Shadow.EmotesByMessage.Add(FTSE_MessageEmotes(Data.Emotes));
				Shadow.MessageTimestamps.Add(E.Timestamp);
				Shadow.MergedCount++;
				
				// Last-wins: update flags and user data with latest message
				Shadow.bHasCommand = E.bHasCommand;
				Shadow.Nickname = Data.Nickname;
				Shadow.ProfilePictureUrl = Data.ProfilePictureUrl;
				Shadow.FollowRole = Data.FollowRole;
				Shadow.bIsModerator = Data.bIsModerator;
				Shadow.bIsSubscriber = Data.bIsSubscriber;
				Shadow.bIsNewGifter = Data.bIsNewGifter;
				Shadow.TopGifterRank = Data.TopGifterRank;
				Shadow.GifterLevel = Data.GifterLevel;
				Shadow.TeamMemberLevel = Data.TeamMemberLevel;
				
				// Refresh CreatedAt for TTL reset and recalculate priority
				Shadow.Timestamp = E.Timestamp;
				Shadow.PriorityScore = ComputePriority(Shadow);
				
				// Re-sort queue to maintain priority order
				Queue.Sort([](const FQueuedTikTokEvent& A, const FQueuedTikTokEvent& B) {
					return A.PriorityScore > B.PriorityScore;
				});
				
				UE_LOG(LogTemp, Log, TEXT("[EventQueue] Chat REPLACE shadow uid=%s, merged=%d, len(comments)=%d, id=%s"), 
					*Data.UniqueId, Shadow.MergedCount, Shadow.Comments.Num(), *Shadow.Id.ToString().Left(8));
				return;
			}
		}
		
		// Shadow ID was found but event no longer exists (expired/evicted)
		ChatShadowByUser.Remove(Data.UniqueId);
		UE_LOG(LogTemp, Log, TEXT("[EventQueue] Chat shadow uid=%s was expired/evicted, cleaning up tracking"), *Data.UniqueId);
	}
	
	if (bShouldMerge)
	{
		// INSERT: Create new shadow event
		FScopeLock Lock(&CriticalSection);
		
		// FIX: registrar la sombra SOLO si el evento entró físicamente a la cola.
		// Si EnqueueTypedEvent descarta (slots llenos o pierde la evicción competitiva),
		// NO debemos dejar un id fantasma en ChatShadowByUser (evita la desincronización
		// que el sweep tenía que limpiar después con "Cleaned expired Chat shadow").
		const bool bInserted = EnqueueTypedEvent(E.EventType, E);
		if (bInserted)
		{
			ChatShadowByUser.Add(Data.UniqueId, E.Id);
			
			UE_LOG(LogTemp, Log, TEXT("[EventQueue] Chat INSERT shadow uid=%s, score=%d, id=%s"), 
				*Data.UniqueId, E.PriorityScore, *E.Id.ToString().Left(8));
			
			RequestImmediatePump(TEXT("Chat INSERT shadow"));
		}
	}
	else
	{
		// Normal enqueue (no lock or different user)
		EnqueueTypedEvent(E.EventType, E);
	}
}
