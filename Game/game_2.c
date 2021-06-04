#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <string.h>
#include <asm/ioctls.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <time.h>

# define dot "/dev/dot"
# define clcd "/dev/clcd"
# define dip "/dev/dipsw"
# define led "/dev/led"
static char tactswDev[] = "/dev/tactsw"; // 수정
static int tactswFd = (-1); // 수정
 

void clcd_input(char clcd_text[]);
int dipsw_input();
int dipsw_input_x();
void led_control(int sw_num);
int declife(int l, int d);

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
		{0x24, 0x24, 0xFF, 0x24, 0x24, 0xFF, 0x27, 0x27} // 9번
	};
	value = start();
	printf("목숨 개수: %d, 난이도: %d\n", value.life, value.start_dif);
	sleep(5);
	
	dot_d = open(dot, O_RDWR);
	
	if(dot_d < 0){
		printf("error\n");
		return 0;
	}
	
	for (j=0; j<10; j++){
		srand((unsigned int)time(NULL));
		k = (int)rand()%9;
		
		unsigned char b;
		int timer = 0;
		
		while(timer < 30){
			write(dot_d, &game[k], sizeof(game[k]));
			usleep(100000);
			
			tactswFd = open(tactswDev, O_RDONLY);
			
			if(tactswFd<0){
				printf("open failed\n");
			}
			
			read(tactswFd, &b, sizeof(b));
			
			if(b){
				tn = b;
				close(tactswFd);
				printf("%d, %d\n", (k + 1), tn);
				
				if (tn == (k + 1)){
					score += 1;
					printf("1점 획득, 총 점수: %d\n", score);
					break;
				}
				else{
					value.life = declife(value.life, value.start_dif);
					printf("목숨 감소, 목숨 개수: %d\n", value.life);
					if (value.life <= 0){
						printf("게임 종료");
						sleep(5);
						exit(0);
					}
				}
			}
			else{
				timer = timer + 1;
			}
			
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
		//return 0;
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

// 목숨 감소 (수정)
int declife(int l, int d){
    	l = l >> 1;
    	if(l <= 0) {
    	    clcd_input("GAME FAIL");
    	    sleep(3);
    	    exit(0);
    	}
    	led_control(l+d);
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
