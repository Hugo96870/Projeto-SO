#!/bin/bash

inputdir=$1
outputdir=$2
maxthreads=$3
#verificar nr args
if [ ! -d "/$inputdir" ] ;
then
    echo "Input directory is not a directory"
    exit 1

elif [ ! -d "/$outputdir" ] ;
then
    echo "Output directory is not a directory"
    exit 1

elif [ $maxthreads -le 0 ] ;
then
    echo "Invalid number of threads"
    exit 1
fi

cd $inputdir
for test in *.txt
do
    for i in $(seq 1 $maxthreads)
    do
        cd ..
        echo InputFile=$test NumThreads=$i
        ./tecnicofs "$inputdir/$test" "$outputdir"/"$test%.txt"-"$i".txt "$i" | tail -n 1   #print mau
        cd $inputdir
    done
done
