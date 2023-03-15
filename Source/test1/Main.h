// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Main.generated.h"

UENUM(BlueprintType)
enum class EMovementStatus : uint8
{
	EMS_Normal UMETA(DisplayName = "Normal"),
	EMS_Sprinting UMETA(DisplayName = "Sprinting"),
	EMS_Dead      UMETA(DisplayName="Dead"),
	EMS_Dodge     UMETA(DisplayName = "Dodge"),

	EMS_MAX UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum class EStaminaStatus :uint8
{
	ESS_Normal UMETA(DisplayName="Normal"),
	ESS_BelowMinimum UMETA(DisplayName="BelowMinimum"),
	ESS_Exhausted UMETA(DisplayName="Exhausted"),
	ESS_ExhaustedRecovering UMETA(DisplayName="ExhaustedRecovering"),

	ESS_MAX UMETA(DisplayName="DefaultMax")
};



UCLASS()
class TEST1_API AMain : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMain();

	TArray<FVector> PickupLocations;

	UPROPERTY(EditDefaultsOnly,Category="SavedData")
	TSubclassOf<class AItemStorage> WeaponStorage;

	UFUNCTION(BlueprintCallable)
	void ShowPickupLocation();

	

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Controller")
	class AMainPlayerController* MainPlayerController;

	UPROPERTY(VisibleAnywhere,BlueprintReadWrite,Category="Enums")
	EMovementStatus MovementStatus;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Enums")
	EStaminaStatus StaminaStatus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float StaminaDrainRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MinSprintStamina;

	float InterpSpeed;
	bool bInterpToEnemy;
	void SetInterpToEnemy(bool Interp);

	UPROPERTY(VisibleAnywhere,BlueprintReadWrite,Category="Combat")
	class AEnemy* CombatTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat")
	FVector CombatTargetLocation;

	FRotator GetLookAtRotationYaw(FVector Target);

	FORCEINLINE void SetCombatTarget(AEnemy* Target) { CombatTarget = Target; }

	

	FORCEINLINE void SetStaminaStatus(EStaminaStatus Status) { StaminaStatus = Status; }
	/* Set movementstatus and running speed*/
	void SetMovementStatus(EMovementStatus Status);

	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Running")
	float RunningSpeed;

	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Running")
	float SprintingSpeed;

	bool bShiftkeyDown;

	/* Pressed down to enable sprinting*/
	void ShiftkeyDown();

	/* Released to stop sprinting*/
	void ShiftkeyUp();

	/** Camera boom positioning the camera behind the player     */ 
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category=Camera, meta=(AllowPrivateAccess="true"))
	class USpringArmComponent* CameraBoom;

	/* Follower Camera*/
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category=Camera,meta=(AllowPrivateAccess="true"))
	class UCameraComponent* FollowCamera;

	/** Base turn rate to scale turning function for camera*/
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category=Camera)
	float BaseTurnRate;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category=Camera)
	float BaseLookUpRate;

	/*
	*
	*Player Stats
	*
	*
	*/

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Player Stats")
	float MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float Health;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player Stats")
	float MaxStamina;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float Stamina;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	int32 Coins;

	void DecrementHealth(float Amount);

	virtual float TakeDamage(float DamageAmount,struct FDamageEvent const& DamageEvent,class AController* EventInstigator,AActor* DamageCauser) override;

	void Die();

	virtual void Jump() override;

	UFUNCTION(BlueprintCallable)
	void IncrementCoin(int32 C_num);
	UFUNCTION(BlueprintCallable)
	void IncrementHealth(float C_num);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	class UParticleSystem* HitParticles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	class USoundCue* HitSound;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bHasCombatTarget;

	FORCEINLINE void SetHasCombatTarget(bool HasTarget) { bHasCombatTarget = HasTarget; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


	bool CanMove(float Value);
	//Called for forwards/backwards input
	void MoveForward(float Value);
	//Called for side to side input
	void MoveRight(float Value);

	bool bMoveForward;
	bool bMoveRight;


	/*Called via input to turn at a given rate
	*@param Rate this is a normalized rate, 1.0 means 100%
	*/
	void TurnAtRate(float Rate);
	/*Called via input to look up/down at a given rate
	*@param Rate this is a normalized rate, 1.0 means 100% of desired look up/down rate
	*/
	void LookUpAtRate(float Rate);

	//Call for Pitch Rotation
	void LookUp(float Value);
	//Call For Yaw Rotation
	void Turn(float Value);
	bool CanMoveForLT(float Value);

	void LMBDown();
	void LMBUp();
	bool bLMBDown;

	void ESCDown();
	void ESCUp();
	bool bESCDown;

	
	


	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Items)
	class AWeapon* EquippedWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Items)
	class AItem* ActiveOverlappingItem;

	void SetEquippedWeapon(AWeapon* WeaponToSet);
	FORCEINLINE AWeapon* GetEquippedWeapon() { return EquippedWeapon; }
	FORCEINLINE void SetActiveOverlappingItem(AItem* Item) { ActiveOverlappingItem = Item; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anims")
	bool bAttacking;

	void Attack();

	UFUNCTION(BlueprintCallable)
	void AttackEnd();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anims")
	class UAnimMontage* CombatMontage;

	UFUNCTION(BlueprintCallable)
	void PlaySwingSound();

	UFUNCTION(BlueprintCallable)
	void DeathEnd();

	void UpdateCombatTarget();

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Combat")
	TSubclassOf<AEnemy> EnemyFilter;

	void SwitchLevel(FName LevelName);

	UFUNCTION(BlueprintCallable)
	void SaveGame();

	UFUNCTION(BlueprintCallable)
	void LoadGame(bool SetPosition);

	UFUNCTION(BlueprintCallable)
	void LoadGameNoSwitch();

};
