import re 
import sys

src_path = sys.argv[1]
print( src_path )
regexp = re.compile("int [__|fat]?\n*")
src = open( src_path, 'r')
for currline in src:
    if regexp.match( currline ):
        print(currline)

