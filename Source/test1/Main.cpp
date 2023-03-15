// Fill out your copyright notice in the Description page of Project Settings.


#include "Main.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Weapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Kismet/KismetMathLibrary.h"
#include "Enemy.h"
#include "MainPlayerController.h"
#include "TestSaveGame.h" 
#include "ItemStorage.h"

// Sets default values
AMain::AMain()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	/* Create CameraBoom */
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 600.f; //Camera follow at this distance
	CameraBoom->bUsePawnControlRotation = true; //Rotate arm based controller

	//GetCapsuleComponent()->InitCapsuleSize();   set size for collision capsule
	GetCapsuleComponent()->SetCapsuleSize(47.f,110.f);

	/* Create Follow Camera*/
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom,USpringArmComponent::SocketName);
	/*Attach the camera to the end of the CameraBoom and let CameraBoom adjust to match the controller orientation*/
	FollowCamera->bUsePawnControlRotation = false;

	//set our turn rates for input
	BaseTurnRate = 65.f;
	BaseLookUpRate = 65.f; 

	/*
	Do not rotate when controller rotats
	Let that just affect the camera
	*/
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	//configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;//Character moves in the direction of input
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);//...at this rotation rate

	GetCharacterMovement()->JumpZVelocity = 650.f;
	GetCharacterMovement()->AirControl = 0.1f;

	MaxHealth = 100.f;
	Health = 65.f;
	MaxStamina = 150.f;
	Stamina = 120.f;
	Coins = 0;

	RunningSpeed = 650.f;
	SprintingSpeed = 950.f;

	bShiftkeyDown = false;
	bLMBDown = false;
	bESCDown = false;

	

	//Initialize Enums
	MovementStatus = EMovementStatus::EMS_Normal;
	StaminaStatus = EStaminaStatus::ESS_Normal;

	StaminaDrainRate = 25.f;
	MinSprintStamina = 50.f;

	InterpSpeed = 15.f;
	bInterpToEnemy = false;
	bHasCombatTarget = false;

	bMoveForward = false;
	bMoveRight = false;
}

// Called when the game starts or when spawned
void AMain::BeginPlay()
{
	Super::BeginPlay();
	
	MainPlayerController = Cast<AMainPlayerController>(GetController());

	FString MapName = GetWorld()->GetMapName();
	MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

	if (MapName != "SunTemple")
	{
		LoadGameNoSwitch();
		
		
	}
	if (MainPlayerController)
	{
		MainPlayerController->GameModeOnly();
	}

	
}

// Called every frame
void AMain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MovementStatus == EMovementStatus::EMS_Dead) return;

	float DeltaStamina = StaminaDrainRate * DeltaTime;

	//control StaminaBar 
	switch (StaminaStatus)
	{
	case EStaminaStatus::ESS_Normal:
		if (bShiftkeyDown&& (bMoveForward || bMoveRight))
		{
			if (Stamina - DeltaStamina <= MinSprintStamina)
			{
				SetStaminaStatus(EStaminaStatus::ESS_BelowMinimum);
				Stamina -= DeltaStamina;
			}
			else
			{
				Stamina -= DeltaStamina;
			}
			
			SetMovementStatus(EMovementStatus::EMS_Sprinting);
			
		}
		else//Shift key up
		{
			if (Stamina + DeltaStamina >= MaxStamina)
			{
				Stamina = MaxStamina;
			}
			else
			{
				Stamina += DeltaStamina;
			}
			SetMovementStatus(EMovementStatus::EMS_Normal);
		}
		break;
	case EStaminaStatus::ESS_BelowMinimum:
		if (bShiftkeyDown && (bMoveForward || bMoveRight))
		{
			if (Stamina - DeltaStamina<=0.f)
			{
				SetStaminaStatus(EStaminaStatus::ESS_Exhausted);
				Stamina = 0;
				SetMovementStatus(EMovementStatus::EMS_Normal);
			}
			else
			{
				Stamina -= DeltaStamina;
	
				SetMovementStatus(EMovementStatus::EMS_Sprinting);
			}
		}
		else//Shift key up
		{
			if (Stamina + DeltaStamina >= MinSprintStamina)
			{
				SetStaminaStatus(EStaminaStatus::ESS_Normal);
				Stamina += DeltaStamina;
			}
			else
			{
				Stamina += DeltaStamina;
			}
			SetMovementStatus(EMovementStatus::EMS_Normal);
		}
		break;
	case EStaminaStatus::ESS_Exhausted:
		if (bShiftkeyDown && (bMoveForward || bMoveRight))
		{
			Stamina = 0.f;

		}
		else//Shift key up
		{
			SetStaminaStatus(EStaminaStatus::ESS_ExhaustedRecovering);
			Stamina += DeltaStamina;
		}
		SetMovementStatus(EMovementStatus::EMS_Normal);
		break;
	case EStaminaStatus::ESS_ExhaustedRecovering:
		//Now that you have been Exhauted, you can't sprint again until you go back to normal
		if(Stamina+DeltaStamina>=MinSprintStamina)
		{
			Stamina += DeltaStamina;
			SetStaminaStatus(EStaminaStatus::ESS_Normal);
			
		}
		else
		{
			Stamina += DeltaStamina;
		}
		SetMovementStatus(EMovementStatus::EMS_Normal);
		break;
	default:
		break;
	}

	if (bInterpToEnemy && CombatTarget)
	{
		FRotator LookAtYaw = GetLookAtRotationYaw(CombatTarget->GetActorLocation());
		FRotator InterpRotation = FMath::RInterpTo(GetActorRotation(), LookAtYaw, DeltaTime, InterpSpeed);
		SetActorRotation(InterpRotation);
	}

	if (CombatTarget)
	{
		CombatTargetLocation = CombatTarget->GetActorLocation();
		if (MainPlayerController)
		{
			MainPlayerController->EnemyLocation = CombatTargetLocation;
		}
	}
}

FRotator AMain::GetLookAtRotationYaw(FVector Target)
{
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Target);
	FRotator LookAtRotationYaw(0.f, LookAtRotation.Yaw, 0.f);
	return LookAtRotationYaw;
}

// Called to bind functionality to input
void AMain::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMain::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AMain::ShiftkeyDown);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AMain::ShiftkeyUp); 

	PlayerInputComponent->BindAction("ESC", IE_Pressed, this, &AMain::ESCDown);
	PlayerInputComponent->BindAction("ESC", IE_Released, this, &AMain::ESCUp);


	PlayerInputComponent->BindAction("LMB", IE_Pressed, this, &AMain::LMBDown);
	PlayerInputComponent->BindAction("LMB", IE_Released, this, &AMain::LMBUp);

	PlayerInputComponent->BindAxis("MoveForward", this, &AMain::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMain::MoveRight);
	
	PlayerInputComponent->BindAxis("Turn", this, &AMain::Turn);// &AMain::TurnAtRate can display it
	PlayerInputComponent->BindAxis("LookUp", this, &AMain::LookUp);//&AMain::LookUpAtRate
	PlayerInputComponent->BindAxis("TurnRate", this, &AMain::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AMain::LookUpAtRate);
}
bool AMain::CanMove(float Value)
{
	if (MainPlayerController)
	{
		
		return (Value != 0.f) && 
			(!bAttacking) &&
			(MovementStatus != EMovementStatus::EMS_Dead) &&
			!MainPlayerController->bPauseMenuVisible;
	}
	else
	{
		return false;
	}
}
bool AMain::CanMoveForLT(float Value)
{
	if (MainPlayerController)
	{

		return (Value != 0.f) &&
			(MovementStatus != EMovementStatus::EMS_Dead) &&
			!MainPlayerController->bPauseMenuVisible;
	}
	else
	{
		return false;
	}
}
void AMain::LookUp(float Value)
{
	if (CanMoveForLT(Value))
	{
		AddControllerPitchInput(Value);
	}
}

void AMain::Turn(float Value)
{
	if (CanMoveForLT(Value))
	{
		AddControllerYawInput(Value);
	}
}
void AMain::MoveForward(float Value)
{

	bMoveForward = false;
	if (CanMove(Value))
	{
		//find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
		
		bMoveForward = true;
	}
}

void AMain::MoveRight(float Value) 
{
	bMoveRight = false;
	if (CanMove(Value))
	{
		//find out which way is side
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);

		bMoveRight = true;
	}
}
void AMain::TurnAtRate(float Rate) {
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AMain::LookUpAtRate(float Rate) {
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AMain::LMBDown()
{
	bLMBDown = true;

	if (MovementStatus == EMovementStatus::EMS_Dead) return;

	if (MainPlayerController) if (MainPlayerController->bPauseMenuVisible) return;

	if (ActiveOverlappingItem)
	{
		AWeapon* Weapon = Cast<AWeapon>(ActiveOverlappingItem);
		if (Weapon)
		{
			Weapon->Equip(this);
			SetActiveOverlappingItem(nullptr);
		}
	}
	else if(EquippedWeapon)
	{
		Attack();
	}
}

void AMain::LMBUp()
{
	bLMBDown = false;
}

void AMain::ESCDown()
{
	bESCDown = true;

	if (MainPlayerController)
	{
		MainPlayerController->TogglePauseMenu();
	}
}
void AMain::ESCUp()
{
	bESCDown = false;
}




void AMain::DecrementHealth(float Amount)
{
	if (Health - Amount <= 0.f)
	{
		Health = 0;
		Die();
	}
	else
		Health -= Amount;
}

void AMain::Die()
{
	if (MovementStatus == EMovementStatus::EMS_Dead) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && CombatMontage)
	{
		AnimInstance->Montage_Play(CombatMontage, 1.0f);
		AnimInstance->Montage_JumpToSection(FName("Death"));
	}
	SetMovementStatus(EMovementStatus::EMS_Dead);
}

void AMain::Jump()
{
	if (MainPlayerController) if (MainPlayerController->bPauseMenuVisible) return;
	if (MovementStatus != EMovementStatus::EMS_Dead)
	{
		Super::Jump();

	}
}

void AMain::DeathEnd()
{
	GetMesh()->bPauseAnims = true;
	GetMesh()->bNoSkeletonUpdate = true;

}
void AMain::IncrementCoin(int32 C_num)
{
	Coins += C_num;
}
void AMain::IncrementHealth(float C_num)
{
	if (Health + C_num > MaxHealth)
	{
		Health = MaxHealth;
	}
	else
	{
		Health += C_num;
	}
}

void AMain::SetMovementStatus(EMovementStatus Status)
{
	MovementStatus = Status;
	if (MovementStatus == EMovementStatus::EMS_Sprinting)
	{
		GetCharacterMovement()->MaxWalkSpeed = SprintingSpeed;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = RunningSpeed;
	}
}

void AMain::ShiftkeyDown()
{
	bShiftkeyDown = true;
}

void AMain::ShiftkeyUp()
{
	bShiftkeyDown = false;
}
void AMain::ShowPickupLocation()
{
	/*
	for (int32 i = 0; i < PickupLocations.Num(); i++)
	{
		UKismetSystemLibrary::DrawDebugSphere(this, PickupLocations[i], 25.f, 30, FLinearColor::Red, 50.f, 0.5f);
	}
	*/
	for (FVector Location : PickupLocations)
	{
		UKismetSystemLibrary::DrawDebugSphere(this, Location, 25.f, 30, FLinearColor::Red, 50.f, 0.5f);
	}
}

void AMain::SetEquippedWeapon(AWeapon* WeaponToSet)
{ 
	if (EquippedWeapon)
	{
		EquippedWeapon->Destroy();
	}
	EquippedWeapon = WeaponToSet; 
}
void AMain::Attack()
{
	if (!bAttacking&&MovementStatus!=EMovementStatus::EMS_Dead)
	{
		bAttacking = true;

		SetInterpToEnemy(true);

		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && CombatMontage)
		{
			int32 Section = FMath::RandRange(0, 1);
			switch (Section)
			{
			case 0:
				AnimInstance->Montage_Play(CombatMontage, 2.2f);
				AnimInstance->Montage_JumpToSection(FName("Attack_1"), CombatMontage);
				break;
			case 1:
				AnimInstance->Montage_Play(CombatMontage, 1.9f);
				AnimInstance->Montage_JumpToSection(FName("Attack_2"), CombatMontage);
				break;
			default:
				break;
			}
			
		}
		
	}

}

void AMain::AttackEnd()
{
	bAttacking = false;
	SetInterpToEnemy(false);
	if (bLMBDown)
	{
		Attack();
	}
}

void AMain::PlaySwingSound()
{
	if (EquippedWeapon->SwingSound)
	{
		UGameplayStatics::PlaySound2D(this, EquippedWeapon->SwingSound);
	}
}

void AMain::SetInterpToEnemy(bool Interp) 
{ 
	bInterpToEnemy = Interp; 
}
float AMain::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (Health - DamageAmount <= 0.f)
	{
		Health = 0;
		Die();
		if (DamageCauser)
		{
			AEnemy* Enemy = Cast<AEnemy>(DamageCauser);
			if (Enemy)
			{
				Enemy->bHasValidTarget = false;
			}
		}
	}
	else
		Health -= DamageAmount;

	return DamageAmount;
}

void  AMain::UpdateCombatTarget()
{
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors, EnemyFilter);

	if (OverlappingActors.Num() == 0)
	{
		//if there is no enemies,remove healthbar
		if (MainPlayerController)
		{
			MainPlayerController->RemoveEnemyHealthBar();
		}
		return;
	}
	AEnemy* CloseEnemy = Cast<AEnemy>(OverlappingActors[0]);
	if (CloseEnemy)
	{
		FVector Location = GetActorLocation();
		float CloseDistance = (CloseEnemy->GetActorLocation()- Location).Size();
		for (auto Actor : OverlappingActors)
		{
			AEnemy* Enemy = Cast<AEnemy>(Actor);
			if (Enemy)
			{
				float DistanceToActor = (Enemy->GetActorLocation()- Location).Size();
				if (DistanceToActor < CloseDistance)
				{
					CloseEnemy = Enemy;
					CloseDistance = DistanceToActor;
				}
			}
			
		}
		if (MainPlayerController)
		{
			MainPlayerController->DisplayEnemyHealthBar();
		}
		SetCombatTarget(CloseEnemy);
		bHasCombatTarget = true;
	}

}

void AMain::SwitchLevel(FName LevelName)
{
	UWorld* World = GetWorld();
		if (World)
		{
			FString CurrentLevel = World->GetMapName();

			FName CurrentLevelName(*CurrentLevel);
			if (CurrentLevelName != LevelName)
			{
				UGameplayStatics::OpenLevel(World, LevelName);
			}
		}
	
}

void AMain::SaveGame()
{
	UTestSaveGame* SaveGameInstance = Cast<UTestSaveGame>(UGameplayStatics::CreateSaveGameObject(UTestSaveGame::StaticClass())); //Mark 

	SaveGameInstance->CharacterStats.Health = Health;
	SaveGameInstance->CharacterStats.MaxHealth = MaxHealth;
	SaveGameInstance->CharacterStats.Stamina = Stamina;
	SaveGameInstance->CharacterStats.MaxStamina = MaxStamina;
	SaveGameInstance->CharacterStats.Coins = Coins;

	FString MapName = GetWorld()->GetMapName();
	MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
	//UE_LOG(LogTemp, Warning, TEXT("   :%s "), *MapName);

	SaveGameInstance->CharacterStats.LevelName = MapName;

	if (EquippedWeapon)
	{
		SaveGameInstance->CharacterStats.WeaponName = EquippedWeapon->Name;
	}

	SaveGameInstance->CharacterStats.Location = GetActorLocation();
	SaveGameInstance->CharacterStats.Rotation = GetActorRotation();

	UGameplayStatics::SaveGameToSlot(SaveGameInstance, SaveGameInstance->PlayerName, SaveGameInstance->UserIndex);
}
void AMain::LoadGame(bool SetPosition)
{
	UTestSaveGame* LoadGameInstance = Cast<UTestSaveGame>(UGameplayStatics::CreateSaveGameObject(UTestSaveGame::StaticClass())); //Mark 

	LoadGameInstance=Cast<UTestSaveGame>(UGameplayStatics::LoadGameFromSlot(LoadGameInstance->PlayerName, LoadGameInstance->UserIndex));

	Health = LoadGameInstance->CharacterStats.Health;
	MaxHealth= LoadGameInstance->CharacterStats.MaxHealth;
	Stamina = LoadGameInstance->CharacterStats.Stamina;
	MaxStamina = LoadGameInstance->CharacterStats.MaxStamina;
	Coins = LoadGameInstance->CharacterStats.Coins;

	if (WeaponStorage)
	{
		AItemStorage* Weapons = GetWorld()->SpawnActor<AItemStorage>(WeaponStorage);
		if (Weapons)
		{
			FString WeaponName = LoadGameInstance->CharacterStats.WeaponName;

			if (WeaponName!=TEXT(""))
			{
				AWeapon* WeaponToEquip = GetWorld()->SpawnActor<AWeapon>(Weapons->WeaponMap[WeaponName]);
				WeaponToEquip->Equip(this);
			}
			
		}
	}

	if (SetPosition)
	{
		SetActorLocation(LoadGameInstance->CharacterStats.Location);
		SetActorRotation(LoadGameInstance->CharacterStats.Rotation);
	}

	SetMovementStatus(EMovementStatus::EMS_Normal);
	GetMesh()->bPauseAnims = false;
	GetMesh()->bNoSkeletonUpdate = false;

	if (LoadGameInstance->CharacterStats.LevelName != TEXT(""))
	{
		FName LevelName = *LoadGameInstance->CharacterStats.LevelName;
		SwitchLevel(LevelName);
	}
}
void AMain::LoadGameNoSwitch()
{
	UTestSaveGame* LoadGameInstance = Cast<UTestSaveGame>(UGameplayStatics::CreateSaveGameObject(UTestSaveGame::StaticClass())); //Mark 

	LoadGameInstance = Cast<UTestSaveGame>(UGameplayStatics::LoadGameFromSlot(LoadGameInstance->PlayerName, LoadGameInstance->UserIndex));

	//Health = LoadGameInstance->CharacterStats.Health;
	MaxHealth = LoadGameInstance->CharacterStats.MaxHealth;
	Stamina = LoadGameInstance->CharacterStats.Stamina;
	MaxStamina = LoadGameInstance->CharacterStats.MaxStamina;
	Coins = LoadGameInstance->CharacterStats.Coins;

	
	
	if (WeaponStorage)
	{
		AItemStorage* Weapons = GetWorld()->SpawnActor<AItemStorage>(WeaponStorage);
		if (Weapons)
		{
			FString WeaponName = LoadGameInstance->CharacterStats.WeaponName;

			if (WeaponName != TEXT(""))
			{
				AWeapon* WeaponToEquip = GetWorld()->SpawnActor<AWeapon>(Weapons->WeaponMap[WeaponName]);
				WeaponToEquip->Equip(this);
			}

		}
	}
	

	SetMovementStatus(EMovementStatus::EMS_Normal);
	GetMesh()->bPauseAnims = false;
	GetMesh()->bNoSkeletonUpdate = false;
}
