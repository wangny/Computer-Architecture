I image必須命名為I_image.txt

每個文件的第一行是PC的初始值(請使用16進位，512 = 0x00000200)
第二行以下則是instruction，格式如下

[label](空白)[instruction]

例如： sumToN slti $t0, $a0, 2
      [label] [  instruction  ]

label是用於branch或jump，若該行不需要label請以"."代替


例如：    .   slti $t0, $a0, 2
      [label] [  instruction  ]

instrution之間可以使用" "",""("")"等符號來區隔

例如：   .    sw   $ra, 4($sp)
      [label] [  instruction  ]

所有的reg皆必須使用數字代號，例如：$sp = $29,$ra = $31


只有 PC(第一行)和 LUI、ANDI、ORI、NORI的immediate必須使用16進位，其餘數字請使用10進位 

每一行的結尾不要有多餘的空白，也不要有註解
文件最後也不要有多餘的空行

附件I_image.txt是一個正確的格式，他對應的是recur.s，請大家參考

===========================================================

D image必須命名為D_image.txt
第一行為 $sp 的初始值，請使用16進位
第二行以下為memory中的值，請使用16進位

例如：0x87654321

附件D_image.txt是一個正確的格式，他對應的是recur的dimage.bin，請大家參考

===========================================================
BUG：

4/4 
	1. 修改 StrToHex 16進位英文字母限用大寫
	2. StrToHex中int sum -> u32 sum

===========================================================
如果有任何BUG請跟我說，我盡量改
大家projecy加油@@/