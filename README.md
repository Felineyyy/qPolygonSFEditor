qPolygonSFEditor文件夹需放置在CloudCompare/plugins/core/Standard文件夹下。
CloudCompare源码文件从官方https://github.com/CloudCompare/CloudCompare自行拉取。

CMakeLists修改
对于CloudCompare/plugins/core/Standard文件夹下的CMakeLists.txt，在文件路径中添加qPolygonSFEditor：
....
add_subdirectory( qSRA )
add_subdirectory( qMeshBoolean )
add_subdirectory( qPolygonSFEditor ) //添加本插件路径

其他地方均不需要做修改。

CloudCompare编译安装
本qPolygonSFEditor插件安装默认为ON，其他插件通过cmake自行指定。其他想改为默认安装的插件也可以在各自目录下的CMakeLists中将默认的OFF部分改为ON：
option( PLUGIN_STANDARD_QPOLYGONSF_EDITOR "Install qPolygonSFEditor plugin" ON )
CloudCompare源码默认build后需要make install来指定插件和部分工具运行路径。由于涉及到路径修改，多数情况下需要系统权限：
sudo make install
而后运行直接输入程序名即可运行：
CloudCompare

qPolygonSFEditor使用说明
qPolygonSFEditor内置绘图逻辑与segment基本一致。
1.选定文件
2.选定需要修改的SF
3.点击插件工具中Polygon SF Editor
4.开始绘制多边形，左键点击绘制，右键结束绘制
5.自动弹出改值窗口，输入目标值，按下Enter即可修改
6.值修改后会默认退出插件，下次修改冲入3~5步骤
