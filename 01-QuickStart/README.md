## Vulkan Quick Start

该目录提供很多Vulkan编写的样例，它们遵循以下原则：

- 旨在以简洁的代码的样例测试单个特性，而不会使用一推花里胡哨、狂拽酷炫的场景告诉你，这里面用了某个特性。
- 使用Vulkan-HPP提供的结构及枚举增加可读性，但不会使用构造函数。
- 浅封装，保证代码逻辑连贯。



## Example List

####  Ex00 - VulkanWindow

简述：创建Vulkan窗口

- 初始化Vulkan-HPP
- 开启验证层
- CommandBuffer初探

####  Ex01 - HelloTriangle

简述：绘制三角形

- 顶点缓存区（Vertex Buffer）
- 流水线（Pipline）
- 渲染流程（RenderPass）

####  Ex02 - StagingBuffer

简述：HostVisible属性的缓冲区可以使用map，unmap进行读写，但它的结构不具有最佳使用性能，因此在Vulkan中会将数据写入到HostVisible属性的暂存缓冲区中，再将之转移到LocalDevice属性的缓冲区中。

- 暂存缓存区

#### Ex03 - IndexBuffer

- 索引缓冲区

####  Ex04 - UniformBuffer

- Uniform缓冲区

#### Ex05 - SingleBuffer

简述：将缓冲区合并到一起能增加缓存命中率，从而提升性能。

- 单一缓冲区

#### Ex06 - Texture

- 纹理

#### Ex07 - Instancing

- 实例化

#### Ex08 - DynamicUniformBuffer

简述：相当于使用一个大的UniformBuffer来存储很多的uniform数据，dynamic是因为它支持offset。

- 动态Uniform缓冲区

#### Ex09 - PushConstants

简述：无需创建缓冲区，直接向着色器中传输变量。

- 推送常量

#### Ex10 - SpecializtionConstants

简述：在Pipline创建时，为着色器设置常量。

- 特化常量

#### Ex11 - Stencil

简述：使用模板测试为三角形描边

- 模板测试

#### Ex12 - Offscreen

简述：使用vulkan生成一张指定颜色的图像，并保存为本地png格式的图片

- 离屏渲染
- 渲染流程
- 新建CommandBuffer
- 提交队列

#### Ex13 - MultiRenderTarget

简述：帧缓存使用多个颜色附件，并将之保持到本地。

- 多渲染目标

#### Ex14 - Query

简述：遮挡查询，渲染管线状态查询，时间戳查询。

- 查询
