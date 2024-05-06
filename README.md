
## Brief
- Manage Blockchain for every group configured
- Manage Blockchain containing all groups, find more here: [GroupRegisterGroup](https://gradido.github.io/gradido_node/classcontroller_1_1_group_register_group.html)

## Install Windows

### Debug
Visual Studio 16 is the max Visual Studio Version supported from this current conanfile.txt
```bash
conan install .. -s build_type=Debug
cmake -DCMAKE_BUILD_TYPE=Debug -G"Visual Studio 16 2019" ..
```

Code doc: https://gradido.github.io/gradido_node/html/index.html

## Install Linux
On my Debian 12
```bash 
sudo apt install libsodium-dev libmpfr-dev libssl-dev
``` 
install rust compiler
```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```
