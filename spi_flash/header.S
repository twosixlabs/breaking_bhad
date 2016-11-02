# Copyright (c) 2016, Joseph Tanen
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Prototypes.
# extern int meat(int argc, char *argv[]);

# I'm not using symbolic registers, e.g., $v0, here because they apparently
#   are not supported in this version of binutils.

.pushsection ".text"
.set noreorder
.ent _start
.globl _start
_start:
    # Don't touch $a0($4), $a1($5).
    # Remember that the sp must always be a multiple of 8.

    # Push gp, ra.
    sw $gp, 0($sp)
    sw $ra, -4($sp)
    sub $sp, $sp, 8
# Clever widget from u-boot. We're branching and linking to set ra then using ra
#   to get the global offset table address stored as an inline data word.
# From stackoverflow (http://stackoverflow.com/a/10981919):
# Branches (b) use a PC-relative displacement while jumps (j) use absolute addresses.
#   The distinction is important for position-independent code. Also, only jumps
#   can be used for indirect control transfer (jr, using a register value).
# We don't want to do absolute in this case, since we want to load relative to
#   the pc, since we put our data of interest somewhere relative to the pc.
# Doing jal caused other stuff to get assembled in that referenced gp, which is
#   bad here, since it's unset.
    bal 1f
    nop
.word _gp
1:  lw $gp, 0($ra)
    # The compiler is using $t9($25) in C-land to load $gp for the function.
    # We can get that automagically here if we just jal meat given something
    #   related to the current configuration, but instead I want to ensure that
    #   it's being done and do it explictly.
    la $25, meat
    # Reserve space for all 4 possible register args. Required by O32 ABI.
    sub $sp, $sp, 16
    jalr $25
    nop

    # Propagate return value. Don't change $v0($2).
    # Release argument reservation.
    add $sp, $sp, 16

    # Pop ra, gp.
    add $sp, $sp, 8
    lw $ra, -4($sp)
    lw $gp, 0($sp)
    jr $ra
    nop
.end _start
.popsection

