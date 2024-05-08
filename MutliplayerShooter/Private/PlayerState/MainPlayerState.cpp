// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState/MainPlayerState.h"
#include "PlayerCharacter.h"
#include "Controller/MainPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "PlayerCharacter.h"
#include "GameMode/MainGameMode.h"
#include "GameMode/TeamsGameMode.h"
#include "Kismet/GameplayStatics.h"

#include "DebugMacros.h"


void AMainPlayerState::BeginPlay()
{
	Super::BeginPlay();


	if (!HasAuthority()) { return; }
	if (!GetWorld()) { return; }


	AMainPlayerController* playerController = Cast<AMainPlayerController>(GetPlayerController());
	if (!playerController) { return; }



	if (ATeamsGameMode* teamsGameMode = Cast<ATeamsGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		teamsGameMode->SetTeamForPlayer(playerController);
		playerController->OnMatchStateSet(teamsGameMode->GetMatchState(), true);
	}
	else if (AMainGameMode* gameMode = Cast<ATeamsGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		playerController->OnMatchStateSet(gameMode->GetMatchState());
	}
	
}

void AMainPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMainPlayerState, deaths);
	DOREPLIFETIME(AMainPlayerState, playerTeam);
}





void AMainPlayerState::AddToScore(float amount)
{
	// we only call this from server, but just in case
	if (!HasAuthority()) { return; }

	SetScore(GetScore() + amount);
	TrySetPlayer();
	SetScoreOnHUD();
}

void AMainPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	TrySetPlayer();
	SetScoreOnHUD();
}





void AMainPlayerState::AddToDeaths(float amount)
{
	if (!HasAuthority()) { return; }


	deaths += amount;
	TrySetPlayer();
	SetDeathsOnHUD();
}

void AMainPlayerState::OnRep_deaths()
{
	TrySetPlayer();
	SetDeathsOnHUD();
}

void AMainPlayerState::OnRep_playerTeam()
{
	TrySetPlayer();
	SetCharacterColor(playerTeam);
}

void AMainPlayerState::SetTeam(ETeams newTeam)
{
	TrySetPlayer();
	playerTeam = newTeam;

	SetCharacterColor(newTeam);
}



void AMainPlayerState::SetCharacterColor(ETeams newTeam)
{
	if (currentPlayerCharacter)
	{
		currentPlayerCharacter->SetTeamColor(newTeam);
	}
}







void AMainPlayerState::UpdateHUD()
{
	TrySetPlayer();
	SetScoreOnHUD();
	SetDeathsOnHUD();
	SetHealthOnHUD();
	SetShieldOnHUD();
	SetGrenadesOnHUD();
	HideAmmoHUD();
}

void AMainPlayerState::HideAmmoHUD()
{
	if (currentPlayerController)
	{
		currentPlayerController->SetHUDAmmo(HIDE_AMMO_AMOUNT);
	}
}

void AMainPlayerState::SetGrenadesOnHUD()
{
	if (currentPlayerController)
	{
		currentPlayerController->SetHUDGrenades(0);
	}
}

void AMainPlayerState::TrySetPlayer()
{
	TrySetPlayerCharacter();
	TrySetPlayerController();
}

void AMainPlayerState::TrySetPlayerController()
{
	if (!currentPlayerController && currentPlayerCharacter)
	{
		currentPlayerController = Cast<AMainPlayerController>(currentPlayerCharacter->GetController());
	}
}

void AMainPlayerState::TrySetPlayerCharacter()
{
	if (!currentPlayerCharacter)
	{
		currentPlayerCharacter = Cast<APlayerCharacter>(GetPawn());
	}
}





void AMainPlayerState::SetScoreOnHUD()
{
	if (currentPlayerController)
	{
		currentPlayerController->SetHUDScore(GetScore());
	}
}

void AMainPlayerState::SetHealthOnHUD()
{
	if (currentPlayerController)
	{
		currentPlayerController->SetHUDHealth();
	}
}

void AMainPlayerState::SetShieldOnHUD()
{
	if (currentPlayerController)
	{
		currentPlayerController->SetHUDShield();
	}
}

void AMainPlayerState::SetDeathsOnHUD()
{
	if (currentPlayerController)
	{
		currentPlayerController->SetHUDDeaths(deaths);
	}
}


AController* AMainPlayerState::GetLastDamagingInstigatorController()
{
	if (currentPlayerController)
	{
		return currentPlayerController->GetLastDamagingInstigatorController();
	}

	return nullptr;
}

