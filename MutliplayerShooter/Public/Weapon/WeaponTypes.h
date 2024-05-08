#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EWeaponOutlineDepthType : uint8
{
	EWODT_None = 0 UMETA(DisplayName = "None"),
	EWODT_Purple = 250 UMETA(DisplayName = "Purple"),
	EWODT_Blue = 251 UMETA(DisplayName = "Blue"),
	EWODT_Tan = 252 UMETA(DisplayName = "Tan"),
	EWODT_Max UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_AssaultRifle UMETA(DisplayName = "Assault Rifle"),
	EWT_RocketLauncher UMETA(DisplayName = "Rocket Launcher"),
	EWT_Pistol UMETA(DisplayName = "Pistol"),
	EWT_SubmachineGun UMETA(DisplayName = "SubmachineGun"),
	EWT_Shotgun UMETA(DisplayName = "Shotgun"),
	EWT_SniperRifle UMETA(DisplayName = "Sniper Rifle"),
	EWT_GrenadeLauncher UMETA(DisplayName = "Grenade Launcher"),

	EWT_MAX UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum class EWeaponReloadAnimationType : uint8
{
	EWRAT_NormalReload UMETA(DisplayName = "Normal Reload"),
	EWRAT_GripReload UMETA(DisplayName = "Grip Reload"),
	EWRAT_ShotgunReload UMETA(DisplayName = "Shotgun Reload"),
	EWRAT_RocketLauncherReload UMETA(DisplayName = "Rocket Launcher Reload"),

	EWRAT_MAX UMETA(DisplayName = "DefaultMAX")
};

namespace WeaponReloadTypeName
{
	const FName NORMAL_RELOAD = FName{ TEXT("AssaultRife") };
	const FName GRIP_RELOAD = FName{ TEXT("Pistol") };
	const FName SHOTGUN_RELOAD = FName{ TEXT("Shotgun") };
	const FName SHOTGUN_RELOAD_END = FName{ TEXT("ShotgunEnd") };
	const FName ROCKET_LAUNCHER_RELOAD = FName{ TEXT("RocketLauncher") };
}


