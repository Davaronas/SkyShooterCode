// Fill out your copyright notice in the Description page of Project Settings.


#include "Controller/MainPlayerController.h"
#include "HUD/MainHUD.h"
#include "PlayerCharacter.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameMode.h"
#include "HUD/AnnouncementWidget.h"
#include "HUD/CharacterOverlayWidget.h"
#include "GameMode/MainGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerCharacter.h"
#include "GameState/MainGameState.h"
#include "PlayerState/MainPlayerState.h"
#include "HelperTypes/HitDirection.h"
#include "HUD/ReturnToMainMenuWidget.h"
#include "Interface/InstigatorTypeInterface.h"
#include "HelperTypes/Teams.h"
#include "PlayerPrefsSave.h"




#include "DebugMacros.h"



void AMainPlayerController::BeginPlay()
{
	Super::BeginPlay();
	mainHUD = Cast<AMainHUD>(GetHUD());
	// this does not get reset on seamless travel, and not updates
	// in SetHUDTimers because it's not 0
	levelStartingTime = 0.f;
	ServerCheckMatchState();

	if (IsLocalController())
	{
		SetInputMode(FInputModeGameOnly{});
		SetShowMouseCursor(false);
	}

	UPROPERTY()
	UPlayerPrefsSave* saveGame = UPlayerPrefsSave::Load();

	if (saveGame)
	{
		mouseSensitivity = saveGame->GetMouseSensitivity();
	}

}


void AMainPlayerController::ShowPlayerBoard()
{
	if (!IsLocalController()) { return; }

	AMainGameState* gameState = Cast<AMainGameState>(UGameplayStatics::GetGameState(this));
	if (!gameState) { return; }

	//GEPRS_Y("ShowPlayerBoard");

	const TArray<APlayerState*>& players = gameState->PlayerArray;

	TArray<FPlayerBoardInfo> playerInfos{};

	for (auto player : players)
	{
		if (player)
		{
			AMainPlayerState* mainPlayerState = Cast<AMainPlayerState>(player);
			if (!mainPlayerState) { return; }

			FPlayerBoardInfo thisPlayerInfo{};
			thisPlayerInfo.playerName = player->GetPlayerName();
			thisPlayerInfo.score = player->GetScore();
			thisPlayerInfo.deaths = mainPlayerState->GetDeaths();
			thisPlayerInfo.ping = player->GetPingInMilliseconds();
			thisPlayerInfo.team = mainPlayerState->GetTeam();
			playerInfos.Add(thisPlayerInfo);
		}
	}

	playerInfos.Sort([](const FPlayerBoardInfo& A, const FPlayerBoardInfo& B) {return A.score > B.score; });

	if (mainHUD)
	{
		mainHUD->DisplayPlayerBoard(playerInfos);
	}
}


void AMainPlayerController::HidePlayerBoard()
{
	if (!IsLocalController()) { return; }

	//GEPRS_Y("HidePlayerBoard");

	if (mainHUD)
	{
		mainHUD->HidePlayerBoard();
	}

}






void AMainPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AMainPlayerController, currentMatchState, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AMainPlayerController, bTeamMatch, COND_OwnerOnly);
}

void AMainPlayerController::OnHudInitialized()
{
	ServerCheckMatchState();
}


void AMainPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	SetHUDTimers();
	CheckClientTimeSync(DeltaTime);
	PollToSetStoredHUDValues();
	PollPlayerState();
	CheckPing(DeltaTime);
	UpdateHUD_Ping();
}

void AMainPlayerController::UpdateHUD_Ping()
{
	if (IsLocalController())
	{
		PlayerState = PlayerState == nullptr ? GetPlayerState<AMainPlayerState>() : PlayerState;
		if (PlayerState)
		{
			if (IsHUD_Usable())
			{
				thisFramePingMs = FMath::CeilToInt(PlayerState->GetPingInMilliseconds());
				if (lastFramePingMs == thisFramePingMs) { return; }

				lastFramePingMs = thisFramePingMs;
				mainHUD->SetPingText(thisFramePingMs);
			}
		}
	}
}

void AMainPlayerController::CheckPing(float DeltaTime)
{
	highPingRunningTime += DeltaTime;
	if (highPingRunningTime > checkPingInterval)
	{
		highPingRunningTime = 0.f;
		PlayerState = PlayerState == nullptr ? GetPlayerState<AMainPlayerState>() : PlayerState;
		if (PlayerState)
		{
			const float ping = PlayerState->GetPingInMilliseconds();
			if (ping > highPingThreshold)
			{
				HighPingWarning();
			}
		}
	}

	/*if (IsHUD_Usable())
	{
		if (mainHUD->IsHighPingAnimationPlaying())
		{
			highPingWarningRunningTime += DeltaTime;
			if (highPingWarningRunningTime > highPingWarningDuration)
			{
				StopHighPingWarning();
				highPingWarningRunningTime = 0;
			}
		}
	}*/
}


void AMainPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!InPawn) { return; }
	if (APlayerCharacter* playerCharacter = Cast<APlayerCharacter>(InPawn))
	{
		SetHUDHealth(playerCharacter->GetCurrentHealth(), playerCharacter->GetMaxHealth());
		SetHUDAmmo(HIDE_AMMO_AMOUNT);

		playerCharacter->SetMouseSensitivity(mouseSensitivity);

		if (returnToMainMenuWidget)
		{
			returnToMainMenuWidget->PlayerGotPawn();
		}
		// set sensitvity
	}
}

void AMainPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (!IsLocalController() || !GetWorld()) { return; }

	ServerRequestTime(GetWorld()->TimeSeconds);
}



void AMainPlayerController::CheckClientTimeSync(float DeltaTime)
{
	if (!IsLocalController() || HasAuthority()) { return; }

	timeSyncRunningTime += DeltaTime;
	if (timeSyncRunningTime > timeSyncFrequency)
	{
		if (GetWorld())
		{
			ServerRequestTime(GetWorld()->TimeSeconds);
			timeSyncRunningTime = 0.f;
		}
	}
}




void AMainPlayerController::ServerRequestLevelStartTime_Implementation()
{
	if (AMainGameMode* gameMode = Cast<AMainGameMode>(GetWorld()->GetAuthGameMode()))
	{
		levelStartingTime = gameMode->GetLevelStartingTime();
		ClientReportLevelStartTime(levelStartingTime);
	}
}

void AMainPlayerController::ClientReportLevelStartTime_Implementation(const float& levelStartTime_)
{
	levelStartingTime = levelStartTime_;
}




void AMainPlayerController::ServerCheckMatchState_Implementation()
{
	if (!HasAuthority()) { return; }
	if (!GetWorld()) { return; }


	if (AMainGameMode* gameMode = Cast<AMainGameMode>(GetWorld()->GetAuthGameMode()))
	{
		warmupTime = gameMode->GetWarmupTime();
		matchTime = gameMode->GetMatchTime();
		levelStartingTime = gameMode->GetLevelStartingTime();
		currentMatchState = gameMode->GetMatchState();
		cooldownTime = gameMode->GetCooldownTime();
		bTeamMatch = gameMode->IsTeamMatch();

		HandleWaitingToStartState();

		if(!IsLocalController())
		ClientReportMatchState(currentMatchState, warmupTime, matchTime, cooldownTime, levelStartingTime, bTeamMatch);
	}
	else
	{
		if (!IsLocalController())
		ClientWaitForGameMode();
	}
}

void AMainPlayerController::ClientReportMatchState_Implementation
(const FName stateOfMatch,
	const float warmupTime_, const float matchTime_, const float cooldownTime_,
	const float levelStartingTime_, bool bIsTeamMatch)
{
	currentMatchState = stateOfMatch;
	warmupTime = warmupTime_;
	matchTime = matchTime_;
	levelStartingTime = levelStartingTime_;
	cooldownTime = cooldownTime_;
	bTeamMatch = bIsTeamMatch;

	OnMatchStateSet(currentMatchState, bTeamMatch);
}


void AMainPlayerController::ClientWaitForGameMode_Implementation()
{
	if (!GetWorld()) { return; }

	GetWorld()->GetTimerManager()
		.SetTimer(waitForGameModeTimer, this, &ThisClass::ServerCheckMatchState, waitForGameModeTime);
}


void AMainPlayerController::OnMatchStateSet(FName matchState, bool bTeams)
{
	currentMatchState = matchState;
	HandleTeamMatch(bTeams);
	HandleMatchStates();
}



void AMainPlayerController::HandleTeamMatch(bool bTeams)
{
	// replicated
	bTeamMatch = bTeams;
	if (bTeams)
	{
		InitTeamScores();
	}
	else
	{
		HideTeamScores();
	}
}

void AMainPlayerController::OnRep_bTeamMatch()
{
	if (bTeamMatch)
	{
		InitTeamScores();
	}
	else
	{
		HideTeamScores();
	}
}

void AMainPlayerController::OnRep_currentMatchState()
{
	HandleMatchStates();
}

void AMainPlayerController::HandleMatchStates()
{
	if (!IsLocalController()) { return; }

	if (!mainHUD) { mainHUD = Cast<AMainHUD>(GetHUD()); }
	if (mainHUD)
	{
		HandleWaitingToStartState();
		HandleInProgressState();
		HandleCooldownState();
	}
	else
	{
		WaitForHUDHandleMatchStates();
	}
}

void AMainPlayerController::WaitForHUDHandleMatchStates()
{
	GetWorldTimerManager().SetTimer
	(matchStateHandleWaitForHUDInitializeTimer, this, &ThisClass::HandleMatchStates, waitForHUDTime);
}


void AMainPlayerController::HandleWaitingToStartState()
{
	// sync level start time with game mode
	

	if (currentMatchState == MatchState::WaitingToStart)
	{
		ServerRequestLevelStartTime();

		if (!mainHUD) { mainHUD = Cast<AMainHUD>(GetHUD()); }
		if (mainHUD)
		{
			mainHUD->AddAnnouncementWidget();
		}
	}
}

void AMainPlayerController::HandleInProgressState()
{
	if (currentMatchState == MatchState::InProgress)
	{
		if (!mainHUD) { mainHUD = Cast<AMainHUD>(GetHUD()); }
		if (mainHUD)
		{
			mainHUD->AddOverlayWidget();

			if (mainHUD->announcementWidget)
			{
				mainHUD->announcementWidget->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}

void AMainPlayerController::HandleCooldownState()
{
	if (currentMatchState == MatchState::Cooldown)
	{
		if (IsHUD_Usable())
		{
			mainHUD->overlayWidget->RemoveFromParent();
		}

		if (mainHUD && !mainHUD->announcementWidget)
		{
			mainHUD->AddAnnouncementWidget();
		}
		if (mainHUD && mainHUD->announcementWidget)
		{
			mainHUD->SetAnnouncementInfoText(CreateCooldownTitleMessage());
			mainHUD->SetAnnouncementText(TEXT("New Match Starts in:"));
			mainHUD->announcementWidget->SetVisibility(ESlateVisibility::Visible);
		}

		if (APlayerCharacter* player = Cast<APlayerCharacter>(GetPawn()))
		{
			player->DisableGameplay();
		}
	}
}

FString AMainPlayerController::CreateCooldownTitleMessage()
{
	if (!GetWorld()) { return FString{}; }

	AMainGameState* gameState = GetWorld()->GetGameState< AMainGameState>();
	AMainPlayerState* thisPlayerState = GetPlayerState<AMainPlayerState>();

	if (gameState && thisPlayerState)
	{
		FString infoText{};

		if (bTeamMatch)
		{
			if (gameState->blueTeamScore > gameState->redTeamScore)
			{
				infoText = FString(TEXT("Blue Team Won\n"));
				infoText.Append(FString::Printf
				(TEXT("With a Score of %.0f to %.0f!"), gameState->blueTeamScore, gameState->redTeamScore));
			}
			else if (gameState->blueTeamScore < gameState->redTeamScore)
			{
				infoText = FString(TEXT("Red Team Won\n"));
				infoText.Append(FString::Printf
				(TEXT("With a Score of %.0f to %.0f!"), gameState->redTeamScore, gameState->blueTeamScore));
			}
			else if(gameState->blueTeamScore == gameState->redTeamScore)
			{
				infoText = FString(TEXT("The Teams Are Tied!\n"));
				infoText.Append(FString::Printf
				(TEXT("%.0f : %.0f"), gameState->redTeamScore, gameState->blueTeamScore));
			}
		}
		else
		{
			const TArray<AMainPlayerState*>& topScorePlayers = gameState->GetTopScoringPlayers();

			if (topScorePlayers.Num() == 0)
			{
				infoText = FString(TEXT("There is no winner..."));
			}
			else if (topScorePlayers.Num() == 1 && topScorePlayers[0] == thisPlayerState)
			{
				infoText = FString(TEXT("You Are The Winner!"));
			}
			else if (topScorePlayers.Num() == 1)
			{
				infoText = FString::Printf(TEXT("The Winner is %s!"),
					*topScorePlayers[0]->GetPlayerName());
			}
			else if (topScorePlayers.Num() > 1)
			{
				infoText = FString(TEXT("Tie! The Winners are:\n"));

				for (AMainPlayerState* playerState : topScorePlayers)
				{
					infoText.Append(FString::Printf(TEXT("%s\n"), *playerState->GetPlayerName()));
				}
			}
		}

		return infoText;
	}

	return FString();

}










void AMainPlayerController::PollToSetStoredHUDValues()
{
	if (!IsLocalController()) { return; }

	if (HUDStoredRequests && IsHUD_Usable())
	{
		if (HUDStoredRequests.bAmmoChanged && HUDStoredRequests.bCarriedAmmoChanged)
		{
			SetHUDAmmo(HUDStoredRequests.ammo, HUDStoredRequests.carriedAmmo);
			HUDStoredRequests.bAmmoChanged = false;
			HUDStoredRequests.bCarriedAmmoChanged = false;
		}

		if (HUDStoredRequests.bAmmoChanged)
		{
			SetHUDAmmo(HUDStoredRequests.ammo);
			HUDStoredRequests.bAmmoChanged = false;
		}

		if (HUDStoredRequests.bCarriedAmmoChanged)
		{
			SetHUDCarriedAmmo(HUDStoredRequests.carriedAmmo);
			HUDStoredRequests.bCarriedAmmoChanged = false;
		}

		if (HUDStoredRequests.bDeathsChanged)
		{
			SetHUDDeaths(HUDStoredRequests.deaths);
			HUDStoredRequests.bDeathsChanged = false;
		}

		if (HUDStoredRequests.bGrenadesChanged)
		{
			SetHUDGrenades(HUDStoredRequests.grenades);
			HUDStoredRequests.bGrenadesChanged = false;
		}

		if (HUDStoredRequests.bHealthChanged)
		{
			SetHUDHealth(HUDStoredRequests.health, HUDStoredRequests.maxHealth);
			HUDStoredRequests.bHealthChanged = false;
		}

		if (HUDStoredRequests.bScoreChanged)
		{
			SetHUDScore(HUDStoredRequests.score);
			HUDStoredRequests.bScoreChanged = false;
		}

		if (HUDStoredRequests.bShieldChanged)
		{
			SetHUDShield(HUDStoredRequests.shield, HUDStoredRequests.maxShield);
			HUDStoredRequests.bShieldChanged = false;
		}
	}

	if (IsHUDInGameMessagesUsable() && !bInitialPollHappened)
	{
		mainHUD->AddInGameMessagesWidget();
		bInitialPollHappened = true;
	}
}

void AMainPlayerController::PollPlayerState()
{
	if (!HasAuthority()) return;

	if (bShouldAnnouncePlayerState && !bInitialPlayerStatePollHappened && PlayerState)
	{
		bInitialPlayerStatePollHappened = true;
		AMainGameMode* gameMode = GetWorld()->GetAuthGameMode<AMainGameMode>();
		if (gameMode)
		{
			gameMode->BroadcastPlayerJoined(PlayerState);
		}
	}
}

void AMainPlayerController::PawnLeavingGame()
{
	if (!HasAuthority()) { return; }

	APlayerCharacter* playerChar = Cast<APlayerCharacter>(GetPawn());

	if (playerChar && !playerChar->HasLeftGame())
	{
		playerChar->ServerPlayerEliminated(true);
	}

	APlayerController::PawnLeavingGame();
}













void AMainPlayerController::UpdateNonFrequentHUD
(const float health,
	const float maxHealth,
	const int32 score, const int32 deaths,
	const int32 ammo, const int32 carriedAmmo, const int32 grenades)
{
	SetHUDHealth(health, maxHealth);
	SetHUDScore(score);
	SetHUDDeaths(deaths);
	SetHUDAmmo(ammo, carriedAmmo);
	SetHUDGrenades(grenades);
}






void AMainPlayerController::ServerRequestTime_Implementation(const float timeOfClientRequest)
{
	if (!GetWorld()) { return; }

	float serverTimeOfReceipt{ GetWorldTimeFloat() };
	ClientReportServerTime(timeOfClientRequest, serverTimeOfReceipt);
}

void AMainPlayerController::ClientReportServerTime_Implementation
(const float timeOfClientRequest, const float timeOfServerReceivedRequest)
{
	if (!GetWorld()) { return; }

	const float currentClientTime{ GetWorldTimeFloat() };

	// Time it took for the message to arrive back to us from the server
	const float roundTripTime{ currentClientTime - timeOfClientRequest };

	// The sending time should not be calculated, that's why
	// we divide by 2, however this is just an estimation
	const float currentServerTime{ timeOfServerReceivedRequest - (roundTripTime / 2.f) };
	singleTripTime = (roundTripTime / 2.f);

	clientServerDelta = currentServerTime - currentClientTime;
}



float AMainPlayerController::GetWorldTimeFloat()
{
	return GetWorld() ? (float)GetWorld()->TimeSeconds : 0.0f;
}


void AMainPlayerController::SetMouseSensitivity(const float& newSens)
{
	mouseSensitivity = newSens;

	APlayerCharacter* playerChar = Cast<APlayerCharacter>(GetPawn());
	if (playerChar)
	{
		playerChar->SetMouseSensitivity(newSens);
	}
}

AController* AMainPlayerController::GetLastDamagingInstigatorController()
{
	APlayerCharacter* playerChar = Cast<APlayerCharacter>(GetPawn());

	if (playerChar)
	{
		return playerChar->GetLastInstigatorController();
	}

	return nullptr;
}

float AMainPlayerController::GetServerTime()
{
	if (!GetWorld()) { return 0.0f; }
	if (HasAuthority()) { return GetWorld()->TimeSeconds; }
	return GetWorld()->TimeSeconds + clientServerDelta;
}







void AMainPlayerController::SetHUDTimers()
{
	//if (GEngine) GEngine->AddOnScreenDebugMessage(1, 1.f, FColor::Blue,
		//FString::Printf(TEXT("Controller time: %.2f, starting time: %.2f"), GetServerTime(), levelStartingTime));


	// this means controller got created earlier than game mode?
	

	if (currentMatchState == MatchState::InProgress)
	{
		SetHUDMainTimer(warmupTime + matchTime - GetServerTime() + levelStartingTime);
	}
	else if (currentMatchState == MatchState::WaitingToStart)
	{
		SetHUDAnnouncementTimer(warmupTime - GetServerTime() + levelStartingTime);
	}
	else if (currentMatchState == MatchState::Cooldown)
	{
		SetHUDAnnouncementTimer(warmupTime + matchTime + cooldownTime - GetServerTime() + levelStartingTime);
	}
}


void AMainPlayerController::SetHUDMainTimer(const float& countdownTimer)
{
	if (!IsHUD_Usable()) { return; }

	// avoid updating UI very frequently
	thisTimeOnHUDseconds = FMath::RoundToInt(countdownTimer);
	if (thisTimeOnHUDseconds == lastTimeOnHUDseconds) { return; }
	lastTimeOnHUDseconds = thisTimeOnHUDseconds;
	mainHUD->SetTimer(countdownTimer);

	
}

void AMainPlayerController::SetHUDAnnouncementTimer(const float& countdownTimer)
{
	if (!mainHUD) { mainHUD = Cast<AMainHUD>(GetHUD()); }
	if (!mainHUD) { return; }

	// avoid updating UI very frequently
	thisTimeOnHUDseconds = FMath::RoundToInt(countdownTimer);
	if (thisTimeOnHUDseconds == lastTimeOnHUDseconds) { return; }
	lastTimeOnHUDseconds = thisTimeOnHUDseconds;

	mainHUD->SetAnnouncementTimer(countdownTimer);
}




void AMainPlayerController::SetHUDHealth(float health, float maxHealth)
{
	if (IsHUD_Usable())
	{
		mainHUD->SetHealth(health, maxHealth);
	}
	else
	{
		HUDStoredRequests.bHealthChanged = true;
		HUDStoredRequests.health = health;
		HUDStoredRequests.maxHealth = maxHealth;
	}
}

void AMainPlayerController::SetHUDHealth()
{
	if (APlayerCharacter* playerCharacter = Cast<APlayerCharacter>(GetPawn()))
	{
		SetHUDHealth(playerCharacter->GetCurrentHealth(), playerCharacter->GetMaxHealth());
	}
}

void AMainPlayerController::SetHUDScore(float score)
{
	if (IsHUD_Usable())
	{
		mainHUD->SetScore(score);
	}
	else
	{
		HUDStoredRequests.bScoreChanged = true;
		HUDStoredRequests.score = score;
	}

}

void AMainPlayerController::SetHUDDeaths(const int32& deaths)
{
	if (IsHUD_Usable())
	{
		mainHUD->SetDeaths(deaths);
	}
	else
	{
		HUDStoredRequests.bDeathsChanged = true;
		HUDStoredRequests.deaths = deaths;
	}
}

void AMainPlayerController::SetHUDGrenades(const int32& grenades)
{
	if (IsHUD_Usable())
	{
		mainHUD->SetGrenades(grenades);
	}
	else
	{
		HUDStoredRequests.bGrenadesChanged = true;
		HUDStoredRequests.grenades = grenades;
	}

}

void AMainPlayerController::SetHUDCrosshairDrawLocation(const FVector2D& location)
{
	if (mainHUD)
	{
		mainHUD->SetCrosshairPosition(location);
	}
}

void AMainPlayerController::SetHUDShield(const float& shield, const float& maxShield)
{
	if (IsHUD_Usable())
	{
		mainHUD->SetShield(shield, maxShield);
	}
	else
	{
		HUDStoredRequests.bShieldChanged = true;
		HUDStoredRequests.shield = shield;
		HUDStoredRequests.maxShield = maxShield;
	}


}

void AMainPlayerController::SetHUDShield()
{
	if (APlayerCharacter* playerCharacter = Cast<APlayerCharacter>(GetPawn()))
	{
		SetHUDShield(playerCharacter->GetCurrentShield(), playerCharacter->GetMaxShield());
	}
}

void AMainPlayerController::SetHUDPlayerInCrosshairName(const FString& playerName, const FColor& displayColor)
{
	if (IsHUD_Usable())
	{
		mainHUD->SetPlayerInCrosshairName(playerName, displayColor);
	}

}

void AMainPlayerController::GetSeamlessTravelActorList(bool bToTransition, TArray<AActor*>& ActorList)
{
	if (bToTransition)
	{
		if (mainHUD && IsLocalController())
		{
			mainHUD->Destroy();
		}
	}
	
	//ActorList.Empty();
	Super::GetSeamlessTravelActorList(bToTransition, ActorList);
}





void AMainPlayerController::ClientHUD_InGameMessage_PlayerJoinedGame_Implementation(const APlayerState* joining)
{
	if (joining && IsHUDInGameMessagesUsable())
	{
		mainHUD->InGameMessage_PlayerJoinedGame(joining->GetPlayerName());
	}
}


void AMainPlayerController::ClientHUD_InGameMessage_PlayerLeftGame_Implementation(const APlayerState* exiting)
{
	if (exiting && IsHUDInGameMessagesUsable())
	{
		mainHUD->InGameMessage_PlayerLeftGame(exiting->GetPlayerName());
	}
}


void AMainPlayerController::ClientHUD_InGameMessage_PlayerElimination_Implementation
(APlayerState* attacker, APlayerState* victim,
	AActor* damageCauser, bool bHeadshot)
{
	if (!victim) { return; }

	if (IsHUDInGameMessagesUsable())
	{
		FString attackerName{};
		FString victimName{};
		FString tool{};
		FColor displayColor{};

		ConfigureEliminationMessage
		(attackerName, victimName, tool, displayColor,  victim, attacker, damageCauser);

		mainHUD->InGameMessage_PlayerElimination(attackerName, victimName, tool, bHeadshot, displayColor);
	}

}

void AMainPlayerController::ClientHUD_InGameMessage_TeamGainedLead_Implementation(const ETeams& team)
{
	if (IsHUDInGameMessagesUsable())
	{
		AMainPlayerState* mPlayerState = GetPlayerState<AMainPlayerState>();

		if (mPlayerState)
		{
			FString teamText{};
			if (team == ETeams::ET_BlueTeam)
			{
				teamText = TEXT("Blue Team");
			}
			else if(team == ETeams::ET_RedTeam)
			{
				teamText = TEXT("Red Team");
			}

			const bool sameTeamAsSelf = mPlayerState->GetTeam() == team;
			
			FColor displayColor = sameTeamAsSelf ? messageColor_GoodForThisPlayer : messageColor_DirectBadForThisPlayer;
			mainHUD->InGameMessage_TeamGainedLead(teamText, displayColor);
		}
	}


}


#define FS(text_) FString(TEXT(text_))
#define FSF(text_, variable) FString::Printf(TEXT(text_), *variable)

void AMainPlayerController::ClientHUD_InGameMessage_PlayerGainedLead_Implementation(const APlayerState* gainedLeadPlayer, bool bTied)
{
	if (!gainedLeadPlayer) { return; }

	if (IsHUDInGameMessagesUsable())
	{
		FString leadName{};
		FColor displayColor{};
		bool bIsSelf{};
		ConfigureGainedLeadMessage(leadName, displayColor, bIsSelf, gainedLeadPlayer);

		mainHUD->InGameMessage_PlayerGainedLead(leadName, bIsSelf, bTied, displayColor);
	}
}

void AMainPlayerController::ConfigureGainedLeadMessage
(FString& outName, FColor& outColor, bool& bIsSelf, const APlayerState* gainedLeadPlayer)
{
	if (gainedLeadPlayer)
	{
		const bool gainedLeadIsSelf = PlayerState == gainedLeadPlayer;
		bIsSelf = gainedLeadIsSelf;
		if (gainedLeadIsSelf)
		{
			outColor = messageColor_GoodForThisPlayer;
			outName = FString{ TEXT("You") };
		}
		else
		{
			outColor = messageColor_DirectBadForThisPlayer;
			outName = gainedLeadPlayer->GetPlayerName();
		}

	}
	else
	{
		outName = TEXT("");
		outColor = FColor::Transparent;
	}


}






void AMainPlayerController::ConfigureEliminationMessage
(FString& outAttackerName, FString& outVictimName, FString& outToolName, FColor& outDisplayColor,
 APlayerState* victim, APlayerState* attacker, AActor* damageCauser)
{
	if (!victim) { return; }

	APlayerState* self = GetPlayerState<APlayerState>();
	if (!self) { return; }
	bool bAttackerIsSelf = self == attacker;
	bool bVictimIsSelf = self == victim;

	if (bAttackerIsSelf && bVictimIsSelf)
	{
		outAttackerName = FS("You");
		outVictimName = FS("Yourself");
		outDisplayColor = messageColor_DirectBadForThisPlayer;
	}
	else if (bAttackerIsSelf && !bVictimIsSelf)
	{
		outAttackerName = FS("You");
		outVictimName = FSF("%s", victim->GetPlayerName());
		outDisplayColor = messageColor_GoodForThisPlayer;
	}
	else if (!bAttackerIsSelf && bVictimIsSelf)
	{
		outAttackerName = attacker ? FSF("%s", attacker->GetPlayerName()) : FS("The Environment");
		outVictimName = FS("You");
		outDisplayColor = messageColor_DirectBadForThisPlayer;
	}
	else if (!bAttackerIsSelf && !bVictimIsSelf)
	{
		outAttackerName = attacker ? FSF("%s", attacker->GetPlayerName()) : FS("The Environment");
		outVictimName = (attacker == victim) ? FS("Themselves") : FSF("%s", victim->GetPlayerName());

		if (bTeamMatch)
		{
			ETeams thisPlayerTeam{};
			if (AMainPlayerState* mainPlayerState = GetPlayerState<AMainPlayerState>())
			{
				thisPlayerTeam = mainPlayerState->GetTeam();
			}

			ETeams victimPlayerTeam{};
			if (AMainPlayerState* victimMainPlayerState = Cast<AMainPlayerState>(victim))
			{
				victimPlayerTeam = victimMainPlayerState->GetTeam();
			}

		//	GEPR_TWO_VARIABLE("This Player Team: %d, Victim Player Team: %d", (uint8)thisPlayerTeam, (uint8)victimPlayerTeam);

			if (thisPlayerTeam == ETeams::ET_NoTeam || victimPlayerTeam == ETeams::ET_NoTeam)
			{
				outDisplayColor = messageColor_IndirectBadForThisPlayer;
			}
			else
			{
				if (thisPlayerTeam == victimPlayerTeam)
				{
					outDisplayColor = messageColor_DirectBadForThisPlayer;
				}
				else
				{
					outDisplayColor = messageColor_GoodForThisPlayer;
				}
			}
			
		}
		else
		{
			outDisplayColor = messageColor_IndirectBadForThisPlayer;
		}
	}

	if (damageCauser)
	{
		if (IInstigatorTypeInterface* instigatorType = Cast<IInstigatorTypeInterface>(damageCauser))
		{
			outToolName = attacker? instigatorType->GetInstigatorTypeName().ToString() : TEXT("");
		}
	}
	else
	{
		outToolName = attacker ? TEXT("Environment") : TEXT("");
	}

}




void AMainPlayerController::ClientHUD_InGameMessage_ServerAnnouncement_Implementation
(const FString& announcement, const FColor& announcementColor)
{
	if (IsHUDInGameMessagesUsable())
	{
		mainHUD->InGameMessage_ServerMessage(announcement, announcementColor);
	}
}




void AMainPlayerController::ClientHUDPlayHitEffect_Implementation(bool bHitShield, bool bWillKill, const float& actualDamageDone)
{
	if (IsHUD_Usable())
	{
		mainHUD->PlayHitEffect(bHitShield, bWillKill, actualDamageDone);
	}
}


void AMainPlayerController::ClientHUDPlayVictimHitEffect_Implementation(const AActor* instigActor)
{
	if (IsHUD_Usable())
	{
		EHitDirection hitDir = GetHitDirection(instigActor);
		mainHUD->PlayVictimHitEffect(hitDir);
	}
}

void AMainPlayerController::ClientMatchIsOver_Implementation(const TArray<class AMainPlayerState*>& winners)
{
	if (winners.Contains(GetPlayerState<AMainPlayerState>()))
	{
		if (gameWonSound)
		{
			UGameplayStatics::PlaySound2D(this, gameWonSound);
		}
	}
	else
	{
		if (gameLostSound)
		{
			UGameplayStatics::PlaySound2D(this, gameLostSound);
		}
	}
}

EHitDirection AMainPlayerController::GetHitDirection(const AActor* instigActor)
{
	APawn* pawn = GetPawn();
	if (!pawn) { return EHitDirection::EHD_Unkown; }
	AController* pawnController = pawn->GetController();
	if (!pawnController) { return EHitDirection::EHD_Unkown; }

	EHitDirection hitDir = EHitDirection::EHD_Unkown;

	if (instigActor)
	{
		FVector instigatorLoc = instigActor->GetActorLocation();
		instigatorLoc.Z = 0;
		FVector selfLoc = pawn->GetActorLocation();
		selfLoc.Z = 0;



		FVector instigatorToSelfDirection = (instigatorLoc - selfLoc).GetSafeNormal();
		instigatorToSelfDirection.Z = 0;

		FVector controllerForwardDirection = FRotationMatrix(pawnController->GetControlRotation()).GetUnitAxis(EAxis::X);
		controllerForwardDirection.Z = 0;

		const float dotProd = FVector::DotProduct(controllerForwardDirection, instigatorToSelfDirection);

		if (dotProd >= 0.75f)
		{
			return hitDir = EHitDirection::EHD_Front;
		}
		else if (dotProd <= -0.75f)
		{
			return hitDir = EHitDirection::EHD_Back;
		}

		const FVector crossProd = FVector::CrossProduct(controllerForwardDirection, instigatorToSelfDirection).GetSafeNormal();

		if (crossProd.Z < 0.f)
		{
			return hitDir = EHitDirection::EHD_Left;
		}
		else
		{
			return hitDir = EHitDirection::EHD_Right;
		}
	}
	else
	{
		return hitDir = EHitDirection::EHD_Back;
	}

}

void AMainPlayerController::HighPingWarning()
{
	if (IsHUD_Usable())
	{
		mainHUD->HighPingWarning();
	}
}

void AMainPlayerController::StopHighPingWarning()
{
	if (IsHUD_Usable())
	{
		mainHUD->StopHighPingWarning();
	}
}





void AMainPlayerController::SetHUDAmmo(const int32& ammo)
{
	if (IsHUD_Usable())
	{
		mainHUD->SetAmmo(ammo);
	}
	else
	{
		HUDStoredRequests.bAmmoChanged = true;
		HUDStoredRequests.ammo = ammo;
	}
}

void AMainPlayerController::SetHUDAmmo(const int32& ammo, const int32& carriedAmmo)
{
	if (IsHUD_Usable())
	{
		mainHUD->SetAmmo(ammo, carriedAmmo);
	}
	else
	{
		HUDStoredRequests.bAmmoChanged = true;
		HUDStoredRequests.bCarriedAmmoChanged = true;
		HUDStoredRequests.ammo = ammo;
		HUDStoredRequests.carriedAmmo = carriedAmmo;
	}
}

void AMainPlayerController::SetHUDCarriedAmmo(const int32& ammo)
{
	if (IsHUD_Usable())
	{
		mainHUD->SetCarriedAmmo(ammo);
	}
	else
	{
		HUDStoredRequests.bCarriedAmmoChanged = true;
		HUDStoredRequests.carriedAmmo = ammo;
	}

}


bool AMainPlayerController::IsHUD_Usable()
{
	if (!IsLocalController()) { return false; }

	if (!mainHUD) { mainHUD = Cast<AMainHUD>(GetHUD()); }
	if (!mainHUD) { return false; }
	if (!mainHUD->IsOverlayWidgetCreated()) mainHUD->AddOverlayWidget(false);
	if (!mainHUD->IsOverlayWidgetCreated()) { return false; }

	return true;
}

bool AMainPlayerController::IsHUDInGameMessagesUsable()
{
	if (!IsLocalController()) { return false; }

	if (mainHUD)
	{
		// we already tried to get, so if we mainHUD is okay, try to initialize
		if(!mainHUD->IsInGameMessagesWidgetCreated())
		{
			mainHUD->AddInGameMessagesWidget();
		}
	}

	return mainHUD && mainHUD->IsInGameMessagesWidgetCreated();
}






void AMainPlayerController::ShowReturnToMainMenu()
{
	if (!IsLocalController()) { return; }

	if (!returnToMainMenuWidgetClass) { return; }
	if (!returnToMainMenuWidget)
	{
		returnToMainMenuWidget = CreateWidget<UReturnToMainMenuWidget>(this, returnToMainMenuWidgetClass);
	}

	if (returnToMainMenuWidget)
	{
		bReturnToMainMenuIsOpen = !bReturnToMainMenuIsOpen;

		if (bReturnToMainMenuIsOpen)
		{
			returnToMainMenuWidget->MenuSetup();
		}
		else
		{
			returnToMainMenuWidget->MenuTeardown();
		}
	}

}


void AMainPlayerController::HideTeamScores()
{
	if (IsHUD_Usable())
	{
		mainHUD->HideTeamScore();
	}
}

void AMainPlayerController::InitTeamScores()
{
	if (IsHUD_Usable())
	{
		mainHUD->InitializeTeamScores();
	}
}

void AMainPlayerController::SetHUDBlueTeamScore(const float& score)
{
	if (IsHUD_Usable())
	{
		mainHUD->SetBlueTeamScore(score);
	}
}

void AMainPlayerController::SetHUDRedTeamScore(const float& score)
{
	if (IsHUD_Usable())
	{
		mainHUD->SetRedTeamScore(score);
	}
}



void AMainPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent) { return; }

	InputComponent->BindAction(FName(TEXT("Menu")), EInputEvent::IE_Pressed, this, &ThisClass::ShowReturnToMainMenu);
	InputComponent->BindAction(FName(TEXT("PlayerBoard")), EInputEvent::IE_Pressed, this, &ThisClass::ShowPlayerBoard);
	InputComponent->BindAction(FName(TEXT("PlayerBoard")), EInputEvent::IE_Released, this, &ThisClass::HidePlayerBoard);

}