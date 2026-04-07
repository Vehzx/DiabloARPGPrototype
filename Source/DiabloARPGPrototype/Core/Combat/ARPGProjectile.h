#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARPGProjectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;

UCLASS()
class DIABLOARPGPROTOTYPE_API AARPGProjectile : public AActor
{
    GENERATED_BODY()

public:
    AARPGProjectile();

    void FireInDirection(const FVector& ShootDirection, float Speed);

protected:
    UPROPERTY(VisibleAnywhere)
    USphereComponent* CollisionComponent;

    UPROPERTY(VisibleAnywhere)
    UProjectileMovementComponent* ProjectileMovement;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float Damage = 20.f;

    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse,
        const FHitResult& Hit);
};