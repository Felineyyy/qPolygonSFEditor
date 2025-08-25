你现在的 README 基本是纯文本，放到 GitHub 上就缺少 Markdown 的格式化。GitHub 默认支持 **Markdown**，你只要加上 `# 标题`、`**加粗**`、代码块 `...` 就能排版得很清楚。

我帮你整理成一个规范的 `README.md`：

````markdown
# qPolygonSFEditor 插件说明

本插件需放置在 `CloudCompare/plugins/core/Standard/` 文件夹下。  
CloudCompare 源码文件请从官方仓库获取：  
👉 [CloudCompare 官方 GitHub](https://github.com/CloudCompare/CloudCompare)

---

## CMakeLists 修改

在 `CloudCompare/plugins/core/Standard/CMakeLists.txt` 中添加本插件路径：

```cmake
add_subdirectory( qSRA )
add_subdirectory( qMeshBoolean )
add_subdirectory( qPolygonSFEditor ) # 添加本插件路径
````

> 其他地方不需要修改。

---

## CloudCompare 编译安装

* 本插件默认安装为 **ON**：

```cmake
option( PLUGIN_STANDARD_QPOLYGONSF_EDITOR "Install qPolygonSFEditor plugin" ON )
```

* 其他插件可在各自 `CMakeLists.txt` 中将默认的 `OFF` 改为 `ON`。
* CloudCompare 源码 build 后，需要执行 `make install` 来指定插件和部分工具运行路径。
  多数情况下需要系统权限：

```bash
sudo make install
```

* 安装完成后，直接输入以下命令即可运行：

```bash
CloudCompare
```

---

## qPolygonSFEditor 使用说明

插件内置绘图逻辑与 **segment** 基本一致。

1. 选定文件
2. 选定需要修改的 SF
3. 点击插件工具中 **Polygon SF Editor**
4. 开始绘制多边形

   * 左键点击：绘制
   * 右键：结束绘制
5. 自动弹出改值窗口，输入目标值，按下 **Enter** 即可修改
6. 修改完成后会默认退出插件，下次修改从步骤 **3 \~ 5** 继续

---
