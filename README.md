# 实时图形渲染引擎

基于 OpenGL 3.3+ 的实时 3D 渲染引擎，包含"双世界"时空场景，可在古代中式庭院与现代废墟之间平滑过渡。

## 功能特性

### 渲染管线
- 前向 + 延迟渲染，G-Buffer 多渲染目标
- 阴影贴图，5x5 PCF 软阴影（2048x2048）
- Bloom 泛光后处理 + 高斯模糊
- 双世界时间过渡特效

### 环境系统
- 昼夜循环，动态天空盒切换
- 水面渲染：双层法线贴图 + 菲涅尔反射/折射
- GPU 实例化草地，带风力动画
- 粒子系统（雨/雪天气效果）
- 程序化地形，多纹理混合（沙地、岩石、泥土）

### 程序化生成
- 体素树系统，可配置密度
- 程序化树木生成，含树叶放置
- 地形感知植被定位

### 资产渲染
- GLTF 模型加载（中式古建筑、动物动画）
- 骨骼动画系统
- 多材质支持（瓦片屋顶、石墙、木材）

### 场景管理
- 场景图 + 节点变换体系
- FBO 离屏渲染管理器
- Shader 热重载（F5）
- 全屏立方体贴图天空盒

## 技术栈

- **语言：** C++
- **图形 API：** OpenGL 3.3+
- **框架：** NCLGL（纽卡斯尔图形库）
- **模型格式：** GLTF
- **纹理加载：** SOIL
- **构建系统：** Visual Studio / CMake

## 操作方式

| 按键 | 功能 |
|------|------|
| WASD | 相机移动 |
| 鼠标 | 视角旋转 |
| F5 | 重载着色器 |
| T | 切换时空过渡 |

## 构建

用 Visual Studio 打开 `GraphicsTutorials.sln`，构建 `Coursework` 项目。

## 项目结构

```
Coursework/          # 主程序
  Renderer           # 核心渲染器
  DeferredRenderer   # 延迟渲染管线
  ShadowSystem       # 阴影映射
  WaterRenderer      # 水面效果
  GrassRenderer      # 实例化草地
  ParticleSystem     # 天气粒子
  DayNightCycle      # 昼夜循环
  SkeletalAnimator   # 骨骼动画
  TreeSystem         # 程序化树木
  BlockRenderer      # 体素渲染
Shaders/             # GLSL 着色器
Textures/            # 纹理资源
Meshes/              # 3D 模型文件
nclgl/               # 图形框架
```
