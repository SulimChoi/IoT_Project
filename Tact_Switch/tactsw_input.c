// tact switch 입력 함수
int tactsw_input(){
	unsigned char c;
	int selected_tact = 0; // false 값 넣기

	if((tactswFd = open( tactswDev, O_RDONLY )) < 0){    
		perror("tact error");
		exit(-1);
	}

	// 1~9번 스위치 사용
	while(1){
		c = tactsw_get(10);
		switch (c) {
				case 1:  selected_tact = 1 ; break; // 1번 구역
				case 2:  selected_tact = 2 ; break; // 2번 구역
				case 3:  selected_tact = 3 ; break; // 3번 구역
				case 4:  selected_tact = 4 ; break; // 4번 구역
				case 5:  selected_tact = 5 ; break; // 5번 구역
				case 6:  selected_tact = 6 ; break; // 6번 구역
				case 7:  selected_tact = 7 ; break; // 7번 구역
				case 8:  selected_tact = 8 ; break; // 8번 구역
				case 9:  selected_tact = 9 ; break; // 9번 구역
				default: printf("press other key\n", c); break; // 기본값 메세지
		}
		return selected_tact; // 어떤 스위치가 눌렸는지 int 형으로 반환함
		}
}
