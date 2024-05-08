#pragma once
UENUM(BlueprintType)
enum class ETeams : uint8
{
	ET_NoTeam UMETA(DisplayName = "NoTeam"),
	ET_BlueTeam UMETA(DisplayName = "BlueTeam"),
	ET_RedTeam UMETA(DisplayName = "RedTeam"),

	ET_MAX UMETA(DisplayName = "DefaultMAX")
};
