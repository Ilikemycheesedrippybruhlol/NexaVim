Meet NexaVim: The Power of Vim, Without the Learning Curve
If you’ve ever wanted the lightning-fast speed of NeoVim but felt intimidated by complex configuration files and "keyboard gymnastics," NexaVim was built for you. It bridges the gap between the simplicity of modern code editors (like VS Code) and the raw efficiency of terminal-based editing.

Why Choose NexaVim?
NexaVim is designed to be "Ready-to-Code." It strips away the frustration of setting up a development environment from scratch while keeping the performance benefits of a lightweight editor.

Zero-Config LSP: Forget manual installations. NexaVim comes with pre-configured Language Server Protocols (LSP) that provide instant auto-completion, hover definitions, and error checking for over 50 languages.

Intuitive Keybindings: While it supports classic Vim motions, NexaVim introduces a "Hybrid Mode." Use Shift + S to switch between Vim Controls and NexaVim Controls. 

Built-in Interactive Dashboard: Start your day with a clean, visual overview of your recent projects, active git branches, and a "Tip of the Day" to help you master the editor at your own pace.

Blazing Performance: Written with a core focus on resource efficiency, NexaVim starts up in under 50ms—even with dozens of plugins active.

Getting Started with NexaVim
Setting up NexaVim is designed to be as straightforward as possible. Following the logic that a great tool should be easy to assemble, you can go from source code to a fully functional editor in just three simple steps.

Installation & Compilation
To get NexaVim running on your system, open your terminal and run the following sequence of commands. This process clones the repository, compiles the source code into a high-performance binary, and launches the application.

Clone the Repository Pull the latest source code directly from GitHub:

git clone https://github.com/Ilikemycheesedrippybruhlol/NexaVim
Compile the Source Enter the directory and use the GNU C++ Compiler to create your executable:

cd NexaVim
g++ -o nexavim nexavim.cpp
Launch NexaVim Start the editor immediately:

./nexavim
