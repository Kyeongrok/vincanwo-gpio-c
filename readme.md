
## 

```bash
sudo apt install -y neovim zip git gcc
```


## 드라이버 및 테스트 소프트웨어 사용 방법
ubuntu Ubuntu 20.04.5
gcc 9.4.0
kernel:5.15.0-67
supported debian,fedora,centos,xubuntu

본 압축 파일에는 다음 6개의 파일이 포함되어 있습니다:"
Makefile : makefile 파일
readme.txt : 본 설명 파일
sio_gpio.c : GPIO 드라이버 파일
sio_gpio.h : GPIO 드라이버 헤더 파일
testgpio.c : GPIO 테스트 검증 프로그램


드라이버 및 테스트 소프트웨어 사용 방법:

중요:
사용하는 메인보드와 관련된 중요한 정보가 포함되어 있습니다. FAE로부터 관련된 설정 값을 요청하여 변경하고 잘 적용하지 않으면 문제가 발생할 수 있습니다:
#define IO_INDEX                   0x2E
#define DEFAULT_DUALIO             0
#define SIO_DID                    0x8728
u8 gpio_pin[8] = {64,66,63,67,23,16,37,14};
위의 매개변수를 변경한 후, 다음 단계를 따라 프로그램을 컴파일하여 기능을 검증하십시오.

이 문장은 드라이버 및 테스트 소프트웨어를 사용하는 방법을 설명하고, 메인보드와 관련된 중요한 정보를 설정해야 함을 강조하고 있습니다.


1. 드라이버 컴파일
make

2. 에러가 없다면, sio_gpio.ko 파일이 생성됩니다. 드라이버를 로드합니다 (root 권한 필요).
insmod sio_gpio.ko

3. "xc_gpio" 장치 정보가 생성되었는지 확인합니다 (또는 dmesg 명령어로 드라이버가 시스템 포트 주소를 요청한 정보를 확인합니다).
ls /dev/

4. 접근 권한을 변경하여 일반 사용자가 장치에 접근할 수 있도록 합니다.
chmod a+rw /dev/xc_gpio

5. 테스트 프로그램 컴파일
gcc -o testgpio testgpio.c

테스트 프로그램의 매개변수 설명:
-a: 모든 포트를 설정합니다. 이 매개변수는 -s 매개변수에 의존합니다.
-p: 특정 포트를 설정합니다. 이 매개변수는 -s 매개변수에 의존합니다.
-s: 포트 값을 설정합니다. 출력 전압은 0과 1이어야 합니다.
-g: 포트 상태를 폴링합니다 (이 경우 출력 포트 상태를 변경할 수 없습니다).
-e: LED 포트를 설정합니다. 이 매개변수는 -s 매개변수에 의존합니다.

예시:
모든 출력 포트를 낮은 전압으로 설정:
./testgpio -a -s 0

모든 출력 포트를 높은 전압으로 설정:
./testgpio -a -s 1

1번 포트를 높은 전압으로 설정:
./testgpio -p 1 -s 1

3번 포트를 높은 전압으로 설정:
./testgpio -p 3 -s 1

1번 포트를 낮은 전압으로 설정:
./testgpio -p 1 -s 0

3번 포트를 낮은 전압으로 설정:
./testgpio -p 3 -s 0

각 포트의 입출력 전압 상태를 폴링:
./testgpio -g

PCH_GPIO12를 높은 전압으로 설정:
./testgpio -e 1 -s 1

PCH_GPIO24를 높은 전압으로 설정:
./testgpio -e 2 -s 1

PCH_GP_LED1를 높은 전압으로 설정:
./testgpio -e 4 -s 1

PCH_GP_LED2를 높은 전압으로 설정:
./testgpio -e 5 -s 1

드라이버 프로그램 설명:
현재 드라이버 프로그램은 일반적인 ioctl 작업만 구현되었으며, write, read 작업은 구현되지 않았습니다. lseek도 구현되지 않았으므로, write, read, lseek 작업을 사용하지 마십시오. 고객은 필요에 따라 이를 구현하여 사용할 수 있습니다.

sio_gpio.h 헤더 파일 정의 설명:
#define SET_GPIO_LOWLEVEL                  // 설정 출력 포트를 낮은 전압으로 설정
#define SET_GPIO_HIGHLEVEL                 // 설정 출력 포트를 높은 전압으로 설정
#define GET_GPIO_LEVEL                     // 포트의 입출력 전압 상태를 가져옵니다. 1은 높은 전압, 0은 낮은 전압
#define SET_LED_LOWLEVEL                   // LED 포트를 낮은 전압으로 설정
#define SET_LED_HIGHLEVEL                  // LED 포트를 높은 전압으로 설정

구체적인 사용 방법은 testgpio.c의 set_port, set_ledport 및 get_port 예제를 참고하십시오. 테스트 프로그램은 표준 파일 읽기/쓰기를 따라 구현되어 있습니다: open -> ioctl -> close. 주의: 드라이버는 read, write, lseek를 구현하지 않았으므로 이 표준 C 라이브러리 함수들을 사용하여 접근하지 마십시오. ioctl은 제외입니다.

