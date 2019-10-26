#ifndef __NIXCK__HEADER__
#define __NIXCK__HEADER__
#if (ARDUINO >= 100)
	#include <Arduino.h>
#else
	#include <WProgram.h>
#endif

#include <stdbool.h>
#include <Wire.h>
#include <avr/wdt.h>
#include "RTClib.h"

// ******** definition for 74HC595N tube ********
#define DSA 5
#define DSB 6
#define DSC 7

#define MRA 8
#define MRB 9
#define MRC 10

#define SHCPA 1
#define SHCPB 2
#define SHCPC 3 

#define STCP 0
// ******** definition for NixieClock switch ********
#define BUTTONA 12
#define BUTTONB 11

// #define setbit(x,y)	x|=(1<<y)//指定的某一位数置1
// #define clrbit(x,y)	x&=~(1<<y)//指定的某一位数置0
// #define reversebit(x,y)	x^=(1<<y)//指定的某一位数取反
// #define getbit(x,y)	 ((x) >> (y)&1)//获取的某一位的值


// ******** definition for NixieClock functions ********

byte getBit(byte number,byte pos);  //适应8位cpu的取位函数
byte oneInByte(byte x);				//统计byte里面1出现的次数
byte getNumber(uint32_t number,byte i,byte length);//取出一个数里面的某一个数

void showNothing();		//function : 息屏
void showTime();		//function : 显示RTC时间
void showCounter();		//function : 显示计时
void showWorldChange(); //function : 显示世界线变动

void startingEffect();	//开机动画
void simulateFlash(byte i,byte level);//模拟闪烁
void roundNixie(byte NixieTube,byte round);

void sendData();		//发送数据到74HC595N译码器
void refresh();			//74HC595N译码器STCP引脚上升沿，输出使能

void lightUpNixie(byte i);			//打开辉光管，使其可以被设置
void closeDownNixie(byte i);		//关闭辉光管，使其不能被设置
void setNixie(byte i,byte number);  //设置辉光管显示的数字
void lightUpPoint(byte i);			//打开小数点
void closeDownPoint();				//关闭小数点

void readFromRTC_Module();		//从RTC时间模块中读出时间


// ******** definition for constant values ********
const uint32_t powList[] = {1,10,100,1000,10000,100000};
const byte movList[8] = {1,2,4,8,16,32,64,128};
const byte SHCP[3] = {SHCPA,SHCPB,SHCPC};
const byte DS[3] = {DSA,DSB,DSC};
const byte MR[3] = {MRA,MRB,MRC};

const byte buildDate[6] = {1,9,1,0,2,5};
const byte LoveDate[6] = {1,9,0,3,1,4};
const byte simulateTime[2][10] = {
									{150,200,90,100,40,60,20,20,10,10},
									{150,200,90,100,40,60,20,20,10,10}
								};

#endif/* __NIXCK__HEADER__ */