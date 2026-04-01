#include "ARPGRangedEnemyCharacter.h"
#include "ARPGEnemyProjectile.h"
#include "DiabloARPGPrototype/Core/AI/ARPGEnemyAIController.h"
#include "DiabloARPGPrototype/Core/AI/ARPGEnemyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

AARPGRangedEnemyCharacter::AARPGRangedEnemyCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    AttackRange = 0.f;
}

void AARPGRangedEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();
}

void AARPGRangedEnemyCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    TimeSinceLastShot += DeltaTime;
    HandleRangedCombatTick(DeltaTime);
}

void AARPGRangedEnemyCharacter::HandleRangedCombatTick(float DeltaTime)
{
    if (!CurrentTarget)
    {
        bIsInFiringPosition = false;
        bIsRetreating = false;

        if (LowHealthIcon)
            LowHealthIcon->SetHiddenInGame(true);

        GetCharacterMovement()->bOrientRotationToMovement = true;
        return;
    }

    if (CurrentState != EEnemyState::Chase)
    {
        // Make sure orientation is restored if we leave chase for any reason
        GetCharacterMovement()->bOrientRotationToMovement = true;
        bIsRetreating = false;

        if (LowHealthIcon)
            LowHealthIcon->SetHiddenInGame(true);

        bIsInFiringPosition = false;
        return;
    }

    float DistToPlayer = FVector::Dist2D(GetActorLocation(), CurrentTarget->GetActorLocation());

    UE_LOG(LogTemp, Warning,
        TEXT("[RANGED DEBUG] State=%s Dist=%.1f Ret=%d FirePos=%d MovingToTarget=%d Overrides=%d"),
        *UEnum::GetValueAsString(CurrentState),
        DistToPlayer,
        bIsRetreating ? 1 : 0,
        bIsInFiringPosition ? 1 : 0,
        bIsMovingToTarget ? 1 : 0,
        OverridesChaseMovement() ? 1 : 0
    );

    // --- TOO CLOSE: retreat ---
    if (DistToPlayer < PreferredMinRange)
    {
        bIsMovingToTarget = false;
        bIsRetreating = true;

        if (LowHealthIcon)
            LowHealthIcon->SetHiddenInGame(false);

        GetCharacterMovement()->bOrientRotationToMovement = false;

        FVector AwayFromPlayer = GetActorLocation() - CurrentTarget->GetActorLocation();
        AwayFromPlayer.Z = 0;
        AwayFromPlayer.Normalize();

        SetActorRotation(AwayFromPlayer.Rotation());

        UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
        const float Angles[] = { 0.f, 30.f, -30.f, 60.f, -60.f, 90.f, -90.f, 120.f, -120.f };

        FVector BestDirection = AwayFromPlayer;
        bool bFoundDirection = false;

        for (float Angle : Angles)
        {
            FRotator Rot(0.f, Angle, 0.f);
            FVector TestDir = Rot.RotateVector(AwayFromPlayer);
            FVector TestPoint = GetActorLocation() + TestDir * RetreatStepSize;

            FNavLocation NavLoc;
            if (NavSys && NavSys->ProjectPointToNavigation(TestPoint, NavLoc))
            {
                BestDirection = TestDir;
                bFoundDirection = true;
                break;
            }
        }

        if (bFoundDirection)
        {
            FVector RetreatDestination = GetActorLocation() + BestDirection * RetreatStepSize;
            if (AARPGEnemyAIController* AICon = GetEnemyAIController())
                AICon->MoveToLocation(RetreatDestination, 5.f);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[RANGED] Cornered, firing from close range"));
            bIsRetreating = false;

            if (LowHealthIcon)
                LowHealthIcon->SetHiddenInGame(true);

            if (AARPGEnemyAIController* AICon = GetEnemyAIController())
                AICon->StopMovement();
        }

        return;
    }

    bIsRetreating = false;

    if (LowHealthIcon)
        LowHealthIcon->SetHiddenInGame(true);

    // --- IN PREFERRED RANGE: stop and fire ---
    if (DistToPlayer >= PreferredMinRange && DistToPlayer <= PreferredMaxRange)
    {
        bIsMovingToTarget = false;
        GetCharacterMovement()->bOrientRotationToMovement = false;

        // Face the player
        FVector Dir = CurrentTarget->GetActorLocation() - GetActorLocation();
        Dir.Z = 0;
        SetActorRotation(Dir.Rotation());

        if (!bIsInFiringPosition)
        {
            bIsInFiringPosition = true;

            if (AARPGEnemyAIController* AICon = GetEnemyAIController())
                AICon->StopMovement();

            bFiringDelayActive = true;
            GetWorldTimerManager().SetTimer(
                FiringDelayTimer,
                [this]()
                {
                    bFiringDelayActive = false;
                },
                FiringDelay,
                false
            );
        }

        if (!bFiringDelayActive && TimeSinceLastShot >= FireCooldown)
        {
            FireProjectile();
            TimeSinceLastShot = 0.f;
        }

        return;
    }

    // --- TOO FAR: chase ---
    bIsInFiringPosition = false;
    bFiringDelayActive = false;
    GetWorldTimerManager().ClearTimer(FiringDelayTimer);
    bIsRetreating = false;

    if (LowHealthIcon)
        LowHealthIcon->SetHiddenInGame(true);

    GetCharacterMovement()->bOrientRotationToMovement = true;

    if (AARPGEnemyAIController* AICon = GetEnemyAIController())
    {
        // Move toward the player, but we don't need to get all the way on top of them.
        // We just want to get inside our preferred band.
        const float AcceptanceRadius = PreferredMaxRange - 150;

        AICon->MoveToActor(CurrentTarget, AcceptanceRadius);
    }

}

void AARPGRangedEnemyCharacter::FireProjectile()
{
    if (!ProjectileClass || !CurrentTarget)
        return;

    UE_LOG(LogTemp, Warning, TEXT("[RANGED] Firing projectile at %s"), *CurrentTarget->GetName());

    FVector Direction = (CurrentTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
    FVector ProjectileSpawnLocation = GetActorLocation() + FVector(0.f, 0.f, 50.f) + Direction * 80.f;
    FRotator SpawnRotation = Direction.Rotation();

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = GetInstigator();

    AARPGEnemyProjectile* Projectile = GetWorld()->SpawnActor<AARPGEnemyProjectile>(
        ProjectileClass,
        ProjectileSpawnLocation,
        SpawnRotation,
        SpawnParams
    );

    if (Projectile)
        Projectile->SetDamage(ProjectileDamage);
}