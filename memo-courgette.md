# Cougette で迷子にならない

## 用語説明 
`image_utils.c` より引用
* RVA: 相対仮想アドレス
* VA: 仮想アドレス <- OS によるRelocation による
* abs32: In an image these are directly stored as VA whose locations are
  stored in the relocation table.
* rel32: In an image these appear in branch/call opcodes, and are represented
  as offsets from an instruction address.

## クラス説明
* Stream
* Sink
* Regionは メモリ領域を記述する; start address と bytes と length measurement が必要
* Element は Ensemble のリージョンでExecutable Type = win32, elf32, win32を持ったもの

### 各種generator
- MakeGenerator: @ `ensemble_create.cc` で win32 なものを生成した
- FindGenerator @ `ensemble_create.cc` : TransformationPatchGenerators を発見して第一近傍をみつける

## 概要
`courgette/courgette_tool.cc` GenerateEnsemblePatch がスタートポイント

"Sink" を Patch の Streami にしてファイル書込みしてGenerateEnsemblePatch @ `ensemble_create.cc` に移行
"Transform Method" : 入力Method を bsddiff が小さくなるように変形
1. disassbble (新旧を AssemblyProgram に直す)
2.  adjst (AssemblyProgram object を作成
3.  encode (AssemblyProgram を raw bytes に直す)

## 1. Disassemble
DisassembleCourgerette

入力はinput pointer の raw file (ほんまか？？？)
Disassembly で 特定の machine instruction を Courgette instruction に直してる

絶対アドレスで用いられている命令を binary file relocation table で見つけだし、相対アッドレスに直す
これは disassemble:ParseDetectedExecutable <- 実際にはそんなものはない
Disassembler の部分class を binary file header を見て決める

### 具体的にソースコードを見る
DisassemblerWind32 とかが担当してる

Disassembler::CreateProgram(bool annotate) @ disassembler.cc
program()を作って
kind: OOS とかの種類
1. PreComputeLabels()
- LabelManager の事前計算 を abs32 と rel32 についてやってる (RvaVisitor)
2. RemoveUnusedRel32Locations()
3. `program->DefualtAssignIndexes()`

#### RvaVisitor (RVA を配列として見ていくのにやる)
- OS依存、abs32, rel32 でやる

### DisasesmblerWin32; ReadHeader
Header 読み込み.
### DisasesmblerWin32; ParseRelocs

実際に Relocation 情報を展開してる

```
272       // Skip the relocs that live outside of the image. It might be the case
273       // if a reloc is relative to a register, e.g.:
274       //     mov    ecx,dword ptr [eax+044D5888h]
```


### DisasesmblerWin32; QuickDetect
ヘッダ見ておわってる？

InsteructionGenerator は BindRepeating らしい
ParseFile は
Section を1つづつ見つけて
-> section file region でないと false かえしてる (プゴラムtと InstructionReceptor取ってきてる

### DisasesmblerWin32; ParseFileRegion
Section, start_file, end_file, receptor とってきてる
ポインナをRVAにして
Origin を得ているか把握して

### ヒストグラム機能, DescribeRVAがあるらしい

RVAtoFileOffset このあたりは AddressTranslator にある
1) FileOffsetToRVA にする

Label : address へのシンボル参照
RVAVisitor: RVA を順番に眺めること

Relocs を取って vector<RVA> のテーブルに格納

Relocation Table を Courgette 命令に変更
machine code が入ったアドレスリストを生成する
PointerToTargetRVA: Read32LittleEndian だった

Rel32 だと4バイト後をとってきてる？ RvaVvisitor_Rel32::Get()

## Adjust
AssemblyProgram を実際のAddress とのMap を作るらしい
Weighting Matching at `adjustment_method_2.cc` に長文説明があるのでそれを参照. 以下翻訳.

Seq1 A1 model : A, B, C
Sequence 2 A2: program U, V, W

A1 の各シンボルは一意な名前やindexをもつ
S1 のsequncence を symbol のindex の列T1 jninaosul
A2 のシンボルをわって S2 を T2 にしたい
T2 は T1 を部分列として持つ

T1;T2 を差分にしたい

S2 を S1 にマッチさせる方法は bractracking なかったら until これ以上マッチが見つからない

それぞれのsymbol U, V in A2 は A1 から候補を引っ張ってきて、各要素は weight をもつ. これはmatch のevidence による

U, V の VariableQueue をもっていて、beest coice が 2番目の選択肢よりどのくらい良いか 'clear cut' か
=> 一番clear cut かを来めて assign してる

ナイーブニヤルト (A, U) の全ペアとそれぞれのペアについて benefit, score, U:=A を求めたらいい
score はA の S1 での出現頻度と同じ文脈でのU の S2 内部での出現頻度

「文脈」の識別には S1, S2 を k-length substrings overwrapping sequence としてみる. 以下 shingles とよぶ.
Two shingles are compatible <=> symbol in one shingle が他のシンボルとマッチできるとき
例: ABC と UVU は compatible でない. A =U and C = U でmatch が衝突するから. UVW, WUV とかと compatible.
Assignment するまでどれがどれに対応するかは分からない
U:=A と代入してから UVW, UWV が compatible となる.

assigns をする以上 shingles の同値類の数はふえていき、それぞれの同値類内部の要素数は減っていく


U:==A の証拠を探す, U を含む shingles がどれだけ A を含む shingles と compatible かで判断.
つまり沢山出現されるsymbol ほど先に代入される

most clear-cut assignment by considering all pairs symbols & each pair 各ペアのoccurence context a を比較しってやるのは実行できる計算量でない.

以下のapproach を実現可能にするため、backward している.

### アルゴリズムの説明

Shingle
- offsetmask
- fixed
- variable
