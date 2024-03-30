# C-- Compiler Front-End 轻量类C语言编译器前端

## 运行环境

1. **系统与内核版本**：GNU Linux Release: Ubuntu 20.04, kernel version 5.13.0-44-generic；
2. **编译器**：GCC version 7.5.0；
3. **分析器生成工具**：GNU Flex version 2.6.4 以及 GNU Bison version 3.0.4

## 文件结构

```
.
├── README.md
// 编译、调试文件
├── Makefile
├── init.gdb
├── testcase
// stage 1 词法语法分析：构造语法树
├── main.c
├── mylexer.l
├── myparser.y
├── tree.c
├── tree.h
├── util.c
└── util.h
// stage 2 语义分析：语法检查和类型检查
├── cmm-L-SDT.txt
├── cmm-S-SDT.txt
├── semantic.c
├── semantic.h
├── sym-table.c
├── sym-table.h
├── type-system.c
├── type-system.h
// stage 3 中间代码生成
├── cmm-IR-SDT.txt
├── sdt.c
├── sdt.h
├── ir-gen.c
├── ir-gen.h
├── out1.ir
```


## 功能实现

### stage1 词法与语法分析

实现了词法分析和语法分析，在没有词法或语法错误的情况下能够输出语法树。

- **词法和语法单元识别**：在`mylexer.l`和`myparser.y`中分别编写识别规则来识别词法单元和语法单元。

- **语法树构造与打印**：语法树使用朴素的多叉树结构，接口在`util.c`中实现：

  ```c
  struct node {
      char*  str;				 // 节点名称			
      int    lineno; 			 // 当前词法/语法单元出现的行号
      size_t arr_len;     	 // 子节点数量
      struct node* *child_arr; // 使用动态指针数组管理子节点
  };
  struct node* create_node(const char* ndname, const int lno);
  void         free_node(struct node *parent);					// 递归释放节点内存
  void         insert_node(struct node *parent, size_t n, ...);	// 使用变长参数插入子节点
  void         print_tree(struct node *parent, int level); 		// 递归打印, level作为节点缩进参考
  struct node* create_syntax_node(const char *ndname, struct node *first_child); //create_node的上层封装
  ```

  词法/语法单元数值`yylval`类型统一为`struct node *`，使用`%union`和`%token`、`%type`来定义：

  ```yyac
  %union {
      struct node * nd;
  }
  %token <nd> INT
  ...
  %type <nd> Program
  ...
  ```

  创建节点操作分别在.l和.y文件中实现，示例如下，`Program`作为开始符号需要**打印语法树**：

  ```flex bison
  // mylexer.l (叶子节点)
  ";"         {yylval.nd = create_node("SEMI", yylineno); return (SEMI);}
  // myparser.y 
  %start Program
  %%
  Program : 	 ExtDefList  {$$ = create_syntax_node("Program", $1); 
  					insert_node($$, 1, $1); if(!err_occur) print_tree($$, 0);};
  ```

- **错误恢复**：

  - 词法错误输出函数`lex_error_hanlde`，语法错误输出修改了函数`yyerror`，通过修改`util.h`中的宏`ERR_OUT_FD`来重定向错误输出；
  - 对于**指数形式的浮点数**以及普通词法错误，会直接调用`lex_error_handle`报出词法错误，并设置`err_occur=1`和`lex_error=1`，避免语法树和语法错误输出。
  - 对于**注释内容**，在`mylexer.l`使用`%x`功能（参考[Flex: Start Conditions](https://www.cs.virginia.edu/~cr4bd/flex-manual/Start-Conditions.html)）**忽略**注释内部内容避免重复报错（详细见代码），设置`cmt_error=1`并使用`yyerror`输出语法错误；
  - 对于**普通语法错误**，在文法中添加**error token**增加产生式来进行识别，主要集中在括号配对部分，其余只需在部分底部语法单元添加即可（错误是自底向上产生的）；识别后自动使用`yyerror`来输出语法错误。详细恢复过程阅读了文档：[Error Recovery (Bison 3.8.1)](https://www.gnu.org/software/bison/manual/html_node/Error-Recovery.html)

- **内存管理**：在`myparser.y`中使用`%destructor`功能来释放**弃用节点**（被discard或cleanup的词法/语法单元）所持有的动态内存，可配合`leak-sanitizer`进行内存泄漏检查（见Makefile)：

  ```yyac
  %destructor { free_node($$);} <nd>
  ```

  bison进行destruct的过程参考bison官方文档：[Destructor Decl (Bison 3.8.1)](https://www.gnu.org/software/bison/manual/html_node/Destructor-Decl.html) 

------

### stage2 语义分析

实现语义分析。过程包括构造类型系统，编写L属性SDT，递归向下语义分析，符号表管理，以及静态类型检查。

#### 2.1 Type System

先构造好类型系统来描述**符号类型**组成：

- `Type System`中的`component`分为`[T]`、`[F]`、`[E]`、`T`、`F`、`S`以及`S List`(即SL)。

- 代码中使用三种struct来分别构成`[T]`、`[F]`和`[E]`这三个基本单位：struct Type 、struct Field 以及struct Exptype，后面的component依赖这些基本单位来构造得到。

  ```c
  // type-system.h
  enum Tenum { B_INT, B_FLOAT, ARRAY, STRUCTURE_VAR, STRUCTURE_DEF, FUNC };
  
  struct Field {
      char*         name; // 符号名称
      struct Type  *type; // 指向信息尾部
      struct Field *next; // 下一个信息头部
  };
  
  struct Type {
      enum Tenum kind;
      union {
  		// ... see the file
      } u;
  };
  
  struct Exptype {
      enum Tenum kind;                // 表达式类型
      enum {LVAL, RVAL} lrval_flag;   // 左右值类型
      struct Type *t_ptr;             // Type 指针，当表达式为特定类型(struct、func)时启用
  };
  ```

- 每一个需要加入符号表中的符号，其指向的**类型信息**为一个`S`。（见`sym-table.h`中符号的struct结构）

- `T`，信息尾部，语义上代表基本变量类型或结构变量类型：

  ```
  KIND  		  COMPOSITION				FUNC
  T_basic:      [T(B_INT)] or [T(B_FLOAT)]              --- make_T_basic(TYPE)
  T_struct_var: [T(STRUCTURE_VAR)] -> S_struct_def      --- make_T_struct(S_struct_def)
  ```

- `F`，信息头部，四种类型以及对应构造方式：

  ```
  F_basic:      [F]->null(T)                            --- make_F_basic(id)  
  F_array:      [F]->[T(ARRAY)]->......->null(T)        --- make_F_array(F_basic or F_arr) 
  F_struct_def: [F]->[T(STRUCTURE_DEF)]->null(SL)       --- make_F_struct(id)
  F_func:       [F]->[T(FUNC)]-+--->null(T)             --- make_F_func(id)
                               `-->null(SL)
  ```

- `S`，类型信息整体，由不同的头部F和尾部T连接得到，有四种类型以及对应构造方式：

  ```
  S_single:     F_basic->(T_basic or T_struct_var)      --- make_S_single(F_basic, T)
  S_array:      F_array->(T_basic or T_struct_var)      --- make_S_array(F_array, T)
  S_struct_def: F_struct_def-> SL                       --- make_S_struct(F_struct_def, SL)
  S_func:       F_func-+-->(T_basic or T_struct_var)    --- make_S_func(T, F_func, SL)
                       `--> SL
  ```

- `SL`：类型信息链表，由多个`S`通过`[F].next`连接得到。

#### 2.2 L属性SDT

编写SDT的过程比较麻烦，尤其是L属性的SDT。在构建好类型系统后，按照类型系统分别弄清楚每个非终结符需要什么样的继承属性和综合属性，属性传递过程是怎样的，每个非终结符都应该画一棵语法分析树，去看看类型的构造是一个什么样的过程。

在构造的SDT中，每个非终结符拥有的属性均使用以下表达形式：

```
(XXX):Y		XXX为component类型(T,F,S,SL), Y为属性名称,常见有t和b,一般以t为综合属性,b为继承属性
```

详细SDT见文件`cmm-L-SDT.txt`。

#### 2.3 递归向下语法分析

递归向下法使用了和DFS近似的算法，在**已经构建好的语法树**上进行从下往上、从左往右的分析，由于每个非终结符有着不同的语义动作，所以需要根据SDT，为每一个非终结符都构造一个分析函数：

```c
// sdt.h
void           sem_Program       (struct node *root); 
void           sem_ExtDefList    (struct node *root); 
void           sem_ExtDef        (struct node *root);
struct Field * sem_ExtDecList    (struct node *root, struct Type *inh_val); // inh_val传递继承属性
// ...
```

#### 2.4 符号表管理

在递归向下分析的过程中，如`ExtDef`, `Dec`等非终结符会构造出完整的类型信息`S`，并将其添加到符号表中。

符号表层次结构为"表头--表节点--符号--S值与[E]值"，分四个层次向下；

由于需要完成stage2中的要求2.2，“**支持变量在多层作用域定义**”，所以符号表采用的是**Imperative Style**的形式，每一个符号节点使用"**十字链表+open hashing散列表**" 的形式来组织，见代码注释：

```c
// sym-table.h
struct header *symbol_table; // 符号表头部数组(动态)，每一个头部指向一个节点组成的链表slot，slot用来规避哈希冲突（即链地址法）
struct header *scope_stack;	 // 作用域头部栈(动态)，不同头部指向不同的"作用域符号节点链表",使用变量stk_top来指向栈顶

// 符号
struct symbol {
    struct Field *sym_field; //  S  in the type system.
    struct Exptype *sym_val; // [E] in the type system.
};

// 符号表节点
struct symbol_node {
    int hval,sval,lineno;			// 节点哈希值,节点所处作用域,符号出现行数
    struct symbol_node *slot_next;  // 指向同一个slot的下一个节点
    struct symbol_node *scope_next; // 指向同一个scope的下一个节点
    struct symbol 		sym;        // 指向符号 
};

// 符号表/作用域栈 的数组元素，代表slot头或scope头
struct header {
    union {
        struct symbol_node *slot_ptr;   
        struct symbol_node *scope_ptr;
    };
};
```

> **Imperative Style 维护风格**：它不会申请多个符号表，而是自始至终在单个符号表上进行动态维护。假设编译器在处理到当前函数f时符号表里有a、b、c这三个变量的定义。当编译器发现函数中出现了一个被“{”和“}”包含的语句块，而在这个语句块中又有新的变量定义时，它会将该变量插入f的符号表里。当语句块中出现任何表达式使用某个变量时，编译器就查找f的符号表。如果查找失败，则报告一个变量未定义的错误；如果查表成功，则返回查到的变量定义；如果出现了变量既在外层又在内层被定义的情况，则要求符号表返回最近的那个定义。每当编译器离开某个语句块时，会将这个语句块中定义的变量全部从表中删除。

> **基于十字链表和open hashing散列表**：这种设计的初衷很简单，除了散列表本身为了解决冲突问题所引入的slot链表之外，它从另一维度也引入scope链表将符号表中属于同一层作用域的所有变量都串起来。

以下是`sym-table.c`中函数对Imperative Style的函数实现：

- `add_symbol`: 每次向散列表中插入元素时，总是将新插入的元素放到该slot下挂的链表以及该层所对应的链表的表头。
- `search_symbol`: 每次查表时如果定位到某个slot，则按顺序遍历这个slot下挂的链表并返回这个slot中符合条件的第一个变量。如此一来便可以保证：如果出现了变量既在外层又在内层被定义的情况，符号表能够返回相对该变量最内层的那个定义。
- `enter_scope`: 每次进入一个语句块，需要为这一层语句块新建一个scope链表用来串联该层中新定义的变量；
- `exit_scope`: 每次离开一个语句块，则需要顺着代表该层语句块的scope链表将所有本层定义变量全部删除。

#### 2.5 错误检查

stage2中进行的错误检查主要为静态类型检查。

- 变量定义检查：由`sym-table.c`来完成检查符号未定义、重定义等错误检查工作；

- 表达式类型检查：通过对比两个**Tenum**值以及指向对应类型信息尾部**T**的ptr指针来实现，函数实现见`sdt.c:compare_exptype`；
- 变量访问检查：查看相应的**Tenum**值以及指向对应类型信息尾部**T**的ptr指针是否符合条件。

每一段错误检查代码由注释包围，例子如下：

```c
struct Exptype * sem_Exp(struct node *root) {
// ...
// Error 10: Exp should be array type.
if(e1->kind != ARRAY) {
    err_occur_sem(10, root->lineno, "Not an array", "^^");
    free(e1); e1 = NULL;
    return NULL;
} assert(e1->kind == ARRAY);
// Error 10: end.
//...
```

#### 2.6 注意事项

> 对于结构体类型等价的判定，每个匿名的结构体类型我们认为均具有一个独有的隐藏名字，以此进行名等价判定。

匿名struct创建时，根据文法，其OptTag为null，这里使用全局变量`anony_num`负责记录匿名struct数量，在函数 `semantic.c:sem_OptTag()` 中实现如下：

```C
struct Field * sem_OptTag         (struct node *root) {
	...
    /* OptTag -> null    {create a anonymous struct name.}	*/ 
    if(root == NULL) {
        fname = (char *)malloc(10);
        sprintf(fname, "anony_%d", anony_num++);
    } 
	...
}
```

------

### stage3

实现中间代码生成。过程包括，构造中间代码表示，自行完善IR生成SDT。总体上采用笨办法：遍历在stage1中生成好的语法树。

#### 3.1 中间代码表示：线性IR实现

首先是单条中间代码的数据结构，使用类似三地址代码的格式，一个中间代码struct对象（IC, InterCode）由 `enum OperatorKind` **代表的操作符** 和 由 `union u` **存储的操作数对象** 组成。中间代码对象由一系列函数 `ir-gen.c:Construct_XXXCode()` 构造。

```c
struct InterCode {
    enum OperatorKind {  
            OP_ADD,   OP_SUB,   OP_MUL,     OP_DIV,
			...
        } kind;
    union {
        struct Operand *dec_func;
        struct {struct Operand *result, *op1, *op2; } binop;
        ...
    } u;
};
```

每个操作数都是一个struct对象，有自己的 `enum kind` 和 由`union u`存储的数据，并且在需要进行输出时，会调用`StrOperand()`对`output_str`进行填充并作为该operand输出字符串。操作数对象由一系列函数 `ir-gen.c:MakeXXXOperand()` 构造。

```c
struct Operand {
    enum {  VALUE_VAR, ... } kind;
    union {
        char   *var_name;
		...
    } u;
    char *output_str;
};
```

为了方便调试，在实现中使用了中间代码表来存储创建好的中间代码对象，而不是一创建就立刻输出。

中间代码表（IC Table）负责记录当前IC数量，使用双向链表形式存储IC，并且拥有两个pointer：`head`和`tail`，每次加入IC时会从tail加入，而需要进行遍历时从head遍历。

```c
struct InterCodeNode { 
    struct InterCode        *code; 
    struct InterCodeNode    *prev, *next; 
};
struct InterCodeTable {
    int ic_num;
    struct InterCodeNode *head, *tail;
} intercode_table;
```

每个中间代码对象创建后，都需要使用函数 `ir-gen.c:InsertInterCode()` 加入到中间代码表 `intercode_table` 中。**注意，在实现中并不会将中间代码连成代码块后再加入到代码表中，而是在每条IC创建好后，直接加入到代码表。所以在SDT中，要注意这一行为。**

#### 3.2 中间代码输出

完成中间代码生成后，需要使用 `PrintInterCodeTable()` 遍历 IC Table 进行输出，`PrintInterCodeTable`会调用 `PrintInterCode()`，一个根据`InterCode.kind`进行输出的函数。

```c
void PrintInterCode(struct InterCode *ic) {
    if(ic->kind == DEC_FUNC) {
        fprintf(OUTPUT_FILENO, "FUNCTION %s :\n",   
                StrOperand(ic->u.dec_func));
        return;
    }
	...
}
```

#### 3.3 引用调用

> 若数组或结构体作为参数传递，需谨记数组和结构体都要求采用**引用调用（Call by Reference）**的参数传递方式，而非普通变量的**值调用（Call by Value）**。

这个定义意味着什么？我们为了简单，先使用一维数组来讲解。

**对于一个ID表示的变量，即使它在定义时是一个数组类型变量，但它会因为其定义的位置（函数参数PARAM or 局部变量DEC）而表现出不同的行为。**

如果该变量`a`为一个PARAM数组，那么 `a` 本身就应该是一个地址值，在取基址时使用时**无需**在变量ID前添加取址符 &。

如果该变量`a`为一个DEC数组，那么 `a` 本身是一个ID，在取基址时**需要**在变量ID前添加取址符 &。

具体不同行为如下：

- **传参类型中间代码**：函数参数为数组类型，那么对于准备传入的数组变量`a`，若为PARAM数组，生成IC `ARG a`；若为DEC数组，生成IC `ARG &a`。
- **数组取址访问**：在**取址**这一行为中，对于数组变量`a`，若为PARAM数组，生成IC `t = a + offset`；若为DEC数组，生成IC `t = &a + offset`。
- **数组域赋值**：（域赋值具体实现3.5讲述）同理假设 PARAM数组`a`,`b`，DEC数组`c`,`d`，比较一下四者：`a = &c` `a = b` `&c = &d` `&c = b`

对于高维数组和结构体变量同理。（由于实验三中保证函数数组类型参数为一维数组，所以可以省去一些代码实现）

#### 3.4 域赋值

对于两个**数组类型**变量二元赋值操作，如`c = a `或是`d[3] = b[5][7]`该怎么办？

在实验二中我们注意到有以下定义：

> 一是关于数组类型的等价机制，同C语言一样，只要数组的基类型和维数相同我们即认为类型是匹配的，例如`int a[10][2]`和`int b[5][3]`属于同一类型；
>
> 二是我们允许类型等价的结构体变量之间的直接赋值（见后面的测试样例），这时的语义是，对应的域相应赋值（**数组域也如此，按相对地址赋值直至所有数组元素赋值完毕或目标数组域已经填满**）；

所以根据第二点可以得到这类赋值操作的定义：数组域赋值。解决这个问题的方案就是为双方对应的数组地址一一创建赋值类型的中间代码：

```c
// ir-gen.c: translate_Exp()
for(int i = 0; i < stuff_size; i++) {
    struct Operand *ta1 = MakeTempOperand();
    struct Operand *ta2 = MakeTempOperand();
    InsertInterCode(Construct_BinOpCode(OP_ADD, ta1, left_addr,  MakeConOperand(i * 4)));
    InsertInterCode(Construct_BinOpCode(OP_ADD, ta2, right_addr, MakeConOperand(i * 4)));
    InsertInterCode(
        Construct_AssignCode(   MakeEvalVarOperand(ta1),
                                MakeEvalVarOperand(ta2))
    );
} 
```

#### 3.5 IR SDT

要注意的一个地方是引用调用和域赋值.

详细见文件 `cmm-IR-SDT.txt`。

------

## 编译方式

本地使用GNU Make进行编译，版本为 *GNU Make 4.2.1*。

- 直接输入`make`即可以得到分析器文件`./parser`；

- 输入`make clean`来清除目录下的生成文件；

- 设置DEBUG=YES来进入**debug模式**，这会在编译过程添加对应参数并进行条件编译，使`./parser`在分析过程中进行自动机输出；

  > 在进行这一步前必须先 `make clean`

- 设置SANITIZE=YES来检查运行过程中是否存在**memory leak**；

- 使用`make test`来进行**自动化测试**，测试输入为对`testcase/`中的`.cmm`文件；

- 使用`make gdb`配合自己编写的`init.gdb`文件，对添加了`-g3`编译的parser进行**调试**；注意，项目中写了一个小的python脚本配合gdb完成断点移动的功能，详细见`gdb_move.py`。

本地Makefile新增部分如下：

```makefile
CC = gcc
FLEX = flex
BISON = bison
CFLAGS = -std=c99 -g3
L_FLAG = 
B_FLAG = -d -v 
DEBUG :=
SANITIZE := 
# ↓————————只在本地有效————————↓
ifeq ($(DEBUG), YES)
    CFLAGS += -DD_MOD # 条件编译，用来在main.c中设置yydebug=1，
	L_FLAG += -d
	B_FLAG += -t
endif
ifeq ($(SANITIZE), YES)
	LEAK_CK := -fsanitize=address
endif
# ————————————————————————————

# 编译目标：src目录下的所有.c文件
# CFILES = ...同OJ Makefile

T_FILES = $(shell find ./testcase -name "*.cmm")
test: $(T_FILES)
	@for file in $^; do \
        echo ">> Parsing $$file ----------------------------------👇"; \
        ./parser "$$file"; \
		echo ""; \
	done
gdb:
	@gdb -x init.gdb
clean:
# ...同OJ Makefile
```

## 文档链接

- **Flex**： [Lexical Analysis With Flex, for Flex 2.6.3: Top (virginia.edu)](https://www.cs.virginia.edu/~cr4bd/flex-manual/index.html#SEC_Contents)
- **Bison**：[Top (Bison 3.8.1) (gnu.org)](https://www.gnu.org/software/bison/manual/html_node/)
- **GDB with Python**：[Extending GDB using Python](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Python.html#Python)

stage1过程遇到的一些问题以及参考的一些文章：

- [c++ - When is %destructor invoked in BISON? - Stack Overflow](https://stackoverflow.com/questions/6401286/when-is-destructor-invoked-in-bison)
- [c - How to free memory in bison? - Stack Overflow](https://stackoverflow.com/questions/46729811/how-to-free-memory-in-bison)
- bison的自我内存管理（栈区）：[Memory Management (Bison 3.8.1) (gnu.org)](https://www.gnu.org/software/bison/manual/html_node/Memory-Management.html)

