#include "DiabloARPGPrototype/Core/Combat/ARPGProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "DiabloARPGPrototype/Core/Player/UHealthComponent.h"

AARPGProjectile::AARPGProjectile()
{
    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
    CollisionComponent->InitSphereRadius(10.f);
    CollisionComponent->SetCollisionProfileName("BlockAllDynamic");
    CollisionComponent->OnComponentHit.AddDynamic(this, &AARPGProjectile::OnHit);
    RootComponent = CollisionComponent;

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bShouldBounce = false;

    InitialLifeSpan = 3.f;
}

void AARPGProjectile::FireInDirection(const FVector& ShootDirection, float Speed)
{
    ProjectileMovement->Velocity = ShootDirection * Speed;
}

void AARPGProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse,
    const FHitResult& Hit)
{
    if (OtherActor)
    {
        if (UHealthComponent* Health = OtherActor->FindComponentByClass<UHealthComponent>())
        {
            Health->ApplyDamage(Damage, GetOwner());
        }
    }

    Destroy();
}