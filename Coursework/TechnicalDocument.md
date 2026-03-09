# CSC8502 Real-Time Graphics 技术文档

## 一、项目概述

本项目是一个基于 **OpenGL 3.3+ Core Profile** 的实时3D渲染引擎，实现了一个包含"古代"与"现代"两个时代场景的动态世界。场景包含程序化生成的地形、水体、植被、动物、粒子天气系统、日夜循环、骨骼动画以及完整的后处理管线。项目采用模块化渲染管线架构，支持前向渲染与延迟渲染双通道。

---

## 二、核心渲染架构

### 2.1 渲染管线架构 (Render Pipeline)

采用 **模块化 RenderPass 架构**，每个渲染阶段封装为独立的 `RenderPass` 子类，通过共享的 `RenderContext` 传递数据：

| 渲染通道 | 职责 |
|---------|------|
| `ShadowPass` | 阴影贴图生成（深度渲染） |
| `ScenePass` / `DeferredScenePass` | 主场景渲染（前向/延迟） |
| `WaterPass` | 水面渲染 |
| `ParticlePass` | 粒子系统渲染 |
| `BloomPass` | 泛光后处理 |
| `TransitionPass` | 时代切换过渡效果 |
| `PresentPass` | 最终输出到屏幕 |

**RenderContext** 作为所有通道的共享数据容器，包含相机矩阵、光照参数、时间变量、FBO句柄等。

### 2.2 前向渲染 (Forward Rendering)

传统的单通道光照计算，每个片元直接计算光照。用于透明物体（水面、草地、粒子）和天空盒。

### 2.3 延迟渲染 (Deferred Rendering)

**G-Buffer 多渲染目标 (MRT)**：

| 附件 | 格式 | 存储内容 |
|-----|------|---------|
| Color 0 | `GL_RGBA16F` | 世界空间位置 + 深度 |
| Color 1 | `GL_RGBA16F` | 世界空间法线 + 镜面强度 |
| Color 2 | `GL_RGBA8` | 漫反射颜色 + 镜面指数 |
| Depth | `GL_DEPTH_COMPONENT24` | 深度缓冲 |

**延迟光照通道**支持最多 **32个动态点光源**，使用屏幕空间球体代理几何体实现高效多光源渲染。光源支持三种动画模式：闪烁(Flicker)、脉冲(Pulse)、随机(Random)。

光照衰减采用**平方反比衰减模型**：

```
attenuation = 1.0 / (1.0 + dist/radius + dist²/radius²)
```

---

## 三、光照与阴影系统

### 3.1 Blinn-Phong 光照模型

全场景使用 **Blinn-Phong** 着色模型，结合环境光、漫反射和镜面反射三分量。镜面高光使用半角向量（Half-Vector）计算。

### 3.2 阴影映射 (Shadow Mapping)

- **阴影贴图分辨率**：2048 × 2048
- **投影方式**：正交投影（方向光）
- **采样方式**：**5×5 PCF (Percentage-Closer Filtering)**，25次采样取平均实现软阴影
- **自适应偏移 (Adaptive Bias)**：根据表面法线与光照方向夹角动态调整偏移量，范围 `[0.001, 0.01]`
- **边界处理**：超出阴影贴图范围的片元默认无阴影

---

## 四、程序化地形生成

### 4.1 地形算法 (CustomTerrain)

基于**余弦函数叠加**的程序化地形生成，非传统高度图加载：

- **盆地 (Basin)**：使用余弦平滑凹陷函数，生成湖泊区域
- **山丘 (Hill)**：使用余弦隆起函数，生成寺庙放置区域
- **河谷 (River Valley)**：沿指定方向的余弦切割，连接湖泊与地图边缘
- **法线计算**：采用中心差分法 (Central Difference) 从高度场推导法线

### 4.2 多纹理地形混合

基于**高度和坡度的多纹理混合**：

| 条件 | 纹理 |
|-----|------|
| 坡度 > 0.6 | 岩石 (Rock) |
| 高度 > 阈值 | 雪/泥 |
| 高度 > 中阈值 | 草地 (Grass) |
| 默认 | 砂土 (Sand) |

使用 `smoothstep` 实现纹理间的平滑过渡。

---

## 五、水体渲染

### 5.1 技术要点

- **双层法线贴图动画 (Dual-Layer Normal Mapping)**：两层法线贴图以不同速度和方向滚动叠加，模拟水面波纹
- **Fresnel 反射**：基于视角的菲涅尔系数控制反射/折射比例
- **深度着色**：基于水深的颜色渐变（浅水区 → 深水区）
- **镜面高光**：集中的高光指数模拟水面光泽
- **透明度混合**：`GL_SRC_ALPHA / GL_ONE_MINUS_SRC_ALPHA`

---

## 六、GPU 实例化渲染

### 6.1 草地渲染 (GrassRenderer)

- **单次绘制调用 (Single Draw Call)** 渲染数千草叶
- 每个实例携带：位置、旋转、缩放、随机种子
- **顶点着色器风动画**：基于正弦波的程序化摆动，风强度可调
- **次表面散射近似 (Subsurface Scattering)**：背光时的半透明效果
- **区域排除**：自动避开山丘、湖泊、地图边缘

### 6.2 体素方块渲染 (BlockRenderer)

- 实例化渲染大量体素方块
- 每实例带法线贴图和纹理变化

### 6.3 体素树系统 (VoxelTreeSystem)

- **程序化树木生长**：递归分支生长算法
- **地形感知放置**：自动查询地形高度放置树木
- **LOD 支持**：基于距离的细节层次

---

## 七、骨骼动画系统

### 7.1 SkeletalAnimator

- **关键帧插值**：位置(LERP) + 旋转(SLERP) + 缩放(LERP) 三通道独立插值
- **骨骼层级遍历**：递归父子骨骼变换矩阵累乘
- **Gram-Schmidt 正交化**：防止浮点精度累积导致矩阵退化，每帧对旋转矩阵进行重正交化
- **程序化回退动画**：当无动画数据时，自动生成基于正弦波的闲置动画（身体摆动 + 腿部行走循环）
- **最大关节数**：128（通过 Uniform 数组传递 `jointMatrices[128]`）
- **4骨骼蒙皮**：每顶点最多受4根骨骼影响，权重归一化

### 7.2 GLTF 模型加载

支持加载 GLTF 2.0 格式的3D模型：
- 中国古庙 (Chinese Temple)
- 狼 (Wolf) — 带骨骼动画
- 公牛 (Bull) — 带骨骼动画

---

## 八、日夜循环系统

### 8.1 DayNightCycle

- **球坐标太阳定位**：太阳沿半球弧形轨迹移动，使用 `sin/cos` 计算空间位置
- **时间段色彩插值**：

| 时间 | 场景 | 光照颜色 |
|-----|------|---------|
| 0:00 - 5:00 | 夜晚 | 深蓝色调 |
| 5:00 - 7:00 | 日出 | 暖橙过渡 |
| 7:00 - 12:00 | 上午 | 渐亮 |
| 12:00 - 17:00 | 下午 | 渐暗 |
| 17:00 - 19:00 | 日落 | 暖橙过渡 |
| 19:00 - 24:00 | 夜晚 | 深蓝色调 |

### 8.2 动态天空盒切换

三套立方体贴图 (Cubemap) 根据时间段自动切换：
- **日间天空盒** (7:00 - 17:00)
- **日出/日落天空盒** (5:00 - 7:00, 17:00 - 19:00)
- **夜间天空盒** (19:00 - 5:00)

### 8.3 太阳/月亮渲染

使用径向渐变着色器 (`sun.frag`) 绘制太阳和月亮圆盘，颜色随时间动态变化。

---

## 九、粒子系统

### 9.1 ParticleSystem

- **GPU 实例化粒子**：单次绘制调用渲染所有粒子
- **Billboard 技术**：粒子始终面向相机
- **双天气模式**：
  - **雨 (Rain)**：垂直拉伸的蓝色半透明粒子，基于速度的运动模糊
  - **雪 (Snow)**：圆形白色粒子，水平漂移，缓慢下落
- **重力与重生**：粒子受重力下落，低于地面时重置到顶部

---

## 十、后处理管线

### 10.1 泛光效果 (Bloom)

三阶段管线：
1. **亮度提取 (Bright Pass)**：`brightPass.frag` 提取亮度超过阈值的片元
2. **高斯模糊 (Gaussian Blur)**：`gaussianBlur.frag` 9-tap 可分离高斯模糊，水平+垂直两遍
3. **合成 (Composite)**：`bloomComposite.frag` 将模糊结果叠加到原始画面

### 10.2 时代切换过渡效果 (Time Transition)

`timeTransition.frag` 实现 **8层程序化视觉特效**：

| 效果层 | 技术 |
|-------|------|
| 色彩分离 (Chromatic Aberration) | RGB 通道独立偏移采样 |
| 波纹扭曲 (Ripple Distortion) | 基于距离的正弦波UV偏移 |
| 白色闪光 (White Flash) | 全屏亮度叠加 |
| 暗角 (Vignette) | 基于UV距离中心的暗化 |
| 胶片颗粒 (Film Grain) | 伪随机噪声叠加 |
| 色调偏移 (Color Shift) | Hue 旋转 |
| 扫描线 (Scan Lines) | 水平条纹遮罩 |
| 模糊 (Motion Blur) | 多方向采样混合 |

---

## 十一、场景管理

### 11.1 "双世界"系统 (Two Worlds)

通过场景图 (`SceneNode`) 管理两套完整的场景资源：
- **古代世界 (Ancient Era)**：中国古庙、狼群、体素树、草地、雨天
- **现代世界 (Modern Era)**：现代建筑、公牛群、不同植被配置

时代切换时触发过渡特效，并平滑切换所有场景元素。

### 11.2 场景图 (Scene Graph)

层级节点结构，每个 `SceneNode` 包含：
- 本地/世界变换矩阵
- 网格 (Mesh) + 纹理 (Texture)
- 子节点列表
- 边界半径（用于视锥剔除）

---

## 十二、相机系统

### 12.1 双模式相机

- **FPS 手动模式**：WASD + 鼠标控制，支持 Shift 加速
- **路径点自动模式**：预定义路径点序列，相机沿路径点平滑移动并自动朝向目标
- **路径点插值**：位置线性插值，方向球面线性插值

---

## 十三、资源管理与工程实践

### 13.1 RAII 资源管理

全面使用 `std::unique_ptr` 管理 GPU 资源和子系统生命周期：
- 着色器 (Shader)
- 网格 (Mesh)
- 渲染子系统 (GrassRenderer, WaterRenderer, ParticleSystem 等)

### 13.2 条件编译调试宏

每个子系统定义独立的调试宏，实现**零开销日志**：

```cpp
// #define GRASS_VERBOSE  // 取消注释以启用
#ifdef GRASS_VERBOSE
    #define GRASS_LOG(x) std::cout << x << std::endl
#else
    #define GRASS_LOG(x)  // 编译期完全消除
#endif
```

### 13.3 集中配置 (RenderConfig.h)

所有渲染参数集中定义为 `constexpr` 常量，便于调优和维护：
- 阴影贴图分辨率、相机参数、地形参数、草地参数
- 光照配置、水面参数、粒子参数
- 过渡效果参数

---

## 十四、技术栈总结

| 类别 | 技术 |
|-----|------|
| 图形 API | OpenGL 3.3+ Core Profile |
| 着色语言 | GLSL 330 core |
| 光照模型 | Blinn-Phong + 延迟光照 |
| 阴影 | Shadow Mapping + 5x5 PCF |
| 实例化 | `glDrawArraysInstanced` / `glDrawElementsInstanced` |
| 动画 | 骨骼蒙皮 (Skeletal Animation) + 程序化动画 |
| 地形 | 程序化生成 (余弦函数叠加) |
| 水体 | 双层法线贴图 + Fresnel |
| 后处理 | Bloom (高斯模糊) + 程序化过渡特效 |
| 粒子 | GPU 实例化 Billboard |
| 模型格式 | GLTF 2.0 |
| 纹理加载 | SOIL (Simple OpenGL Image Library) |
| 资源管理 | RAII (std::unique_ptr) |
| 构建系统 | Visual Studio / CMake |
| 语言 | C++17 |

---

## 十五、性能优化措施

1. **GPU 实例化**：草地、粒子、体素方块均使用单次绘制调用
2. **延迟渲染**：多光源场景仅对可见片元计算光照
3. **可分离高斯模糊**：将 O(n²) 降为 O(2n) 的两遍模糊
4. **密度控制**：草地实例数量可动态调节，无需重建几何体
5. **条件编译日志**：Release 构建零调试输出开销
6. **集中常量配置**：`constexpr` 编译期求值，无运行时开销

---

## 十六、着色器文件清单

| 文件 | 用途 |
|-----|------|
| `terrain.vert / .frag` | 地形渲染，多纹理混合 + PCF 阴影 |
| `water.vert / .frag` | 水面渲染，双层法线 + Fresnel |
| `sceneNode.vert / .frag` | 场景节点，Phong 光照 + 环境贴图 |
| `shadow.vert / .frag` | 阴影深度通道 |
| `skybox.vert / .frag` | 天空盒立方体贴图 |
| `sun.vert / .frag` | 太阳/月亮径向渐变 |
| `grass_instanced.vert / .frag` | GPU 实例化草地 + 风动画 |
| `particle.vert / .frag` | Billboard 粒子 |
| `block.vert / .frag` | 实例化体素方块 |
| `brightPass.frag` | Bloom 亮度提取 |
| `gaussianBlur.frag` | 可分离高斯模糊 |
| `bloomComposite.frag` | Bloom 合成 + 过渡效果 |
| `timeTransition.frag` | 8层程序化过渡特效 |
| `deferred_geometry.vert / .frag` | G-Buffer MRT 输出 |
| `deferred_lighting.vert / .frag` | 延迟光照计算 |
| `copy.vert / .frag` | 屏幕空间直通 |
| `skinned.vert / .frag` | 骨骼蒙皮动画 |
| `debug_unlit.vert / .frag` | 纯色调试渲染 |
