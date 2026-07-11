// Utility Blueprint conversion nodes to bridge TikFinity types to TikStudio types
#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "EventQueue/TSEventTypes.h"
// TikFinity public headers (structs we convert from)
#include "Events/TikTokChatEvent.h"
#include "Events/TikTokRoomUserEvent.h"
#include "TikStudioConversionLibrary.generated.h"

/**
 * Blueprint conversion helpers (autocast) to convert TikFinity structs to TikStudio structs.
 * These nodes allow wiring arrays/structs across modules directly in Blueprints.
 */
UCLASS()
class TIKSTUDIOPLUGIN_API UTikStudioConversionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	// Element converter: TikTokEmoteInfo -> FTSE_EmoteInfo
	UFUNCTION(BlueprintPure, Category="TikStudio|Compat", meta=(DisplayName="To TSE Emote Info", CompactNodeTitle="->", BlueprintAutocast))
	static FTSE_EmoteInfo ConvertToTSE_EmoteInfo(const FTikTokEmoteInfo& In);

	// Array converter: TArray<TikTokEmoteInfo> -> TArray<FTSE_EmoteInfo>
	UFUNCTION(BlueprintPure, Category="TikStudio|Compat", meta=(DisplayName="To TSE Emote Info Array", CompactNodeTitle="->", BlueprintAutocast))
	static TArray<FTSE_EmoteInfo> ConvertToTSE_EmoteInfoArray(const TArray<FTikTokEmoteInfo>& In);

	// Optional full event converter: TikTokChatEvent -> FTSE_ChatIn (includes Emotes)
	UFUNCTION(BlueprintPure, Category="TikStudio|Compat", meta=(DisplayName="To TSE Chat In", CompactNodeTitle="->", BlueprintAutocast))
	static FTSE_ChatIn ConvertToTSE_ChatIn(const FTikTokChatEvent& In);

	// Element converter: TikTokTopViewerData -> FTSE_TopViewerInfo
	UFUNCTION(BlueprintPure, Category="TikStudio|Compat", meta=(DisplayName="To TSE Top Viewer Info", CompactNodeTitle="->", BlueprintAutocast))
	static FTSE_TopViewerInfo ConvertToTSE_TopViewerInfo(const FTikTokTopViewerData& In);

	// Array converter: TArray<TikTokTopViewerData> -> TArray<FTSE_TopViewerInfo>
	UFUNCTION(BlueprintPure, Category="TikStudio|Compat", meta=(DisplayName="To TSE Top Viewer Info Array", CompactNodeTitle="->", BlueprintAutocast))
	static TArray<FTSE_TopViewerInfo> ConvertToTSE_TopViewerInfoArray(const TArray<FTikTokTopViewerData>& In);

	// Optional full event converter: TikTokRoomUserEvent -> FTSE_RoomUserIn (includes TopViewers)
	UFUNCTION(BlueprintPure, Category="TikStudio|Compat", meta=(DisplayName="To TSE Room User In", CompactNodeTitle="->", BlueprintAutocast))
	static FTSE_RoomUserIn ConvertToTSE_RoomUserIn(const FTikTokRoomUserEvent& In);
};