# ClipboardMonitor-Win32
A lightweight Windows desktop application for logging clipboard content with full Unicode support.
一个轻量级Windows桌面应用，用于监控和记录剪贴板内容，具备完整的Unicode支持。
## Features

- **实时监控剪贴板**：自动捕获并记录所有复制操作
- **完整字符支持**：支持所有Unicode字符，包括空格、换行和特殊符号
- **自定义保存路径**：可选择日志文件保存位置，支持最近路径记忆
- **简单易用**：直观的图形界面，一键开始/停止监控
- **轻量级**：资源占用少，不影响系统性能


## Installation

1. 下载最新版本的可执行文件（.exe）
2. 双击运行，无需安装


## Usage

1. 启动应用程序
2. 选择日志文件保存路径（默认保存到文档目录）
3. 点击"开始监测剪贴板"按钮
4. 所有复制的内容将自动保存到指定文件
5. 点击"停止监测"结束监控


## Technical Details

- **开发语言**：C++
- **平台**：Windows
- **依赖项**：Win32 API, Standard C++ Library
- **编码支持**：UTF-16（带BOM标记）


## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.


## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/your-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin feature/your-feature`)
5. Open a Pull Request


## Acknowledgments

- Thanks to all contributors who helped improve this project
- Inspired by the need for a simple, reliable clipboard monitoring tool
