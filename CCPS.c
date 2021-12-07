#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio_ext.h>

//인터페이스 좌표 관련
#define MAX_X 75
#define TEMP_X 15
#define RPM_X 30
#define QOE_X 45
#define REFRIGERANT_X 60
#define CHART_SPACE 15
#define NAME_SPACE 16
#define VALUE_SPACE 18
#define SIMUL_SPACE 22

//시간 관련
#define UPDATE_TURM 1
#define SIMUL_TIME 15

// 속도 RPM 연료 냉각수 온도 계산 관련
#define Tire_Perimeter_Ratio 1850
#define Fuel_efficiency 9
#define InitSpeed 80
#define Initacceleration 3
#define Min 60
#define InitFeul 80

#define SIZE 256
//표시될 data들 프로세스간 통신이 될때 사용됨
int temperature = 200;
int rpm = 6000;
int qoe = 100;
int refrigerant = 1000;

// 프로세스 통신에 필요한 int 형 배열과 버퍼
int pipe1[2], pipe2[2], pipe3[2], pipe4[2], pipe5[2];
int buffer1, buffer2, buffer3, buffer4, buffer5;
int Temperature = 80;
int Input_temperature = 3;
int Qoe = InitFeul;
int Refrigerant = 0;
int RPM = 0;

int sync_th = 0;

//커서 함수
void move_cur(int, int);

//초기화 관련 함수
void drawBoard();
void drawInfo();
void initPanel();

//쓰레드 함수
pthread_t readValueth;
pthread_t updateTempth;
pthread_t updateRPMth;
pthread_t updateQoeth;
pthread_t updateRefrigerantth;

//동기화를 위한 tool
pthread_mutex_t mut;
pthread_mutex_t rpmut;
pthread_cond_t cmd;
pthread_mutex_t cursorMutex;

//시뮬레이션 관련 함수
void simulate();
void simulMenu();

//패널 업데이트 함수
void updatePanel(int temperature, int rpm, int qoe, int refrigerant);

//시뮬레이션 메뉴 값 범위 에러 함수
void range_error();

//알람 시그널 함수
void stopThread();

void *readValue(void *); //프로세스 통신용 함수
void *updateTemp(void *);
void *updateRPM(void *);
void *updateQoe(void *);
void *updateRefrigerant(void *);

//계산 함수들
void *Figures_Temp(void *);
void *Figures_RPM(void *);
void *Figures_Qoe(void *);
void *Figures_Refrigerant(void *);
void run_thread();

int main()
{
  pid_t pid;
  // 파이프 생성및 오류 검사
  if (pipe(pipe1) == -1 || pipe(pipe2) == -1 || pipe(pipe3) == -1 || pipe(pipe4) == -1 || pipe(pipe5) == -1)
  {
    perror("pipe failed\n");
    exit(0);
  }
  // 자식 프로세스 생성 및 오류 검사
  if ((pid = fork()) < 0)
  {
    perror("fork error\n");
    exit(1);
  }
  else if (pid == 0)
  {
    //자식 프로세스
    //thread1 2 3 을 시작
    run_thread();
  }
  else
  {
    // 부모 프로세스
    signal(SIGALRM, stopThread);            //프로세스에 무어잇인 발생했음을 알리는 메시지
    pthread_mutex_init(&cursorMutex, NULL); //뮤텍스(mutex)는 쓰레드가 공유하는 데이터 영역을 보호하기 위해서 사용되는 도구이다. pthread_mutex_init는 뮤텍스 객체를 초기화 시키기 위해서 사용한다.
                                            //상호배제"가 되겠네요. 해석의 의미는 모호하지만 뮤텍스 객체를 두 쓰레드가 동시에 사용할 수 없다는 것입니다.
    initPanel();                            //패널 시작
    simulate();                             //0또는 1을 입력하여 시작
  }
  return 0;
}

void move_cur(int x, int y)
{
  printf("\033[%d;%df", y, x);
}

//초기 화면을 지우고 인터페이스의 틀을 그려주는 함수
void drawBoard()
{
  int i = 1;

  system("clear"); //윈도우는 cls, 리눅스는 clear
  move_cur(0, 0);
  printf("===========================================================================");
  for (i = 1; i < 20; i++)
  {
    move_cur(0, i);
    printf("=");
    move_cur(MAX_X, i);
    printf("=");
  }
  move_cur(0, 20);
  printf("===========================================================================");
}

//Panel 에서 보여줘야할 그래프를 초기화 해주는 함수
void drawInfo()
{
  int x = 0;
  int y = 0;
  int i = 0;

  //속도 부분 초기화
  for (i = 10; i > 0; i--)
  {
    move_cur(TEMP_X - 4, i + 5); //15 - 4, i + 5
    if (i % 2 != 0)
      printf("%3d", (NAME_SPACE - 3 - i) * 20);
  }
  move_cur(TEMP_X - 2, NAME_SPACE); //15 - 2, 16
  printf("%5s", "SPEED");
  move_cur(TEMP_X - 2, VALUE_SPACE); //15 - 2, 18
  printf("%3d", 0);

  //RPM부분 초기화
  for (i = 12; i > 0; i--)
  {
    move_cur(RPM_X - 4, i + 3); // 30 - 4, i + 3
    if (i % 2)
      printf("%3d", (NAME_SPACE - 3 - i) / 2); //16 - 3 - i / 2
  }
  move_cur(RPM_X - 2, NAME_SPACE); //30 - 2, 16
  printf("%5s", "RPM");
  move_cur(RPM_X - 2, VALUE_SPACE); //30 - 2, 18
  printf("%3d", 0);

  //연료 부분 초기화
  move_cur(QOE_X - 4, NAME_SPACE - 10); // 45 - 4, 16 - 10
  printf("%3c", 'F');
  move_cur(QOE_X - 4, NAME_SPACE - 1); // 45 - 4, 16 - 1
  printf("%3c", 'E');
  move_cur(QOE_X - 2, NAME_SPACE); // 45 - 2, 16
  printf("%5s", "FUEL");
  move_cur(QOE_X - 2, VALUE_SPACE); // 45 - 2, 18
  printf("%3d", 0);

  //냉각수 부분 초기화
  move_cur(REFRIGERANT_X - 2, NAME_SPACE); // 60 - 2, 16
  printf("%5s", "COOLANT");
  move_cur(REFRIGERANT_X - 2, VALUE_SPACE); // 60 - 2, 18
  printf("%3d", 0);
}

//Panel 부분을 초기화 해주는 함수
void initPanel()
{
  drawBoard(); //초기 화면을 지우고 인터페이스의 틀을 그려주는 함수
  drawInfo();  //Panel 에서 보여줘야할 그래프를 초기화 해주는 함수 === 내부에 정보를 그려줌
}

//시뮬레이팅을 위한 함수
//1 입력 시 simulMenu 시작
void simulate()
{
  int input = -1;

  move_cur(0, SIMUL_SPACE); //0, 22
  printf("What do you want? (0 : exit, 1 : simulation) >>");
  __fpurge(stdin); //리눅스에서 __fpurge(stdin)
  scanf("%d", &input);
  switch (input)
  {
  case 0:
    move_cur(0, 0);
    system("clear");
    exit(0);
  case 1:
    simulMenu();
    break;
  default:
    range_error();
  }
  while (1)
  {
  }
}

//시뮬레이션 메뉴 함수
//속도를 입력함
void simulMenu()
{
  int input = -1;
  int p1Temp = 0;
  int p1Input_Temp = 0;
  int check;
  int i = 0; //for 문 제어 변수

  close(pipe1[0]);
  while (1)
  {

    move_cur(0, SIMUL_SPACE - 1);
    printf("                        ");
    move_cur(0, SIMUL_SPACE);
    printf("                                                         ");
    move_cur(0, SIMUL_SPACE);
    printf("Please input Temperature (20~160) >>");
    __fpurge(stdin); //리눅스에서 __fpurge(stdin) 입력 버퍼 지우기
    scanf("%d", &p1Speed);
    move_cur(0, SIMUL_SPACE);
    printf("                                                         ");
    if (p1Temp <= 160 && p1Temp >= 20)
    {
      move_cur(0, SIMUL_SPACE - 1);
      printf("                        ");
      move_cur(0, SIMUL_SPACE);
      printf("Please input Acceleration (1~10) >>");
      __fpurge(stdin); //리눅스에서 __fpurge(stdin) 입력 버퍼 지우기
      scanf("%d", &p1Input_Temp);
      if (p1Input_Temp >= 1 && p1Input_Temp <= 10)
      {
        //이부분에 프로세스간 통신 작성하면 됨
        write(pipe1[1], &p1Temp, sizeof(int));
        write(pipe1[1], &p1Input_Temp, sizeof(int));
        //스레드 생성 및 알람 등록

        if (pthread_create(&updateTempth, NULL, &updateTemp, NULL) < 0)
          exit(2);
        if (pthread_create(&updateRPMth, NULL, &updateRPM, NULL) < 0)
          exit(2);
        if (pthread_create(&updateQoeth, NULL, &updateQoe, NULL) < 0)
          exit(2);
        if (pthread_create(&updateRefrigerantth, NULL, &updateRefrigerant, NULL) < 0)
          exit(2);

        alarm(SIMUL_TIME);
      }
      else
        range_error();

      break;
    }
    else
      range_error();
    break;
  }
}

//시뮬레이션의 값 범위 에러 메시지 함수
void range_error()
{
  printf("정확한 값을 입력해주세요");
}

//value에 따라 패널을 업데이트 해주는 thread들
void *updateTemp(void *arg)
{
  int i = 0;
  int k = 0;
  int tempChart = 0;
  int nread = 0;

  //close(pipe2[1]);
  while (1)
  {
    if (sync_th != 0)
      continue;
    fcntl(pipe2[0], F_SETFL, O_NONBLOCK); //fcntl함수는 이미 열린 파일의 속성들을 변경할 수 있습니다.
                                          //파일 상태 속성들을 세번째 인수로 받아 설정합니다. O_APPEND, O_NONBLOCK, O_SYNC, O_DSYNC, O_RSYNC, O_FSYNC, O_ASYNC 속성만 변경할 수 있습니다. 읽기, 쓰기 관련 플래그(O_WRONLY, O_RDONLY, O_RDWR)은 조회할 수는 있지만 변경할 수 없음을 유의하세요.

    switch (nread = read(pipe2[0], &buffer2, sizeof(int)))
    {
    case -1:
      if (errno == EAGAIN)
      {
        break;
      }
    default:
      temperature = buffer2;
      break;
    }

    //이 두 함수는 mutex를 이용하여 임계 구역을 진입할때 그 코드 구역을 잠그고 다시 임계 구역이 끝날때 다시 풀어 다음 스레드가 진입할 수 있도록 합니다.
    pthread_mutex_lock(&cursorMutex);

    tempChart = (int)((double)temperature / 30);
    for (i = 0; i < 10; i++)
    {
      move_cur(TEMP_X, CHART_SPACE - i); //15, 15 - i
      printf("  ");
    }
    for (i = 0; i < tempChart; i++)
    {
      move_cur(TEMP_X, CHART_SPACE - i); //15, 15 - i
      printf("O");
    }
    move_cur(TEMP_X - 2, VALUE_SPACE); //15 - 2, 18
    printf("%5d", temperature);

    sync_th++;
    pthread_mutex_unlock(&cursorMutex);
  }
}
void *updateRPM(void *arg)
{
  int i = 0;
  int rpmChart = 0;
  int nread = 0;

  //close(pipe3[1]);
  while (1)
  {
    if (sync_th != 1)
      continue;
    fcntl(pipe3[0], F_SETFL, O_NONBLOCK);

    switch (nread = read(pipe3[0], &buffer3, sizeof(int)))
    {
    case -1:
      if (errno == EAGAIN)
      {
        break;
      }
    default:
      rpm = buffer3;
      break;
    }
    pthread_mutex_lock(&cursorMutex);

    rpmChart = (int)((double)rpm / 500);
    for (i = 0; i < 12; i++)
    {
      move_cur(RPM_X, CHART_SPACE - i); //30, 15 - i
      printf("  ");
    }
    for (i = 0; i < rpmChart; i++)
    {
      move_cur(RPM_X, CHART_SPACE - i); //30, 15 - i
      printf("O");
    }
    move_cur(RPM_X - 2, VALUE_SPACE); //30 - 2, 18
    printf("%5d", rpm);
    sync_th++;
    pthread_mutex_unlock(&cursorMutex);
  }
}
void *updateQoe(void *arg)
{
  int i = 0;
  int QoeChart = 0;
  int nread = 0;

  //  close(pipe4[1]);
  while (1)
  {
    if (sync_th != 2)
      continue;
    fcntl(pipe4[0], F_SETFL, O_NONBLOCK);

    switch (nread = read(pipe4[0], &buffer4, sizeof(int)))
    {
    case -1:
      if (errno == EAGAIN)
      {
        break;
      }
    default:
      qoe = buffer4;
      break;
    }

    pthread_mutex_lock(&cursorMutex);

    qoeChart = (int)((double)qoe / 10);
    for (i = 0; i < 10; i++)
    {
      move_cur(QOE_X, CHART_SPACE - i); //45, 15 - i
      printf("  ");
    }
    for (i = 0; i < fuelChart; i++)
    {
      move_cur(QOE_X, CHART_SPACE - i); //45, 15 - i
      printf("O");
    }
    move_cur(QOE_X - 2, VALUE_SPACE); //45 - 2, 18
    printf("%5d", qoe);
    sync_th++;
    pthread_mutex_unlock(&cursorMutex);
  }
}
void *updateRefrigerant(void *arg)
{
  int i = 0;
  int refrigerantChart = 0;
  int nread = 0;

  //  close(pipe5[1]);
  while (1)
  {
    if (sync_th != 3)
      continue;
    fcntl(pipe5[0], F_SETFL, O_NONBLOCK);

    switch (nread = read(pipe5[0], &buffer5, sizeof(int)))
    {
    case -1:
      if (errno == EAGAIN)
      {
        break;
      }
    default:
      refrigerant = buffer5;
      break;
    }

    pthread_mutex_lock(&cursorMutex);

    refrigerantChart = (int)((double)refrigerant / 10);
    for (i = 0; i < 10; i++)
    {
      move_cur(REFRIGERANT_X, CHART_SPACE - i); //60, 15 - i
      printf("  ");
    }
    for (i = 0; i < refrigerantChart; i++)
    {
      move_cur(REFRIGERANT_X, CHART_SPACE - i); //60, 15 - i
      printf("O");
    }
    move_cur(REFRIGERANT_X - 2, VALUE_SPACE); // 60 - 2, 18
    printf("%5d", refrigerant);
    sync_th = 0;
    pthread_mutex_unlock(&cursorMutex);
  }
}
void stopThread()
{

  pthread_mutex_lock(&cursorMutex);
  sleep(1);

  move_cur(0, SIMUL_SPACE + 1); //0, 22 + 1
  exit(0);
}
// 속도값을 계산하는 함수
// 초기에 속력값과 가속도값을 다른 프로세스에서 받아와 계산함
void *Figures_Temp(void *arg)
{
  //  close(pipe1[1]);
  //close(pipe2[0]);

  int nread = 0;
  read(pipe1[0], &buffer1, sizeof(int));
  Temp = buffer1;
  read(pipe1[0], &buffer1, sizeof(int));
  Input_temperature = buffer1;
  while (1)
  {
    /*fcntl(pipe1[0], F_SETFL, O_NONBLOCK);

		fcntl(pipe6[0], F_SETFL, O_NONBLOCK);
		switch (nread = read(pipe1[0], &buffer1, sizeof(int))) {
		case -1:
		if (errno == EAGAIN) {
		break;
		}
		default:
		Speed = buffer1;
		break;
		}
		switch (nread = read(pipe6[0], &buffer6, sizeof(int))) {
		case -1:
		if (errno == EAGAIN) {
		break;
		}
		default:
		Acceleration = buffer6;
		break;
		}*/

    pthread_mutex_lock(&mut);
    if (Temperature < 220)
    {
      Temperature += Input_temperature;
    }
    else if (Temperature >= 220)
    {
      Temperature -= Input_temperature;
    }
    write(pipe2[1], &Temperature, sizeof(int));
    //pthread_cond_signal(&cmd);
    pthread_mutex_unlock(&mut);
    //  printf("Speed\t: %.2lf (km/h)\n", Speed);
    sleep(1);
  }
}
// RPM 값을 계산하는 함수
// 스레드로 계산되는 속도값을 읽어와 순간 RPM을 계산하는 함수
// RPM = 속도 * 1000000 / 60분 / 타이어의 지름
void *Figures_RPM(void *arg)
{
  //close(pipe3[0]);

  while (1)
  {

    //pthread_mutex_lock(&mut);
    pthread_mutex_lock(&rpmut);
    RPM = Speed * 1000000 / Min / Tire_Perimeter_Ratio; // 60 / 1850
    write(pipe3[1], &RPM, sizeof(int));
    //  pthread_cond_wait(&cmd, &mut);
    pthread_mutex_unlock(&rpmut);
    //  pthread_mutex_unlock(&mut);
    sleep(1);
    //  printf("RPM\t: %.2lf (RPM)\n", RPM);
  }
}
// 연료의 양을 계산하는 함수
// 소모량 = 이동거리/8.7
void *Figures_Qoe(void *arg)
{

  //close(pipe4[0]);
  while (1)
  {

    double result;
    //pthread_mutex_lock(&mut);

    result = RPM * Tire_Perimeter_Ratio / 1000000;

    //  pthread_cond_wait(&cmd, &mut);

    //  pthread_mutex_unlock(&mut);
    Qoe = Qoe - (result / 1000) / Fuel_efficiency;
    write(pipe4[1], &Qoe, sizeof(int));
    //printf("fuel\t: %.2lf (L)\n", Fuel);
    sleep(1);
  }
}
// 냉각수 온도
// 실제로는 센서로 측정하지만
// 연관 관계를 나타내기위헤 속도에 맞춰서 계산함
void *Figures_Refrigerant(void *arg)
{
  //close(pipe5[0]);

  while (1)
  {

    //pthread_mutex_lock(&mut);
    if (Temp == 0)
    {
      Refrigerant = 0;
    }
    else if (Temp > 0 && Temp <= 40)
    {
      Refrigerant = 40;
    }
    else if (Temp > 0 && Temp <= 80)
    {
      Refrigerant = 76;
    }
    else if (Temp > 80 && Temp <= 140)
    {
      Refrigerant = 82;
    }
    else if (Temp > 140 && Temp <= 160)
    {
      Refrigerant = 97;
    }
    else if (Temp > 160 && Temp < 200)
    {
      Refrigerant = 95;
    }
    write(pipe5[1], &Refrigerant, sizeof(int));
    //  pthread_cond_wait(&cmd, &mut);
    //  pthread_mutex_unlock(&mut);
    //printf("Coolant\t: %.2lf (℃  )\n", Coolant);
    sleep(1);
  }
}
void run_thread()
{
  int re = 0;
  pthread_mutex_init(&mut, NULL); //첫번째 인자인 mutex는 초기화 시킬 mutex객체이다. 초기화 시킬 때 뮤텍스의 특징을 정의할 수 있는데, 이는 두번째 인자인 attr를 통해서 이루어진다. 기본 뮤텍스 특징을 사용하길 원한다면 NULL을 이용하면 된다.
  pthread_mutex_init(&rpmut, NULL);
  pthread_cond_init(&cmd, NULL); //조건 변수를 초기화합니다. 이 함수말고 정적으로 조건 변수를 초기화할 경우에는 PTHREAD_CONT_INITIALIZER 상수를 이용해서 초기화할 수도 있습니다.
  pthread_t thread1, thread2, thread3, thread4;

  re = pthread_create(&thread1, NULL, Figures_Temp, NULL); //쓰레드 생성 기본
  if (re < 0)
  {
    printf("threads[0] Figures_Temp error\n");
    exit(0);
  }
  re = pthread_create(&thread2, NULL, Figures_RPM, NULL);
  if (re < 0)
  {
    printf("threads[1] Figures_RPM error\n");
    exit(0);
  }
  re = pthread_create(&thread3, NULL, Figures_Qoe, NULL);
  if (re < 0)
  {
    printf("threads[2] Figures_Qoe error\n");
    exit(0);
  }
  re = pthread_create(&thread4, NULL, Figures_Refrigerant, NULL);
  if (re < 0)
  {
    printf("threads[3] Figures_Refrigerant error\n");
    exit(0);
  }
  sleep(5000);
  pthread_mutex_destroy(&mut); //만약 뮤텍스를 동적으로 생성(pthread_mutex_init을 이용하여 초기화)했다면 이 함수를 사용하는 함수가 pthread_mutex_destroy입니다.
  pthread_cond_destroy(&cmd);  //pthread_cond_destroy() 함수는, 조건 변수 cond 에 의해 할당할 수 있었던 리소스를 해방합니다.
  pthread_mutex_destroy(&rpmut);
}