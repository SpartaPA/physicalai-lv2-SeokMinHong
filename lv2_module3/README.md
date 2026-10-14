# 모듈 3 — HSV 기반 파란 물체 검출

[개인 보고서](report.md) · [실험 결과](results/) · [rqt 캡처 모아보기](results/rqt_review.png)

## 코드 출처와 구성

이 저장소에 있던 ROS 2 `vision` 패키지를 기반으로 실험했다.

| 코드 | 역할 |
| --- | --- |
| [image.cpp](vision/src/vision/src/image.cpp) | RealSense 카메라 영상 발행. 이번 bag 실험에서는 실행하지 않음 |
| [mask.cpp](vision/src/vision/src/mask.cpp) | HSV 범위와 Opening/Closing으로 Mask 생성, 별도 Overlay 발행 |
| [detection.cpp](vision/src/vision/src/detection.cpp) | Contour 면적 필터, Bounding Box 메시지·검출 로그 발행 |

코드 경로와 실행 파일 이름은 유지했으며, 비교 수치는 ROS 파라미터로 명시했다.

## 실행 환경과 입력

- Ubuntu 26.04.1 LTS, ROS 2 Lyrical
- C++17, OpenCV, cv_bridge, vision_msgs, librealsense2
- 증거 저장: Python 3, rclpy, rosbag2_py, NumPy, rqt_image_view, Qt 6
- 입력: 로컬 rosbag `vision_run2`. 원본 bag은 저장소에 포함하지 않는다.
- MCAP, `/image_raw`, 1920×1080 bgr8, 3,371프레임, 약 112.411초
- 비교: bag 약 5초 지점의 원본 프레임을 네 조건에 동일하게 사용
- 목표 없음: bag 첫 프레임과 첫 10개 연속 프레임

## 빌드와 실행

아래 명령의 시작 위치는 `lv2_module3/`이다. 각 ROS 터미널에서 환경을 불러와야 한다.

```bash
source /opt/ros/lyrical/setup.bash
cd vision
colcon build --packages-select vision
source install/setup.bash
cd ..
export ROS_DOMAIN_ID=84
```

### bag 전체를 직접 재생해 보기

각 터미널에서 위 ROS 환경과 같은 도메인을 사용한다. 실시간 카메라와 bag 입력을 동시에 발행하지 않는다. 아래 `/path/to/vision_run2`는 별도로 준비한 로컬 bag 경로로 바꾼다.

```bash
# 터미널 1: 입력
ros2 bag play /path/to/vision_run2 --loop

# 터미널 2: 선택한 Mask 설정
ros2 run vision mask --ros-args -p h_low:=104 -p h_high:=120 -p s_low:=150 -p s_high:=255 -p v_low:=8 -p v_high:=255 -p kernel_size:=5 -p min_area:=500.0

# 터미널 3: 선택한 검출 설정
ros2 run vision detection --ros-args -p min_area:=500.0

# 별도 터미널: 수신 확인
ros2 run rqt_image_view rqt_image_view /image_raw
ros2 topic echo /detections
```

rqt의 토픽 선택에서 `/image_mask`, `/image_contour`도 확인할 수 있다. 위 명령은 별도로 준비한 bag을 전체 재생하는 방법이다. 보고서의 정량 비교는 동일한 원본 프레임으로 수행했으며, 제출 자료는 `results/`에서 확인한다. 원본 bag과 자동화 도구는 제출 대상에 포함하지 않는다.

## 설정값 변경·적용 방법

| 조건 | `/mask.s_low` | `/detection.min_area` (px²) |
| --- | --- | --- |
| 실험 1-A | 200 | 500.0 |
| 실험 1-B | 150 | 500.0 |
| 실험 2-A | 150 | 500.0 |
| 실험 2-B | 150 | 20000.0 |
| 최종 선택·목표 없음 확인 | 150 | 500.0 |

고정 설정은 H=104~120, S 상한=255, V=8~255, kernel_size=5, `/mask.min_area=500.0`이다. 두 노드의 `min_area`는 별개이다. `/mask`의 값은 Overlay, `/detection`의 값은 검출 메시지에 영향을 준다.

```bash
# 실행 중 변경 예시
ros2 param set /mask s_low 150
ros2 param set /detection min_area 500.0

# 적용값 확인
ros2 param get /mask s_low
ros2 param get /detection min_area
ros2 param dump /mask
ros2 param dump /detection
```

두 노드는 매 프레임 값을 읽으므로 파라미터 변경에는 재빌드·재시작이 필요 없다. 소스의 `declare_parameter` 기본값을 수정한 경우에는 빌드 후 노드를 재시작해야 한다. 현재 소스의 검출 기본값은 마지막 비교 조건인 **20000.0**이므로, 최종 선택값으로 실행할 때는 위 명령처럼 **`-p min_area:=500.0`을 명시**한다.

## 제출 결과 위치

```text
lv2_module3/
├── README.md
├── report.md
└── results/
    ├── topic_list.txt
    ├── info_*.txt
    ├── hypotheses.txt
    ├── summary.json
    ├── rqt_review.png
    ├── exp1_A/
    ├── exp1_B/
    ├── exp2_A/
    ├── exp2_B/
    └── no_target/
```

| 파일 | 포함 내용 |
| --- | --- |
| `topic_list.txt`, `info_*.txt` | 실제 토픽 목록과 발행·구독·QoS 정보 |
| 각 조건의 `metrics.json` | 실제 조회한 설정값, 원본 시각·해시, 흰 픽셀 수, Contour 면적, 검출 결과 |
| `raw.png`, `mask.png`, `overlay.png` | 실제 수신 이미지 원본 |
| `rqt_raw.png`, `rqt_mask.png`, `rqt_overlay.png` | 실제 rqt 화면 캡처 |
| `detections.yaml` | 빈 배열을 포함한 실제 검출 메시지 |
| `mask.log`, `detection.log`, `rqt_*.log` | 각 실행의 로그. 빈 검출일 때 detection 로그는 비어 있을 수 있음 |
| `no_target/liveness.json` | 시각이 증가하는 첫 10프레임의 빈 검출과 Mask 확인 |

보고서의 선택값은 **S 하한 150, 최소 검출 면적 500 px²**이다. 동일 프레임에서 목표 영역을 보존하고 목표 누락을 피한 근거이며, 다른 조명·거리·배경에 대한 일반적 최적값은 아니다.
