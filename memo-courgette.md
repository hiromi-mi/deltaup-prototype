# Cougette で迷子にならない
Author: hiromi-mi

## 用語説明 
`image_utils.c` より引用
* RVA: 相対仮想アドレス
* VA: 仮想アドレス <- OS によるRelocation による
* abs32: In an image these are directly stored as VA whose locations are
  stored in the relocation table.
* rel32: In an image these appear in branch/call opcodes, and are represented
  as offsets from an instruction address.

## クラス
* (Source/Sink)Stream
* Regionは メモリ領域を記述する; start address と bytes と length measurement が必要
* Element は Ensemble のリージョンでExecutable Type = win32, elf32, win32を持ったもの
* Shingle は LabelInfos の固定長の文字列. Trace の中に現れる (?)
* We repesent the Shingle by the position of one of the occurrences in the Trace.
* AssignmentCandidates は LabelInfo のPriority Queue
- Update() して label を score に直している
* LabelToScore : LabelToInfo と OrderLabelInfoと int のmap
* Trace = LabelInfo のVector
*  std::set<Shingle, InterningLess> OwningSet;
* FreqView: Shingle Instance のヒストグラム
* Ensemble = Element[] (ソースコード領域が複数あるのがElement)
- ShinglePatternLess
- ShinglePatternPointerLess

### 各種generator
- MakeGenerator: @ `ensemble_create.cc` で win32 なものを生成した
- FindGenerator @ `ensemble_create.cc` : TransformationPatchGenerators を発見して第一近傍をみつける

## 概要
`courgette/courgette_tool.cc` GenerateEnsemblePatch がスタートポイント

ファイル書込みしてGenerateEnsemblePatch @ `ensemble_create.cc` に移行
"Transform Method" : 入力Method を bsdiff が小さくなるように変形
1. disassbble (新旧を AssemblyProgram に直す)
2.  adjst (AssemblyProgram object を作成
3.  encode (AssemblyProgram を raw bytes に直す)

## 1. Disassemble
DisassembleCourgerette

入力はinput pointer の raw file
Disassembly で 特定の machine instruction を Courgette instruction に直してる

絶対アドレスで用いられている命令を binary file relocation table で見つけだし、相対アドレスに直す
これは disassemble:ParseDetectedExecutable <- 実際にはそんなものはない
Disassembler の部分class を binary file header を見て決める

### 具体的にソースコードを見る
DisassemblerWin32 とかが担当してる

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
ヘッダ見て判定

InsteructionGenerator は BindRepeating らしい
ParseFile は
Section を1つづつ見つけて
-> section file region でないと false かえしてる (プゴラムtと InstructionReceptor取ってきてる

### DisasesmblerWin32; ParseFileRegion
Section, start_file, end_file, receptor とってきてる

なお、ヒストグラム機能, DescribeRVAがあるらしい

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

adjustment method は `adjustment_method.cc` と `adjustment_method_2.cc` (!) に記載されている.

`adjustment_method` に含まれているのは以下の通り.
###LabelInfo ラベル情報
双方向線形リスト. アドレスの上下で並んでいる.
* `is_model_`: モデルに入っているか
* `refs`: 参照回数
* positions (ラベルを参照するposition のリスト)

### Graph Matching Assignment
ふつうにやると指数時間かかるが、eager matching により実用的に.

* strongest match はプログラムの特定ラベルへの参照数が変化しない, RVA のずれは同一
* ラベル同士の対応がつくと、対応するラベルの近くのラベルも似ている可能性が高い : TryExtendAssignment, TryExtendSequence..
* 2つのラベルが対応していると, ???
```
//  * If two labels correspond, then we can try to match up the references
//    before and after the labels in the reference stream.  For this to be
//    practical, the number of references has to be small, e.g. each label has
//    exactly one reference.
```
@ `adjustment_method.cc` l.119
よくわからない

### TryExtendAssignment

### AssignmentProblem: 上のGraph Matching を実際に実装
このグラフ構造は LabelInfo を枝としてもつ.
TrySolveNode() にて貪欲法をそれぞれの node に対して適用

ラベルが連結リストで持ってる
プログラムとモデルの木構造を作る. 木構造を対応させる

places : sequence への index
edge が empty() でなければ Extended();  -> 木構造が埋まっている ( @ ExtendNode)
m_trace (Label情報全部？) を持って呼び出して、trae ごとに対して

##  adjustment_method_2.cc
AssemblyProgram を実際のAddress とのMap を作るらしい
`adjustment_method_2.cc` に長文説明があるのでそれを参照.

Seq1 A1 model : A, B, C
Sequence 2 A2: program U, V, W

A1 の各シンボルは一意な名前やindexをもつ
S1 のsequncence を symbol のindex の列T1 jninaosul
A2 のシンボルをわって S2 を T2 にしたい
T2 は T1 を部分列として持つ

T1;T2 を差分構造として持ちたい

S2 を S1 にマッチさせる方法は なかったら これ以上マッチが見つからないまで backtracking すること

それぞれのsymbol U, V in A2 は A1 から候補を引っ張ってきて、各要素は weight をもつ. これはmatch のevidence による

U, V の VariableQueue をもっていて、beest coice が 2番目の選択肢よりどのくらい良いか 'clear cut' か
=> 一番clear cut かを決めて assign してる

ナイーブに行なうと (A, U) の全ペアとそれぞれのペアについて benefit, score, U:=A を求めたらいいことになる。ここで
score はA の S1 での出現頻度と同じ文脈でのU の S2 内部での出現頻度

「文脈」の識別には S1, S2 を k-length substrings overwrapping sequence としてみる. 以下 shingles とよぶ.
Two shingles are compatible <=> symbol in one shingle が他のシンボルとマッチできるとき
例: ABC と UVU は compatible でない. A =U and C = U でmatch が衝突するから. UVW, WUV とかと compatible.
Assignment するまでどれがどれに対応するかは分からない
U:=A と代入してから UVW, UWV が compatible となる.

assigns をする以上 shingles の同値類の数はふえていき、それぞれの同値類内部の要素数は減っていく.

U:==A の証拠を探す, U を含む shingles がどれだけ A を含む shingles と compatible かで判断.
つまり沢山出現されるsymbol ほど先に代入される

most clear-cut assignment by considering all pairs symbols & each pair 各ペアのoccurence context a を比較するのがは実行できる計算量でない.

以下のapproach を実現可能にするため、backward している.

### AdjustmentMethodTest
MakeProgram でプログラムを作成している. これら2つのプログラムはlabel の指すアドレスの絶対値 (kRvaA, kRvaB) だけがずれている.
Adjustment すると同じアドレスになる.

### アルゴリズムの説明

Shingle
- offsetmask
- fixed
- variable

### Adjustor : Adjustor を実際に行なっている.
Finish() を呼び
* プログラム中のindex を外し
* CollectTraces を呼ぶ.
* abs, rel それぞれについて Solve()
* RemainingIndexes() を Assign する
#### CollectTraces()
各ラベルについて, Trace にラベル情報を作成して追加してる

#### Solve()
LabelInfo をリンクして, AssignmentProblem a を定義. そして a.Solve() を解く.

