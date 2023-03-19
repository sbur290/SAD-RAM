//tutorial showing how  to get basic mixed-mode debugging (.NET application calling a native c++ DLL).
//https://learn.microsoft.com/en-us/visualstudio/debugger/how-to-debug-managed-and-native-code?view=vs-2022
/*
1>------ Build started: Project: TestDll2, Configuration: Debug Win32 ------
1>C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Microsoft\VC\v160\Microsoft.CppBuild.targets(411,5): error MSB8020: 
The build tools for v143 (Platform Toolset = 'v143') cannot be found. To build using the v143 build tools, please install v143 build tools.  
Alternatively, you may upgrade to the current Visual Studio tools by selecting the Project menu or right-click the solution, and then selecting "Retarget solution".
1>Done building project "TestDll2.vcxproj" -- FAILED.
========== Build: 0 succeeded, 1 failed, 0 up-to-date, 0 skipped ==========
*/