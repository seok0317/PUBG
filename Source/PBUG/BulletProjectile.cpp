#include "BulletProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"

ABulletProjectile::ABulletProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true; // 서버-클라이언트 복제
	SetReplicateMovement(true); // 위치와 속도를 서버가 강제로 동기화함

	// 충돌체 (총알 크기)
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(10.0f);

	// [중요] 콜라이더의 충돌 끄기
	// 대신 트레이스만 허용
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Overlap); // 벽

	// 캐릭터랑은 오버랩
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	RootComponent = CollisionComp;

	// 2. 투사체 컴포넌트 (탄도학)
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;

	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->bInitialVelocityInLocalSpace = true;

	// [네트워크 동기화]
	bReplicates = true;
	SetReplicateMovement(true);

	// 스피어트레이스 
	ProjectileMovement->bSweepCollision = true;

	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &ABulletProjectile::OnOverlap);
}

void ABulletProjectile::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 서버에서만 실행 & 맞은 대상이 존재함 & 맞은 대상이 나(총알)도 아니고 나를 쏜 사람(Owner)도 아님
	if (GetLocalRole() == ROLE_Authority && OtherActor && OtherActor != this && OtherActor != GetOwner())
	{
		UE_LOG(LogTemp, Warning, TEXT("Hit Target: %s"), *OtherActor->GetName());

		// 데미지 전달 (20만큼 데미지)
		UGameplayStatics::ApplyDamage(
			OtherActor,
			DamageAmount,
			GetInstigatorController(),
			this,
			UDamageType::StaticClass()
		);

		// 적이나 벽에 맞았으니 총알 제거
		Destroy();
	}
}