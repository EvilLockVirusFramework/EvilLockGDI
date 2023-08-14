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
	//����GDI���е����ݵ���һ������
	void copy(GDI& C);
	//��ʼ�������ڴ��豸
	void InitializeGDI(SPGDI mem);
	void StartGDIGraphics();
	//�����洦����ͼ�ĳ�ʼ״̬������ʾ��ͼ���ڣ��������涨λ�ڴ������У�
	//��GDI״̬Ϊ��ʡ�ͷ����Ϊ0�ȣ���������
	void StartGDI();
	//�ı�GDI״̬Ϊ̧�ʡ��Ӵ�ʱ�������ƶ���������Ļ����ͼ��
	void PenUp();
	//�ı�GDI״̬Ϊ��ʡ��Ӵ�ʱ�������ƶ�������Ļ����ͼ��
	void PenDown();
	//��������ͷ��ǰ����ĽǶȡ�
	int GDIHeading();
	//��������ĵ�ǰλ�á�
	Point* GDIPos();
	//��������ͷ�ĵ�ǰ������ǰ�ƶ�����steps����
	void Move(int steps);
	//�ı�����ͷ�ĵ�ǰ������ʱ����תdegrees�ȡ�
	void Turn(double degrees);
	//�������ƶ����µ�λ��newPos����������״̬����ͬʱ��ͼ��
	void MoveGDITo(Point newPos);
	//�ı�����ͷ�ĵ�ǰ����Ϊ���������������angle�ȡ�
	void TurnTTo(double angle);
	//��������GDI��RGB
	void SetGDIColor(PRGBQUAD &RGB);
	private:
	// mygdi;
};