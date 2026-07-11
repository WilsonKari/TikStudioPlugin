#include "Compatibility/TikStudioConversionLibrary.h"

#include "EventQueue/TikStudioEventQueue.h"

// Element converter
FTSE_EmoteInfo UTikStudioConversionLibrary::ConvertToTSE_EmoteInfo(const FTikTokEmoteInfo& In)
{
	FTSE_EmoteInfo Out;
	Out.EmoteId = In.EmoteId;
	Out.EmoteImageUrl = In.EmoteImageUrl;
	return Out;
}

// Array converter
TArray<FTSE_EmoteInfo> UTikStudioConversionLibrary::ConvertToTSE_EmoteInfoArray(const TArray<FTikTokEmoteInfo>& In)
{
	TArray<FTSE_EmoteInfo> Out;
	Out.Reserve(In.Num());
	for (const FTikTokEmoteInfo& Item : In)
	{
		Out.Add(ConvertToTSE_EmoteInfo(Item));
	}
	return Out;
}

// Full chat event converter
FTSE_ChatIn UTikStudioConversionLibrary::ConvertToTSE_ChatIn(const FTikTokChatEvent& In)
{
	FTSE_ChatIn Out;
	// Map base user info
	Out.UniqueId = In.UniqueId;
	Out.Nickname = In.Nickname;
	Out.ProfilePictureUrl = In.ProfilePictureUrl;
	Out.FollowRole = In.FollowRole;
	Out.bIsModerator = In.bIsModerator;
	Out.bIsSubscriber = In.bIsSubscriber;
	Out.bIsNewGifter = In.bIsNewGifter;
	Out.TopGifterRank = In.TopGifterRank;
	Out.GifterLevel = In.GifterLevel;
	Out.TeamMemberLevel = In.TeamMemberLevel;
	// Chat-specific
	Out.Comment = In.Comment;
	Out.Emotes = ConvertToTSE_EmoteInfoArray(In.Emotes);
	return Out;
}

// TopViewer element converter
FTSE_TopViewerInfo UTikStudioConversionLibrary::ConvertToTSE_TopViewerInfo(const FTikTokTopViewerData& In)
{
	FTSE_TopViewerInfo Out;
	Out.UniqueId = In.UniqueId;
	Out.Nickname = In.Nickname;
	Out.ProfilePictureUrl = In.ProfilePictureUrl;
	Out.CoinCount = In.CoinCount;
	Out.bIsModerator = In.bIsModerator;
	Out.bIsSubscriber = In.bIsSubscriber;
	Out.GifterLevel = In.GifterLevel;
	Out.TeamMemberLevel = In.TeamMemberLevel;
	return Out;
}

// TopViewer array converter
TArray<FTSE_TopViewerInfo> UTikStudioConversionLibrary::ConvertToTSE_TopViewerInfoArray(const TArray<FTikTokTopViewerData>& In)
{
	TArray<FTSE_TopViewerInfo> Out;
	Out.Reserve(In.Num());
	for (const FTikTokTopViewerData& Item : In)
	{
		Out.Add(ConvertToTSE_TopViewerInfo(Item));
	}
	return Out;
}

// Full RoomUser event converter
FTSE_RoomUserIn UTikStudioConversionLibrary::ConvertToTSE_RoomUserIn(const FTikTokRoomUserEvent& In)
{
	FTSE_RoomUserIn Out;
	Out.ViewerCount = In.ViewerCount;
	Out.TopGifterRank = In.TopGifterRank;
	Out.TopViewers = ConvertToTSE_TopViewerInfoArray(In.TopViewers);
	return Out;
}