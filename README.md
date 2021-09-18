# QuadSLAM
[QuadDump](https://github.com/wakewakame/QuadDump)で録画したデータをもとに3Dデータを生成するためのプログラムです。

# 動作確認済み環境
- Ubuntu 20.04  
- MacOS Big Sur  
- Windows 10, Visual Studio 2019  

# ビルド手順
## 必要なツールのインストール
### Ubuntuの場合

```sh
sudo apt install git cmake
```

### MacOSの場合

```sh
brew install git cmake
```

### Windowsの場合
`Visual Studio` と `Git` と `CMake` が必要です。  
以下のURLからダウンロード、インストールして下さい。  

Visual Studio: [https://visualstudio.microsoft.com/ja/downloads/](https://visualstudio.microsoft.com/ja/downloads/)  
CMake: [https://cmake.org/download/](https://cmake.org/download/)  
Git: [https://git-scm.com/downloads](https://git-scm.com/downloads)  

## リポジトリのクローン

```sh
git clone https://github.com/wakewakame/QuadSLAM
cd QuadSLAM
git submodule update --init --recursive
```

## ビルド
### Ubuntu or MacOSの場合

```sh
mkdir build
cd build
cmake ..
cmake --build .
```

### Windowsの場合

```sh
mkdir build
cd build
cmake ..
cmake --build . --target ALL_BUILD --config Release
```

## 実行
### Ubuntu or MacOSの場合
```sh
./quadslam <QuadDumpで録画したディレクトリへのパス>
```

### Windowsの場合
```sh
.\Release\quadslam.exe <QuadDumpで録画したディレクトリへのパス>
```
