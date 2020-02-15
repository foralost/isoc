#!/usr/bin/python3

import os, sys, random

gvc_dir = "dir"
gvc_file = "fl"
gv_curr_dir = 0
gv_curr_file = 0
gv_file_max = 300

argc = len(sys.argv)
random.seed()

def gen_dirs(szStartPath : str, lvl : int):
    global gv_curr_dir, gv_file_max, gv_curr_file
    if lvl == gv_dir_max:
        return
    i = 0
    j = 0
    lv_dir_counter = 0
    while i < gv_dir_count:
        if random.randrange(1024) % 2 == 0:
            gv_curr_dir += 1
            szNewPath = szStartPath + gvc_dir + str(gv_curr_dir)
            os.makedirs(szNewPath)
            while j < gv_file_max:
                os.system("echo \"im file number " + str(gv_curr_file) + " \" >> " +szStartPath+"file" + str(gv_curr_file)+".txt")
                gv_curr_file += 1
                j+=1

            gen_dirs( szNewPath + '/', lvl + 1)
        i+=1


gv_dir_max = int(sys.argv[1])
gv_dir_count = int(sys.argv[1])

os.system("rm -rf isodir")
os.system("rm output.iso")

os.mkdir("isodir")
gen_dirs("isodir/", 0)

os.system("mkisofs -iso-level 1 -o output.iso isodir/") 
