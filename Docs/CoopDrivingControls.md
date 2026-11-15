# 4인 협동 운전 조작

## 실행

1. 프로젝트를 빌드한 뒤 에디터를 다시 연다. 새 GameInstance 설정과 C++ 클래스가 포함되어 있어 기존 에디터의 Live Coding만으로 확인하지 않는다.
2. `TestVehicle`을 열고 Play 설정에서 **Number of Players = 4**, **Net Mode = Play As Listen Server**, **New Editor Window (PIE)**로 실행한다.
3. 네 명이 준비되면 기존 `DrivingGameMode`가 Briefing / Steering / Pedals / Gear를 한 번씩 랜덤 배정한다. 각 창 위쪽 HUD에서 담당 역할을 확인한다.
4. 해당 창을 클릭하여 입력 포커스를 준 뒤 담당 키를 사용한다. 한 PC에서 네 창을 실행하면 동시에 여러 창에 키보드 입력을 보낼 수 없으므로 실제 동시 협동은 여러 PC에서도 확인한다.

| 역할 | 조작 |
| --- | --- |
| Steering | A / D 좌우 조향. 동시에 누르면 조향 입력 0 |
| Pedals | W 누르는 동안 가속. Space 반복 누르기로 제동 |
| Gear | 상승: ↑ → ↓ ↑ / 하강: ↓ ← ↑ ↓ |
| Briefing | 운전 조작 없음. 기존 브리핑 역할 유지 |

## 브레이크 및 변속 규칙

- 첫 Space 입력은 연타를 준비하며 제동량을 추가하지 않는다.
- 이전 유효 입력에서 0.6초 이내 다시 누르면 제동량이 0.35 증가한다. 최대 1.0이다.
- 제동량은 초당 0.6 감소한다. 키를 계속 누르고 있어도 추가 입력으로 처리하지 않는다.
- 0.06초 미만 중복 입력은 무시한다.
- 변속 패턴을 끝까지 완성하면 한 단만 변속한다. 오입력 또는 다음 키까지 2초 초과 시 입력을 초기화한다.
- 차량은 1단으로 시작하며 자동 변속은 꺼진다. 0은 중립, -1은 후진이다. 최고 전진 기어는 실제 차량의 Forward Gear Ratios 개수에 맞춰 제한한다.
- BP_DrivingPlayerController의 Class Defaults → Driving에서 연타 수치, 변속 패턴, 제한 시간을 조정할 수 있다. 패턴은 서로 구분되게 설정하며 한 패턴을 다른 패턴의 완성 전 접두사로 만들지 않는다.

## 연결 구조

- 역할은 기존 `CoopPlayerState.PlayerRoleComponent`에서 읽는다. 기존 결손 ID 배정은 수정하지 않았다.
- GameMode가 기존 `CoopVehicle`을 찾거나 `SharedVehicleClass`로 한 대 생성하고 모든 운전 컨트롤러에 같은 참조를 복제한다. 배치 차량을 쓸 경우 한 대만 둔다.
- 플레이어는 빈 Pawn을 소유하고 차량은 직접 소유하지 않는다. 각 클라이언트는 차량의 기존 카메라를 ViewTarget으로 사용한다.
- BP_DrivingGameMode에 남아 있는 걷기용 컨트롤러 설정은 InitGame에서 운전 컨트롤러로 보정한다. PlayerState와 HUD도 필요한 기본 클래스로 보정한다.
- TestVehicle에 저장된 기존 TestGameMod는 새 `CoopDrivingGameInstance`에서 협동 DrivingGameMode로 전환한다. 다른 맵의 GameMode 설정은 유지한다.
- Untitled → InGameMapTest라는 기존 BP_LobbyGameMode의 목적지 설정은 유지한다. InGameMapTest에서도 같은 협동 차량을 생성한다. 로비에서 TestVehicle로 이동하고 싶으면 BP_LobbyGameMode의 Next Level Path를 `/Game/Maps/TestVehicle`로 지정한다.
- 입력은 W/A/D/Space/방향키의 실제 상태와 눌림 이벤트를 읽는다. 기존 IMC_Driving/IA 자산을 변경하지 않으며 이 컨트롤러에서 등록하지 않는다.
- 가속/조향 상태를 변경 시 및 0.1초마다 보내고, 서버에서 0.5초 동안 상태를 받지 못하면 해당 입력을 해제한다. 역할 변경 및 퇴장 시에도 입력을 정리한다.
- 서버 측 제동 감쇠와 시간 초과는 `Tick`에서 실행한다. `PlayerTick`은 서버의 원격 플레이어에게 호출되지 않으므로 로컬 키 입력에만 사용한다.

## 확인할 동작

- 4명에게 중복 없이 네 역할이 배정되고 차량 한 대를 보고 있는지 확인한다.
- 담당 외의 키가 차량에 영향을 주지 않는지 확인한다. 호스트와 클라이언트 역할을 바꿔서 확인한다.
- Space 한 번/길게 누르기는 지속 제동하지 않으며 연타 시 HUD의 Brake 값이 증가하는지 확인한다.
- 잘못된 패턴, 시간 초과, 정상 상승/하강, 중립/후진 및 최고 기어 제한을 확인한다.
- 가속/조향 중 창 포커스 이동 및 담당 플레이어 퇴장 시 입력이 남지 않는지 확인한다.
- Untitled에서 4명이 구역에 모여 이동한 뒤에도 역할 HUD와 차량 조작이 동작하는지 확인한다.
- 4명 미만에서는 기존 정책대로 역할을 배정하지 않는다. 중도 참가/재접속 시 역할 재배정은 이번 범위에 포함하지 않는다.

## 자동 검사

`CoopDriving.Input.BrakeRequiresRepeatedTaps` 및 `CoopDriving.Input.GearPatterns`를 Unreal Session Frontend의 Automation에서 실행할 수 있다. 연타 누적/상한/감쇠와 변속의 정상 입력/오입력/시간 초과를 검사한다.

빌드 명령:

```powershell
$env:UBA_ROOT = 'C:\Git_Repository\Coop_Driving\Saved\AgentUBA'
& 'C:\Program Files\Epic Games\UE_5.5\Engine\Build\BatchFiles\Build.bat' Coop_DrivingEditor Win64 Development '-Project=C:\Git_Repository\Coop_Driving\Coop_Driving.uproject' -WaitMutex -NoHotReloadFromIDE -NoUBA -UBALocal
```

제한된 실행 환경에서는 UBA의 임시 저장 위치를 프로젝트의 Saved 아래로 지정하여 ProgramData 쓰기 오류를 피했다. 이 명령으로 최종 Editor 타깃의 컴파일과 링크가 통과했다.

검증 범위: C++/UHT/링크 빌드 및 diff 검사는 완료했다. 이 실행 환경에서는 UnrealEditor-Cmd가 로그/결과를 남기지 않고 종료되어 자동 테스트의 실제 실행 결과와 4인 PIE의 동작은 아직 확인하지 못했다. 위 수동 확인 절차를 수행해야 한다.
