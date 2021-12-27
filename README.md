# Vulkan教程

## 学习途径

- 教程
  - [极客教程（译）](https://geek-docs.com/vulkan/vulkan-tutorial/vulkan-tutorial-index.html)：概念讲解，主要是Vulkan中各个数据结构的概念及使用。
  - [https://vulkan-tutorial.com](https://vulkan-tutorial.com)一个比较火的Vulkan基础教程，文档和代码齐全。
  - [https://www.vulkan.org/learn](https://www.vulkan.org/learn)：vulkan学习的整合网站，里面有很多资源和工具。
  - [https://developer.nvidia.com/transitioning-opengl-vulkan](https://developer.nvidia.com/transitioning-opengl-vulkan)：如何从OpenGL转到Vulkan。
- 代码示例：从代码上手学习绝对是最快的一种方式，但需要你对Vulkan的整体结构都有一定认识，否则你会走很多弯路，下面的网站中有很多的样例：
  - [https://github.com/Overv/VulkanTutorial](https://github.com/Overv/VulkanTutorial)：教程的附带代码。
  - [https://github.com/KhronosGroup/Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp)：Vulkan官方提供了很多使用Vulkan HPP的样例
  - [https://github.com/SaschaWillems/Vulkan](https://github.com/SaschaWillems/Vulkan)：拥有非常完善的样例，涵盖了大多数Vulkan的特性和一些高级图形算法。

### 推荐資料

- Vulkan 官方文档 
  - [https://www.khronos.org/registry/vulkan/specs](https://www.khronos.org/registry/vulkan/specs/)
  - [https://www.khronos.org/registry/vulkan/#apispecs](https://www.khronos.org/registry/vulkan/#apispecs)

- 图形学：

  网站：

  - **实时渲染的知识整合**：[http://www.realtimerendering.com](http://www.realtimerendering.com)
  - OpenGL官网上的一些示例：[https://www.opengl.org/archives/resources/code/samples/sig99/advanced99/notes/node1.html ](https://www.opengl.org/archives/resources/code/samples/sig99/advanced99/notes/node1.html )

  书籍：

  - 《GPU Gems》[https://developer.nvidia.com/gpugems/gpugems/foreword](https://developer.nvidia.com/gpugems/gpugems/foreword)
  - 《3D数学基础》《全局光照算法》《计算机图形学》...

  <img src="https://i0.hdslb.com/bfs/article/266b78da71b4185f239f3508e6f5c3dc9de7aa91.png@612w_641h_progressive.webp" width="200px">

  教程：（Games系列）：[https://www.bilibili.com/video/BV1X7411F744](https://www.bilibili.com/video/BV1X7411F744)

- 引擎

  - UE ：[https://www.cnblogs.com/timlly/](https://www.cnblogs.com/timlly/)
  
  - 《游戏引擎架构》
  
    <img src="https://bkimg.cdn.bcebos.com/pic/2cf5e0fe9925bc3122742eca5cdf8db1cb1370b2?x-bce-process=image/watermark,image_d2F0ZXIvYmFpa2UxMTY=,g_7,xp_5,yp_5/format,f_auto" width="180px">
  

## [Vulkan与OpenGL的区别](https://geek-docs.com/vulkan/vulkan-tutorial/vulkan-and-opengl.html)

我的使用体验：

- 繁琐：OpenGL绘制基本三角形可能需要一两百行代码，Vulkan的代码量得翻几倍。
- 明确：OpenGL通过对一个假想的渲染管线进行操作，而Vulkan中能确切地接触到渲染管线的结构。
- 刺激：OpenGL出现内存泄漏，顶多说把进程搞崩溃，Vulkan中你就等着重启吧。
- 巴适：开启了验证层的Vulkan拥有报错提示，可以明确指出问题在哪。

### 为什么要学Vulkan

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

目前vulkan的教程整体比较少，大多是一些零零散散的概念和代码，国外有一些不错的教程，但也存在一些问题：一大堆的文字概念，代码封装的到处都是，Demo写的是狂拽酷炫，看完效果：”卧槽，秀。卧槽，nb“，再看代码：”我尼玛“，这对初学者来说是很不友好的，这也是笔者为什么想写这个教程的原因，由于笔者工作上的事情比较多，会提前写好代码，文档可能写的会比较慢，还请大家见谅。

## 开发环境

- Vulkan - SDK （**Vulkan - HPP**）

  > 使用Vulkan必须安装[Vulkan SDK](https://vulkan.lunarg.com/)，Vulkan使用C风格的API，跟OpenGL一样恶心——使用宏来定义各种类型格式
  >
  > 不过好在Vulkan提供了c++绑定，位于头文件`vulkan.hpp`中，使用它我们可以编写可读性更好的代码。

- Qt 6.2.0（如果你已经有Qt5，则无需更换）

  > Qt提供Vulkan的窗口支持，使用它，可以在学习初期跳过Surface，SwapChain，ComandBuffer，Device等一系列跟渲染关联不大的内容，快速上手Vulkan核心的渲染流程，从而大大降低学习难度。

- VS 2019（如果你有VS2017，无需更换）

  > VS 加上两个插件（ 番茄助手 + CodeMaid）后，编码体验比Qt Creator 好一些 ，但安装VS的主要原因还是因为 MSVC 比 MinGW 更通用，你也可以在Qt Creator 上使用 MSVC。

- Cmake 64 位

  > 很多初学的小伙伴原来可能没接触过自动化的项目构建，都是使用IDE的图形界面来管理项目工程，这在小项目中使用是没什么问题的，大项目中会存在很多子项目，各种依赖都需要定义，还有就是项目跨平台的问题，比如mac上没有vs，开发一般使用xcode，你拿个vs的工程给xcode也不行啊，cmake是一个不错的选择，使用它可以生成各个IDE的工程文件。在开源项目中能看到有个CMakeLists.txt，别提有多亲切了，最重要的是使用cmake能做一些自动化的构建，比如vulkan的shader使用的是sprv格式，使用前必须使用命令行工具转换，通过cmake我们就可以直接把glsl添加到项目里，当代码发生变动时，自动调用脚本进行处理。

- Git & TortoiseGit

  > Git是当前比较主流的版本管理工具 ，而TortoiseGit是它的图形界面，基本现在在实际工作都会使用，还不会用的小伙伴可以学下。

### 推荐工具

- [Typora](https://www.typora.io/)：Markdown编辑器，Markdown是一种[轻量级标记语言](https://baike.baidu.com/item/轻量级标记语言/52671915)，使用它可以快速编写美观的文档，现在使用非常普遍，它的语法很简单，大家可以学下。

  

**如果你没有上述环境的话，请[点击这里](./Doc/环境搭建.md)查阅环境搭建文档**



## 构建 Build

如果你刚刚才安装上面的环境，请先重启电脑

使用 Qt Creator 或 VS 可直接打开 CmakeLists.txt

推荐使用Cmake GUI 打开 CMakeLists.txt （或者将其拖入CMake GUI），修改构建路径为[QtVulkan-build]，防止生成新文件在原先的代码目录下

![](https://github.com/Italink/QtVulkan/blob/main/Doc/Src/Cmake01.png)

然后点击【Configure】进行配置，选择【Visual Studio 16 2019】（如果你是 2017 则选 2017 的版本）

![](https://github.com/Italink/QtVulkan/blob/main/Doc/Src/Cmake02.png)

配置过程中可能会出现报错，这是因为初次使用，导致CMake无法找到Qt的目录，你在红色的条目上能看到它的Value是【Not Founded】，点击该条目，右侧会出现一个按钮，点击它选择路径，按下方的路径根据你自己的QT安装路径来进行选择，选择完毕点击【Configure】，然后继续选择路径消除报错，直到没有报错产生。

![](https://github.com/Italink/QtVulkan/blob/main/Doc/Src/Cmake03.png)

然后再点击【Generate】，就会再构建目录下生成VS工程文件，再点击【Open Project】可打开 VS 工程 

在解决方案菜单中，右键项目，在菜单栏点击【设为启动项】，可运行对应项目

![](https://github.com/Italink/QtVulkan/blob/main/Doc/Src/Cmake04.png)

## 项目结构说明

- QuickStart ：Vulkan的入门样例
- Advance：Vulkan的进阶样例
