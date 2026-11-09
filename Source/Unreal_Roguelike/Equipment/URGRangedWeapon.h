#pragma once

#include "CoreMinimal.h"
#include "Equipment/URGWeapon.h"
#include "URGRangedWeapon.generated.h"

/** 탄약 변경 알림 (CurrentAmmo, MagazineSize) */
DECLARE_MULTICAST_DELEGATE_TwoParams(FDelegateOnAmmoChanged, int32, int32);

/**
 * 히트스캔 원거리 무기.
 * 소유자의 시점(카메라)에서 WeaponTrace 채널로 라인트레이스해 명중을 판정하고,
 * 머즐에서 명중 지점까지 코스메틱 총알(트레일·착탄 연출)을 날린다.
 * 데미지는 총알이 명중 지점에 도착한 시점에 적용해 연출과 타이밍을 맞춘다. 탄창/재장전을 가진다.
 */
UCLASS()
class UNREAL_ROGUELIKE_API AURGRangedWeapon : public AURGWeapon
{
	GENERATED_BODY()

public:
	AURGRangedWeapon();

protected:
	/** 스폰 직후 탄창을 가득 채운다 (BP에서 지정한 MagazineSize 반영). 장착 알림 전에 채워져야 HUD가 올바른 값을 읽는다 */
	virtual void PostInitializeComponents() override;

public:
	/** 히트스캔 발사. 탄약을 소모하고 명중 시 데미지를 적용한다 */
	virtual void Attack(FGameplayTag InAttackType) override;

	/** 탄약이 있어야 공격 가능 */
	virtual bool CanAttack() const override;

	/** 탄창이 가득 차지 않았으면 재장전 가능 */
	virtual bool CanReload() const override;

	/** 탄창 재장전 (즉시 탄약을 채운다. 대기 시간은 캐릭터가 타이머로 처리) */
	virtual void Reload() override;

	/** 발사 가능한 탄약 보유 여부 */
	bool HasAmmo() const;

	/** 저장해 둔 탄약으로 되돌린다 (스테이지 이동 복원용) */
	void RestoreAmmo(int32 InAmmo);

	FORCEINLINE int32 GetCurrentAmmo() const { return CurrentAmmo; }
	FORCEINLINE int32 GetMagazineSize() const { return MagazineSize; }

	virtual class UAnimMontage* GetReloadMontage() const override;

	/** 머즐 월드 위치 (InShotIndex번째 탄이 나가는 소켓 — 소켓 목록을 순환) */
	FVector GetMuzzleLocation(int32 InShotIndex = 0) const;

protected:
	/** 탄 하나를 트레이스해 총알 연출을 시작하고, 명중 시 총알 도착 시점에 데미지를 적용하도록 예약한다 */
	void FireShot(const FVector& InTraceStart, const FVector& InDirection, const FVector& InMuzzleLocation);

	/**
	 * 명중 지점 데미지: ExplosionRadius가 있으면 범위, 없으면 명중 액터 점 데미지
	 * @param InDamage 발사 시점에 계산해 둔 데미지 (도착 시점에는 공격 타입이 바뀌어 있을 수 있다)
	 * @param bInCrit 발사 시점에 굴린 치명타 여부 (데미지 숫자 표시용)
	 */
	void ApplyShotDamage(const FHitResult& InHit, float InDamage, bool bInCrit);

	/** 발사 반동: 소유 플레이어 카메라를 흔들고 시점을 위로 올린다 (적이 쏘면 아무것도 하지 않는다) */
	void ApplyFireFeedback() const;

public:
	/** 탄약 변경 시 호출되는 델리게이트 (HUD 구독용) */
	FDelegateOnAmmoChanged OnAmmoChanged;

protected:
	/** 발사 시 머즐→명중지점으로 날아가는 코스메틱 총알 클래스 */
	UPROPERTY(EditAnywhere, Category = "RangedWeapon|Visual")
	TSubclassOf<class AURGBullet> BulletClass;

	/** 발사 시 머즐에 스폰할 파티클 */
	UPROPERTY(EditAnywhere, Category = "RangedWeapon|Visual")
	TObjectPtr<class UParticleSystem> MuzzleFX;

	/** 사거리 */
	UPROPERTY(EditAnywhere, Category = "RangedWeapon|Fire")
	float FireRange = 10000.0f;

	/** 한 번 공격에 나가는 탄 수 (산탄·미사일 다발). 탄약은 공격당 1발만 소모 */
	UPROPERTY(EditAnywhere, Category = "RangedWeapon|Fire", meta = (ClampMin = "1"))
	int32 ShotCount = 1;

	/** 탄 퍼짐 반각 (도). 0이면 정확히 조준 방향 */
	UPROPERTY(EditAnywhere, Category = "RangedWeapon|Fire", meta = (ClampMin = "0", ClampMax = "45"))
	float SpreadAngle = 0.0f;

	/** 착탄 지점 범위 데미지 반지름 (cm). 0이면 명중 액터에만 점 데미지 */
	UPROPERTY(EditAnywhere, Category = "RangedWeapon|Fire", meta = (ClampMin = "0"))
	float ExplosionRadius = 0.0f;

	/** 탄창 용량 */
	UPROPERTY(EditAnywhere, Category = "RangedWeapon|Ammo")
	int32 MagazineSize = 30;

	/** 재장전 시 캐릭터가 재생할 몽타주 (없으면 캐릭터의 ReloadDuration 타이머만) */
	UPROPERTY(EditAnywhere, Category = "RangedWeapon|Ammo")
	TObjectPtr<class UAnimMontage> ReloadMontage;

	/** 현재 탄약 (시작 시 MagazineSize로 채움) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RangedWeapon|Ammo")
	int32 CurrentAmmo = 0;

	/** 머즐 소켓 이름 목록 (무기 메시 또는 소유 캐릭터 메시). 탄이 여러 발이면 순서대로 번갈아 나간다 */
	UPROPERTY(EditAnywhere, Category = "RangedWeapon|Socket")
	TArray<FName> MuzzleSocketNames = { TEXT("Muzzle_01") };

	/** 발사음 */
	UPROPERTY(EditAnywhere, Category = "RangedWeapon|Sound")
	TObjectPtr<class USoundBase> FireSound;

	/** 발사 시 카메라 셰이크 세기 (0이면 흔들지 않음) */
	UPROPERTY(EditAnywhere, Category = "RangedWeapon|Feedback", meta = (ClampMin = "0"))
	float FireShakeScale = 0.3f;

	/** 발사 한 번에 시점이 위로 튀는 각도 (도). 다시 내려오지 않으므로 연사는 작게 */
	UPROPERTY(EditAnywhere, Category = "RangedWeapon|Feedback", meta = (ClampMin = "0", ClampMax = "10"))
	float RecoilPitch = 0.3f;

};

