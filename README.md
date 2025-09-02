# XHCI-DEBUG-TOOL
This application implements the behavior of BAR using pcileech callback.  
The purpose of this project is to first understand how xhci works using callbacks, as it is very difficult to implement in verilog from scratch.  
It is very difficult to confirm a lot of information using text alone.  
I also use imgui to visualize the status of registers, etc.  

## related projects
[FL1100](https://github.com/cur-114/FL1100)  
firmware source code  
[usb-response-generator](https://github.com/cur-114/usb-response-generator)  
converting usb packet dump to .COE  
[FPGA-BAR-DEBUG](https://github.com/cur-114/FPGA-BAR-DEBUG)  
xHCI implementation learning project  
## project for debug used together with FL1100 (branch: debug)  
[debugger-linux](https://github.com/cur-114/debugger-linux)  
project for debugging through QEMU and internal firmware operations  
[log-viewer](https://github.com/cur-114/log-viewer)  
project to display transmission data from debugger-linux in a GUI format  

---

discord: _cur
