# Buddy
###### tags: Paper
[manual page](http://buddy.sourceforge.net/manual/main.html)


## ROBDD
之後的 BDD 都是 ROBDD
為了方便表示，以下 ROBDD 都用 BDD 代替表示
### BDD operation
#### Apply
兩個 BDD 做 "operation"的基礎架構，用 DP 來加速
概念: 如果兩個 ROBDD 都已經遞迴到 terminal node，則回傳兩個 var bitwise 運算，如果兩邊的 var order 相等，則分別遞迴 APP(low) 及APP(high)，![](https://i.imgur.com/JoP0Hxu.png)
否則 order 較小的優先做，另外一個保持原本的 boolean expression
**Apply 演算法**
![](https://i.imgur.com/sCQzNaP.png)

舉例:
![](https://i.imgur.com/5d2zE9V.png)

#### Restrict
給某些變數 truth assignment 後得到的 BDD
給值的表示法: \[0/x~3~,1/x~5~,1/x~6~\]，代表 x~3~=0，x~5~=1，x~6~=1
**Restrict 舉例:**
一個 boolean expression $t = (x_1 \iff x_2)\vee x_3$，given $[0/x_2]$，low(x~1~) = low(x~1~)取low(x~2~)，high(x~1~)同理， 得到的 BDD 為 $(\neg x_1 \vee x_3)$
![](https://i.imgur.com/lwp8JGI.png =300x250)
演算法如下，因為 restrict 可能會讓 t^x1^ 跟另外的 (sub)ROBDD t'相等，所以回傳的結果要 call MK (bdd_makenode) 來保持 reduced
![](https://i.imgur.com/CupHkkn.png)

## bdd_makenode 內部實作(buddy)
```c=
int bdd_makenode(unsigned int level, int low, int high)
{
   register BddNode *node;
   register unsigned int hash;
   register int res;

#ifdef CACHESTATS
   bddcachestats.uniqueAccess++;
#endif
   
      /* check whether childs are equal */
   if (low == high)
      return low;

      /* Try to find an existing node of this kind */
   hash = NODEHASH(level, low, high);
   res = bddnodes[hash].hash;

   while(res != 0)
   {
      if (LEVEL(res) == level  &&  LOW(res) == low  &&  HIGH(res) == high)
      {
#ifdef CACHESTATS
	 bddcachestats.uniqueHit++;
#endif
	 return res;
      }

      res = bddnodes[res].next;
#ifdef CACHESTATS
      bddcachestats.uniqueChain++;
#endif
   }
   
      /* No existing node -> build one */
#ifdef CACHESTATS
   bddcachestats.uniqueMiss++;
#endif

      /* Any free nodes to use ? */
   if (bddfreepos == 0)
   {
      if (bdderrorcond)
	 return 0;
      
         /* Try to allocate more nodes */
      bdd_gbc();

      if ((bddnodesize-bddfreenum) >= usednodes_nextreorder  &&
	   bdd_reorder_ready())
      {
	 longjmp(bddexception,1);
      }

      if ((bddfreenum*100) / bddnodesize <= minfreenodes)
      {
	 bdd_noderesize(1);
	 hash = NODEHASH(level, low, high);
      }

         /* Panic if that is not possible */
      if (bddfreepos == 0)
      {
	 bdd_error(BDD_NODENUM);
	 bdderrorcond = abs(BDD_NODENUM);
	 return 0;
      }
   }

      /* Build new node */
   res = bddfreepos;
   bddfreepos = bddnodes[bddfreepos].next;
   bddfreenum--;
   bddproduced++;
   
   node = &bddnodes[res];
   LEVELp(node) = level;
   LOWp(node) = low;
   HIGHp(node) = high;
   
      /* Insert node */
   node->next = bddnodes[hash].hash;
   bddnodes[hash].hash = res;

   return res;
}
```

line 16: `HASH` 為一個 **marco**，分別代入 `node_level`, `low child`, `high child`，$PAIR(a,b)= (a+b)*(a+b+1)/(2+a)$
HASH 的部分還不了解其中的意義，只知道這樣能夠得到一個 bdd node table 的 index，接著就在這個 bdd list 尋找有沒有相同 node

```c 
line 142 in kernel.c
#define NODEHASH(lvl,l,h) (TRIPLE(lvl,l,h) % bddnodesize)

line 143 in kernel.h
#define TRIPLE(a,b,c)  ((unsigned int)(PAIR((unsigned int)c,PAIR(a,b))))

line 142 in kernel.h
#define PAIR(a,b)      ((unsigned int)((((unsigned int)a)+((unsigned int)b))*(((unsigned int)a)+((unsigned int)b)+((unsigned int)1))/((unsigned int)2)+((unsigned int)a)))
```
#### SATcount
檢查有多少種 variable assignment 使 boolean function *t* satify (evaluate to True)
![](https://i.imgur.com/aNQ5pma.png)

**演算法**
![](https://i.imgur.com/DDAJrPE.png)

## Install
下載完 buddy-2.4.tar.gz 之後
```
$ tar -xvf buddy-2.4.tar.gz
```
解壓縮資料夾
```
$ cd buddy-2.4
$ ./configure
$ make
$ make install
```
安裝 library，會將 lib & header file 裝到 /usr/local/lib /usr/local/include
:::danger
如果 make 發生 error，用 sudo make
:::

接著就可以使用這個 library 了
```c
#include <bdd.h>
```

compile 時需要加入 -lbdd
```c
gcc myfile.c -o myfile -lbdd
```

如果執行時出現，代表找不到 so 檔
```c
error while loading shared libraries: libbdd.so.0: cannot open shared object file: No such file or directory
```
執行以下指令應該就可以執行了
```
$ sudo ldconfig
```

如果還是不行，可能要去修改 /etc/ld.so.conf
加入 /usr/local/lib

## Practice
bdd_init(int nodenum, int cachesize) // 初始化
bdd_setvarnum(int); // 使用變數個數

## Finite Domain Blocks
用變數 blocks 來表示 integer value
for example:
$V_0 ... V_1$ are used to encode `12`, then $V_0=0,\ V_1=0,\ V_2=1,\ V_3=1$

## Efficiency Concerns
如果想要 high performance in terms of BDD operation
1. BDD node設越大越好，但是會造成 reorder 越慢
2. small cache ratios
3. set `maxincrease`

## implementation
1. 所有 node 都被存放在一個 array 裡，且可用 hash 來加速尋找相同 BDD nodes when bdd_makenode
2. 每個 node 都有一個 reference count，

# Real-time Verification
得到predicate後...

## compute atomic predicates
先將所有 predicates 轉換成 atomic predicate
$A(\{P\}) = \{P, \overline P\}$
不同 predicate 間的交集若不為空則產生新的 atomic predicate，關係式如下:
$for\ P_1,P_2:two\ sets\ of\ predicates\\A(\{P_1\})=\{b_1,...,b_l\}\\A(\{P_2\})=\{d_1,...,b_m\}\\A(\{P_1\cup P_2\})=\{a_i=b_j\wedge d_k,j\in\{1,...l\},k\in\{1,...m\}\}$
最多會產生 `lxm` 個 predicates，而實際上大部分的交集都會是 false (因為經過 Algo 2 的 priority add 後，同一台 router 間的 predicate 會 disjoint)

:::warning
ACL predicates 跟 forwarding predicates 是分開計算的
:::
### 不太重要的 ordering compute (影響 computation time)
在計算 atomic predicate 時的挑選 predicate 的順序
*    Random selection: 隨機選擇，或是像 Algo 3 照順序
*    Smallest ACL first: 從最少 rule 的 predicate 開始，因為 rule 越少，被劃分的 block 數越少
*    Selection by box: for forwarding predicate，box指的是 switch/router ，同一個 box 的 header space 在計算 forwarding predicate 時已經互為 disjoint

### AP tree construction
算完 AP 後，根據 S(P): integer set，來建出 AP tree，用來減少 search time，論文中提出 3 種 ordering
![](https://i.imgur.com/aVhKdiJ.png)
*    Pruned: 若在所有 AP 中， $P_1$只出現一次$(P_1\wedge\overline P_2,...\wedge\overline P_n)$，則只會建 $P_1$為 leaf node，該 leaf node 即為($P_1\wedge\overline P_2,...\wedge\overline P_n)$ 
*    Quick-Ordering: 根據在 atomic predicate 出現的次數建 AP tree，越多次的越先建，這樣在建 pruned AP tree 時效果越好
*    Optimized: DP 的方法使 avg. height 最低

## rule update
rule 更新時，會影響 predicate，也因此影響 atomic predicate