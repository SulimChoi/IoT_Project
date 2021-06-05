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

static char tactswDev[] = "/dev/tactsw";
static int tactswFd = (-1);
 
void clcd_input(char clcd_text[]);
int dipsw_input();
int dipsw_input_x();
unsigned char tactsw_get(int tmo);
int tactsw_input();
void led_control(int sw_num);
int declife(int l, int d);
int calc_score(int life);

typedef struct _values{
	int life;
	int start_dif;
}values;

values start();

int main(){
	values value;
	int dot_d, i, j, k; 
	int score = 0;
	int tn = 0; // 수정(tn: tact num)
	int plus_score, tmp; //목숨과 난이도에 따른 점수 가중치 
	char sc[20];
	char text[100], cpy[] = "score: " ; //clcd 점수 출력문 
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
	clcd_input("game start");
	printf("목숨 개수: %d, 난이도: %d\n", value.life, value.start_dif);
	sleep(5);
	
	clcd_input("score: 0");
	
	dot_d = open(dot, O_RDWR);
	
	if(dot_d < 0){
		printf("error\n");
		return 0;
	}
	
	for (j=0; j<15; j++){
		srand((unsigned int)time(NULL));
		k = (int)rand()%9;
		
		unsigned char b;
		int timer = 0;
		
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
					
					if (value.life <= 0){
						printf("게임 종료");
						sleep(5);
						exit(0);
					}
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
		
			if (value.life <= 0){
				printf("시간 초과 게임 종료");
				sleep(5);
				exit(0);
			}
			dot_d = open(dot, O_RDWR);
		}
	}
	close(dot_d);
	return 0;
}		

values start(){
	// 게임 시작 및 종료 여부 질의 
	int num, life, start_dif;
	values value;
	clcd_input("start:1 end:anykey");
	num = dipsw_input();
	
	if (num != 1){
		exit(0);
	}
	
	// 스위치 원상복귀 
	while(1){
		clcd_input("switch back to original condition");
		num = dipsw_input_x();
		if(num == 0){
			break;
		}
	}
	
	//목숨 설정
	clcd_input("life setting");
	life = dipsw_input();
	led_control((255 - life));
	
	//시작 난이도 설정 
	clcd_input("difficulty setting");
	start_dif = dipsw_input();
	while(start_dif == life) {
    		start_dif = dipsw_input();
    		if(start_dif != life) break;
    		usleep(200000);
	}
	led_control((255 - start_dif));
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
    	    clcd_input("GAME OVER");
    	    sleep(3);
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
