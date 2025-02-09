#!/bin/bash
mkdir test
cp -r code ./test
cd ./test/code
cat 14.c
a=0
while [ $a -le 15 ]
do
	gcc -c $a.c
	let a=a+1
done
gcc *.o -o hello
cp hello ..
rm hello
cd ..
./hello 2>err.txt
mv err.txt ..
cd ..
chmod u+r+w-x err.txt
chmod g+r-w+x err.txt
chmod o+r-w+x err.txt
b=1
c=1
if [ $# -eq 1 ]
then
	b=$1
elif [ $# -eq 2 ]
then
	b=$1
	c=$2
fi
d=$(( b + c ))
sed -n ' '$d'p ' err.txt >&2
