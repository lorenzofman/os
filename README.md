Useful reference commands:

`./t1.out --csv -t --minBufSize=4096 --maxBufSize=8192 --minBlockSize=1024 -r 1 -w 1`

`./t1.out --disk --create --elevator --random --save=Disk.dat --cylinders=32 --surfaces=4 --sectorsPerTrack=32 --blockSize=4096 --RPM=7200 --overheadTime=100 --transferTime=690 --seekTime=10000 --sectorInterleaving=3 --filePath=Samples/sample.txt --seed=100 `



`./t1.out -dce --random --save=Disk.dat --cylinders=32 --surfaces=4 --sectorsPerTrack=32 --blockSize=4096 --RPM=7200 --overheadTime=100 --transferTime=690 --seekTime=10000 --sectorInterleaving=3 --filePath=Samples/sample.txt --seed=100 `