# setting clang as default compiler

```bash
# get path to clang and clang++


/home/linuxbrew/.linuxbrew/opt/llvm/bin/clang++
/home/linuxbrew/.linuxbrew/opt/llvm/bin/clang



update-alternatives --install /usr/bin/cc cc /home/linuxbrew/.linuxbrew/opt/llvm/bin/clang  800 \
--slave /usr/bin/c++ c++ /home/linuxbrew/.linuxbrew/opt/llvm/bin/clang++

# Script to install all LLVM packages

wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh all

## validation

# installs all to
/usr/bin/clang-17 --version

# add to shell
echo 'export PATH=/usr/bin:$PATH' >> ~/.zshrc
alias clang='/usr/bin/clang-17'
alias clang++='/usr/bin/clang++-17'
clang++ --version

# confirm libc++ added
dpkg -l | grep libc++


# update default compilers
export CC=/usr/bin/clang-17
export CXX=/usr/bin/clang++-17

sudo update-alternatives --install /usr/bin/cc cc /usr/bin/clang-17 100
sudo update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++-17 100

```

#### building from source

```bash
# check number cpus available 20
nproc

# get the linker
sudo apt update
sudo apt install lld
lld --version
ld.lld --version

# prereqs
sudo apt update
sudo apt install build-essential cmake ninja-build git python3

git clone https://github.com/llvm/llvm-project.git

sed 's#fetch = +refs/heads/\*:refs/remotes/origin/\*#fetch = +refs/heads/main:refs/remotes/origin/main#' -i llvm-project/.git/config


cd llvm-project

mkdir build
cd build

cmake -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
-DLLVM_ENABLE_PROJECTS="clang;lld" \
-DLLVM_PARALLEL_{COMPILE,LINK}_JOBS=10 \
-DLLVM_USE_LINKER=lld \
-DCMAKE_INSTALL_PREFIX=build /usr/local

# build
ninja
# tests
ninja check
# install
ninja install

# build the rest
$ mkdir build
$ cmake -G Ninja -S runtimes -B build -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind" # Configure
$ ninja -C build cxx cxxabi unwind                                                        # Build
$ ninja -C build check-cxx check-cxxabi check-unwind                                      # Test
$ ninja -C build install-cxx install-cxxabi install-unwind



```
