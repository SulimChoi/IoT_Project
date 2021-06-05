#include <termios.h>
#include <sys/signal.h>
#include <asm/ioctls.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <math.h>

# define dot "/dev/dot"
# define clcd "/dev/clcd"
# define dip "/dev/dipsw"
# define led "/dev/led"
# define fnd "/dev/fnd"

static char tactswDev[] = "/dev/tactsw";
static int tactswFd = (-1);

// 함수 선언부 
void clcd_input(char clcd_text[]);
int dipsw_input();
int dipsw_input_x();
unsigned char tactsw_get(int tmo);
int tactsw_input();
void led_control(int sw_num);
unsigned char fnd_input(int num);
int fnd_print(int score);
int declife(int l, int d);
int calc_score(int life);

typedef struct _values{
	int life;
	int start_dif;
}values;
int score = 0; // 점수

values start();

int main(){
	values value; // 목숨, 난이도 구조체 
	int dot_d, i, j, k; 
	int tn = 0; // 수정(tn: tact num)
	int plus_score, tmp; //목숨과 난이도에 따른 점수 가중치 
	char sc[20]; // 점수 형변환을 위한 변수 
	char text[100], cpy[] = "SCORE: " ; //clcd 점수 출력문 
	unsigned char game[9][8] = 
	{
		{0xE4, 0xE4, 0xFF, 0x24, 0x24, 0xFF, 0x24, 0x24}, // 1번
		{0x3C, 0x3C, 0xFF, 0x24, 0x24, 0xFF, 0x24, 0x24}, // 2번
		{0x27, 0x27, 0xFF, 0x24, 0x24, 0xFF, 0x24, 0x24}, // 3번
		{0x24, 0x24, 0xFF, 0xE4, 0xE4, 0xFF, 0x24, 0x24}, // 4번
		{0x24, 0x24, 0xFF, 0x3C, 0x3C, 0xFF, 0x24, 0x24}, // 5번
		{0x24, 0x24, 0xFF, 0x27, 0x27, 0xFF, 0x24, 0x24}, // 6번
		{0x24, 0x24, 0xFF, 0x24, 0x24, 0xFF, 0xE4, 0xE4}, // 7번
		{0x24, 0x24, 0xFF, 0x24, 0x24, 0xFF, 0x3C, 0x3C}, // 8번
		{0x24, 0x24, 0xFF, 0x24, 0x24, 0xFF, 0x27, 0x27}  // 9번
	};
	// 초기 설정 
	value = start();
	plus_score = calc_score(value.life);
	tmp = (int)(log10((double)value.start_dif)/log10(2));
	
	// 게임 시작 
	clcd_input("GAME START");
	printf("목숨 개수: %d, 난이도: %d\n", value.life, value.start_dif);
	sleep(5);
	
	clcd_input("SCORE: 0");
	
	dot_d = open(dot, O_RDWR);
	
	if(dot_d < 0){
		printf("error\n");
		return 0;
	}
	
	// 랜덤 패턴 시작 
	for (j=0; j<15; j++){
		srand((unsigned int)time(NULL));
		k = (int)rand()%9;
		
		unsigned char b;
		int timer = 0;
		
		// 정해진 시간 동안 tact 스위치 입력 검사 
		while(timer < 80/tmp){
			write(dot_d, &game[k], sizeof(game[k]));
			usleep(100000);
			
			tactswFd = open(tactswDev, O_RDONLY);
			
			if(tactswFd<0){
				printf("open failed\n");
			}
			
			read(tactswFd, &b, sizeof(b));
			
			// 버튼 입력 시 조건 시작 
			if(b){
				tn = b;
				close(tactswFd);
				printf("%d, %d\n", (k + 1), tn);
				
				// 제대로 버튼 누를 시 점수 증가 및 반복문 탈출 
				if (tn == (k + 1)){
					text[0] = '\0';
					strcpy(text, cpy);
					score += plus_score * (tmp);
					sprintf(sc, "%d", score);
					strcat(text, sc);
					printf(text);
					close(dot_d);
					clcd_input(text);
					printf("1점 획득, 총 점수: %d\n", score);
					
					dot_d = open(dot, O_RDWR);
					if(dot_d < 0){
						printf("inner dot_d open error_1\n");
						return 0;
					}
					break;
				}
				// 틀린 버튼 입력 시 목숨 감소 
				else{
					close(dot_d);
					value.life = declife(value.life, value.start_dif);
					printf("목숨 감소, 목숨 개수: %d\n", value.life);
					
					dot_d = open(dot, O_RDWR);
				}
			}
			else{
				timer = timer + 1;
			}
			
		}
		// 시간 초과될 때 까지 버튼 못누를 시 목숨 감소 
		if(timer > 80/tmp - 1){
			close(dot_d);
			value.life = declife(value.life, value.start_dif);
			printf("시간 초과 목숨 감소, 목숨 개수: %d\n", value.life);
		
			dot_d = open(dot, O_RDWR);
		}
	}
	// segment에 최종 점수 출력 
	close(dot_d);
	close(tactswFd);
	clcd_input("GAME OVER");
	fnd_print(score);
	sleep(5);
	return 0;
}		

values start(){
	// 게임 시작 및 종료 여부 질의 
	int num, life, start_dif;
	int led_long; // chipled를 열기 위한 변수 
	values value;
	clcd_input("START:1 END:ANY KEY");
	num = dipsw_input();
	
	if (num != 1){
		exit(0);
	}
	
	// 스위치 원상복귀 
	while(1){
		clcd_input("switch back to original");
		num = dipsw_input_x();
		if(num == 0){
			break;
		}
	}
	
	//목숨 설정
	clcd_input("Life setting");
	life = dipsw_input();
	led_control((255 - life));
	
	//시작 난이도 설정 
	clcd_input("Difficulty setting");
	start_dif = dipsw_input();
	while(start_dif == life) {
    		start_dif = dipsw_input();
    		if(start_dif != life) break;
    		usleep(200000);
	}
	
	// 목숨, 난이도 설정이 끝나면 3초 동안 LED 점등	
	led_long = open(led, O_RDWR);
	
	if(led_long < 0){
		printf("led 디바이스 드라이버가 없습니다.");
	}
	int t = (255 - start_dif);
	 
	write(led_long, &t, sizeof(unsigned char));
	sleep(3);
	close(led_long);

	start_dif = start_dif - life;
	
	// life와 start_dif 값 리턴
	value.life = life;
	value.start_dif = start_dif;
	
	return value; 
}

// 목숨 감소
int declife(int l, int d){
    	l = l >> 1;
    	if(l <= 0) {
    		close(tactswFd);
    	    clcd_input("GAME OVER");
    	    fnd_print(score);
    	    sleep(5);
    	    exit(0);
    	}
    	led_control((255 -(l+d)));
    	return l;
}


// clcd 조작 함수 
void clcd_input(char clcd_text[]){
	int clcd_d;
	clcd_d = open(clcd, O_RDWR);
	
	if(clcd_d < 0){
		printf("clcd 디바이스 드라이버가 없습니다.\n");
	}
	
	write(clcd_d, clcd_text, strlen(clcd_text));
	close(clcd_d);
}

// dip 스위치 입력 함수
int dipsw_input(){
	int dip_d;
	unsigned char c;
	
	dip_d = open(dip, O_RDWR);
	
	while(1){
		read(dip_d, &c, sizeof(c));
		if(c)
			break;
		usleep(200000);
	}
	close(dip_d);
	return c;
}

// dip 스위치 초기화
int dipsw_input_x(){
	int dip_d;
	unsigned char c;
	
	dip_d = open(dip, O_RDWR);
	
	while(1){
		read(dip_d, &c, sizeof(c));
		if(c == 0)
			break;
		usleep(200000);
	}
	close(dip_d);
	return c;
}

// chip LED 출력 함수
void led_control(int sw_num){
	int led_d;
	unsigned char data;
	
	led_d = open(led, O_RDWR);
	
	if(led_d < 0){
		printf("led 디바이스 드라이버가 없습니다.");
	}
	 
	write(led_d, &sw_num, sizeof(unsigned char));
	usleep(200000);
	led_d = close(led_d);
} 

// 목숨 선택에 따른 점수 가중치 함수 
int calc_score(int life){
	int plus_score = 0;
	switch(life){ 
		case 1: plus_score = 4; break;
		case 2: plus_score = 2; break;
		case 4: plus_score = 1; break;
		case 8: plus_score = 1; break;
	}
	return plus_score;
}

// fnd에 최종 score를 출력해주는 함수
int fnd_print(int score){
	int fnd_d, k;
	unsigned char fnd_data[4];
	
	fnd_d = open(fnd, O_RDWR);
	
	if(fnd_d < 0){
		printf("fnd 디바이스 드라이버가 없습니다.\n");
	}
	
	printf("%d", score);
	
	if (score < 10){
		for (k = 0; k < 3; ++k)
			fnd_data[k] = ~0x00;
		
		fnd_data[3] = fnd_input(score);
	}
	
	else if (score >= 10 && score < 100){
		for (k = 0; k < 2; ++k)
			fnd_data[k] = ~0x00;
		
		fnd_data[2] = fnd_input(score/10);
		printf("%d", score/10);
		
		fnd_data[3] = fnd_input(score%10);
		printf("%d", score%10);
	}
	
	else if (score >= 100 && score < 1000){
		fnd_data[0] = ~0x00;
		fnd_data[1] = fnd_input((score % 1000) / 100);
		fnd_data[2] = fnd_input((score % 100) / 10);
		fnd_data[3] = fnd_input(score % 10);
	}
	
	write(fnd_d, fnd_data, sizeof(fnd_data));
	sleep(5);
	
	close(fnd_d);
	return 0;
}

// fnd_data에 넣을 값을 반환해주는 함수
unsigned char fnd_input(int num){
	unsigned char c;
	switch(num){
		case 0: c = ~0x3f; break;
		case 1: c = ~0x06; break;
		case 2: c = ~0x5b; break;
		case 3: c = ~0x4f; break;
		case 4: c = ~0x66; break;
		case 5: c = ~0x6d; break;
		case 6: c = ~0x7d; break;
		case 7: c = ~0x07; break;
		case 8: c = ~0x7f; break;
		case 9: c = ~0x67; break;
		default: c = ~0x00; break;
	}
	return c;
}
