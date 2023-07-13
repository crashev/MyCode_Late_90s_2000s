./drawtopn -f foo0.png -l 2 -n 0 
./drawtopn -f foo1.png -l 2 -n 1 -d  1.1.1.1.1024-2.2.2.2.1024 100000 
./drawtopn -f foo2.png -l 2 -n 2 -d  1.1.1.1.1024-2.2.2.2.1024 100000 2.1.1.1.1025-2.2.2.2.1024 200000 
./drawtopn -f foo6.png -l 2 -n 6 -d  1.1.1.1.1024-2.2.2.2.1024 100000 2.1.1.1.1025-2.2.2.2.1024 200000 3.1.1.1.1026-2.2.2.2.1024 300000 4.1.1.1.1027-2.2.2.2.1024 400000 5.1.1.1.1028-2.2.2.2.1024 500000 6.1.1.1.1028-2.2.2.2.1024 500000 
# try a few lies, these should fail with usage errors
# 1 data claimed, none offered
./drawtopn -f foobad1.png -l 2 -n 1 -d  
# 2 data claimed, one offered
./drawtopn -f foobad2.png -l 2 -n 2 -d  1.1.1.1.1024-2.2.2.2.1024 
# 2 data claimed, three offered
./drawtopn -f foobad3.png -l 2 -n 2 -d  1.1.1.1.1024-2.2.2.2.1024 boop
