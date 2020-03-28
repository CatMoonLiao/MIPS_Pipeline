# MIPS_Pipeline
using C++ to implement MIPS pipeline

by this program, you can look in how pipeline work in MIPS

<p><a href="https://commons.wikimedia.org/wiki/File:MIPS_Architecture_(Pipelined).svg#/media/File:MIPS_Architecture_(Pipelined).svg"><img src="https://upload.wikimedia.org/wikipedia/commons/thumb/e/ea/MIPS_Architecture_%28Pipelined%29.svg/1200px-MIPS_Architecture_%28Pipelined%29.svg.png" alt="MIPS Architecture (Pipelined).svg"></a><br>由 <a href="//commons.wikimedia.org/wiki/User:Inductiveload" title="User:Inductiveload">Inductiveload</a> - <span class="int-own-work" lang="zh-tw">自己的作品</span>, 公有領域, <a href="https://commons.wikimedia.org/w/index.php?curid=5769084">連結</a></p>

What's in input file?
---
MIPS instructions in binary,

no any space and "\n",which need to seperate by self(32bit/instruction) 

if need to change input file, modify switch in line 62


What's in output file?
---
if need to change output file, modify function in line 370

format:

CC #clock cycle:

Registers:

(value in registers)

...


Data Memory:

(value in memory)

...

IF/ID:

(temp content in IF/ID register)

ID/EX:

(content in ID/EX register)

EX/MEM:

(content in EX/MEM register)

MEM/WB:

(content in MEM/WB register)

==================
next clock cycle
