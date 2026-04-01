#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARPGEnemyProjectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;

UCLASS()
class DIABLOARPGPROTOTYPE_API AARPGEnemyProjectile : public AActor
{
    GENERATED_BODY()

public:
    AARPGEnemyProjectile();

    void SetDamage(float InDamage) { Damage = InDamage; }

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    USphereComponent* CollisionComponent;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* ProjectileMesh;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UProjectileMovementComponent* ProjectileMovement;

    UPROPERTY(EditAnywhere, Category = "Projectile")
    float ProjectileSpeed = 800.f;

    UPROPERTY(EditAnywhere, Category = "Projectile")
    float LifeSpan = 5.f;

    float Damage = 10.f;

    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse,
        const FHitResult& Hit);
};