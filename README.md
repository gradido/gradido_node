
## Brief
- Manage Blockchain for every group configured
- Manage Blockchain containing all groups, find more here: [GroupRegisterGroup](https://gradido.github.io/gradido_node/classcontroller_1_1_group_register_group.html)

## Install Linux

### Dependencies
```bash
apt install libsodium-dev libmpfr-dev libssl-dev
```

### Build
```bash
mkdir build
cd build
cmake ..
make protoc -j$(nproc) # needed for parsing protobuf files
make GradidoNode -j$(nproc)
```
## Install Windows

### Debug
Visual Studio 16 is the max Visual Studio Version supported from this current conanfile.txt
```bash
cd gradido_node
conan install . --output-folder=build --build=missing
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="conan_toolchain.cmake" -DCMAKE_BUILD_TYPE=Release
```

Code doc: https://gradido.github.io/gradido_node/html/index.html

## Install Linux
On Debian 12
```bash 
sudo apt install libsodium-dev libmpfr-dev libssl-dev libbsd-dev gcc-multilib libc++-dev
``` 
install rust compiler
```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

ON debian 12 64Bit
cmake .. -DUSE_INSTALLED_SSL=On -DCMAKE_BUILD_TYPE=Release -DTARGET=x86_64-linux-gnu  -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake

On Debian 12 ARM64 Server
cmake .. -DOPENSSL_ROOT_DIR=/usr/lib/aarch64-linux-gnu -DCMAKE_BUILD_TYPE=Release -DUSE_INSTALLED_SSL=On -DTARGET=aarch64-linux-gnu -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake