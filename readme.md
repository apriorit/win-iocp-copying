# WinAPI IOCP Programming: Scalable File I/O 

## About

The solution implements copying a folder with multiple files using asynchronous I/O in Windows. It is created mainly as a demonstration of WinAPI IOCP programming.

Author: @Andrew0xFF

## Implementation

The project is created in C++.

It implements simultaneous copying of files within a folder using asynchronous I/O operations.  Windows I/O completion ports are used as the main tool.
The provided sources cover all stages, starting from IOCP creation and file opening to the successful / unsuccessful operation completion handling.

To get a brief IOCP functioning tutorial as well as simple Windows IO completion ports example explanations please read the [related article](https://www.apriorit.com/dev-blog/412-win-api-programming-iocp).

## License

Licensed under the MIT license. © Apriorit.
