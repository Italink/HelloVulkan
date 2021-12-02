# Vulkan教程

#### 学习途径

- 教程
  - [极客教程（译）](https://geek-docs.com/vulkan/vulkan-tutorial/vulkan-tutorial-index.html)：概念讲解，主要是Vulkan中各个数据结构的概念及使用。
  - https://vulkan-tutorial.com/：一个比较火的Vulkan基础教程，文档和代码齐全。
  - https://www.vulkan.org/learn：vulkan学习的整合网站，里面有很多资源和工具。
  - https://developer.nvidia.com/transitioning-opengl-vulkan：如何从OpenGL转到Vulkan。
- Demo：从代码上手学习绝对是最快的一种方式，但需要你对Vulkan的整体结构都有一定认识，否则你会走很多弯路，下面的网站中有很多的样例：
  - https://github.com/Overv/VulkanTutorial：教程附带代码。
  - https://github.com/KhronosGroup/Vulkan-Hpp：Vulkan官方提供了很多使用Vulkan HPP的样例
  - ！！https://github.com/SaschaWillems/Vulkan：拥有非常完善的样例，涵盖了大多数Vulkan的特性和一些高级图形算法。

#### 推荐資料

Vulkan的书籍不多，由于它只是一个API，一个用于绘图的工具，做引擎和渲染的重点，并不在这上面，所以下面给大家推荐一下我觉得比较不错的资料：

- Vulkan 官方文档 
  - https://www.khronos.org/registry/vulkan/specs/
  - https://www.khronos.org/registry/vulkan/#apispecs

- OpenGL文档里面含有很多图形学内容：https://www.opengl.org/archives/resources/code/samples/sig99/advanced99/notes/node1.html 

- 图形学：

  书籍：

  - 《GPU Gems》https://developer.nvidia.com/gpugems/gpugems/foreword
  - 《3D数学基础》《全局光照算法》《计算机图形学》...

  

  <img src="https://i0.hdslb.com/bfs/article/266b78da71b4185f239f3508e6f5c3dc9de7aa91.png@612w_641h_progressive.webp" alt="img" style="zoom: 50%;" />

  教程：（Games系列）：https://www.bilibili.com/video/BV1X7411F744

- 引擎

  -   UE ：https://www.cnblogs.com/timlly/
  - 《游戏引擎架构》

  <img src="https://bkimg.cdn.bcebos.com/pic/2cf5e0fe9925bc3122742eca5cdf8db1cb1370b2?x-bce-process=image/watermark,image_d2F0ZXIvYmFpa2UxMTY=,g_7,xp_5,yp_5/format,f_auto" style="zoom: 25%;" />



#### [Vulkan与OpenGL的区别](https://geek-docs.com/vulkan/vulkan-tutorial/vulkan-and-opengl.html)

我的使用体验：

- 繁琐：OpenGL绘制基本三角形可能需要一两百行代码，Vulkan的代码量得翻几倍。
- 明确：OpenGL通过对一个假想的渲染管线进行操作，而Vulkan中能确切地接触到渲染管线的结构。
- 刺激：OpenGL出现内存泄漏，顶多说把进程搞崩溃，Vulkan中你就等着重启吧。
- 巴适：开启了验证层的Vulkan拥有报错提示，可以明确指出问题在哪。

#### 为什么要学Vulkan

- 相较于OpenGL，Vulkan允许你掌控更多的细节，这意味着：
  - 更多的优化空间：能针对特定的应用场景优化渲染。
  - 明确地控制渲染流程：不再像OpenGL一样，大多操作交由驱动来实现。
- OpenGL已不做维护，Vulkan是未来。

### 本教程的目的

- 从OpenGL快速上手Vulkan。
- 借助Qt和Vulkan-HPP，编写简短、优雅的代码结构让可读性更高。
- 借助Github的版本管理回顾整体代码，而不是只有代码片段。
- 教程讲述的不只是Vulkan，还包含现代C++的开发流。

### 笔者感言

目前vulkan的教程整体比较少，国内大多是一些零零散散的概念和代码，国外有一些不错的教程，但也存在一些问题：一大堆的文字概念，代码封装的到处都是，Demo写的是狂拽酷炫，看完效果：”卧槽，秀。卧槽，nb“，再看代码：”我尼玛“，这也是笔者为什么想写这个教程的原因，由于笔者工作上的事情比较多，会提前写好代码，文档可能写的会比较慢，还请大家见谅。



## 开发环境

- Vulkan - SDK （**Vulkan - HPP**）

  > 使用Vulkan必须安装[Vulkan SDK](https://vulkan.lunarg.com/)，Vulkan使用C风格的API，跟OpenGL一样恶心——使用宏来定义各种类型格式
  >
  > 不过好在Vulkan提供了c++绑定，位于头文件`vulkan.hpp`中，使用它我们可以编写可读性更高的代码。

- Qt 6.2.0（如果你已经有Qt5，则无需更换）

  > Qt提供Vulkan的窗口支持，使用它，可以在学习初期跳过Surface，SwapChain，ComandBuffer，Device等一系列跟渲染关联不大的内容，快速上手Vulkan核心的渲染体系，从而大大降低学习难度。

- VS 2019（如果你有VS2017，无需更换）

  > VS 加上两个插件（ 番茄助手 + CodeMaid）后，编码体验比Qt Creator 好一些 ，但安装VS的主要原因还是因为 MSVC 比 MinGW 更通用，你也可以在Qt Creator 上使用 MSVC。

- Cmake 64 位

  > 很多初学的小伙伴原来可能没接触过自动化的项目构建，都是使用IDE的图形界面来管理项目工程，这在小项目中使用是没什么问题的，大项目中会存在很多子项目，各种依赖都需要定义，最重要的是项目跨平台的问题，比如mac上没有vs，开发一般使用xcode，你拿个vs的工程给xcode也不行啊，cmake是一个不错的选择（），使用它可以生成各个IDE的工程文件。在开源项目中能看到有个CMakeLists.txt，别提有多亲切了。

- Git & TortoiseGit

  > Git是当前比较主流的版本管理工具 ，而TortoiseGit是它的图形界面，基本现在在实际工作都会使用，还不会用的小伙伴可以学下。

### 推荐工具

- [Typora](https://www.typora.io/)：Markdown编辑器，Markdown是一种[轻量级标记语言](https://baike.baidu.com/item/轻量级标记语言/52671915)，使用它可以快速编写美观的文档，现在使用非常普遍，它的语法很简单，大家可以学下。

  

**如果你没有上述环境的话，请[点击这里](./Doc/环境搭建.md)查阅环境搭建文档**



## 构建 Build
