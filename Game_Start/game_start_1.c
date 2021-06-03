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
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

# define clcd "/dev/clcd"
# define dip "/dev/dipsw"
# define led "/dev/led"

void clcd_input(char clcd_text[]);
int dipsw_input();
int dipsw_input_x();
void led_control(int sw_num);
int declife(int l, int d);

int main(){
	// 게임 시작 및 종료 여부 질의 
	int num, life, start_dif;
	clcd_input("start:1 end:anykey");
	num = dipsw_input();
	
	if (num != 1){
		//return 0;
		exit(0);
	}
	// 스위치 원상복귀 
	while(1){
		clcd_input("switch back to original condition");
		usleep(1000000);
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

	while(1){
		usleep(900000);
		usleep(900000);
		usleep(900000);
		break;
	}	
	
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
} 

// 목숨 감소 
int declife(int l, int d){
	l = l >> 1;
	led_control(l+d);
	return l;
}
