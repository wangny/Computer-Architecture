I image�����R�W��I_image.txt

�C�Ӥ�󪺲Ĥ@��OPC����l��(�Шϥ�16�i��A512 = 0x00000200)
�ĤG��H�U�h�Oinstruction�A�榡�p�U

[label](�ť�)[instruction]

�Ҧp�G sumToN slti $t0, $a0, 2
      [label] [  instruction  ]

label�O�Ω�branch��jump�A�Y�Ӧ椣�ݭnlabel�ХH"."�N��


�Ҧp�G    .   slti $t0, $a0, 2
      [label] [  instruction  ]

instrution�����i�H�ϥ�" "",""("")"���Ÿ��ӰϹj

�Ҧp�G   .    sw   $ra, 4($sp)
      [label] [  instruction  ]

�Ҧ���reg�ҥ����ϥμƦr�N���A�Ҧp�G$sp = $29,$ra = $31


�u�� PC(�Ĥ@��)�M LUI�BANDI�BORI�BNORI��immediate�����ϥ�16�i��A��l�Ʀr�Шϥ�10�i�� 

�C�@�檺�������n���h�l���ťաA�]���n������
���̫�]���n���h�l���Ŧ�

����I_image.txt�O�@�ӥ��T���榡�A�L�������Orecur.s�A�Фj�a�Ѧ�

===========================================================

D image�����R�W��D_image.txt
�Ĥ@�欰 $sp ����l�ȡA�Шϥ�16�i��
�ĤG��H�U��memory�����ȡA�Шϥ�16�i��

�Ҧp�G0x87654321

����D_image.txt�O�@�ӥ��T���榡�A�L�������Orecur��dimage.bin�A�Фj�a�Ѧ�

===========================================================
BUG�G

4/4 
	1. �ק� StrToHex 16�i��^��r�����Τj�g
	2. StrToHex��int sum -> u32 sum

===========================================================
�p�G������BUG�и�ڻ��A�ںɶq��
�j�aprojecy�[�o@@/