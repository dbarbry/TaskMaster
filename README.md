# TaskMaster

In order to launch the project use the Makefile like such:

```
make                        - compile the project, giving 2 executables
                            one for the server
                            one for the client.
make server <file.conf>     - launch the server.
make client                 - launch the client.
make kill                   - kill the daemonized server.
make fclean                 - to clean everything
make clean_log              - to clean logs
```

In order to use clang-format file:

Install C/C++ extension
Then in your JSON settings of vscode:

```
{
    "C_Cpp.formatting": "clangFormat",
    "editor.formatOnSave": true,
    "editor.defaultFormatter": "ms-vscode.cpptools"
}
```
