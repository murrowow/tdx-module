#// Copyright (C) 2023 Intel Corporation                                          
#//                                                                               
#// Permission is hereby granted, free of charge, to any person obtaining a copy  
#// of this software and associated documentation files (the "Software"),         
#// to deal in the Software without restriction, including without limitation     
#// the rights to use, copy, modify, merge, publish, distribute, sublicense,      
#// and/or sell copies of the Software, and to permit persons to whom             
#// the Software is furnished to do so, subject to the following conditions:      
#//                                                                               
#// The above copyright notice and this permission notice shall be included       
#// in all copies or substantial portions of the Software.                        
#//                                                                               
#// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS       
#// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   
#// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL      
#// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES             
#// OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,      
#// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE            
#// OR OTHER DEALINGS IN THE SOFTWARE.                                            
#//                                                                               
#// SPDX-License-Identifier: MIT

"""
 * @file pad_binary.py
 * @brief A tool to pad TDX module remaining bytes till page boundary with HLT opcodes.
"""

import sys
import os

PAGE_SIZE = 0x1000
HLT_OPCODE = 0xF4

def main():
  binary = sys.argv[1]
  statinfo = os.stat(binary)
  padding = PAGE_SIZE - (statinfo.st_size % PAGE_SIZE)
  f = open(binary,"ab")
  for i in range(padding):
    f.write(bytes([HLT_OPCODE]))
  f.close()

if __name__=='__main__':
    main()
