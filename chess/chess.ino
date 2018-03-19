#include <TFT.h>
#include <SPI.h>
#define CS 10
#define DC 9
#define RST 8

struct p{
public:
	byte type;
	uint16_t color;
	p(uint16_t _color, uint8_t _type){
		color = _color;
		type = _type;
	}
};

uint16_t ally=0xFFFF;
uint16_t enemy=0x0000;
const byte cursor[] = { 6, 7, 2, 5 };//left, up, dn, right
const byte select1 = A1, select2=A2;
const byte piezo = 3;
byte efield[8] = { 0 }, afield[8] = {0};//king이 이동불가능한 필드
byte ramX=8, ramY=8, fixX=8, fixY=8;
byte castlingState = 0, queenSideR = 0, kingSideR = 0, kingX = 4, kingY = 7;
byte flag=0, count=0, sel=0, aCheckState=0, eCheckState=0;
TFT display = TFT(CS, DC, RST);
p* board[8][8];
int8_t  eKingX=4, eKingY=0;
int8_t stack[27][2];
int8_t temp = -1,top = -1, buffer[5];//buffer 0:befY, 1:befX, 2:movY, 3:movX
int8_t X = 0, Y = 7, pp;
int8_t attackPath[8][2], deffendMove[8][2];


//0=pawn, 1=bishop, 2=knight, 3=rook, 4=queen, 5=king
uint16_t piece[6][16] = 
{ { 0x0000, 0x0000, 0x0000, 0x0000, 0x03C0, 0x07E0, 0x07E0, 0x07E0,
0x03C0, 0x0180, 0x03C0, 0x07E0, 0x07E0, 0x0FF0, 0x1FF8, 0x0000 }
, { 0x0000, 0x0000, 0x0180, 0x0380, 0x0760, 0x0EF0, 0x07E0, 0x03C0, 
0x07E0, 0x03C0, 0x03C0, 0x07E0, 0x0FF0, 0x1FF8, 0x0000, 0x0000 }
, { 0x0000, 0x0040, 0x01C0, 0x02C0, 0x07E0, 0x0FF0, 0x05F0, 0x01E0,
0x01E0, 0x01C0, 0x07E0, 0x0FF0, 0x1FF8, 0x1FF8, 0x0000, 0x0000 }
, { 0x0000, 0x0000, 0x1998, 0x1FF8, 0x1FF8, 0x07E0, 0x07E0, 0x07E0,
0x07E0, 0x07E0, 0x07E0, 0x0FF0, 0x1FF8, 0x1FF8, 0x0000, 0x0000 }
, { 0x0000, 0x0000, 0x1008, 0x1998, 0x0FF0, 0x07E0, 0x03C0, 0x03C0,
0x0180, 0x03C0, 0x03C0, 0x07E0, 0x0FF0, 0x1FF8, 0x0000, 0x0000 }
, { 0x0000, 0x0180, 0x03C0, 0x0180, 0x0DB0, 0x07E0, 0x03C0, 0x03C0,
0x03C0, 0x03C0, 0x07E0, 0x07E0, 0x0FF0, 0x1FF8, 0x0000, 0x0000 }
};

void setup(){
	Serial.begin(9600);
	initDisplay();
	for (int8_t i = 3; i >= 0; i--)
		pinMode(cursor[i], INPUT_PULLUP);
	pinMode(select1, INPUT_PULLUP);
	pinMode(select2, INPUT_PULLUP);
	pinMode(piezo, OUTPUT);//기본설정
raven:
	mainMenu();//메인메뉴
	if (Serial.available())
		clearBuffer();//버퍼 모두 제거
	if (dice()){
		clearBuffer();
		clearAllElements();
		goto raven;
	}
	clearAllElements();
	clearBuffer();
	display.noStroke();
	drawBoard(0, 16);
	initBoard();
	drawPieceOnBoard();
	if (ally == 0xFFFF)
		drawCursor(0, 7);
}

void loop(){
	if (flag==1){//플래그가 쥐어졌을 때
		for (size_t i = 0; i < 4; i++){
			if (digitalRead(cursor[i]) == LOW){
				switch (i){
				case 0://left
					if (0 < X)
						X--;
					break;
				case 1://up
					if (0 < Y)
						Y--;
					break;
				case 2://down
					if (Y < 7)
						Y++;
					break;
				case 3://right
					if (X < 7)
						X++;
					break;
				}
				clearCursor();
				drawCursor(X, Y);
				sound();
				delay(150);
				break;
			}
		}
		if (digitalRead(select1) == LOW){
			sound();
			if (click()){//true라면
				gameEnd(1, eCheckState);
				return;
			}
		}
	}
	else if (Serial.available()){//플래그가 쥐어지지 않았을 때
		if (Serial.peek() != 'A'){
			buffer[count] = Serial.read() - 48;
			Serial.print(buffer[count]);
		}
		else{
			Serial.read();
			count++;
		}
		if (count == 5){
			flag = 1;
			count = 0;
			if (converter()){
				gameEnd(0, aCheckState);
				flag = 0;
				return;
			}
			clearBuffer();
			drawCursor(X,Y);
		}
	}
}

void clearAllElements(){
	display.fillScreen(0);
}

void mainMenu(){
	display.setTextSize(3);
	display.stroke(255,255,255);
	display.text("Chess", 10, 10);
	display.fill(255, 255, 255);
	for (int8_t y = 15; y >= 0; y--){
		for (int8_t x = 15; x >= 0; x--){
			if (1 << (15 - x)&(*(piece[2] + y)))
				display.rect(40+x*3, 36+y*3, 3, 3);
		}
	}
	display.triangle(24, 72, 15, 86, 40, 86);
	display.triangle(24, 72, 40, 86, 45, 72);
	display.triangle(82, 72, 88, 86, 98, 72);
	display.triangle(88, 86, 98, 72, 115, 86);
	display.triangle(41, 87, 27, 118, 105, 118);
	display.triangle(41, 87, 105, 118, 87, 87);
	unsigned long current, prev = 0;
	byte loop=1;
	byte default_ = 0xFF;
	byte button_s[] = {cursor[0],cursor[1],cursor[2],cursor[3],select1, select2};
	display.stroke(default_, default_, default_);
	display.setTextSize(1);
	display.text("ver1.0.3",80,153);
	display.text("TM", 98, 25);
	while (loop){
		current = millis();
		if (current - prev > 600){
			default_=~default_;
			display.stroke(0, default_, 0);
			display.text("click button~!!", 25, 128);
			prev = current;
		}
		for (int8_t i = 5; i >= 0;i--){
			if (digitalRead(button_s[i]) == LOW){
				sound();
				delay(100);
				loop = 0;
				break;
			}
		}
	}
	clearAllElements();
	display.noStroke();
}

void gameEnd(byte op1, byte checkState){
	for (int8_t y = 7; y >= 0; y--){
		for (int8_t x = 7; x >= 0;x--){
			if (board[y][x] != NULL)
				delete board[y][x];//메모리 해제
		}
	}
	drawGameEndBar(op1, checkState);
}

void drawGameEndBar(byte op, byte checkState){
	display.stroke(255, 255, 255);
	display.line(0,75,128,75);
	display.line(0, 86, 128, 86);
	display.noStroke();
	display.fill(0, 0, 0);
	display.rect(0,76,128,10);
	display.textSize(1);
	if (op){
		if (checkState){
			display.stroke(0, 0, 255);
			display.text("CheckMate~!! YOU WIN~", 0, 78);
		}
		else{
			display.stroke(0, 255, 0);
			display.text("StaleMate~!! DRAW...", 0, 78);
		}
	}
	else{
		if (checkState){
			display.stroke(255, 0, 0);
			display.text("CheckMate~!! YOU LOSE", 0, 78);
		}
		else{
			display.stroke(0, 255, 0);
			display.text("StaleMate~!! DRAW...", 0, 78);
		}
	}
	display.noFill();
}

void sound(){
	analogWrite(piezo, 6);
	delay(50);
	digitalWrite(piezo, LOW);
}

void _sound(){
	analogWrite(piezo, 12);
	delay(50);
	digitalWrite(piezo, LOW);
}

void clearBuffer(){
	byte trash;
	while (Serial.available())
		trash=Serial.read();
}

byte converter(){//packet: befY, befX, movY, movX, type
	if (eCheckState){//적의 체크상태 해제
		clearCheck(eKingX, eKingY, enemy);
		eCheckState = 0;
	}
	for (int8_t i = 3; i >= 0; i--)
		buffer[i] = 7 - buffer[i];
	uint8_t data = clear(buffer[1], buffer[0]);
	display.noStroke();
	display.fill(data, data, data);
	display.rect(buffer[1] * 16 + 1, (buffer[0]+1) * 16 + 1, 14, 14);
	if (board[buffer[2]][buffer[3]] != NULL){//이동하는 자리에 말이 존재한다면
		delete board[buffer[2]][buffer[3]];//메모리 해제
		data = clear(buffer[3], buffer[2]);
		display.fill(data, data, data);
		display.rect(buffer[3] * 16 + 1, (buffer[2]+1) * 16 + 1, 14, 14);
	}
	if (buffer[4] == 5){//캐슬링인지
		eKingX = buffer[3];
		eKingY = buffer[2];
		if (buffer[3]==buffer[1]-2){
			board[buffer[2]][buffer[1] - 1] = board[0][0];
			board[0][0] = NULL;
			drawPiece(buffer[1] - 1, buffer[2],piece[3],enemy);
			uint8_t data = clear(0, 0);
			display.fill(data, data, data);
			display.rect(0 * 16, 16 + 0 * 16, 16, 16);
		}
		else if (buffer[3] == buffer[1] + 2){
			board[buffer[2]][buffer[1] + 1] = board[0][7];
			board[0][7] = NULL;
			drawPiece(buffer[1] + 1, buffer[2], piece[3], enemy);
			uint8_t data = clear(7, 0);
			display.fill(data, data, data);
			display.rect(7 * 16, 16 + 0 * 16, 16, 16);
		}
	}
	board[buffer[2]][buffer[3]] = board[buffer[0]][buffer[1]];
	board[buffer[0]][buffer[1]] = NULL;
	if (buffer[2] == 7 && board[buffer[2]][buffer[3]]->type == 0)
		board[buffer[2]][buffer[3]]->type=buffer[4];
	drawPiece(buffer[3], buffer[2], piece[buffer[4]], enemy);
	_sound();
	for (int8_t i = 7; i >= 0; i--)//clear
		efield[i] = 0;
	setField(enemy,efield,1);//set
	if (efield[kingY] & (0x01 << kingX)){//check라면
		aCheckState=check(buffer[3], buffer[2], ally, kingX, kingY, 1);//checkPath 구함
		if (aCheckState == 0){
			exception();
		}
	}
	display.noFill();
	if (ePieceMovable(-1, efield, ally, aCheckState, kingX, kingY))//아군 말이 이동할 수 있는 필드가 없다면
		return 1;
	return 0;
}

void exception(){
	int8_t array1[4][4] = { { 1, 1, 8, 8 }, {-1,1,-1,8}, {-1,-1,-1,-1}, {1,-1,8,-1} };
	int8_t array2[4][5] = { { 1, 1, kingX,kingY,8}, { 1, -1,kingX,kingY, -1}, { 0, 1,kingY,kingX, 8}, { 0, -1,kingY,kingX, -1 } };
	for (int8_t i = 3; i >= 0;i--){
		temp = -1;
		if (cross1(kingY + array1[i][0], kingX + array1[i][1], array1[i][2], array1[i][3], array1[i][0], array1[i][1], enemy, 1, 4)){
			change();
			return;
		}
	}
	for (int8_t i = 3; i >= 0; i--){
		temp = -1;
		if (cross2(array2[i][0], array2[i][3] + array2[i][1], array2[i][2], array2[i][4],array2[i][1], enemy, 3, 4)){
			change();
			return;
		}
	}
}

void change(){
	pp = -1;
	while (temp != -1){
		pp++;
		attackPath[pp][0] = deffendMove[temp][0];
		attackPath[pp][1] = deffendMove[temp][1];
		temp--;
	}
	drawCheck(kingX, kingY, ally);
	aCheckState = 1;
}

void sendData(){
	unsigned long current, prev = 0;
	byte buffer[] = { fixY , fixX, ramY, ramX, board[ramY][ramX]->type};
	while (true){
		current = millis();
		if (count == 5){
			count = 0;
			clearBuffer();
			break;
		}
		if (current - prev > 500){
			Serial.print(buffer[count]);
			prev = current;
		}
		if (Serial.available()){
			if (Serial.read() - 48 == buffer[count]){
				count++;
				Serial.print('A');
			}
		}
	}
}

void push(int8_t x, int8_t y){//0->x, 1->y
	top++;
	stack[top][0] = x;
	stack[top][1] = y;
}

void _push(int8_t x, int8_t y){
	temp++;
	deffendMove[temp][0] = x;
	deffendMove[temp][1] = y;
}

void initDisplay(){//init display
	display.begin();
	display.background(0, 0, 0);
	display.setRotation(2);
}

//160*128 size
void drawBoard(size_t _x, size_t _y){
	uint8_t field = 0xFF;
	for (int8_t y = 128; y >= _y; y -= 16){
		for (size_t x = _x; x <= 132; x += 16){
			if (field == 0x00)
				display.fill(190, 190, 190);
			else
				display.fill(135, 135, 135);
			display.rect(x, y, 16, 16);
			field = ~field;
		}
	}
	display.noFill();
}

void drawPieceOnBoard(){
	for (int8_t y = 6; y >= 0; y -= 6){
		for (int8_t x = 7; x >= 0; x--){
			drawPiece(x, y, piece[board[y][x]->type], board[y][x]->color);
			drawPiece(x, y + 1, piece[board[y + 1][x]->type], board[y + 1][x]->color);
		}
	}
}

void drawPiece(int8_t _x, int8_t _y, uint16_t *piece, uint16_t color){
	int8_t col = (_x * 0x0F) + (0x01 * _x);
	uint8_t row = 0x11 + (_y * 0x10);
	if (_y == 7 && board[_y][_x]->color == enemy)
		row =129;
	else if (_y != 0 && board[_y][_x]->type==0)
		row --;
	else if (_y == 0 && board[_y][_x]->type == 0)
		row --;
	draw(col, row, piece, color);
}

void drawPiece(int8_t _x,uint16_t *piece, uint16_t color){
	int8_t col = 11 + 18 * _x;
	int8_t row = 69;
	draw(col, row, piece, color);
}

void draw(int8_t col, uint8_t row, uint16_t *piece, int16_t color){
	for (int8_t y = 15; y >= 0; y--){
		for (int8_t x = 15; x >= 0; x--){
			if (1 << (15 - x)&(*(piece + y)))
				display.drawPixel(col + x, row + y, color);
		}
	}
}

void drawMovable(){
	int8_t data[2];
	int8_t tmp = top;
	display.fill(0, 0, 190);
	while (tmp != -1){
		data[0] = stack[tmp][0];
		data[1] = stack[tmp][1];
		tmp--;
		display.rect(data[0] * 16+1, (data[1]+1) * 16+1, 14,14);
		if (board[data[1]][data[0]] != NULL && board[data[1]][data[0]]->color!=ally)//보드에 말이 있는경우 다시그림
			drawPiece(data[0], data[1], piece[board[data[1]][data[0]]->type], enemy);
	}
	display.noFill();
}

void clearMovable(uint8_t x, uint8_t y, byte click){
	if (click == 0)
		return;
	uint8_t _data;
	int8_t data[2];
	while (top != -1){
		data[0] = stack[top][0];
		data[1] = stack[top][1];
		top--;
		_data = clear(data[0], data[1]);
		display.noStroke();
		display.fill(_data, _data, _data);
		display.rect(data[0] * 16 + 1, (data[1] + 1) * 16 + 1, 14, 14);
		if (board[data[1]][data[0]] != NULL)//보드에 말이 있는경우 다시그림
			drawPiece(data[0], data[1], piece[board[data[1]][data[0]]->type], enemy);
	}
	display.noFill();
}

void drawFixCursor(){
	display.stroke(0, 0, 255);
	display.rect(fixX * 16, 16 + fixY * 16, 16, 16);
}

void clearFixCursor(){
	uint8_t data;
	if (fixX != 8){
		data = clear(fixX, fixY);
		display.stroke(data, data, data);
		display.rect(fixX * 16, 16 + fixY * 16, 16, 16);
	}
}

void drawCursor(uint8_t x, uint8_t y){
	ramX = x;
	ramY = y;
	display.stroke(0, 0, 0);
	display.rect(ramX * 16, 16 + ramY * 16, 16, 16);
}

void clearCursor(){
	uint8_t data;
	if (ramX != 8){
		if ((ramX == fixX) && (ramY == fixY)){
			drawFixCursor();
			return;
		}
		data = clear(ramX, ramY);
		display.stroke(data, data, data);
		display.rect(ramX * 16, 16 + ramY * 16, 16, 16);
	}
}

uint8_t clear(uint8_t x, uint8_t y){
	byte point = 135;
	if ((y + 1) % 2 == 1)
		point += 65;
	if ((x + 1) % 2 == 0){
		if (point == 135)
			point += 65;
		else
			point -= 65;
	}
	return point;
}

byte dice(){//wireless, set whoes turn is first
	byte my, other, link = 0;
	randomSeed(analogRead(A0));//seed설정
	my = (byte)random(0, 9);
	unsigned long current1, prev1 = 0, current2, prev2 = 0;
	byte default1 = 0xFF, default2 = 0xFF, count = 0;
	byte button_s[] = { cursor[0], cursor[1], cursor[2], cursor[3], select1, select2 };
	char* str[] = { "s", "e", "a", "r", "c", "h", "-", "p", "l", "a", "y", "e", "r" };
	while (true){
		current1 = millis();
		current2 = millis();
		if (current1 - prev1 > 300){//0.3sec마다 search-player문자 출력
			outputWord1(str, &default1, &count);
			prev1 = current1;
		}
		if (current2 - prev2 > 700){//0.7sec마다 데이터 전송, 문자 출력
			Serial.print(my);
			if (link)
				return 0;
			outputWord2(&default2);
			prev2 = current2;
		}
		for (int8_t i = 5; i >= 0; i--){//button read
			if (digitalRead(button_s[i]) == LOW){
				sound();
				delay(100);
				return 1;
			}
		}
		if (Serial.available()){
			if (link == 0){
				Serial.print(my);
				other = Serial.read() - 48;
				if (comparison(other, my)){
					link = 1;
				}
				else{
					randomSeed(analogRead(A0));
					my = (byte)random(0, 9);
				}
			}
		}
	}
}

byte comparison(byte other, byte my){
	if (other == my)
		return 0;
	else if (other < my){
		flag = 1;
		return 1;
	}
	else if (other > my){
		kingX = 3;
		eKingX = kingX;
		ally = ~ally;
		enemy = ~enemy;
		return 1;
	}
}

void outputWord1(char* str[], byte* default1, byte* count){
	display.stroke(0, *default1, 0);
	display.text(str[*count], 25 + (*count) * 6, 72);
	(*count)++;
	if (*count == 13){
		*default1 = ~(*default1);
		*count = 0;
	}
}

void outputWord2(byte* default2){
	*default2 = ~(*default2);
	display.stroke(*default2, *default2, *default2);
	display.text("click button,", 25, 128);
	display.text("then go to menu", 15, 136);
}

void setField(uint16_t color, byte* field, int8_t v){
	for (int8_t y = 7; y >= 0; y--){
		for (int8_t x = 7; x >= 0; x--){
			if (board[y][x] == NULL)
				continue;
			else if (board[y][x]->color == color)
				enableField(x, y, color, field, v);
		}
	}
}

void extraction(){//체크상태일때 공격경로를 방어할 수 있는 좌표를 추출
	int8_t temporary[8][2],tmp, count = -1;
	while (top != -1){
		tmp = pp;
		while (tmp != -1){
			if (attackPath[tmp][0] == stack[top][0] && attackPath[tmp][1] == stack[top][1]){
				count++;
				temporary[count][0] = stack[top][0];
				temporary[count][1] = stack[top][1];
				break;
			}
			tmp--;
		}
		top--;
	}
	top = count;
	while (count != -1){
		stack[count][0] = temporary[count][0];
		stack[count][1] = temporary[count][1];
		count--;
	}
}

void _extraction(){
	int8_t temporary[8][2], tmp, count = -1;
	while (top != -1){
		tmp = temp;
		while (tmp != -1){
			if (deffendMove[tmp][0] == stack[top][0] && deffendMove[tmp][1] == stack[top][1]){
				count++;
				temporary[count][0] = stack[top][0];
				temporary[count][1] = stack[top][1];
				break;
			}
			tmp--;
		}
		top--;
	}
	top = count;
	while (count != -1){
		stack[count][0] = temporary[count][0];
		stack[count][1] = temporary[count][1];
		count--;
	}
	temp = -1;
}

void loadPiece(uint8_t _x, uint8_t _y, int8_t v, byte* field, uint16_t color, byte checkState, int8_t kingX, int8_t kingY){
	switch (board[_y][_x]->type){
	case 0:
		pawnField(_x, _y, v, color, checkState,kingX, kingY);
		break;
	case 1:
		bishopField(_x, _y, color, checkState, kingX, kingY);
		break;
	case 2:
		knightField(_x, _y, color, checkState, kingX, kingY);
		break;
	case 3:
		rookField(_x, _y, color, checkState, kingX, kingY);
		break;
	case 4:
		queenField(_x, _y, color, checkState, kingX, kingY);
		break;
	case 5:
		kingField(_x, _y, field, color);
		break;
	}
}

void initBoard(){
	uint16_t bit = enemy;
	uint8_t pawn=0x01, general=0x00;
	for (int8_t i = 1; i >= 0; i--){
		for (int8_t x = 7; x >= 0; x--)
			board[pawn][x] = new p(bit, 0);
		for (int8_t x = 2; x >= 0; x--){
			board[general][x] = new p(bit, 3 - x);
			board[general][7-x] = new p(bit, 3 - x);
		}
		for (int8_t x = 1; x >= 0; x--){
			if (ally == 0x0000){
				board[general][x + 3] = new p(bit, 5 - x);
				continue;
			}
			board[general][x + 3] = new p(bit, 4 + x);
		}
		pawn = 0x06;
		general = 0x07;
		bit = ally;
	}
	for (int8_t y = 5; y >= 2; y--){
		for (int8_t x = 7; x >= 0; x--)
			board[y][x] = NULL;
	}
}

byte importantPiece(int8_t x, int8_t y,int8_t kingX, int8_t kingY, uint16_t color){
	temp = -1;
	if (abs(x - kingX) == abs(y - kingY)){//대각선상에 위치한다면
		if (x < kingX&&y < kingY){//leftUp
			if (cross1(y + 1, x + 1, 8, 8, 1, 1, color, 5, -1))//rightDown
				return cross1(y - 1, x - 1, -1, -1, -1, -1, ~color, 1, 4);//leftup
		}
		else if (x < kingX&&y > kingY){//leftDown
			if (cross1(y - 1, x + 1, -1, 8, -1, 1, color, 5, -1))//rightUp
				return cross1(y+1,x-1,8,-1,1,-1,~color,1,4);//leftDown
		}
		else if (x > kingX&&y > kingY){//rightDown
			if (cross1(y - 1, x - 1, -1, -1, -1, -1, color, 5, -1))//leftUp
				return cross1(y+1,x+1,8,8,1,1,~color,1,4);//rightDown
		}
		else if (x > kingX&&y < kingY){//rightUp
			if (cross1(y + 1, x - 1, 8, -1, 1, -1, color, 5, -1))//leftDown
				return cross1(y-1,x+1,-1,8,-1,1,~color,1,4);//rightUp
		}
	}
	else if (x == kingX || y == kingY){//직선상에 위치한다면
		if (x == kingX && kingY > y){//up
			if (cross2(1, y + 1, x, 8, 1, color, 5, -1))//down
				return cross2(1,y-1,x,-1,-1,~color,3,4);//up
		}
		else if (x == kingX && kingY < y){//down
			if (cross2(1, y - 1, x, -1, -1, color, 5, -1))//up
				return cross2(1,y+1,x,8,1,~color,3,4);//down
		}
		else if (y == kingY && kingX > x){//left
			if (cross2(0, x + 1, y, 8, 1, color, 5, -1))//right
				return cross2(0, x-1,y,-1,-1,~color,3,4);//left
		}
		else if (y == kingY && kingX < x){//right
			if (cross2(0, x - 1, y, -1, -1, color, 5, -1))//left
				return cross2(0,x+1,y,8,1,~color,3,4);//right
		}
	}
	return 0;
}

byte cross1(int8_t startY, int8_t startX, int8_t endY, int8_t endX, int8_t opY, int8_t opX, uint16_t color, int8_t type1, int8_t type2){
	for (int8_t sY = startY, sX = startX; sY != endY && sX != endX; sY+=opY, sX+=opX){
		if (board[sY][sX] != NULL){
			if (board[sY][sX]->color == color && (board[sY][sX]->type == type1 || board[sY][sX]->type == type2)){
				if (type1 != 5)
					_push(sX, sY);
				return 1;
			}
			else
				return 0;
		}
		_push(sX, sY);
	}
	return 0;
}

byte cross2(byte dir, int8_t path, int8_t fixValue,int8_t endPath, int8_t op, uint16_t color, int8_t type1, int8_t type2){
	int8_t v, point[2];
	if (dir==1){//y라면
		v = 1;
		point[0] = fixValue;
		point[1] = path;
	}
	else{
		v = 0;
		point[0] = path;
		point[1] = fixValue;
	}
	for (; point[v]!= endPath; point[v]+=op) {
		if (board[point[1]][point[0]] != NULL){
			if (board[point[1]][point[0]]->color == color && 
				(board[point[1]][point[0]]->type == type1 || board[point[1]][point[0]]->type == type2)){
				if (type1 != 5)
					_push(point[0], point[1]);
				return 1;
			}
			else
				return 0;
		}
		_push(point[0], point[1]);
	}
	return 0;
}

void pawnField(int8_t x, int8_t y, int8_t v, uint16_t color, byte checkState, int8_t kingX, int8_t kingY){
	int8_t dir=0, rev=-1, row=6;
	if (v == 1)
		row = 1;
	if (y + v >= 0 && board[y + v][x] == NULL){ //if front is NULL
		push(x, y - 1);
		if ((y == 6) && (board[y + (v*2)][x] == NULL))//if front of front is NULL
			push(x, y + (v * 2));
	}
	for (int8_t i = 1; i >= 0; i--){
		if (x != dir && board[y -1][x +rev] != NULL){
			if (board[y + v][x+rev]->color==~color)
				push(x + rev, y + v);
		}
		dir = 7;
		rev = 1;
	}
	if (importantPiece(x,y,kingX,kingY,color))
		_extraction();
	if (checkState==1)
		extraction();
}

void knightField(int8_t x, int8_t y, uint16_t color, byte checkState, int8_t kingX, int8_t kingY){
	if (importantPiece(x, y, kingX, kingY, color))
		return;
	int8_t Point[8][2] = { 
		{ x - 2, y + 1 }, { x - 2, y - 1 }, { x + 2, y + 1 }, { x + 2, y - 1 },
		{ x - 1, y + 2 }, { x + 1, y + 2 }, { x - 1, y - 2 }, { x + 1, y - 2 } };
	for (int8_t i = 7; i >= 0; i--){
		if ((Point[i][0] >= 0) && (Point[i][0] <= 7) && (Point[i][1] >= 0) && (Point[i][1] <= 7)){
			if (board[Point[i][1]][Point[i][0]] != NULL && board[Point[i][1]][Point[i][0]]->color == color)
				continue;
			push(Point[i][0], Point[i][1]);
		}
	}
	if (checkState == 1)
		extraction();
}

void bishopField(int8_t x, int8_t y, uint16_t color, byte checkState, int8_t kingX, int8_t kingY){
	bLeftUP(y-1, x-1, color);
	bLeftDn(y+1, x-1, color);
	bRightUp(y-1, x+1, color);
	bRightDn(y+1, x+1, color);
	if (importantPiece(x, y, kingX, kingY, color))
		_extraction();
	if (checkState == 1)
		extraction();
}

void bLeftUP(int8_t up, int8_t left, uint16_t color){
	for (int8_t u = up, l = left; l >= 0 && u >= 0; u--, l--){
		if (board[u][l] != NULL){
			if (board[u][l]->color == color)
				break;
			else{
				push(l, u);
				break;
			}
		}
		push(l, u);
	}
}

void bLeftDn(int8_t down, int8_t left, uint16_t color){
	for (int8_t d = down, l = left; l >= 0 && d <= 7; d++, l--){
		if (board[d][l] != NULL){
			if (board[d][l]->color == color)
				break;
			else{
				push(l, d);
				break;
			}
		}
		push(l, d);
	}
}

void bRightDn(int8_t down, int8_t right, uint16_t color){
	for (int8_t d = down, r = right; r <= 7 && d <= 7; d++, r++){
		if (board[d][r] != NULL){
			if (board[d][r]->color == color)
				break;
			else{
				push(r, d);
				break;
			}
		}
		push(r, d);
	}
}

void bRightUp(int8_t up, int8_t right, uint16_t color){
	for (int8_t u = up, r = right; r <= 7 && u >= 0; u--, r++){
		if (board[u][r] != NULL){
			if (board[u][r]->color == color)
				break;
			else{
				push(r, u);
				break;
			}
		}
		push(r, u);
	}
}

void rookField(int8_t x, int8_t y, uint16_t color, byte checkState, int8_t kingX, int8_t kingY){
	rUp(y-1,x, color);
	rDn(y+1,x, color);
	rLeft(y,x-1, color);
	rRight(y,x + 1, color);
	if (importantPiece(x, y, kingX, kingY, color))
		_extraction();
	if (checkState == 1)
		extraction();
}

void rUp(int8_t up, int8_t x, uint16_t color){
	for (int8_t path = up; path >= 0; path--){
		if (board[path][x] != NULL){
			if (board[path][x]->color == color)
				break;
			else{
				push(x, path);
				break;
			}
		}
		push(x, path);
	}
}

void rDn(int8_t down, int8_t x, uint16_t color){
	for (int8_t path = down; path <= 7; path++){
		if (board[path][x] != NULL){
			if (board[path][x]->color == color)
				break;
			else{
				push(x,path);
				break;
			}
		}
		push(x, path);
	}
}

void rLeft(int8_t y, int8_t left, uint16_t color){
	for (int8_t path = left; path >= 0; path--){
		if (board[y][path] != NULL){
			if (board[y][path]->color == color)
				break;
			else{
				push(path, y);
				break;
			}
		}
		push(path, y);
	}
}

void rRight(int8_t y, int8_t right, uint16_t color){
	for(int8_t path = right; path <= 7; path++){
		if (board[y][path] != NULL){
			if (board[y][path]->color == color)
				break;
			else{
				push(path, y);
				break;
			}
		}
		push(path, y);
	}
}

void queenField(int8_t x, int8_t y, uint16_t color, byte checkState, int8_t kingX, int8_t kingY){
	bLeftUP(y - 1, x - 1, color);
	bLeftDn(y + 1, x - 1, color);
	bRightUp(y - 1, x + 1, color);
	bRightDn(y + 1, x + 1, color);
	rUp(y - 1, x, color);
	rDn(y + 1, x, color);
	rLeft(y, x - 1, color);
	rRight(y, x + 1, color);
	if (importantPiece(x, y, kingX, kingY, color))
		_extraction();
	if (checkState == 1)
		extraction();
}

void kingField(int8_t x, int8_t y, byte* field, uint16_t color){
	int8_t Point[8][2] = {
		{ x, y + 1 }, { x, y - 1 }, { x + 1, y + 1 }, { x + 1, y - 1 },
		{ x - 1, y + 1 }, { x - 1, y - 1 }, { x + 1, y }, { x - 1, y } };
	for (int8_t i = 7; i >= 0; i--){
		if (Point[i][0] >= 0 && Point[i][0] <= 7 && Point[i][1] >= 0 && Point[i][1] <= 7){
			if (board[Point[i][1]][Point[i][0]] != NULL && board[Point[i][1]][Point[i][0]]->color == color)
				continue;
			if (field[Point[i][1]]&(0x01 << Point[i][0]))
				continue;
			push(Point[i][0], Point[i][1]);
		}
	}
	checkCastling(color, field, x, y);
}

void promotion(uint8_t x, uint8_t y){
	if (board[y][x]->type == 0 && y == 0){
		uint8_t data;
		drawPromotionBar();
		drawPcursor(1);
		board[y][x]->type=pSelect();
		data=clear(x, y);
		display.noStroke();
		display.fill(data, data, data);
		display.rect(x * 16 + 1, (y + 1) * 16 + 1, 14, 14);
		drawPiece(x,y,piece[board[y][x]->type], ally);
		clearPromotionBar();
	}
}

void drawPromotionBar(){
	display.stroke(0, 0, 0);
	display.fill(255, 255, 255);
	display.rect(24, 54, 80, 36);
	display.fill(0, 0, 0);
	display.rect(25,55,78,10);
	display.textSize(1);
	display.stroke(255,255,255);
	display.text("Promotion~!!", 27, 56);
	display.fill(190, 190, 190);
	for(int8_t x = 4; x >= 1; x--){
		display.stroke(0, 0, 0);
		display.rect(11+ 18* x, 69, 16, 16);
		drawPiece(x, piece[x], ally);
	}
	display.noFill();
}

void drawPcursor(uint8_t x){
	display.stroke(0, 0, 255);
	display.rect(11 + 18 * x, 69, 16, 16);
}

void clearPcursor(uint8_t x){
	display.stroke(0, 0, 0);
	display.rect(11 + 18 * x, 69, 16, 16);
}

int8_t pSelect(){
	int8_t pfixX = 1;
	while (true){
		if (digitalRead(cursor[0]) == LOW){
			if (pfixX > 1){
				clearPcursor(pfixX);
				pfixX--;
				drawPcursor(pfixX);
			    delay(200);
			}
		}
		else if (digitalRead(cursor[3]) == LOW){
			if (pfixX < 4){
				clearPcursor(pfixX);
				pfixX++;
				drawPcursor(pfixX);
				delay(200);
			}
		}
		if (digitalRead(select1) == LOW){
			delay(200);
			break;
		}
	}
	return pfixX;
}

void clearPromotionBar(){
	uint8_t data;
	for  (int8_t y = 4; y >= 2; y--){
		for (int8_t x = 6; x >= 1; x--){
			data = clear(x, y);
			display.fill(data, data, data);
			display.rect(x * 16, (y + 1) * 16, 16, 16);
			if (board[y][x] != NULL)
				drawPiece(x, y, piece[board[y][x]->type], board[y][x]->color);
		}
	}
}

void checkCastling(uint16_t color, byte* field, int8_t kingX, int8_t kingY){//킹이 이동하는범위가 안전한지를 검사하는 조건문필요
	if ((field[kingY] & (0x01 << kingX)))
		return;
	if (color == 0xFFFF){
		wQueenSide(field);
		wKingSide(field);
	}
	else if (color == 0x0000){
		bQueenSide(field);
		bKingSide(field);
	}
}

void wQueenSide(byte* field){
	if (queenSideR == 0 && castlingState == 0 && board[7][0]->color == ally){
		for (int8_t x = 3; x >= 1; x--){
			if (x < 3 && (field[7] & (0x01<<x)))
				return;
			if (board[7][x] != NULL)
				return;
		}
		push(2, 7);
	}
}

void wKingSide(byte* field){
	if (kingSideR == 0 && castlingState == 0 &&board[7][7]->color==ally){
		for (int8_t x = 6; x >= 5; x--){
			if ((field[7] & (0x01 << x)))
				return;
			if (board[7][x] != NULL)
				return;
		}
		push(6, 7);
	}
}

void bQueenSide(byte* field){
	if (queenSideR == 0 && castlingState == 0 && board[7][7]->color == ally){
		for (int8_t x = 6; x >= 4; x--){
			if (x < 6 && (field[7] & (0x01 << x)))
				return;
			if (board[7][x] != NULL)
				return;
		}
		push(5, 7);
	}
}

void bKingSide(byte* field){
	if (kingSideR == 0 && castlingState == 0 && board[7][0]->color == ally){
		for (int8_t x = 2; x >= 1; x--){
			if ((field[7] & (0x01 << x)))
				return;
			if (board[7][x] != NULL)
				return;
		}
		push(1, 7);
	}
}

void castling(){
	int8_t x = 2, r=7, x_=1;
	for(int8_t i= 2; i> 0; i--){
		if (ramX == kingX+x){
			uint8_t data;
			data = clear(r, 7);
			display.fill(data, data, data);
			display.rect(r*16, 8*16, 16, 16);
			board[7][kingX + x_] = board[7][r];
			board[7][r] = NULL;
			drawPiece(kingX+x_, 7, piece[3], ally);
			return;
		}
		x = -2;
		r = 0;
		x_ = -1;
	}
}

byte click(){
	if (sel == 0){
		if (board[ramY][ramX] != NULL&&board[ramY][ramX]->color == ally){//말을 선택했을 때
			fixX = ramX;
			fixY = ramY;//커서 고정
			drawFixCursor();
			loadPiece(fixX, fixY, -1, efield, ally, aCheckState, kingX, kingY);
			drawMovable();
			sel = 1;
		}
	}
	else if (sel == 1){
		if (board[ramY][ramX] != NULL && board[ramY][ramX]->color == ally){//말을 선택했을 때
			if (ramX == fixX && ramY == fixY &&top != -1){//이미 선택한 말이라면
				delay(150);
				return 0;
			}
			clearFixCursor();
			fixX = ramX;
			fixY = ramY;//커서 고정
			clearMovable(8, 8, sel);
			drawFixCursor();
			loadPiece(fixX, fixY, -1, efield, ally,aCheckState, kingX, kingY);
			drawMovable();
		}
		else{
			int8_t tmp = top;
			while (tmp != -1){
				if ((ramX == stack[tmp][0]) && (ramY == stack[tmp][1])){
					byte data;
					clearFixCursor();
					clearMovable(fixX, fixY, sel);
					if (board[ramY][ramX] != NULL)
						delete board[ramY][ramX];//메모리 해제
					board[ramY][ramX] = board[fixY][fixX];
					if (board[ramY][ramX]->type == 5){//king일시
						castling();//castling check
						kingX = ramX;
						kingY = ramY;
						castlingState = 1;
					}
					else if (board[ramY][ramX]->type == 3){//rook일시
						if (ally == 0xFFFF){
							if (fixX == 0)
								queenSideR = 1;
							else if (fixX == 7)
								kingSideR = 1;
						}
						else{
							if (fixX == 0)
								kingSideR = 1;
							else if (fixX == 7)
								queenSideR = 1;
						}
					}
					board[fixY][fixX] = NULL;
					data = clear(ramX, ramY);
					display.fill(data, data, data);
					display.rect(ramX * 16, 16 + ramY * 16, 16, 16);
					drawPiece(ramX, ramY, piece[board[ramY][ramX]->type], ally);
					data = clear(fixX, fixY);
					display.fill(data, data, data);
					display.rect(fixX * 16, 16 + fixY * 16, 16, 16);
					promotion(ramX, ramY);//promotion check
					//eCheckState=check(ramX, ramY, enemy, eKingX, eKingY, -1);//체크인지 검사
					for (int8_t i = 7; i >= 0; i--)//clear
						afield[i] = 0;
					setField(ally, afield, -1);//set
					if (afield[eKingY] & 1 << eKingX){
						eCheckState = 1;
						drawCheck(eKingX, eKingY, enemy);
					}
					if (aCheckState){//체크였다면
						clearCheck(kingX, kingY, ally);
						aCheckState = 0;
					}
					if (ePieceMovable(1, afield, enemy, eCheckState, eKingX, eKingY)){//적 말이 이동할 수 있는 필드가 없다면
						sendData();//데이터 전송
						return 1;
					}
					sendData();//데이터 전송
					fixX = 8;
					fixY = 8;
					sel = 0;
					flag = 0;//플래그 반전
					break;
				}
				else
					tmp--;
			}
		}
	}
	delay(150);
	return 0;
}

void clearCheck(int8_t kingX, int8_t kingY, uint16_t color){
	byte data;
	data = clear(kingX, kingY);
	display.fill(data, data, data);
	display.rect(kingX * 16, 16 + kingY * 16, 16, 16);
	display.noFill();
	drawPiece(kingX, kingY, piece[5], color);
}

byte ePieceMovable(int8_t v, byte* field, uint16_t color, byte checkState, int8_t kingX, int8_t kingY){//이동이 가능한 말이 있는지를 검사
	for (int8_t y = 7; y >= 0; y--){
		for (int8_t x = 7; x >= 0; x--){
			top = -1;
			if (board[y][x] != NULL && board[y][x]->color == color){
				loadPiece(x, y, v, field, color, checkState, kingX, kingY);
				if (top != -1){
					top = -1;
					return 0;
				}
			}
		}
	}
	return 1;
}

//clear first, move second
void enableField(int8_t x, int8_t y, uint16_t color, byte* field, int8_t v){
	switch (board[y][x]->type){
	case 0:
		pawnEnable(x, y, field, color, v);
		break;
	case 1:
		bishopEnable(x, y, field, color);
		break;
	case 2:
		knightEnable(x, y, field);
		break;
	case 3:
		rookEnable(x, y, field, color);
		break;
	case 4:
		queenEnable(x, y, field, color);
		break;
	case 5:
		kingEnable(x, y, field);
		break;
	}
}

void pawnEnable(int8_t x, int8_t y, byte* field, uint16_t color, int8_t v){
	if (x-1 != -1)
		field[y+v] |= (0x01 << (x-1));
	if (x+1 != 8)
		field[y+v] |= (0x01 << (x+1));
}

void bishopEnable(uint8_t x, uint8_t y, byte* field, uint16_t color){
	int8_t up = y - 1, down = y + 1, left = x - 1, right = x + 1;
	for (int8_t u = up, l = left; l >= 0 && u >= 0; u--, l--){//leftUp
		field[u] |= (0x01 << l);
		if (board[u][l]!=NULL && board[u][l]->color == color)
			break;
		else if (board[u][l]!=NULL && board[u][l]->type != 5)
			break;
	}
	for (int8_t d = down, l = left; l >= 0 && d <= 7; d++, l--){//leftDown
		field[d] |= (0x01 << l);
		if (board[d][l]!=NULL && board[d][l]->color == color)
			break;
		else if (board[d][l] != NULL && board[d][l]->type != 5)
			break;
	}
	for (int8_t d = down, r = right; r <= 7 && d <= 7; d++, r++){//rightDown
		field[d] |= (0x01 << r);
		if (board[d][r]!=NULL && board[d][r]->color == color)
			break;
		else if (board[d][r] != NULL && board[d][r]->type != 5)
			break;
	}
	for (int8_t u = up, r = right; r <= 7 && u >= 0; u--, r++){//rightUp
		field[u] |= (0x01 << r);
		if (board[u][r]!=NULL && board[u][r]->color == color)
			break;
		else if (board[u][r] != NULL && board[u][r]->type != 5)
			break;
	}
}

void knightEnable(uint8_t x, uint8_t y, byte* field){
	int8_t Point[8][2] = {
		{ x - 2, y + 1 }, { x - 2, y - 1 }, { x + 2, y + 1 }, { x + 2, y - 1 },
		{ x - 1, y + 2 }, { x + 1, y + 2 }, { x - 1, y - 2 }, { x + 1, y - 2 } };
	for (int8_t i = 7; i >= 0; i--){
		if ((Point[i][0] >= 0) && (Point[i][0] <= 7) && (Point[i][1] >= 0) && (Point[i][1] <= 7)){
			field[Point[i][1]] |= (0x01<<Point[i][0]);
		}
	}
}

void rookEnable(uint8_t x, uint8_t y, byte* field, uint16_t color){
	int8_t up = y - 1, down = y + 1, left = x - 1, right = x + 1;
	for (int8_t path = up; path >= 0; path--) {//up
		field[path] |= (0x01<<x);
		if (board[path][x] != NULL && board[path][x]->color == color)
			break;
		else if (board[path][x] != NULL && board[path][x]->type != 5)
			break;
	}
	for (int path = down; path <= 7; path++) {//down
		field[path] |= (0x01 << x);
		if (board[path][x] != NULL && board[path][x]->color == color)
			break;
		else if (board[path][x] != NULL && board[path][x]->type != 5)
			break;
	}
	for (int path = left; path >= 0; path--) {//left
		field[y] |= (0x01 << path);
		if (board[y][path] != NULL && board[y][path]->color == color)
			break;
		else if (board[y][path] != NULL && board[y][path]->type != 5)
			break;
	}
	for (int path = right; path <= 7; path++) {//right
		field[y] |= (0x01 << path);
		if (board[y][path] != NULL && board[y][path]->color == color)
			break;
		else if (board[y][path] != NULL && board[y][path]->type != 5)
			break;
	}
}

void queenEnable(uint8_t x, uint8_t y, byte* field, uint16_t color){
	rookEnable(x, y, field, color);
	bishopEnable(x, y, field, color);
}

void kingEnable(uint8_t x, uint8_t y, byte* field){
	int8_t Point[8][2] = {
		{ x, y + 1 }, { x, y - 1 }, { x + 1, y + 1 }, { x + 1, y - 1 },
		{ x - 1, y + 1 }, { x - 1, y - 1 }, { x + 1, y }, { x - 1, y } };
	for (int8_t i = 7; i >= 0; i--){
		if (Point[i][0] >= 0 && Point[i][0] <= 7 && Point[i][1] >= 0 && Point[i][1] <= 7){
			field[Point[i][1]] |= (0x01<<Point[i][0]);
		}
	}
}

void drawCheck(int8_t x, int8_t y, uint16_t color){
	display.fill(190, 0, 0);
	display.rect(x * 16 + 1, (y + 1) * 16 + 1, 14, 14);
	drawPiece(x, y, piece[5], color);//체크 상태 표시
}

byte check(int8_t x, int8_t y, uint16_t color, int8_t kingX, int8_t kingY, int8_t v){
	int8_t state=0;
	pp = 0;
	switch (board[y][x]->type){
	case 0:
		state=pawnAttack(x, y,color, v);
		break;
	case 1:
		state=bishopAttack(x, y,kingX, kingY);
		break;
	case 2:
		state = knightAttack(x, y, color);
		break;
	case 3:
		state = rookAttack(x, y, kingX, kingY);
		break;
	case 4:
		state = rookAttack(x, y,kingX, kingY);
		if (state)
			break;
		state = bishopAttack(x, y, kingX, kingY);
		break;
	}
	if (state==1){//체크라면
		drawCheck(kingX, kingY,color);
		display.noFill();
	}
	return state;
}

byte pawnAttack(int8_t x, int8_t y, uint16_t color, int8_t v){
	if (-1 != x - 1){
		if (board[y + v][x - 1] != NULL && board[y + v][x -1]->type == 5 && board[y + v][x -1]->color == color)
			return pawnSave(x, y);
	}
	if (x + 1 != 8){
		if (board[y + v][x + 1] != NULL && board[y + v][x + 1]->type == 5 && board[y + v][x + 1]->color == color)
			return pawnSave(x, y);
	}
	return 0;
}

byte pawnSave(int8_t x, int8_t y){
	attackPath[pp][0] = x;
	attackPath[pp][1] = y;
	return 1;
}

byte bishopAttack(int8_t x, int8_t y, int8_t kingX, int8_t kingY){
	byte state=0;
	if (abs(x - kingX) != abs(y - kingY))//움직인 말과 킹이 대각선상에 위치하지 않는다면
		return 0;
	if (x>kingX&&y>kingY)//leftUp
		state=bLeftUPAttack(y - 1, x - 1, kingX, kingY);
	else if (x>kingX&&y<kingY)//leftDown
		state = bLeftDnAttack(y + 1, x - 1, kingX, kingY);
	else if (x<kingX&&y<kingY)//rightDown
		state = bRightDnAttack(y + 1, x + 1, kingX, kingY);
	else if (x < kingX&&y > kingY)//rightUp
		state = bRightUpAttack(y - 1, x + 1, kingX, kingY);
	if (state){
		attackPath[pp][0] = x;
		attackPath[pp][1] = y;
		return 1;
	}
	return 0;
}

byte bLeftUPAttack(int8_t up, int8_t left, int8_t kingX, int8_t kingY){
	for (int8_t u = up, l = left; l > kingX && u > kingY; u--, l--){
		if (board[u][l] != NULL)
			return 0;
		attackPath[pp][0] = l;
		attackPath[pp][1] = u;
		pp++;
	}
	return 1;
}

byte bLeftDnAttack(int8_t down, int8_t left, int8_t kingX, int8_t kingY){
	for (int8_t d = down, l = left; l > kingX && d < kingY; d++, l--){
		if (board[d][l] != NULL)
			return 0;
		attackPath[pp][0] = l;
		attackPath[pp][1] = d;
		pp++;
	}
	return 1;
}

byte bRightDnAttack(int8_t down, int8_t right, int8_t kingX, int8_t kingY){
	for (int8_t d = down, r = right; r < kingX && d < kingY; d++, r++){
		if (board[d][r] != NULL)
			return 0;
		attackPath[pp][0] = r;
		attackPath[pp][1] = d;
		pp++;
	}
	return 1;
}

byte bRightUpAttack(int8_t up, int8_t right, int8_t kingX, int8_t kingY){
	for (int8_t u = up, r = right; r < kingX && u > kingY; u--, r++){
		if (board[u][r] != NULL)
			return 0;
		attackPath[pp][0] = r;
		attackPath[pp][1] = u;
		pp++;
	}
	return 1;
}

byte knightAttack(int8_t x, int8_t y, uint16_t color){
	int8_t Point[8][2] = {
		{ x - 2, y + 1 }, { x - 2, y - 1 }, { x + 2, y + 1 }, { x + 2, y - 1 },
		{ x - 1, y + 2 }, { x + 1, y + 2 }, { x - 1, y - 2 }, { x + 1, y - 2 } };
	for (int8_t i = 7; i >= 0; i--){
		if ((Point[i][0] >= 0) && (Point[i][0] <= 7) && (Point[i][1] >= 0) && (Point[i][1] <= 7)){
			if (board[Point[i][1]][Point[i][0]] != NULL && board[Point[i][1]][Point[i][0]]->type == 5
				&& board[Point[i][1]][Point[i][0]]->color == color){
				attackPath[0][0] = x;
				attackPath[0][1] = y;
				attackPath[1][0] = Point[i][0];
				attackPath[1][1] = Point[i][1];
				pp = 1;
				return 1;
			}
		}
	}
	return 0;
}

byte rookAttack(int8_t x, int8_t y, int8_t kingX, int8_t kingY){
	if (x == kingX || y == kingY){//움직인 말과 킹이 직선상에 위치한다면
		byte state = 0;
		if (x == kingX && kingY < y)//up
			state = rUpAttack(y - 1, x,kingY);
		else if (x == kingX && kingY > y)//down
			state = rDnAttack(y + 1, x, kingY);
		else if (y == kingY && kingX < x)//left
			state = rLeftAttack(y, x - 1, kingX);
		else if (y == kingY && kingX > x)//right
			state = rRightAttack(y, x + 1, kingX);
		if (state){
			attackPath[pp][0] = x;
			attackPath[pp][1] = y;
			return 1;
		}
	}
	return 0;
}

byte rUpAttack(int8_t up, int8_t x,int8_t kingY){
	for (int8_t path = up; path > kingY; path--) {//up
		if (board[path][x] != NULL)
				return 0;
		attackPath[pp][0] = x;
		attackPath[pp][1] = path;
		pp++;
	}
	return 1;
}

byte rDnAttack(int8_t down, int8_t x,int8_t kingY){
	for (int8_t path = down; path < kingY; path++) {//down
		if (board[path][x] != NULL)
				return 0;
		attackPath[pp][0] = x;
		attackPath[pp][1] = path;
		pp++;
	}
	return 1;
}

byte rLeftAttack(int8_t y, int8_t left, int8_t kingX){
	for (int8_t path = left; path > kingX; path--) {//left
		if (board[y][path] != NULL)
				return 0;
		attackPath[pp][0] = path;
		attackPath[pp][1] = y;
		pp++;
	}
	return 1;
}

byte rRightAttack(int8_t y, int8_t right,int8_t kingX){
	for (int8_t path = right; path < kingX; path++) {//right
		if (board[y][path] != NULL)
				return 0;
		attackPath[pp][0] = path;
		attackPath[pp][1] = y;
		pp++;
	}
	return 1;
}