#!/bin/sh

NUM_PARAMS=3
if [ "$#" -lt ${NUM_PARAMS} ]; then
   echo "Illegal number of parameters: $# expected ${NUM_PARAMS}"
   echo "1 - the platform: {Linux, MaxOs, Centos}"
   echo "2 - the model to run on"
   echo "3 - the query input file to use"
   echo "4 - [optional] the parameter that goes straight to the tool, e.g. -c"
   exit 1
fi

FILTER='| grep -v "\-\-\-" | grep -v "Gram ctx hash" | grep -v "CPU" | grep -v "vmsize" | grep -v "Cultivating" | grep -v "Counting words in ARPA file" | grep -v " memory allocation strategy." | grep -v "Counting all words" | grep -v "#buckets divider"'
echo ${FILTER}

echo "c2wa"
eval "../dist/Release__${1}_/back-off-language-model-smt ${4} -m ${2} -q ${3} -t c2wa ${FILTER} > release.c2wa.out"
echo "c2dm"
eval "../dist/Release__${1}_/back-off-language-model-smt ${4} -m ${2} -q ${3} -t c2dm ${FILTER} > release.c2dm.out"
echo "w2ca"
eval "../dist/Release__${1}_/back-off-language-model-smt ${4} -m ${2} -q ${3} -t w2ca ${FILTER} > release.w2ca.out"
echo "w2ch"
eval "../dist/Release__${1}_/back-off-language-model-smt ${4} -m ${2} -q ${3} -t w2ch ${FILTER} > release.w2ch.out"
echo "c2dh"
eval "../dist/Release__${1}_/back-off-language-model-smt ${4} -m ${2} -q ${3} -t c2dh ${FILTER} > release.c2dh.out"
echo "g2dm"
eval "../dist/Release__${1}_/back-off-language-model-smt ${4} -m ${2} -q ${3} -t g2dm ${FILTER} > release.g2dm.out"
echo "h2dm"
eval "../dist/Release__${1}_/back-off-language-model-smt ${4} -m ${2} -q ${3} -t h2dm ${FILTER} > release.h2dm.out"

echo "----> c2wa vs. c2dm"
diff release.c2wa.out release.c2dm.out > diff.c2wa.c2dm.out
cat diff.c2wa.c2dm.out | wc -l

echo "----> c2wa vs. w2ca"
diff release.c2wa.out release.w2ca.out > diff.c2wa.w2ca.out
cat diff.c2wa.w2ca.out | wc -l

echo "----> c2wa vs. w2ch"
diff release.c2wa.out release.w2ch.out > diff.c2wa.w2ch.out
cat diff.c2wa.w2ch.out | wc -l

echo "----> c2wa vs. c2dh"
diff release.c2wa.out release.c2dh.out > diff.c2wa.c2dh.out
cat diff.c2wa.c2dh.out | wc -l

echo "----> c2wa vs. g2dm"
diff release.c2wa.out release.g2dm.out > diff.c2wa.g2dm.out
cat diff.c2wa.g2dm.out | wc -l

echo "----> c2wa vs. h2dm"
diff release.c2wa.out release.h2dm.out > diff.c2wa.h2dm.out
cat diff.c2wa.h2dm.out | wc -l

echo "------------------------------"

echo "----> c2dm vs. c2wa"
diff release.c2dm.out release.c2wa.out > diff.c2dm.c2wa.out
cat diff.c2dm.c2wa.out | wc -l

echo "----> c2dm vs. w2ca"
diff release.c2dm.out release.w2ca.out > diff.c2dm.w2ca.out
cat diff.c2dm.w2ca.out | wc -l

echo "----> c2dm vs. w2ch"
diff release.c2dm.out release.w2ch.out > diff.c2dm.w2ch.out
cat diff.c2dm.w2ch.out | wc -l

echo "----> c2dm vs. c2dh"
diff release.c2dm.out release.c2dh.out > diff.c2dm.c2dh.out
cat diff.c2dm.c2dh.out | wc -l

echo "----> c2dm vs. g2dm"
diff release.c2dm.out release.g2dm.out > diff.c2dm.g2dm.out
cat diff.c2dm.g2dm.out | wc -l

echo "----> c2dm vs. h2dm"
diff release.c2dm.out release.h2dm.out > diff.c2dm.h2dm.out
cat diff.c2dm.h2dm.out | wc -l
