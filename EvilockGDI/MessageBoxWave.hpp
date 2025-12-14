#ifndef MESSAGEBOX_WAVE_HPP
#define MESSAGEBOX_WAVE_HPP

#include "MessageBoxWindow.hpp"

class WaveEffect {
private:



	/**
	 * @brief													: 波浪粒子结构体
	 * @details													: 存储单个波浪粒子的状态和位置
	 */
	struct WaveParticle {
		MessageBoxWindow* window;
		int currentX, currentY;									///< 当前位置
		bool isAlive;
		int index;												///< 在队列中的索引

		WaveParticle(MessageBoxWindow* win, int x, int y, int idx)
			: window(win), currentX(x), currentY(y), isAlive(true), index(idx) {
		}
	};



	static std::list<WaveParticle> particles;					///< 活动粒子列表
	static std::mutex particlesMutex;							///< 粒子列表互斥锁
	static std::atomic<bool> isWaveRunning;						///< 波浪特效运行标志
	static std::thread waveThread;								///< 波浪特效线程



	static std::vector<std::pair<int, int>> trajectoryPoints;	///< 轨迹点队列
	static int currentTrajectoryIndex;							///< 当前轨迹段索引
	static float progress;										///< 在当前轨迹段中的进度 (0.0 - 1.0)
	static int moveStep;										///< 移动步长
	static int spacing;											///< 窗口间距
	static int screenWidth, screenHeight;



public:



	/**
	 * @brief													: 创建轨迹波浪特效（指定起始位置）
	 * @details													: 创建一系列消息框沿预定义轨迹移动形成波浪效果
	 * 
	 * @param[in]		text									: 消息文本
	 * @param[in]		caption									: 标题
	 * @param[in]		uType									: 消息框类型
	 * @param[in]		startX									: 起始位置X坐标（-1表示停止特效）
	 * @param[in]		startY									: 起始位置Y坐标（-1表示停止特效）
	 * @param[in]		queueLength								: 队列长度
	 * @param[in]		stepSize								: 移动步长
	 * @param[in]		windowSpacing							: 窗口间距
	 * @param[in]		creationDelay							: 创建延迟（毫秒）
	 */
	static void CreateWaveEffectAt(const std::wstring& text, const std::wstring& caption,
		UINT uType, int startX, int startY, int queueLength = 15, int stepSize = 4,
		int windowSpacing = 25, int creationDelay = 150) {

		if (startX == -1 && startY == -1) {
			StopWaveEffect();
			return;
		}														///< 如果传入(-1, -1),则停止特效

		if (isWaveRunning) {
			StopWaveEffect();
		}

		isWaveRunning = true;

		
		screenWidth = GetSystemMetrics(SM_CXSCREEN);
		screenHeight = GetSystemMetrics(SM_CYSCREEN);			///< 获取屏幕尺寸

		
		DefineTrajectory(startX, startY);						///< 定义运动轨迹点,使用指定的起始位置

		currentTrajectoryIndex = 0;
		progress = 0.0f;
		moveStep = stepSize;
		spacing = windowSpacing;

		waveThread = std::thread([=]() {
			WaveEffectThread(text, caption, uType, queueLength, creationDelay);
			});
	}



	/**
	 * @brief													: 创建轨迹波浪特效（使用默认起始位置）
	 * @details													: 创建一系列消息框沿预定义轨迹移动形成波浪效果
	 * 
	 * @param[in]		text									: 消息文本
	 * @param[in]		caption									: 标题
	 * @param[in]		uType									: 消息框类型
	 * @param[in]		queueLength								: 队列长度,默认15
	 * @param[in]		stepSize								: 移动步长,单位像素,默认4像素
	 * @param[in]		windowSpacing							: 窗口间距,单位像素,默认25像素
	 * @param[in]		creationDelay							: 创建延迟,单位毫秒,默认150毫秒
	 */
	static void CreateWaveEffect(const std::wstring& text, const std::wstring& caption,
		UINT uType, int queueLength = 15, int stepSize = 4, int windowSpacing = 25,
		int creationDelay = 150) {

		
		int startX = 0;
		int startY = GetSystemMetrics(SM_CYSCREEN) / 2;			///< 使用默认起始位置（屏幕左侧中间）

		CreateWaveEffectAt(text, caption, uType, startX, startY, queueLength, stepSize,
			windowSpacing, creationDelay);
	}



	/**
	 * @brief													: 创建自定义轨迹的波浪特效
	 * @details													: 创建一系列消息框沿指定轨迹点移动形成波浪效果
	 * 
	 * @param[in]		text									: 消息文本
	 * @param[in]		caption									: 标题
	 * @param[in]		uType									: 消息框类型
	 * @param[in]		customTrajectory						: 自定义轨迹点列表
	 * @param[in]		queueLength								: 队列长度,默认15
	 * @param[in]		stepSize								: 移动步长,单位像素,默认4像素
	 * @param[in]		windowSpacing							: 窗口间距,单位像素,默认25像素
	 * @param[in]		creationDelay							: 创建延迟,单位毫秒,默认150毫秒
	 */
	static void CreateCustomWaveEffect(const std::wstring& text, const std::wstring& caption,
		UINT uType, const std::vector<std::pair<int, int>>& customTrajectory,
		int queueLength = 15, int stepSize = 4, int windowSpacing = 25, int creationDelay = 150) {

		if (isWaveRunning) {
			StopWaveEffect();
		}

		isWaveRunning = true;

		screenWidth = GetSystemMetrics(SM_CXSCREEN);
		screenHeight = GetSystemMetrics(SM_CYSCREEN);			///< 获取屏幕尺寸

		trajectoryPoints = customTrajectory;					///< 使用自定义轨迹

		
		if (trajectoryPoints.size() >= 2 &&
			(trajectoryPoints.front().first != trajectoryPoints.back().first ||
				trajectoryPoints.front().second != trajectoryPoints.back().second)) {
			trajectoryPoints.push_back(trajectoryPoints.front());
		}														///< 确保轨迹是闭合的（最后一个点回到第一个点）

		currentTrajectoryIndex = 0;
		progress = 0.0f;
		moveStep = stepSize;
		spacing = windowSpacing;

		waveThread = std::thread([=]() {
			WaveEffectThread(text, caption, uType, queueLength, creationDelay);
			});
	}



	/**
	 * @brief													: 停止波浪特效
	 * @details													: 停止所有消息框的创建和移动,并关闭现有的消息框
	 */
	static void StopWaveEffect() {
		isWaveRunning = false;
		if (waveThread.joinable()) {
			waveThread.join();
		}

		std::lock_guard<std::mutex> lock(particlesMutex);
		for (auto& particle : particles) {
			if (particle.window && particle.window->IsAlive()) {
				particle.window->Close();
			}
		}
		particles.clear();
		trajectoryPoints.clear();
	}



	/**
	 * @brief													: 检查波浪特效是否在运行
	 * details													: 返回当前波浪特效的运行状态
	 */
	static bool IsWaveRunning() {
		return isWaveRunning;
	}


	/**
	 * @brief													: 获取当前队列长度
	 * @details													: 返回当前活跃的消息框数量
	 */
	static int GetQueueLength() {
		std::lock_guard<std::mutex> lock(particlesMutex);
		return static_cast<int>(particles.size());
	}



	/**
	 * @brief 													: 获取当前轨迹点
	 */
	static std::vector<std::pair<int, int>> GetTrajectoryPoints() {
		return trajectoryPoints;
	}



private:



	/**
	 * @brief													: 定义轨迹点
	 * @details													: 基于指定的起始位置定义一条预设轨迹
	 * 
	 * @param[in]		startX									: 起始位置X坐标
	 * @param[in]		startY									: 起始位置Y坐标
	 */
	static void DefineTrajectory(int startX, int startY) {
		trajectoryPoints.clear();

		///< 基于指定的起始位置定义轨迹
		///< 轨迹：起始点 -> 中间底部 -> 右上角 -> 回到起始点
		trajectoryPoints.push_back({ startX, startY });
		trajectoryPoints.push_back({ screenWidth / 2, screenHeight - 100 });
		trajectoryPoints.push_back({ screenWidth - 100, 100 });
		trajectoryPoints.push_back({ startX, startY });
	}



	/**
	 * @brief													: 波浪特效线程函数
	 * @details													: 持续创建和更新消息框位置,形成波浪效果
	 * 
	 * @param[in]		text									: 消息文本
	 * @param[in]		caption									: 消息标题
	 * @param[in]		uType									: 消息框类型
	 * @param[in]		queueLength								: 队列长度
	 * @param[in]		creationDelay							: 创建延迟,单位毫秒
	 */
	static void WaveEffectThread(const std::wstring& text, const std::wstring& caption,
		UINT uType, int queueLength, int creationDelay) {

		int creationCounter = 0;
		std::vector<std::pair<int, int>> particleTrail;			///< 记录头部历史位置

		while (isWaveRunning) {
			UpdateHeadPosition();								///< 更新头部位置（沿着轨迹移动）

			particleTrail.push_back({GetCurrentHeadPosition()});///< 记录头部位置到轨迹历史

			
			if (particleTrail.size() > static_cast<size_t>(queueLength)) {
				particleTrail.erase(particleTrail.begin());
			}													///< 保持轨迹历史长度与队列长度一致

			if (particles.size() < static_cast<size_t>(queueLength) && creationCounter <= 0) {
				CreateNewParticle(text, caption, uType);
				creationCounter = creationDelay / 16;
			}													///< 创建新粒子(如果队列不够长)

			if (creationCounter > 0) {
				creationCounter--;
			}

			UpdateParticlePositions(particleTrail);				///< 更新所有粒子的位置（沿着相同的轨迹）

			CleanupDeadParticles();								///< 清理死亡粒子

			std::this_thread::sleep_for(
				std::chrono::milliseconds(16));					///< ~60fps
		}
	}



	/**
	 * @brief													: 更新头部位置
	 * @details													: 计算头部沿轨迹移动的新位置
	 */
	static void UpdateHeadPosition() {
		if (trajectoryPoints.size() < 2) return;

		int currentIdx = currentTrajectoryIndex;
		int nextIdx = (static_cast<unsigned long long>(currentIdx) + 1) % trajectoryPoints.size();

		const auto& startPoint = trajectoryPoints[currentIdx];
		const auto& endPoint = trajectoryPoints[nextIdx];

		float dx = static_cast<float>(endPoint.first - startPoint.first);
		float dy = static_cast<float>(endPoint.second - startPoint.second);
		float distance = std::sqrt(dx * dx + dy * dy);			///< 计算两点之间的距离

		if (distance == 0) return;								///< 避免除零

		float progressIncrement = moveStep / distance;
		progress += progressIncrement;							///< 计算每一步的进度增量

		if (progress >= 1.0f) {
			progress = 0.0f;
			currentTrajectoryIndex = nextIdx;
		}														///< 如果完成当前段,移动到下一段
	}



	/**
	 * @brief													: 获取当前头部位置
	 * @details													: 计算头部在当前轨迹段中的插值位置
	 * @return													: 当前头部位置的坐标对 (x, y)
	 */
	static std::pair<int, int> GetCurrentHeadPosition() {
		if (trajectoryPoints.size() < 2) return { 0, 0 };

		int currentIdx = currentTrajectoryIndex;
		int nextIdx = (static_cast<unsigned long long>(currentIdx) + 1) % trajectoryPoints.size();

		const auto& startPoint = trajectoryPoints[currentIdx];
		const auto& endPoint = trajectoryPoints[nextIdx];

		///< 线性插值计算当前位置
		float x = startPoint.first + (endPoint.first - startPoint.first) * progress;
		float y = startPoint.second + (endPoint.second - startPoint.second) * progress;

		return { static_cast<int>(x), static_cast<int>(y) };
	}



	/**
	 * @brief													: 创建新粒子
	 * @details													: 在当前头部位置创建一个新的消息框粒子
	 * 
	 * @param[in]		text									: 消息文本
	 * @param[in]		caption									: 消息标题
	 * @param[in]		uType									: 消息框类型
	 */
	static void CreateNewParticle(const std::wstring& text, const std::wstring& caption, UINT uType) {
		auto headPos = GetCurrentHeadPosition();

		auto* msgBox = Msgbox::ShowAsync(nullptr, text, caption, uType, headPos.first, headPos.second);
		if (msgBox) {
			int index = static_cast<int>(particles.size());
			WaveParticle newParticle(msgBox, headPos.first, headPos.second, index);

			std::lock_guard<std::mutex> lock(particlesMutex);
			particles.push_back(newParticle);
		}
	}



	/**
	 * @brief													: 更新所有粒子位置
	 * @details													: 根据轨迹历史更新每个粒子的位置
	 * 
	 * @param[in]		trail									: 头部位置的历史轨迹
	 */
	static void UpdateParticlePositions(const std::vector<std::pair<int, int>>& trail) {
		std::lock_guard<std::mutex> lock(particlesMutex);

		if (particles.empty() || trail.empty()) return;

		///< 每个粒子对应轨迹历史中的一个位置
		auto particleIt = particles.begin();

		///< 从轨迹历史的尾部开始分配（最新的位置给第一个粒子）
		int trailIndex = static_cast<int>(trail.size()) - 1;

		for (; particleIt != particles.end() && trailIndex >= 0; ++particleIt, --trailIndex) {
			if (trailIndex < static_cast<int>(trail.size())) {
				const auto& targetPos = trail[trailIndex];

				particleIt->currentX = targetPos.first;
				particleIt->currentY = targetPos.second;

				///< 移动窗口到新位置
				if (particleIt->window && particleIt->window->IsAlive()) {
					SetWindowPos(particleIt->window->GetHandle(), nullptr,
						particleIt->currentX, particleIt->currentY, 0, 0,
						SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
				}
			}
		}
	}



	/**
	 * @brief													: 清理死亡粒子
	 * @details													: 移除不再存活的粒子并关闭其窗口
	 */
	static void CleanupDeadParticles() {
		std::lock_guard<std::mutex> lock(particlesMutex);

		auto it = particles.begin();
		while (it != particles.end()) {
			if (!it->isAlive || !it->window || !it->window->IsAlive()) {
				if (it->window) {
					it->window->Close();
					delete it->window;
				}
				it = particles.erase(it);
			}
			else {
				++it;
			}
		}

		int index = 0;
		for (auto& particle : particles) {
			particle.index = index++;
		}														///< 重新索引
	}
};



std::list<WaveEffect::WaveParticle> WaveEffect::particles;
std::mutex WaveEffect::particlesMutex;
std::atomic<bool> WaveEffect::isWaveRunning(false);
std::thread WaveEffect::waveThread;



std::vector<std::pair<int, int>> WaveEffect::trajectoryPoints;
int WaveEffect::currentTrajectoryIndex = 0;
float WaveEffect::progress = 0.0f;
int WaveEffect::moveStep = 4;
int WaveEffect::spacing = 25;
int WaveEffect::screenWidth = 0;
int WaveEffect::screenHeight = 0;

#endif                                                          ///< !MESSAGEBOX_WAVE_HPP
