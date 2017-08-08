# BGSA
BGSA is a bit-parallel global sequence alignment toolkit for multi-core and many-core architectures. 

# Usage
* Step 1: Use the _generator_ program to  generate the kernel source with your specified score and architecture. And you need the Java runtime 1.7 or higher.
```
cd Generator
java -jar generator.jar -M 2 -I -3 -G -5 -a sse
```

* Step 2: Move the generated file to the BGSA source folder accroding to your selected architecture, and then run `make` command.
```
mv generated/align_core.c ../BGSA_SSE/
cd ../BGSA_SSE/
make
```

* Step 3: Run a test. 
```
./aligner -q sample-data/query.txt -d sample-data/subject.txt -f data/result.txt
```

* Step 4: Conver the result file to readable format.
```
./convert -r data/result.txt
```

# Supprot or Contact

If you have any questions, please contact: Weiguo,Liu ( weiguo.liu@sdu.edu.cn).

