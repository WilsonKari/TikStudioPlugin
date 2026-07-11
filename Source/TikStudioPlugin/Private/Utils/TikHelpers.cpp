// Wilson Karl 2025

#include "Utils/TikHelpers.h"
#include "Engine/Engine.h"

float UTikHelpers::CalculateEventScore(const FTikEvent& Event, const FTikEventWeights& Weights, float HypeMultiplier)
{
	float Weight = GetEventWeight(Event.EventType, Weights);
	float BaseScore = Weight;
	
	// Ajustes específicos por tipo de evento (migrado de helpers.js)
	if (Event.EventType == ETikEventType::GIFT)
	{
		// Buscar giftValue en EventData (equivale al DiamondCount del regalo)
		const FString* GiftValuePtr = Event.EventData.Find(TEXT("giftValue"));
		if (GiftValuePtr && !GiftValuePtr->IsEmpty())
		{
			float GiftValue = FCString::Atof(**GiftValuePtr);
			BaseScore = Weight * FMath::Max(GiftValue, 1.0f);
		}

	}

	
	return BaseScore * HypeMultiplier;
}

ETikFlavorType UTikHelpers::DetermineDominantFlavor(const TArray<FTikEvent>& RecentEvents, float DominanceThreshold)
{
	if (RecentEvents.Num() == 0)
	{
		return ETikFlavorType::IDLE;
	}

	// Calcular totales por tipo de evento (migrado de helpers.js)
	TMap<ETikEventType, float> Totals;
	float TotalScore = 0.0f;

	for (const FTikEvent& Event : RecentEvents)
	{
		// CRITICAL FIX: Use the actual calculated score instead of fixed 1.0f
		// This ensures flavor analysis reflects real event impact with configured weights
		// The score includes: base weight + hype multiplier + special conditions (gifts, etc.)
		float EventScore = Event.Score; // Use the preserved calculated score from event processing

		if (Totals.Contains(Event.EventType))
		{
			Totals[Event.EventType] += EventScore;
		}
		else
		{
			Totals.Add(Event.EventType, EventScore);
		}
		TotalScore += EventScore;
	}

	// Buscar el tipo dominante
	for (const auto& TypeTotal : Totals)
	{
		float DominanceRatio = TypeTotal.Value / TotalScore;
		if (DominanceRatio >= DominanceThreshold)
		{
			switch (TypeTotal.Key)
			{
				case ETikEventType::GIFT:
					return ETikFlavorType::GENEROSITY;
				case ETikEventType::LIKE:
					return ETikFlavorType::HYPE;
				case ETikEventType::COMMENT:
				case ETikEventType::SHARE:
					return ETikFlavorType::ENGAGEMENT;
				default:
					return ETikFlavorType::COMMUNITY;
			}
		}
	}

	return ETikFlavorType::COMMUNITY;
}

FLinearColor UTikHelpers::GetStateColor(ETikDirectorState State)
{
	// Migrado de getStateColor en helpers.js
	switch (State)
	{
		case ETikDirectorState::DORMANT:
			return FLinearColor(0.419f, 0.447f, 0.502f); // #6B7280
		case ETikDirectorState::ACTIVE:
			return FLinearColor(0.063f, 0.725f, 0.506f); // #10B981
		case ETikDirectorState::INTENSIVE:
			return FLinearColor(0.961f, 0.620f, 0.043f); // #F59E0B
		case ETikDirectorState::COOLDOWN:
			return FLinearColor(0.937f, 0.278f, 0.278f); // #EF4444
		case ETikDirectorState::LURKING:
			return FLinearColor(0.545f, 0.361f, 0.965f); // #8B5CF6
		default:
			return FLinearColor(0.419f, 0.447f, 0.502f); // Default to DORMANT color
	}
}

FLinearColor UTikHelpers::GetFlavorColor(ETikFlavorType Flavor)
{
	// Migrado de getFlavorInfo en helpers.js
	switch (Flavor)
	{
		case ETikFlavorType::IDLE:
			return FLinearColor(0.419f, 0.447f, 0.502f); // #6B7280
		case ETikFlavorType::COMMUNITY:
			return FLinearColor(0.231f, 0.510f, 0.965f); // #3B82F6
		case ETikFlavorType::GENEROSITY:
			return FLinearColor(0.925f, 0.286f, 0.600f); // #EC4899
		case ETikFlavorType::HYPE:
			return FLinearColor(0.961f, 0.620f, 0.043f); // #F59E0B
		case ETikFlavorType::ENGAGEMENT:
			return FLinearColor(0.063f, 0.725f, 0.506f); // #10B981
		default:
			return FLinearColor(0.419f, 0.447f, 0.502f); // Default to IDLE color
	}
}

FString UTikHelpers::FormatTime(float Timestamp)
{
	// Migrado de formatTime en helpers.js
	FDateTime DateTime = FDateTime::FromUnixTimestamp(Timestamp);
	return DateTime.ToString(TEXT("%H:%M:%S"));
}

FString UTikHelpers::EventTypeToString(ETikEventType EventType)
{
	switch (EventType)
	{
		case ETikEventType::COMMENT:
			return TEXT("comment");
		case ETikEventType::FOLLOW:
			return TEXT("follow");
		case ETikEventType::GIFT:
			return TEXT("gift");
		case ETikEventType::LIKE:
			return TEXT("like");
		case ETikEventType::SHARE:
			return TEXT("share");
		default:
			return TEXT("unknown");
	}
}

ETikEventType UTikHelpers::StringToEventType(const FString& EventTypeString)
{
	if (EventTypeString.Equals(TEXT("comment"), ESearchCase::IgnoreCase))
	{
		return ETikEventType::COMMENT;
	}
	else if (EventTypeString.Equals(TEXT("follow"), ESearchCase::IgnoreCase))
	{
		return ETikEventType::FOLLOW;
	}
	else if (EventTypeString.Equals(TEXT("gift"), ESearchCase::IgnoreCase))
	{
		return ETikEventType::GIFT;
	}
	else if (EventTypeString.Equals(TEXT("like"), ESearchCase::IgnoreCase))
	{
		return ETikEventType::LIKE;
	}
	else if (EventTypeString.Equals(TEXT("share"), ESearchCase::IgnoreCase))
	{
		return ETikEventType::SHARE;
	}
	
	return ETikEventType::COMMENT; // Default
}

FString UTikHelpers::DirectorStateToString(ETikDirectorState State)
{
	switch (State)
	{
		case ETikDirectorState::DORMANT:
			return TEXT("DORMANT");
		case ETikDirectorState::ACTIVE:
			return TEXT("ACTIVE");
		case ETikDirectorState::INTENSIVE:
			return TEXT("INTENSIVE");
		case ETikDirectorState::COOLDOWN:
			return TEXT("COOLDOWN");
		case ETikDirectorState::LURKING:
			return TEXT("LURKING");
		default:
			return TEXT("UNKNOWN");
	}
}

FString UTikHelpers::FlavorTypeToString(ETikFlavorType Flavor)
{
	switch (Flavor)
	{
		case ETikFlavorType::IDLE:
			return TEXT("IDLE");
		case ETikFlavorType::COMMUNITY:
			return TEXT("COMMUNITY");
		case ETikFlavorType::GENEROSITY:
			return TEXT("GENEROSITY");
		case ETikFlavorType::HYPE:
			return TEXT("HYPE");
		case ETikFlavorType::ENGAGEMENT:
			return TEXT("ENGAGEMENT");
		default:
			return TEXT("UNKNOWN");
	}
}

float UTikHelpers::GetEventWeight(ETikEventType EventType, const FTikEventWeights& Weights)
{
	switch (EventType)
	{
		case ETikEventType::COMMENT:
			return Weights.Comment;
		case ETikEventType::FOLLOW:
			return Weights.Follow;
		case ETikEventType::GIFT:
			return Weights.Gift;
		case ETikEventType::LIKE:
			return Weights.Like;
		case ETikEventType::SHARE:
			return Weights.Share;
		default:
			return 1.0f;
	}
}

float UTikHelpers::GetDecayRate(ETikDirectorState State, const FTikDecayRates& DecayRates)
{
	switch (State)
	{
		case ETikDirectorState::DORMANT:
			return DecayRates.Dormant;
		case ETikDirectorState::ACTIVE:
			return DecayRates.Active;
		case ETikDirectorState::INTENSIVE:
			return DecayRates.Intensive;
		default:
			return DecayRates.Active; // Default to ACTIVE decay rate
	}
}