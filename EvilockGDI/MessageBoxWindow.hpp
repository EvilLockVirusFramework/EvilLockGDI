/*****************************************************************//**
 * @file				MessageBoxWindow.hpp
 * @brief				消息框窗口操作
 * 
 * @author				EvilLockVirusFramework
 * @date				2025-10-6
 *********************************************************************/



#ifndef MESSAGEBOX_WINDOW_HPP
#define MESSAGEBOX_WINDOW_HPP



#pragma once
#include "common.hpp"



/**
 * @brief														: 消息框窗口封装类
 * @details														: 封装对消息框窗口的操作,包括位置、大小、移动等
 */
class MessageBoxWindow {
private:
	HWND hWnd;													///< 窗口句柄
	int xPos;													///< X 坐标
	int yPos;													///< Y 坐标
	int windowWidth;											///< 窗口宽
	int windowHeight;											///< 窗口高
	bool hasCollided;											///< 是否碰到边缘
	bool isAlive;												///< 窗口是否还存在

	std::thread moveThread;										///< 自动移动线程
	std::atomic<bool> stopMoving;								///< 停止移动标志
	int moveStepX;												///< X方向移动步长
	int moveStepY;												///< Y方向移动步长
	int moveMode;												///< 移动模式,包括BOUNCE(反弹)和STOP(停止)
	int moveInterval;											///< 移动间隔,单位毫秒



	/**
	 * @brief													: 自动更新窗口位置和大小信息
	 */
	void AutoUpdate() {
		if (!hWnd || !IsWindow(hWnd)) {
			isAlive = false;
			return;
		}														///< 窗口不存在则标记为不存活

		RECT rect;
		if (GetWindowRect(hWnd, &rect)) {
			xPos = rect.left;
			yPos = rect.top;
			windowWidth = rect.right - rect.left;
			windowHeight = rect.bottom - rect.top;
		}
	}



	/**
	 * @brief													: 自动移动线程函数
	 * @details													: 当stopMoving为false且窗口存活时,持续移动窗口
	 */
	void AutoMoveThread() {
		while (!stopMoving && IsAlive()) {
			Move(moveStepX, moveStepY, moveMode);
			std::this_thread::sleep_for(
				std::chrono::milliseconds(moveInterval));
		}
	}

	

	/**
	 * @brief													: 带自动延迟的移动函数
	 * @details													: 处理边缘碰撞逻辑,并在移动后自动延迟
	 * 
	 * @param[in]		deltaX									: X方向移动距离
	 * @param[in]		deltaY									: Y方向移动距离
	 * @param[in]		mode									: 移动模式,包括BOUNCE(反弹)和STOP(停止)
	 * @param[in]		delayMs									: 延迟时间,单位毫秒,0表示无延迟
	 */
	void MoveWithDelay(int deltaX, int deltaY, int mode, int delayMs) {
		if (!IsAlive()) return;

		AutoUpdate();

		if (hasCollided) {
			deltaX = -deltaX;
			deltaY = -deltaY;
			int edge = IsAtEdge(deltaX, deltaY, mode);
			if (edge == 1) hasCollided = false;
			if (edge == 2) return;
		}
		else {
			int edge = IsAtEdge(deltaX, deltaY, mode);
			if (edge == 1) {
				if (mode == BOUNCE) hasCollided = true;
			}
			else if (edge == 2) {
				return;
			}
		}

		xPos += deltaX;
		yPos += deltaY;
		SetWindowPos(hWnd, nullptr, xPos, yPos,
			0, 0, SWP_NOSIZE | SWP_NOZORDER);

		if (delayMs > 0) {
			std::this_thread::sleep_for(
				std::chrono::milliseconds(delayMs));			///< 自动延迟
		}
	}

public:
	/**
	 * @brief													: 构造函数
	 * @details													: 初始化成员变量,并根据窗口句柄自动更新位置和大小
	 * 
	 * @param[in]		window									: 窗口句柄,默认为nullptr
	 */
	MessageBoxWindow(HWND window = nullptr) : hWnd(window), xPos(0), yPos(0),
		windowWidth(0), windowHeight(0),
		hasCollided(false), isAlive(true),
		stopMoving(true), moveStepX(0), moveStepY(0),
		moveMode(BOUNCE), moveInterval(50) {
		if (hWnd) {
			AutoUpdate();
		}
	}


	/**
	 * @brief													: 析构函数
	 * @details													: 确保自动移动线程被正确停止
	 */
	~MessageBoxWindow() {
		StopAutoMove();
	}



	/**
	 * @brief													: 设置窗口句柄并更新位置和大小信息
	 * 
	 * @param[in]		window									: 窗口句柄
	 */
	void SetWindow(HWND window) {
		hWnd = window;
		isAlive = true;
		AutoUpdate();
	}



	/**
	 * @brief													: 获取窗口句柄
	 * @return													: 窗口句柄
	 */
	HWND GetHandle() const {
		return hWnd;
	}



	/**
	 * @brief													: 检查窗口是否还存活
	 * @return													: 窗口存活状态
	 */
	bool IsAlive() const {
		if (!hWnd || !IsWindow(hWnd)) {
			return false;
		}
		return isAlive;
	}



	/**
	 * @brief													: 关闭窗口
	 */
	void Close() {
		StopAutoMove();
		if (hWnd && IsWindow(hWnd)) {
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			isAlive = false;
		}
	}



	/* ============== 普通移动方法（带自动延迟） ============== */



	/**
	 * @brief													: 检查移动后是否会碰到边缘
	 * @return													: 0=未碰到边缘, 1=碰到边缘并反弹, 2=碰到边缘并停止
	 * 
	 * @param[in]		deltaX									: X方向移动距离
	 * @param[in]		deltaY									: Y方向移动距离
	 * @param[in]		mode									: 移动模式,包括BOUNCE(反弹)和STOP(停止)
	 */
	int IsAtEdge(int deltaX, int deltaY, int mode) const {
		if (!IsAlive()) return 0;

		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);

		if (xPos + deltaX <= 0 ||
			xPos + deltaX + windowWidth >= screenWidth ||
			yPos + deltaY <= 0 ||
			yPos + deltaY + windowHeight >= screenHeight) {
			if (mode == BOUNCE) return 1;
			if (mode == STOP) return 2;
		}
		return 0;
	}



	/**
	 * @brief													: 移动窗口（带自动延迟）
	 * @details													: 处理边缘碰撞逻辑,并在移动后自动延迟
	 * 
	 * @param[in]		deltaX									: X方向移动距离
	 * @param[in]		deltaY									: Y方向移动距离
	 * @param[in]		mode									: 移动模式,包括BOUNCE(反弹)和STOP(停止)
	 * @param[in]		delayMs									: 延迟时间,单位毫秒,0表示无延迟,默认100毫秒
	 */
	void Move(int deltaX, int deltaY, int mode, int delayMs = 100) {
		MoveWithDelay(deltaX, deltaY, mode, delayMs);
	}

	

	/**
	 * @brief													: 向上移动（带自动延迟）
	 * @details													: 处理边缘碰撞逻辑,并在移动后自动延迟
	 * 
	 * @param[in]		dt										: 移动距离
	 * @param[in]		mode									: 移动模式,包括BOUNCE(反弹)和STOP(停止)
	 * @param[in]		delayMs									: 延迟时间,单位毫秒,0表示无延迟,默认100毫秒
	 */
	void MoveUp(int dt, int mode, int delayMs = 100) {
		MoveWithDelay(0, -dt, mode, delayMs);
	}



	/**
	 * @brief													: 向下移动（带自动延迟）
	 * @details													: 处理边缘碰撞逻辑,并在移动后自动延迟
	 *
	 * @param[in]		dt										: 移动距离
	 * @param[in]		mode									: 移动模式,包括BOUNCE(反弹)和STOP(停止)
	 * @param[in]		delayMs									: 延迟时间,单位毫秒,0表示无延迟,默认100毫秒
	 */
	void MoveDown(int dt, int mode, int delayMs = 100) {
		MoveWithDelay(0, dt, mode, delayMs);
	}



	/**
	 * @brief													: 向左移动（带自动延迟）
	 * @details													: 处理边缘碰撞逻辑,并在移动后自动延迟
	 *
	 * @param[in]		dt										: 移动距离
	 * @param[in]		mode									: 移动模式,包括BOUNCE(反弹)和STOP(停止)
	 * @param[in]		delayMs									: 延迟时间,单位毫秒,0表示无延迟,默认100毫秒
	 */
	void MoveLeft(int dt, int mode, int delayMs = 100) {
		MoveWithDelay(-dt, 0, mode, delayMs);
	}



	/**
	 * @brief													: 向右移动（带自动延迟）
	 * @details													: 处理边缘碰撞逻辑,并在移动后自动延迟
	 *
	 * @param[in]		dt										: 移动距离
	 * @param[in]		mode									: 移动模式,包括BOUNCE(反弹)和STOP(停止)
	 * @param[in]		delayMs									: 延迟时间,单位毫秒,0表示无延迟,默认100毫秒
	 */
	void MoveRight(int dt, int mode, int delayMs = 100) {
		MoveWithDelay(dt, 0, mode, delayMs);
	}



	/**
	 * @brief													: 弹簧衰减晃动效果
	 * @details													: 窗口在原位置附近来回晃动,晃动幅度逐渐减小直至停止
	 * 
	 * @param[in]		shakeCount								: 晃动次数,默认8次
	 * @param[in]		maxIntensity							: 最大晃动强度,单位像素,默认15像素
	 * @param[in]		delayMs									: 每次晃动延迟时间,单位毫秒,默认30毫秒
	 */
	void Shake(int shakeCount = 8, int maxIntensity = 15, int delayMs = 30) {
		if (!IsAlive()) return;

		AutoUpdate();

		int originalX = xPos, originalY = yPos;

		for (int i = 0; i < shakeCount; i++) {
			float decay = static_cast<float>(shakeCount - i) / shakeCount;
			int currentIntensity = static_cast<int>(
				maxIntensity * decay);							///< 计算衰减的强度

			if (currentIntensity < 1) currentIntensity = 1;

			SetWindowPos(hWnd, nullptr,
				originalX + currentIntensity, originalY,
				0, 0, SWP_NOSIZE | SWP_NOZORDER);
			std::this_thread::sleep_for(
				std::chrono::milliseconds(delayMs));

			SetWindowPos(hWnd, nullptr,
				originalX - currentIntensity, originalY,
				0, 0, SWP_NOSIZE | SWP_NOZORDER);
			std::this_thread::sleep_for(
				std::chrono::milliseconds(delayMs));			///< 左右晃动

			SetWindowPos(hWnd, nullptr,
				originalX, originalY + currentIntensity,
				0, 0, SWP_NOSIZE | SWP_NOZORDER);
			std::this_thread::sleep_for(
				std::chrono::milliseconds(delayMs));

			SetWindowPos(hWnd, nullptr,
				originalX, originalY - currentIntensity,
				0, 0, SWP_NOSIZE | SWP_NOZORDER);
			std::this_thread::sleep_for(
				std::chrono::milliseconds(delayMs));			///< 上下晃动
		}

		SetWindowPos(hWnd, nullptr, originalX, originalY,
			0, 0, SWP_NOSIZE | SWP_NOZORDER);					///< 恢复原始位置
	}



	/* ==================== 自动移动方法 ==================== */



	/**
	 * @brief													: 开始自动移动
	 * @details													: 内部启动一个线程持续移动窗口,直到调用StopAutoMove或窗口销毁
	 * 
	 * @param[in]		stepX									: X方向移动步长
	 * @param[in]		stepY									: Y方向移动步长
	 * @param[in]		mode									: 移动模式,包括BOUNCE(反弹)和STOP(停止),默认BOUNCE
	 * @param[in]		interval								: 移动间隔,单位毫秒,默认50毫秒
	 */
	void StartAutoMove(int stepX, int stepY, int mode = BOUNCE, int interval = 50) {
		StopAutoMove();

		moveStepX = stepX;
		moveStepY = stepY;
		moveMode = mode;
		moveInterval = interval;
		stopMoving = false;

		moveThread = std::thread(&MessageBoxWindow::AutoMoveThread, this);
	}



	/**
	 * @brief													: 自动向指定方向移动
	 * @details													: 判断方向字符串并调用StartAutoMove
	 * 
	 * @param[in]		direction								: 移动方向,可选"up","down","left","right","random"
	 * @param[in]		step									: 移动步长,默认15
	 * @param[in]		mode									: 移动模式,包括BOUNCE(反弹)和STOP(停止),默认BOUNCE
	 * @param[in]		interval								: 移动间隔,单位毫秒,默认50毫秒
	 */
	void StartAutoMoveDirection(const std::string& direction, int step = 15, int mode = BOUNCE, int interval = 50) {
		if (direction == "up") StartAutoMove(0, -step, mode, interval);
		else if (direction == "down") StartAutoMove(0, step, mode, interval);
		else if (direction == "left") StartAutoMove(-step, 0, mode, interval);
		else if (direction == "right") StartAutoMove(step, 0, mode, interval);
		else if (direction == "random") StartAutoMoveRandom(step, mode, interval);
	}

	

	/**
	 * @brief													: 开始随机自动移动
	 * @details													: 内部启动一个线程持续随机移动窗口,直到调用StopAutoMove或窗口销毁
	 * 
	 * @param[in]		maxStep									: 最大随机步长,范围[-maxStep, maxStep],默认15
	 * @param[in]		mode									: 移动模式,包括BOUNCE(反弹)和STOP(停止),默认BOUNCE
	 * @param[in]		interval								: 移动间隔,单位毫秒,默认50毫秒
	 */
	void StartAutoMoveRandom(
		int maxStep = 15, int mode = BOUNCE, int interval = 50
	) {
		StopAutoMove();

		moveMode = mode;
		moveInterval = interval;
		stopMoving = false;

		moveThread = std::thread([this, maxStep, mode, interval]() {
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<int> dist(-maxStep, maxStep);

			while (!stopMoving && IsAlive()) {
				int stepX = dist(gen);
				int stepY = dist(gen);
				MoveWithDelay(stepX, stepY, mode, 0);			///< 自动移动内部不需要额外延迟
				std::this_thread::sleep_for(
					std::chrono::milliseconds(interval));
			}
			});
	}



	/**
	 * @brief													: 停止自动移动
	 * @details													: 设置停止标志并等待线程结束
	 */
	void StopAutoMove() {
		stopMoving = true;
		if (moveThread.joinable()) {
			moveThread.join();
		}
	}

	

	/**
	 * @brief													: 获取当前窗口位置
	 * @details													: 通过引用参数返回X和Y坐标
	 * 
	 * @param[out]		x										: X坐标
	 * @param[out]		y										: Y坐标
	 */
	void GetPosition(int& x, int& y) const {
		x = xPos, y = yPos;
	}

	

	/**
	 * @brief													: 获取当前窗口大小
	 * @details													: 通过引用参数返回宽度和高度
	 * 
	 * @param[out]		width									: 窗口宽度
	 * @param[out]		height									: 窗口高度
	 */
	void GetSize(int& width, int& height) const {
		width = windowWidth, height = windowHeight;
	}
};

class Msgbox {
private:
	static int MsgBox_X;
	static int MsgBox_Y;

	static std::vector<MessageBoxWindow*> allMessageBoxes;		///< 存储所有弹窗对象的全局列表
	static std::atomic<int> nextId;
	static std::mutex boxesMutex;
	static std::condition_variable newBoxCV;

	/**
	 * @brief													: CBT钩子函数,用于在创建窗口时设置其位置
	 * @details													: 当捕获到创建窗口的消息时,如果窗口没有父窗口,则设置其位置为指定的MsgBox_X和MsgBox_Y
	 * @return													: 调用下一个钩子函数的返回值
	 * 
	 * @param[in]		nCode									: 钩子代码
	 * @param[in]		wParam									: 附加消息参数,具体含义取决于nCode
	 * @param[in]		lParam									: 附加消息参数,具体含义取决于nCode
	 */
	static LRESULT CALLBACK CBTProc(
		int nCode, WPARAM wParam, LPARAM lParam
	) {
		if (nCode == HCBT_CREATEWND) {
			CBT_CREATEWND* s = reinterpret_cast<CBT_CREATEWND*>(
				lParam);
			if (s->lpcs->hwndParent == NULL) {
				s->lpcs->x = MsgBox_X;
				s->lpcs->y = MsgBox_Y;
			}
		}
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	/**
	 * @brief													: 消息框钩子,用于捕获消息框窗口句柄
	 * @details													: 当捕获到激活窗口的消息时,创建一个新的MessageBoxWindow对象并添加到全局列表中,同时通知等待的新窗口创建
	 * @return													: 调用下一个钩子函数的返回值
	 * 
	 * @param[in]		nCode									: 钩子代码
	 * @param[in]		wParam									: 附加消息参数,具体含义取决于nCode
	 * @param[in]		lParam									: 附加消息参数,具体含义取决于nCode
	 */
	static LRESULT CALLBACK GetMsgBoxHook(
		int nCode, WPARAM wParam, LPARAM lParam
	) {
		if (nCode == HCBT_ACTIVATE) {
			HWND hwnd = (HWND)wParam;
			
			MessageBoxWindow* msgBox = new MessageBoxWindow(
				hwnd);											///< 创建新的MessageBoxWindow对象并添加到列表

			std::lock_guard<std::mutex> lock(boxesMutex);
			allMessageBoxes.push_back(msgBox);
			newBoxCV.notify_all();								///< 通知有新窗口创建
		}
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}



	/**
	 * @brief													: 带钩子的MessageBox函数,用于在指定位置显示消息框
	 * @details													: 设置CBT钩子以拦截消息框创建,并在创建时设置其位置为指定的X和Y坐标
	 * @return													: MessageBox函数的返回值
	 * 
	 * @param[in]		hWnd									: 父窗口句柄
	 * @param[in]		lpText									: 消息文本
	 * @param[in]		lpCaption								: 标题文本
	 * @param[in]		uType									: 消息框类型
	 * @param[in]		X										: X坐标
	 * @param[in]		Y										: Y坐标
	 */
	static int MessageBoxWithHook(HWND hWnd,
		LPCWSTR lpText, LPCWSTR lpCaption,
		UINT uType, int X, int Y
	) {
		HHOOK cbtHook = SetWindowsHookEx(
			WH_CBT, &CBTProc, NULL, GetCurrentThreadId()
		);
		HHOOK msgBoxHook = SetWindowsHookEx(
			WH_CBT, &GetMsgBoxHook, NULL, GetCurrentThreadId()
		);

		MsgBox_X = X, MsgBox_Y = Y;

		int result = MessageBoxW(hWnd, lpText, lpCaption, uType);

		if (cbtHook) UnhookWindowsHookEx(cbtHook);
		if (msgBoxHook) UnhookWindowsHookEx(msgBoxHook);

		return result;
	}



	/**
	 * @brief													: 线程函数,在新线程中显示消息框
	 * @details													: 调用带钩子的MessageBox函数以确保消息框在指定位置显示
	 * 
	 * @param[in]		hWnd									: 父窗口句柄
	 * @param[in]		text									: 消息文本
	 * @param[in]		caption									: 标题文本
	 * @param[in]		uType									: 消息框类型
	 * @param[in]		x										: X坐标
	 * @param[in]		y										: Y坐标
	 */
	static void MessageBoxThread(HWND hWnd,
		const std::wstring& text, const std::wstring& caption,
		UINT uType, int x, int y
	) {
		MessageBoxWithHook(hWnd,
			text.c_str(), caption.c_str(), uType, x, y);
	}



public:



	/**
	 * @brief													: 在指定位置显示消息框并返回对象(非阻塞版本)
	 * @details													: 内部启动一个新线程显示消息框,并等待其创建完成后返回对应的MessageBoxWindow对象
	 * @return													: 新创建的MessageBoxWindow对象指针,若超时则返回nullptr
	 * 
	 * @param[in]		hWnd									: 父窗口句柄
	 * @param[in]		text									: 消息文本
	 * @param[in]		caption									: 标题文本
	 * @param[in]		uType									: 消息框类型
	 * @param[in]		x										: X坐标
	 * @param[in]		y										: Y坐标
	 */
	static MessageBoxWindow* ShowAsync(HWND hWnd, const std::wstring& text,
		const std::wstring& caption, UINT uType,
		int x, int y) {
		// 保存当前消息框数量,用于确定新创建的是哪个
		size_t startCount;
		{
			std::lock_guard<std::mutex> lock(boxesMutex);
			startCount = allMessageBoxes.size();
		}

		// 在新线程中显示消息框
		std::thread t(MessageBoxThread, hWnd, text, caption, uType, x, y);
		t.detach(); // 分离线程,使其独立运行

		// 等待新窗口创建完成
		std::unique_lock<std::mutex> lock(boxesMutex);
		if (newBoxCV.wait_for(lock, std::chrono::seconds(3),
			[startCount]() { return allMessageBoxes.size() > startCount; })) {
			// 返回最新创建的消息框对象
			return allMessageBoxes.back();
		}

		return nullptr; // 超时
	}



	/**
	 * @brief													: 在指定位置显示消息框并返回对象(阻塞版本,保持向后兼容)
	 * @details													: 内部调用非阻塞版本的ShowAsync函数
	 * @return 
	 * 
	 * @param[in]		hWnd									: 父窗口句柄
	 * @param[in]		text									: 消息文本
	 * @param[in]		caption									: 标题文本
	 * @param[in]		uType									: 消息框类型
	 * @param[in]		x										: X坐标
	 * @param[in]		y										: Y坐标
	 */
	static MessageBoxWindow* Show(HWND hWnd,
		const std::wstring& text, const std::wstring& caption,
		UINT uType, int x, int y) {
		return ShowAsync(hWnd, text, caption, uType, x, y);
	}



	/**
	 * @brief													: 创建多个随机位置的消息框并返回对象列表(非阻塞版本)
	 * @details													: 内部启动多个线程依次创建消息框,并返回所有创建的MessageBoxWindow对象列表
	 * @return													: MessageBoxWindow对象指针列表
	 * 
	 * @param[in]		text									: 消息文本
	 * @param[in]		caption									: 标题文本
	 * @param[in]		uType									: 消息框类型
	 * @param[in]		num										: 创建数量
	 */
	static std::vector<MessageBoxWindow*> CreateRandomWindowsAsync(
		const std::wstring& text, const std::wstring& caption,
		UINT uType, int num) {

		std::vector<MessageBoxWindow*> result;
		std::random_device rd;
		std::mt19937 gen(rd());

		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);
		std::uniform_int_distribution<int> distX(
			100, screenWidth - 400);
		std::uniform_int_distribution<int> distY(
			100, screenHeight - 200);

		for (int i = 0; i < num; ++i) {
			int randomX = distX(gen);
			int randomY = distY(gen);

			MessageBoxWindow* msgBox = ShowAsync(
				NULL, text, caption, uType, randomX, randomY);
			if (msgBox) {
				result.push_back(msgBox);
			}

			std::this_thread::sleep_for(
				std::chrono::milliseconds(100));
		}

		return result;
	}



	/**
	 * @brief													: 创建多个随机位置的消息框并返回对象列表(保持向后兼容)
	 * @details													: 内部调用非阻塞版本的CreateRandomWindowsAsync函数
	 * @return													: MessageBoxWindow对象指针列表
	 * 
	 * @param[in]		text									: 消息文本
	 * @param[in]		caption									: 标题文本
	 * @param[in]		uType									: 消息框类型
	 * @param[in]		num										: 创建数量
	 */
	static std::vector<MessageBoxWindow*> CreateRandomWindows(
		const std::wstring& text, const std::wstring& caption,
		UINT uType, int num) {

		return CreateRandomWindowsAsync(text, caption, uType, num);
	}

	

	/**
	 * @brief													: 关闭指定的单个消息框
	 * @details													: 查找并关闭传入的MessageBoxWindow对象对应的窗口
	 * @return													: 是否成功关闭
	 * 
	 * @param[in]		msgBox									: 要关闭的MessageBoxWindow对象指针
	 */
	static bool CloseMessageBox(MessageBoxWindow* msgBox) {
		if (!msgBox) return false;

		std::lock_guard<std::mutex> lock(boxesMutex);

		auto it = std::find(
			allMessageBoxes.begin(), allMessageBoxes.end(),
			msgBox);											///< 查找并关闭指定的消息框
		if (it != allMessageBoxes.end()) {
			(*it)->Close();
			return true;
		}

		return false;
	}



	/**
	 * @brief													: 通过窗口句柄关闭单个消息框
	 * @details													: 查找并关闭对应句柄的消息框窗口
	 * @return													: 是否成功关闭
	 * 
	 * @param[in]		hWnd									: 要关闭的窗口句柄
	 */
	static bool CloseMessageBoxByHandle(HWND hWnd) {
		if (!hWnd) return false;

		std::lock_guard<std::mutex> lock(boxesMutex);

		for (auto* msgBox : allMessageBoxes) {
			if (msgBox->GetHandle() == hWnd) {
				msgBox->Close();
				return true;
			}
		}

		return false;
	}

	

	/**
	 * @brief													: 通过索引关闭单个消息框
	 * @details													: 查找并关闭对应索引的消息框窗口
	 * @return													: 是否成功关闭
	 * 
	 * @param[in]		index									: 要关闭的消息框索引
	 */
	static bool CloseMessageBoxByIndex(int index) {
		std::lock_guard<std::mutex> lock(boxesMutex);

		if (index >= 0 && index < static_cast<int>(allMessageBoxes.size()))
		{
			allMessageBoxes[index]->Close();
			return true;
		}

		return false;
	}



	/**
	 * @brief													: 获取所有活跃的消息框对象
	 * @details													: 清理已关闭的窗口后返回当前存活的MessageBoxWindow对象列表
	 * @return													: MessageBoxWindow对象指针列表
	 */
	static std::vector<MessageBoxWindow*> GetAllMessageBoxes() {
		std::lock_guard<std::mutex> lock(boxesMutex);

		
		auto it = std::remove_if(allMessageBoxes.begin(), allMessageBoxes.end(),
			[](MessageBoxWindow* msgBox) {
				if (!msgBox->IsAlive()) {
					delete msgBox;
					return true;
				}
				return false;
			});

		allMessageBoxes.erase(it, allMessageBoxes.end());		///< 清理已关闭的窗口

		return allMessageBoxes;
	}

	

	/**
	 * @brief													: 清理所有已关闭的消息框对象
	 * @details													: 删除所有不再存活的MessageBoxWindow对象,释放内存
	 */
	static void Cleanup() {
		std::lock_guard<std::mutex> lock(boxesMutex);

		auto it = std::remove_if(allMessageBoxes.begin(), allMessageBoxes.end(),
			[](MessageBoxWindow* msgBox) {
				if (!msgBox->IsAlive()) {
					delete msgBox;
					return true;
				}
				return false;
			});

		allMessageBoxes.erase(it, allMessageBoxes.end());
	}

	

	/**
	 * @brief													: 关闭所有消息框(fixed ver.)
	 * @details													: 先关闭所有窗口,然后清理已关闭的窗口对象
	 * 
	 */
	static void CloseAll() {
		std::lock_guard<std::mutex> lock(boxesMutex);

		for (auto* msgBox : allMessageBoxes) {
			if (msgBox && msgBox->IsAlive()) {
				msgBox->Close();
			}
		}														///< 先关闭所有窗口

		
		auto it = std::remove_if(allMessageBoxes.begin(), allMessageBoxes.end(),
			[](MessageBoxWindow* msgBox) {
				if (!msgBox->IsAlive()) {
					delete msgBox;
					return true;
				}
				return false;
			});													///< 然后清理

		allMessageBoxes.erase(it, allMessageBoxes.end());
	}

	

	/**
	 * @brief													: 获取活跃的消息框数量
	 * @details													: 清理已关闭的窗口后返回当前存活的消息框数量
	 * @return													: 活跃的消息框数量
	 */
	static size_t GetMessageBoxCount() {
		std::lock_guard<std::mutex> lock(boxesMutex);
		return allMessageBoxes.size();
	}

	

	/**
	 * @brief													: 通过索引获取消息框对象
	 * @details													: 清理已关闭的窗口后返回对应索引的MessageBoxWindow对象,若索引无效则返回nullptr
	 * @return													: MessageBoxWindow对象指针或nullptr
	 * 
	 * @param[in]		index									: 要获取的消息框索引
	 */
	static MessageBoxWindow* GetMessageBoxByIndex(int index) {
		std::lock_guard<std::mutex> lock(boxesMutex);

		if (index >= 0 && index < static_cast<int>(allMessageBoxes.size())) {
			return allMessageBoxes[index];
		}

		return nullptr;
	}

	

	/**
	 * @brief													: 通过窗口句柄获取消息框对象
	 * @details													: 查找并返回对应句柄的MessageBoxWindow对象,若未找到则返回nullptr
	 * @return													: MessageBoxWindow对象指针或nullptr
	 * 
	 * @param hWnd												: 要获取的窗口句柄
	 */
	static MessageBoxWindow* GetMessageBoxByHandle(HWND hWnd) {
		std::lock_guard<std::mutex> lock(boxesMutex);

		for (auto* msgBox : allMessageBoxes) {
			if (msgBox->GetHandle() == hWnd) {
				return msgBox;
			}
		}

		return nullptr;
	}
};



int Msgbox::MsgBox_X = 0;
int Msgbox::MsgBox_Y = 0;
std::vector<MessageBoxWindow*> Msgbox::allMessageBoxes;
std::atomic<int> Msgbox::nextId(0);
std::mutex Msgbox::boxesMutex;
std::condition_variable Msgbox::newBoxCV;



#endif                                                          ///< !MESSAGEBOX_WINDOW_HPP
