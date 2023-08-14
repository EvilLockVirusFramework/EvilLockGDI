#pragma once
#include <iostream>
#include <conio.h>
#include <cstdio>
#include <string>
#include <cstdlib>
#include <cmath>
#include"type.h"
class EvilLock_SPGDI {
public:
	//复制GDI类中的数据到另一个类中
	void copy(GDI& C);
	//初始化虚拟内存设备
	void InitializeGDI(SPGDI mem);
	void StartGDIGraphics();
	//令桌面处于作图的初始状态。即显示作图窗口，并将桌面定位在窗口正中；
	//置GDI状态为落笔、头朝向为0度（正东方向）
	void StartGDI();
	//改变GDI状态为抬笔·从此时起，桌面移动将不在屏幕上作图。
	void PenUp();
	//改变GDI状态为落笔。从此时起，桌面移动将在屏幕上作图。
	void PenDown();
	//返回桌面头当前朝向的角度。
	int GDIHeading();
	//返回桌面的当前位置。
	Point* GDIPos();
	//依照桌面头的当前朝向，向前移动桌面steps步．
	void Move(int steps);
	//改变桌面头的当前朝向，逆时针旋转degrees度。
	void Turn(double degrees);
	//将桌面移动到新的位置newPos。如果是落笔状态，则同时作图。
	void MoveGDITo(Point newPos);
	//改变桌面头的当前朝向为，从正东方向起的angle度。
	void TurnTTo(double angle);
	//设置桌面GDI的RGB
	void SetGDIColor(PRGBQUAD &RGB);
	private:
	// mygdi;
};