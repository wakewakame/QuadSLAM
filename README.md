# QuadSLAM
[QuadDump](https://github.com/wakewakame/QuadDump)で録画したデータをもとに3Dデータを生成するためのプログラムです。

# ビルド手順
## Ubuntu
### 必要なツールのインストール

```sh
sudo apt install -y git cmake
```

### リポジトリのクローン

```sh
git clone https://github.com/wakewakame/QuadSLAM
cd QuadSLAM
git submodule update --init --recursive
```

### ビルド

```sh
mkdir build
cd build
cmake ..
make -j
```

### 実行

```sh
./bin/quadslam <QuadDumpで録画したディレクトリへのパス>
```

## MacOS
作成中

## Windows
作成中