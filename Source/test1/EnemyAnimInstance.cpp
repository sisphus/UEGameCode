// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyAnimInstance.h"
#include "Enemy.h"

void UEnemyAnimInstance::NativeInitializeAnimation() {
	if (Pawn == nullptr) {
		Pawn = TryGetPawnOwner();

		if (Pawn)
		{
			Enemy = Cast<AEnemy>(Pawn);
		}
	}

}
//most important Animation Function
void UEnemyAnimInstance::UpdateAnimationProperties()
{
	if (Pawn == nullptr) {
		Pawn = TryGetPawnOwner();

		if (Pawn)
		{
			
			Enemy = Cast<AEnemy>(Pawn);
		}
	}
	if (Pawn)
	{
		//just to get speed
		FVector Speed = Pawn->GetVelocity();
		FVector LateralSpeed = FVector(Speed.X, Speed.Y, 0.f);
		MovementSpeed = LateralSpeed.Size();

		
	}
}