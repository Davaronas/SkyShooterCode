// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "HelperTypes/HitDirection.h"
#include "HelperTypes/Teams.h"

#include "MainPlayerController.generated.h"

#define HIDE_AMMO_AMOUNT -99




USTRUCT(BlueprintType)
struct FHUDDelayInitialize
{
	GENERATED_BODY()


	bool bHealthChanged{ false };
	float health{};
	float maxHealth{};

	bool bScoreChanged{ false };
	int32 score{};

	bool bDeathsChanged{ false };
	int32 deaths{};

	bool bAmmoChanged{ false };
	int32 ammo{ HIDE_AMMO_AMOUNT };

	bool bCarriedAmmoChanged{ false };
	int32 carriedAmmo{};

	bool bGrenadesChanged{ false };
	int32 grenades{};

	bool bShieldChanged{ false };
	float shield{};
	float maxShield{};


	operator bool() const
	{
		return bHealthChanged || bScoreChanged || bDeathsChanged || bAmmoChanged || bCarriedAmmoChanged ||
			bGrenadesChanged || bShieldChanged;
	}

};

/**
 * 
 */
UCLASS()
class MUTLIPLAYERSHOOTER_API AMainPlayerController : public APlayerController
{
	GENERATED_BODY()


public:

	virtual void SetupInputComponent() override;

	void SetHUDHealth(float health, float maxHealth);
	void SetHUDHealth();
	void SetHUDScore(float score);
	void SetHUDDeaths(const int32& deaths);
	void SetHUDAmmo(const int32& ammo);
	void SetHUDAmmo(const int32& ammo, const int32& carriedAmmo);
	void SetHUDCarriedAmmo(const int32& ammo);
	void SetHUDTimers();
	void SetHUDGrenades(const int32& grenades);
	void SetHUDCrosshairDrawLocation(const FVector2D& location);
	void SetHUDShield(const float& shield, const float& maxShield);
	void SetHUDShield();
	void SetHUDPlayerInCrosshairName(const FString& playerName, const FColor& displayColor);

	virtual void GetSeamlessTravelActorList(bool bToTransition, TArray< AActor* >& ActorList) override;


	void HideTeamScores();
	void InitTeamScores();

	void SetHUDBlueTeamScore(const float& score);
	void SetHUDRedTeamScore(const float& score);

	/*
	* InGameMessages
	*/

	UFUNCTION(Client, Reliable)
	void ClientHUD_InGameMessage_PlayerElimination
	(class APlayerState* attacker, class APlayerState* victim,
		AActor* damageCauser, bool bHeadshot);

	UFUNCTION(Client, Reliable)
	void ClientHUD_InGameMessage_PlayerLeftGame(const class APlayerState* exiting);

	UFUNCTION(Client, Reliable)
	void ClientHUD_InGameMessage_PlayerJoinedGame(const class APlayerState* joining);

	UFUNCTION(Client, Reliable)
	void ClientHUD_InGameMessage_PlayerGainedLead(const class APlayerState* gainedLeadPlayer, bool bTied);

	UFUNCTION(Client, Reliable)
	void ClientHUD_InGameMessage_TeamGainedLead(const ETeams& team);

	UFUNCTION(Client, Reliable)
	void ClientHUD_InGameMessage_ServerAnnouncement(const FString& announcement,  const FColor& announcementColor);

	/*
	* 
	*/


	void ConfigureEliminationMessage
	(FString& outAttackerName, FString& outVictimName, FString& outToolName, FColor& outDisplayColor,
		class APlayerState* victim, class APlayerState* attacker, class AActor* damageCauser);

	void ConfigureGainedLeadMessage(FString& outName, FColor& outColor, bool& bIsSelf, const class APlayerState* gainedLeadPlayer);
	
	UFUNCTION(Client, Reliable)
	void ClientHUDPlayHitEffect(bool bHitShield, bool bWillKill, const float& actualDamageDone);

	UFUNCTION(Client, Reliable)
	void ClientHUDPlayVictimHitEffect(const class AActor* instigActor);

	EHitDirection GetHitDirection(const AActor* instigActor);

	void HighPingWarning();
	void StopHighPingWarning();

	UFUNCTION(Client, Reliable)
	void ClientMatchIsOver(const TArray<class AMainPlayerState*>& winners);

	UPROPERTY(EditAnywhere)
	class USoundBase* gameWonSound{};
	UPROPERTY(EditAnywhere)
	class USoundBase* gameLostSound{};

	

	virtual void OnPossess(APawn* InPawn) override;

	// This is the earliest point where the player is associated
	// with the controller, sync server and client time here
	virtual void ReceivedPlayer() override;
	virtual void Tick(float DeltaTime) override;

	void UpdateHUD_Ping();

	void CheckClientTimeSync(float DeltaTime);
	void OnMatchStateSet(FName matchState, bool bTeams = false);


	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Called from HUD begin play
	void OnHudInitialized();

	FORCEINLINE void BroadcastLoginOncePlayerStateIsInitialized() { bShouldAnnouncePlayerState = true; }


protected:
	virtual void BeginPlay() override;


	void ShowPlayerBoard();
	void HidePlayerBoard();



	UPROPERTY(VisibleAnywhere)
	float mouseSensitivity{1.f};




	bool bInitialPollHappened{ false };
	void PollToSetStoredHUDValues();

	bool bShouldAnnouncePlayerState{ false };
	bool bInitialPlayerStatePollHappened{ false };
	void PollPlayerState();

	virtual void PawnLeavingGame() override;
	
	void ShowReturnToMainMenu();
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UUserWidget> returnToMainMenuWidgetClass{};
	UPROPERTY()
	class UReturnToMainMenuWidget* returnToMainMenuWidget{};
	bool bReturnToMainMenuIsOpen{ false };



	void HandleTeamMatch(bool bTeams);



	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_bTeamMatch)
	bool bTeamMatch = false;
	UFUNCTION()
	void OnRep_bTeamMatch();





	// Get world time that is synced with the server


	/*
	*	Sync Time between client and server
	*/

	// Requests for the server time, passing in the time of sending from the client
	UFUNCTION(Server, Reliable)
	void ServerRequestTime(const float timeOfClientRequest);

	// Reports server time to client, passing in both the time of sending the request,
	// and receiving the request
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(const float timeOfClientRequest, const float timeOfServerReceivedRequest);


	FTimerHandle waitForGameModeTimer{};
	UPROPERTY(EditDefaultsOnly)
	float waitForGameModeTime{ 0.15f };
	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();



	

	UFUNCTION(Client, Reliable)
	void ClientReportMatchState
	(const FName stateOfMatch,
		const float warmupTime_, const float matchTime_, const float cooldownTime_,
		const float levelStartingTime_, bool bIsTeamMatch);

	UFUNCTION(Server, Reliable)
	void ServerRequestLevelStartTime();
	UFUNCTION(Client, Reliable)
	void ClientReportLevelStartTime(const float& levelStartTime_);

	UFUNCTION(Client, Reliable)
	void ClientWaitForGameMode();

	void UpdateNonFrequentHUD(const float health = 100,
	const float maxHealth = 100,
	const int32 score = 0,
	const int32 deaths = 0,
	const int32 ammo = 0,
	const int32 carriedAmmo = 0,
	const int32 grenades = 0);


	void HandleCooldownState();
	FString CreateCooldownTitleMessage();
	void HandleWaitingToStartState();
	void HandleInProgressState();

	void SetHUDAnnouncementTimer(const float& countdownTimer);
	void SetHUDMainTimer(const float& countdownTimer);

	void CheckPing(float DeltaTime);

private:
	UPROPERTY()
	class AMainHUD* mainHUD{};

	// difference between client and server time
	float clientServerDelta{ 0.f };

	float warmupTime{};
	float matchTime{};
	float cooldownTime{};

	float levelStartingTime{};

	float singleTripTime{ 0.f };

	UPROPERTY(EditAnywhere, Category = "Time")
	float timeSyncFrequency = 10.f;
	float timeSyncRunningTime{};
	int32 thisTimeOnHUDseconds{};
	int32 lastTimeOnHUDseconds{};
	float GetWorldTimeFloat();

	int32 thisFramePingMs{};
	int32 lastFramePingMs{};

	UPROPERTY(ReplicatedUsing = OnRep_currentMatchState)
	FName currentMatchState{};
	UFUNCTION()
	void OnRep_currentMatchState();

	void HandleMatchStates();

	void WaitForHUDHandleMatchStates();

	bool IsHUD_Usable();
	bool IsHUDInGameMessagesUsable();

	FHUDDelayInitialize HUDStoredRequests{};


	FTimerHandle matchStateHandleWaitForHUDInitializeTimer{};
	float waitForHUDTime{ 0.15f };


	float highPingRunningTime{};
	float highPingWarningRunningTime{};
	UPROPERTY(EditDefaultsOnly, Category = "Ping");
	float highPingWarningDuration{ 2.f };
	UPROPERTY(EditDefaultsOnly, Category = "Ping");
	float checkPingInterval{ 8.f };
	UPROPERTY(EditDefaultsOnly, Category = "Ping");
	float highPingThreshold = 50.f;



	UPROPERTY(EditDefaultsOnly, Category = "MainPlayerController|MessageColors",
		meta = (DisplayName = "Good Message Color"))
	FColor messageColor_GoodForThisPlayer{};


	UPROPERTY(EditDefaultsOnly, Category = "MainPlayerController|MessageColors",
		meta = (DisplayName = "Direct Bad Message Color"))
	FColor messageColor_DirectBadForThisPlayer{};


	UPROPERTY(EditDefaultsOnly, Category = "MainPlayerController|MessageColors",
		meta = (DisplayName = "Indirect Bad Message Color"))
	FColor messageColor_IndirectBadForThisPlayer{};


public:
	FORCEINLINE float GetSingleTripTime() { return singleTripTime; }

	void SetMouseSensitivity(const float& newSens);
	FORCEINLINE float GetMouseSensitivity() { return mouseSensitivity; }


	class AController* GetLastDamagingInstigatorController();

	virtual float GetServerTime();
	
};
