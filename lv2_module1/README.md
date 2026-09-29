# lv2_module1 — 장비·환경·실행 방법

Level 2 모듈 1 과제(문제 1~4)의 실행 환경과 근거 파일 위치를 정리합니다.
해석과 답안은 [report.md](report.md)에 있습니다.

---

## 1. 장비

| 구분 | 내용 |
|---|---|
| 원격 호스트 | Raspberry Pi, 호스트명 `pa03` |
| OS | Ubuntu 22.04.5 LTS (Jammy Jellyfish) |
| 아키텍처 | `aarch64` |
| 접속 방식 | SSH (`who` 기준 `pts/0`) |
| 제어 보드 | OpenCR R1.0 (Board Ver `0x170208000`) |
| 보드 USB 인식 | `Bus 001 Device 003: ID 0483:5740 STMicroelectronics Virtual COM Port` |
| 사용 포트 | `/dev/ttyACM0`|
| 구동기 | ROBOTIS Dynamixel **XM430-W210**, **ID 2** |

## 2. 환경

| 구분 | 내용 |
|---|---|
| 작업 디렉터리 | `~/pa-opencr-build` (라즈베리파이 내부) |
| 펌웨어 바이너리 | `output/opencr_position_p.ino.bin` (102 KB) |
| 업로더 | `uploader-src/arduino/opencr_develop/opencr_ld/opencr_ld` (opencr_ld ver 1.0.4) |
| 시리얼 모니터 | `python3 -m serial.tools.miniterm`, 115200-8-N-1 |
| 예제 이름 | `opencr_position_p` (OpenCR 측 위치 P 제어 예제) |

## 3. 실행 방법

### 3-1. 환경 확인

```bash
hostname
cat /etc/os-release
uname -m
who
lsusb
ls -l /dev/ttyACM*
```

### 3-2. 펌웨어 업로드

```bash
export BASE="$HOME/pa-opencr-build"
PORT=/dev/ttyACM0
UPLOADER="$BASE/uploader-src/arduino/opencr_develop/opencr_ld/opencr_ld"
set -o pipefail
"$UPLOADER" "$PORT" 115200 \
  "$BASE/output/opencr_position_p.ino.bin" 1 \
  2>&1 | tee "$BASE/upload.log"
```

성공 판정은 `CRC OK` → `[OK] Download` → `jump_to_fw` → `jump finished` 출력으로 확인합니다.

### 3-3. 목표 입력 및 응답 기록

```bash
python3 -m serial.tools.miniterm "$PORT" 115200 --eol LF -e
```

miniterm 프롬프트에서 아래 한 줄을 입력합니다.

```
s <Kp> <speed_limit_deg_s> <angle_deg>
```

| 실행 | 입력한 명령 | Kp | 속도 상한 | 상대 목표각 |
|---|---|---|---|---|
| 실행 A | `s 10 30 30` | 10 s⁻¹ | 30 °/s | +30 ° |
| 실행 B | `s 60 30 30` | 60 s⁻¹ | 30 °/s | +30 ° |

종료는 `x` 입력(→ `STOP: user` 출력)으로 수행했습니다.

## 4. 결과 파일 위치

과제 안내의 파일명과 실제 제출 파일의 대응은 다음과 같습니다.

| 과제 안내 항목 | 제출 파일 | 내용 |
|---|---|---|
| `results/환경확인.txt` 또는 화면 캡처 | [`results/environment.png`](results/environment.png) | 호스트명, Ubuntu 버전, 아키텍처, USB 인식, 사용 포트와 권한 |
| `results/upload.log` 또는 화면 캡처 | [`results/upload.png`](results/upload.png) | 라즈베리파이에서 수행한 업로드 명령과 성공 출력 |
| `results/실행A.log` 또는 화면 캡처 | [`results/A_log.txt`](results/A_log.txt) | 실행 A(Kp=10) 전체 시리얼 기록 (목표 변경 이후 26행) |
| (동일, 화면 캡처) | [`results/A_Start.png`](results/A_Start.png), [`results/A_finish.png`](results/A_finish.png) | 실행 A 시작 시점 / 종료·정지 확인 화면 |
| 문제 3 비교용 실행 B | [`results/B_log.txt`](results/B_log.txt) | 실행 B(Kp=60) 전체 시리얼 기록 |
| (동일, 화면 캡처) | [`results/B_start.png`](results/B_start.png), [`results/B_finish.png`](results/B_finish.png) | 실행 B 시작 시점 / 종료 화면 |
