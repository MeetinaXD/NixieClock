/*
	Author 		: 	MeetinaXD
					(meetinaxd@ltiex.com)
	Last Edit 	: 	Octo 27,2019. 20:09 (UTC + 08)
	Program 	:	NixieClock Control Unit (ATMega 328p-u in Arduino)
	Modify Logs	:	
					* Octo 27,20:09, 增加防中毒和看门狗功能 
					* Octo 31,14:28, 修改isOverTime中的错误；更改喂狗次序，修复一开机为功能3时导致重启的问题
					*
	WARNING:
		THIS PROGRAM IS NOT A FREE SOFTWARE, YOU ARE NOT
		ALLOW TO REDISTRIBUTE IT AND/OR MODIFY IT FOR 
		PROFIT WITHOUT AUTHOR'S PERMISSION.

		IF YOU ARE PERSONAL USE,NOT FOR PARTICULAR PURPOSE,
		YOU CAN USE BUT YOU SHOULD ACCEPT THE GNU LESSER 
		GENERAL PUBLIC LICENSE,AND COPY THIS LICENSE ALONG 
		WITH THIS PROGRAM.
		************** ALL RIGHTS RESERVED. **************
*/

#include "./NixieClock.h"

//看门狗 https://blog.csdn.net/sdlgq/article/details/50396518

RTC_DS1307 RTC; //时钟模块

byte bits[6] = {0,0,0,0,0,0};//当前数字位
byte old_bits[6] = {0,0,0,0,0,0};//上一个数字位
byte Data[3] = {69,69,69};//45 45 45

byte Date[6] = {19,7,30,15,26,0};//日期时间储存变量
// const byte LoveDateTime[6] = {19,3,14,17,0,0};//日期时间储存变量

DateTime NextRefreshTime;
byte state = 0;		//开关功能选择
byte old_state = 0; //上一个开关状态

bool change = true;//世界线是否产生变动

/* 0.息屏 1.时间 2.计时 3.世界线 */
void (*funcList[4])(void) = {showNothing,showTime,showCounter,showWorldChange};

void setup() {
	DDRC = 0xFF;
	PORTC = 0xF;//前四位控制小数点
	Wire.begin();
	RTC.begin();

	pinMode(BUTTONA,INPUT);
	pinMode(BUTTONB,INPUT);

	for (byte i = 0;i < 3;i++){
		pinMode(DS[i],OUTPUT);
		digitalWrite(DS[i],LOW);
		pinMode(MR[i],OUTPUT);
		digitalWrite(MR[i],LOW);
		digitalWrite(MR[i],HIGH);
		pinMode(SHCP[i],OUTPUT);
	}	
	pinMode(STCP,OUTPUT);
	digitalWrite(STCP,LOW);

	digitalWrite(SHCPA,HIGH);
	digitalWrite(SHCPB,HIGH);
	digitalWrite(SHCPC,HIGH);

	/* isrunnning方法存在问题，每次编译后首次运行都返回true，已经弃用。 */

	// if (!RTC.isrunning()){//如果DS1302模块没有在运行
	// 	/* 设置为编译时的时间 */
	// 	RTC.adjust(DateTime(__DATE__, __TIME__));
	// }

	/* 开启看门狗，8秒无响应后重启机器 */
	wdt_enable(WDTO_8S);
	randomSeed(RTC.now().unixtime());
	startingEffect(); //展示开机效果
	NextRefreshTime = RTC.now() + TimeSpan(__OVER_TIME__);
}

void loop(){
	// state = 1;
	//要先喂狗，否则世界线变动会重启
	wdt_reset();//记得喂狗
	state = (digitalRead(BUTTONA) << 1) | digitalRead(BUTTONB);
	funcList[state]();
	if(isOverTime())	//是否已经到刷新时间
		refreshNixie();
	
}

/* 取位操作，最多八位 */

byte getBit(byte number,byte pos){
	return ((number & movList[pos - 1]) > 0);
}

/* 设置辉光管显示的数字 */

void setNixie(byte i,byte number){
	if(bits[i] != 0xFF)
		bits[i] = number & 0xF;
}

/* 开启辉光管显示，参数：辉光管编号，注意开启后辉光管仍然无显示 */

void lightUpNixie(byte i){
	if (bits[i] == 0xF)
		bits[i] = 0xE;
}

/* 关闭辉光管显示，参数：辉光管编号 */

void closeDownNixie(byte i){
	bits[i] = 0xB;
}

/* 点亮小数点 */

void lightUpPoint(byte i){
	byte p = i;
	if(i == 4) p = 5;
	if(i == 5) p = 4;
	PORTC &= 0xF0;
	PORTC |= (p & 0xF);
}

/* 熄灭小数点*/

void closeDownPoint(){
	PORTC |= 0xF;
}


/* 74HC595N译码器输出引脚使能 */

void refresh(){
	digitalWrite(STCP,LOW);
	digitalWrite(STCP,HIGH);//上升沿
}

/* 发送数据到74HC595N译码器 */

void sendData(){
	Data[0] = bits[0] << 4 | bits[1];
	Data[1] = bits[2] << 4 | bits[3];
	Data[2] = bits[4] << 4 | bits[5];
	byte s = 69;
	for (byte k = 0;k < 3;k++){
		byte v = 0;
		for (byte i = 8;i > 0;i--){
			v = getBit(Data[k],i);
			digitalWrite(DS[k],v);
			digitalWrite(SHCP[k],LOW);
			digitalWrite(SHCP[k],HIGH);
		}
	}
	refresh();
}

/* 是否已经超时 */

bool isOverTime(){
	TimeSpan gap = NextRefreshTime - RTC.now();
	if (gap.totalseconds() < 0){ //已经超时
		return true;
	}
	return false;
}

/* 辉光管防中毒,刷新辉光管 */

void refreshNixie(){
	/*
		1.闪烁所有的辉光管
		2.同时轮转所有的辉光管
	*/
	simulateFlash(0x3F,0); //闪烁所有辉光管
	roundNixie(0x3F,2); //轮转所有的辉光管，2圈。
	//更新下次刷新时间
	NextRefreshTime = RTC.now() + TimeSpan(__OVER_TIME__);
}

/* 显示世界线变动 */

void showWorldChange(){
	/* 
		逻辑实现：	1.世界线未变动时，小数点0闪烁（慢闪）
					2.世界线即将产生变动时，小数点0快闪，然后变化
					3.世界线会不稳定，表现为突然没有征兆变化为另外的数字，然后过一会儿恢复为原来的数字
					4.随机有荧光管频闪的效果
	 */
	if (state != old_state){//状态被改变
		old_state = state;
		for (byte i = 0; i < 6; i++){//关闭所有辉光管
			closeDownNixie(i);
		}
		sendData();
		refresh();
		change = true;
	}
	//世界线变动
	if (change){
		simulateFlash(1,0);
		lightUpNixie(0);
		setNixie(0,random(0,1));
		roundNixie(1,2);
		for (int i = 0; i < 10; i++){ //小数点快闪
			lightUpPoint(0);
			delay(100);
			closeDownPoint();
			delay(100);
		}
		byte NixieTube = 0;
		for (byte i = 1; i < 6; i++){ //点亮辉光管，设置随机数后开始轮转
			NixieTube = 1 << i;
			lightUpNixie(i);
			setNixie(i,random(0,9)&0xF);
			roundNixie(NixieTube,2);
			delay(35);
		}
		change = false;
	}
	/* 随机频闪 */
	if (random(1,64) == 4){ //世界线变动
		change = true;
		return;
	}
	if (random(1,32) == 5)
		simulateFlash(0x3F,0); //模拟闪烁效果（所有辉光管）
	if (random(1,64) == 6){//世界线不稳定
		for (int i = 0; i < 6; i++){
			old_bits[i] = bits[i];
			bits[i] = random(0,9)&0xF;
		}
		lightUpPoint(0);
		sendData();
		refresh();
		delay(random(800,3000));
		for (int i = 0; i < 6; i++)
			bits[i] = old_bits[i];
		sendData();
		refresh();
	}

	{//什么都没有发生的时候
		lightUpPoint(0);
		delay(500);
		closeDownPoint();
		delay(500);
	}
	// delay(35);
}

/* 黑屏，什么都不显示 */

void showNothing(){
	if (state!=old_state){
		old_state = state;
		for (int i = 0; i < 6; i++)
			closeDownNixie(i);
		sendData();
		refresh();
	}
	delay(40);
}

/* 显示当前时间 */

void showTime(){
	old_state = state;
	byte NixieTube = 0;
	readFromRTC_Module();
	bits[0] = Date[3] / 10;
	bits[1] = Date[3] % 10;
	bits[2] = Date[4] / 10;
	bits[3] = Date[4] % 10;
	bits[4] = Date[5] / 10;
	bits[5] = Date[5] % 10;
	for (byte i = 0; i < 6; i++){
		if(old_bits[i] != bits[i]){
			NixieTube |= 1 << i;
			old_bits[i] = bits[i];
		}
	}
	sendData();
	refresh();
	if(NixieTube != 0){//时间发生改变
		roundNixie(NixieTube,1);
		sendData();	//发送各辉光管数据到74HC595N译码器中
		refresh();	//译码器使能
	}
	delay(40);//下次检测时间变动的间隔
}

/* 显示计时 */

void showCounter(){
	/*1.先显示日，并固定五秒 2.显示时分秒*/
	/*期间如果发生了日的变换，则重复以上步骤*/

	/*
		1.逻辑：全灭->左对齐only点亮有的灯
	*/
	TimeSpan gap = RTC.now() - LoveDateTime;
	uint32_t days = gap.days();
	byte days_len = 0;
	byte NixieTube = 0;
	for (uint32_t i = days; i > 0;i /= 10,days_len++);
	if (state != old_state){//状态被改变，需要先显示日
		old_state = state;
		for (byte i = 0; i < 6; i++){//关闭所有辉光管
			closeDownNixie(i);
		}
		sendData();
		refresh();
		for (byte i = 0; i < days_len; i++){
			NixieTube = 1 << i;
			lightUpNixie(i);
			setNixie(i,getNumber(days,i + 1,days_len));
			roundNixie(NixieTube,2);
			delay(35);
		}
		for (byte i = 0; i < 6; i++){//打开所有辉光管
			lightUpNixie(i);
		}
		for (int i = 0; i < 10; i++){
			lightUpPoint(4);
			delay(100);
			closeDownPoint();
			delay(100);
		}
	}
	setNixie(0,gap.hours() / 10);
	setNixie(1,gap.hours() % 10);
	setNixie(2,gap.minutes() / 10);
	setNixie(3,gap.minutes() % 10);
	setNixie(4,gap.seconds() / 10);
	setNixie(5,gap.seconds() % 10);
	for (byte i = 0; i < 6; i++){
		if(old_bits[i] != bits[i]){
			NixieTube |= 1 << i;
			old_bits[i] = bits[i];
		}
	}
	if(NixieTube != 0){//时间还没有发生改变
		roundNixie(NixieTube,1);
		sendData();	//发送各辉光管数据到74HC595N译码器中
		refresh();	//译码器使能
	}
	delay(40);//下次检测时间变动的间隔
}

/* 从RTC时间模块中读取时间 */

void readFromRTC_Module(){
	DateTime now = RTC.now();
	Date[0] = now.year() - 2000;
	Date[1] = now.month();
	Date[2] = now.day();
	Date[3] = now.hour();
	Date[4] = now.minute();
	Date[5] = now.second();
}
byte oneInByte(byte x){
	byte sum = 0;
	for (byte i = 0; i < 8; i++)
		sum += (x & (1<<i)) > 0 ? 1 : 0;
	return sum;
}
byte getNumber(uint32_t number,byte i,byte length){
	if(i < 0 || i > length) return -1;
	number /= powList[length - i];
	number %= 10;
	return number;
}
/* 通电后效果 */

void startingEffect(){
	
	/* 首先全黑，然后全部一个个跑马灯 */
	byte NixieTube = 0;
	for (byte i = 0; i < 6; i++){
		closeDownNixie(i);
	}
	sendData();
	refresh();
	/* 模拟荧光灯闪烁 */
	for (byte i = 0; i < 6; i++){
		// lightUpNixie(i);
		setNixie(i,buildDate[i]);
		simulateFlash(1<<i,0);
	}
	delay(1000);
	/* 展示loveTime */
	for (byte i = 0; i < 6; i++){
		NixieTube = 1 << i;
		lightUpNixie(i);
		setNixie(i,LoveDate[i]);
		roundNixie(NixieTube,2);
		delay(35);
	}
	delay(500);
}
void roundNixie(byte NixieTube,byte round){
	for (byte i = 0; i < round * 10; i++){
		for (byte j = 0; j < 6; j++){
			if((NixieTube & (1<<j)) == 0) continue;
			if(++bits[j] == 10) bits[j] = 0;
		}
		sendData();
		refresh();
		delay(5);
	}
}

/* 模拟荧光管闪烁效果，参数：辉光管编号 */

void simulateFlash(byte NixieTube,byte level){
	for (int i = 0; i < 6; i++)
		old_bits[i] = bits[i];
	for (byte i = 0; i < 5; i++){
		for (byte j = 0; j < 6; j++){
			if((NixieTube & (1<<j)) == 0) continue;
			closeDownNixie(j);
		}
		sendData();
		refresh();
		delay(simulateTime[level][i*2]);
		for (byte j = 0; j < 6; j++){
			if((NixieTube & (1<<j)) == 0) continue;
			lightUpNixie(j);
			setNixie(j,old_bits[j]);
		}
		sendData();
		refresh();
		delay(simulateTime[level][i*2 + 1]);
	}
	for (byte j = 0; j < 8; j++){
		if((NixieTube & (1<<j)) == 0) continue;
		lightUpNixie(j);
		setNixie(j,old_bits[j]);
	}
	sendData();
	refresh();
}
/*
	跑马灯
*/

// void showTime(){
// 	bits[5]++;
// 	for (int i = 4;i > 0;i--){
// 		if(bits[i + 1] == 10){
// 			bits[i + 1] = 0;
// 			bits[i]++;
// 		}
// 	}
// 	if(bits[0] == 10){
// 		for(int i = 0;i < 6;i++)
// 			bits[i] = 0;
// 		}
// }
