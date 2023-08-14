#include"type.h"
void ApplyDesktopEffect()
{
    GDI hdc;

    // �������������Ҫ�Ĳ������������ͼ�Ρ������ı��������ɫ�ȵ�
    RECT rect;
    GetClientRect(GetDesktopWindow(), &rect);
    HBRUSH hBrush = CreateSolidBrush(RGB(255, 0, 0));  // ����һ����ɫˢ��
    FillRect(hdc, &rect, hBrush);  // ��������������
    DeleteObject(hBrush);  // ɾ��ˢ�Ӷ���

    ReleaseDC(NULL, hdc);  // �ͷ��豸������
}

int main()
{
    ApplyDesktopEffect();

    return 0;
}
