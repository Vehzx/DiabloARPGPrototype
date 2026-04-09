#include "ARPGEnemyProjectile.h"
#include "ARPGEnemyCharacter.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "DiabloARPGPrototype/Core/Player/UHealthComponent.h"

AARPGEnemyProjectile::AARPGEnemyProjectile()
{
    PrimaryActorTick.bCanEverTick = false;

    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    CollisionComponent->SetSphereRadius(15.f);
    CollisionComponent->SetCollisionProfileName(TEXT("Projectile"));
    RootComponent = CollisionComponent;

    ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    ProjectileMesh->SetupAttachment(RootComponent);
    ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    ProjectileMovement->InitialSpeed = ProjectileSpeed;
    ProjectileMovement->MaxSpeed = ProjectileSpeed;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bShouldBounce = false;
    ProjectileMovement->ProjectileGravityScale = 0.f;

    CollisionComponent->OnComponentHit.AddDynamic(this, &AARPGEnemyProjectile::OnHit);
}

void AARPGEnemyProjectile::BeginPlay()
{
    Super::BeginPlay();
    SetLifeSpan(LifeSpan);

    // Ignore collision on the enemy that spawned the projecile
    if (AActor* MyOwner = GetOwner())
    {
        CollisionComponent->MoveIgnoreActors.Add(MyOwner);
    }
}

void AARPGEnemyProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (!OtherActor || OtherActor == GetOwner())
        return;

    // Destroy on hitting an enemy but deal no damage
    if (OtherActor->IsA(AARPGEnemyCharacter::StaticClass()))
    {
        Destroy();
        return;
    }

    UHealthComponent* HealthComp = OtherActor->FindComponentByClass<UHealthComponent>();
    if (HealthComp)
    {
        HealthComp->ApplyDamage(Damage, GetOwner());
    }

    Destroy();
}