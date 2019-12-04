Useful reference commands:

`./t1.out --csv -t --minBufSize=4096 --maxBufSize=8192 --minBlockSize=1024 -r 1 -w 1`

`./t1.out --disk --create --elevator --random --save=Disk.dat --cylinders=32 --surfaces=4 --sectorsPerTrack=32 --blockSize=4096 --RPM=7200 --overheadTime=100 --transferTime=690 --seekTime=10000 --sectorInterleaving=3 --filePath=Samples/sample.txt --seed=100 `



Creates a new disk and save it. Set the op to write and all the disk parameters. Also set the path for a file to be read to disk (written into it). Basically this will copy the file into the disk

./t1.out --disk --elevator --create --write --save=Disk.dat --cylinders=32 --surfaces=4 --sectorsPerTrack=32 --blockSize=4096 --RPM=7200 --overheadTime=100 --transferTime=690 --seekTime=10000 --sectorInterleaving=1 --filePath=Samples/sample.txt

We need to specify the size since the disk doesn't track it and we don't have a file system. This command will read the file. It's important to keep the same sectorInterleaving, specify a new output (filePath) and use the same mode (sequential/read). This is not checked by the disk since it would be the responsability of a file system which is not implemented

./t1.out --disk --load=Disk.dat --elevator --read --sectorInterleaving=1 --filePath=Samples/sample2.txt --size=16384