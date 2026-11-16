# 운전 라운드와 목적지 설정

## 공통 흐름

`MenuMap → Untitled 로비 → 선택한 운전 맵 → 목적지 → 같은 로비 → 다음 운전 맵`

- LAN 세션을 종료하지 않고 서버가 전원을 함께 이동시킨다.
- 인게임에 4명이 도착하면 역할을 배정한다. 목적지에 공용 차량의 원점이 들어오면 운전을 종료한다.
- 종료 시 운전 입력을 끊고 역할을 None, 결손 ID 목록을 빈 배열로 만든다. 청취·발화 제한도 해제한다.
- 로비 도착 시에도 배정을 초기화하고 LAN 입장을 다시 연다. 새 로비의 준비 구역과 새 운전 GameMode는 매 판 초기 상태로 시작한다.
- 다음 판의 역할은 다시 무작위로 뽑는다. 우연히 이전 판과 같은 역할이 나올 수 있다.

## 에디터에서 설정하기

새 C++ 클래스가 추가되었으므로 에디터를 재시작하고 빌드한 코드를 사용한다.

1. `BP_LobbyGameMode` → Class Defaults → **Travel → Driving Map**에서 제작한 맵을 선택한다.
   - 비워 두면 기존 **Next Level Path**를 사용한다. 기존 기본 맵 연결을 유지하기 위한 호환 경로다.
   - **In Game Game Mode**에는 `BP_DrivingGameMode` 또는 그 파생 BP를 지정한다. 맵에 저장된 GameMode Override와 관계없이 이 운전 규칙으로 입장한다.
2. 운전 맵을 열고 Place Actors에서 **Driving Destination Zone**을 검색하여 목적지에 배치한다.
   - Details에서 **DestinationBounds → Box Extent**를 조절한다. 이동·회전·크기 조절을 지원한다.
   - 판정은 차량 원점이 박스 안에 있는지 0.1초마다 서버에서 검사한다. 차량 메시의 Overlap 설정에 의존하지 않는다.
   - 박스는 판정 영역이며 게임 중 목적지 표지판이나 장식은 별도로 제작한다.
   - 출발 차량이 목적지 안에서 생성되지 않도록 떨어뜨려 배치한다.
3. 차량은 기존과 같이 `CoopVehicle` 파생 BP 한 대를 배치하거나, 운전 GameMode의 **Shared Vehicle Class**로 생성한다.
4. 맵을 직접 PIE 실행할 때도 같은 규칙을 쓰려면 World Settings의 **GameMode Override**를 `BP_DrivingGameMode`로 설정한다.
   - 직접 실행 시 복귀 위치는 운전 GameMode의 **Return Lobby Map / Return Lobby Game Mode**를 사용한다. 기본값은 `Untitled / BP_LobbyGameMode`다.
   - 로비에서 출발하면 실제 출발 로비의 맵과 GameMode를 자동으로 전달하므로 이 기본값보다 우선한다.
5. 패키징할 때 추가 운전 맵과 커스텀 로비를 Project Settings → Packaging → **List of maps to include in a packaged build**에 추가한다.

맵1·맵2·맵3에는 공통 운전 GameMode와 목적지 액터를 그대로 사용한다. 맵마다 별도의 라운드 종료 코드를 작성할 필요가 없다.

## 실행 중 맵 선택

서버의 LobbyGameMode에 Blueprint 함수 **Set Driving Map**을 호출하면 다음 출발 맵을 변경할 수 있다. 준비 구역 카운트다운이 끝나기 전에 설정한다. 클라이언트에서 직접 호출해도 변경되지 않는다. 이후 맵 선택 UI를 붙일 때 서버 요청을 통해 호출한다.

선택값은 현재 로비 인스턴스에만 적용된다. 복귀 후에는 BP 기본값으로 시작한다. 자동 순환이나 클리어 진행도 저장은 포함하지 않는다.

## 수동 확인

1. 리슨 서버 4인으로 로비에서 준비 구역에 모여 선택한 운전 맵으로 이동한다.
2. 네 역할과 기존 결손이 적용되고 한 차량을 조작하는지 확인한다.
3. 플레이어 Pawn이나 다른 차량을 목적지에 넣어도 종료되지 않는지 확인한다. 공용 차량으로 도착하면 전원이 로비로 돌아와야 한다.
4. 역할이 None이고 결손 목록이 비었으며, `LANVoiceStatus`에서 Mute=0 / Deaf=0인지 확인한다. 실제 음성 송수신도 검사한다.
5. 로비에서 이동·점프·준비 구역이 정상 동작하고, 다시 출발하면 네 역할이 새로 배정되는지 확인한다. 최소 두 판을 연속 진행한다.
6. 다른 운전 맵을 선택하여 동일한 출발·복귀 흐름을 확인한다.

로비의 PlayerStart는 준비 구역 밖에 배치한다. 구역 안에서 네 명이 생성되면 기존 준비 조건에 따라 다음 판이 자동으로 시작될 수 있다.

## 로비 그룹 스폰

- 로비의 PlayerStart 하나에 Actor Tags 또는 Player Start Tag로 `LobbySpawn`을 지정하면 그 위치를 그룹 중심으로 사용한다. 태그가 없으면 이름순 첫 PlayerStart를 사용한다. 다른 멀리 떨어진 PlayerStart로 분산하지 않는다.
- 기본 배치는 중심 주변의 2×2 형태이며 캐릭터 중심 간격은 180cm다. 실제 캡슐이 더 크면 지름과 여유 공간에 맞춰 간격을 늘린다. 회전은 중심 PlayerStart의 Yaw를 따른다.
- BP_LobbyGameMode → Class Defaults → **Lobby Spawn**에서 `Spawn Center Tag`, `Spawn Spacing`, `Spawn Max Radius`, `Spawn Clearance`를 설정한다. 기본 최대 반경은 400cm, 캐릭터 캡슐 사이 최소 여유는 20cm다.
- 바닥·경사·장애물·기존 캐릭터 간격을 서버에서 검사한 뒤 스폰한다. 기본 자리가 막혀 있으면 지정 반경 안에서 다른 자리를 찾는다. 한 명씩 입장할 때와 인게임에서 돌아올 때 모두 적용한다.
- 반경 내 안전한 자리가 없으면 스폰을 대기하고 0.5초마다 다시 시도한다. 겹치는 위치나 먼 PlayerStart에 강제로 스폰하지 않는다. 맵에는 네 명이 설 수 있는 바닥 공간을 확보해야 한다.
- `CoopDriving.Lobby.GroupSpawn`은 바닥 없는 상태에서의 대기, 회전된 중심 주변 4인 간격, 먼 PlayerStart 무시, 전체 영역이 막힌 상태의 대기를 검사한다. PIE 스크립트도 최초 입장 및 두 번의 복귀마다 모든 클라이언트의 4인 캡슐 간격과 그룹 범위를 검사한다.
- 그룹 스폰 변경 후 Editor 모듈(`2165`) 빌드 및 자동 테스트 7개가 통과했다(공간 부족 대기 테스트의 예상 경고 포함). 4인 PIE에서 최초 입장과 두 번의 로비 복귀 모두 180cm 간격의 2×2 배치와 모든 클라이언트의 캡슐 간격 검사를 통과했다. 결과는 `Saved/Automation/LobbySpawn` 및 `Saved/Logs/LobbySpawnPIE.log`에 남긴다.

자동 테스트 `CoopDriving.Round.ResetAndReassign`은 역할·모든 결손 초기화, PlayerState 교체 시 초기화 상태 보존, 다음 판 배정을 검사한다. `CoopDriving.Round.DestinationBounds`는 회전·크기 조절된 목적지의 내부/외부 판정을 검사한다. 실제 멀티플레이 이동·음성 검증은 별도로 수행한다.

## 검증 결과 (2026-11-16)

- C++/UHT 컴파일 및 별도 이름의 Editor 모듈 링크 통과. 기본 DLL은 사용 중이라 별도 모듈(`2163`)로 빌드했으며, 새 에디터에서 해당 모듈을 로드하여 검사했다.
- `CoopDriving` 자동 테스트 6개 모두 통과. 결과: `Saved/Automation/RoundFlow/index.json`.
- `Tools/test_round_flow.py`로 4인 리슨 PIE에서 `Untitled → InGameMapTest → Untitled → TestVehicle → Untitled` 통과. 각 판 네 역할의 클라이언트 복제, 목적지 종료, 네 명의 복귀 및 역할·결손 초기화를 확인했다.
- 테스트에서는 준비 구역을 실행 중에만 넓히고 목적지 액터를 실행 중에 생성했다. 저장된 맵이나 BP에는 목적지를 추가하지 않았다. 실제 사용할 맵에 목적지 배치가 필요하다.
- 음성을 끈 테스트에서 정책 상태만 확인했으며 여러 PC의 실제 마이크 송수신·음질은 별도 확인이 필요하다.
