# Todo
- add\_subdirectory()だとOpenCVとCinderが両方ともzlibに依存してるのでターゲット名の競合を起こすっぽい?
	- ExternalProject\_Addを使うことで解決できるらしいので、これを試してみる
- CMakeのCMAKE\_CXX\_STANDARDとかがモダンじゃないようなので、モダンな書き方を調べて修正
- 警告を厳しくして、エラーとして扱うようにする
- cinderで三角形を描画
- QuadSLAMとCinderを統合
- Windowsでファイルパスに非ASCII文字が含まれる場合への対処
