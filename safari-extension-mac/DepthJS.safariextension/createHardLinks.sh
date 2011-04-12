#!/bin/bash
echo "rm -rf extension-common"
rm -rf extension-common
echo "mkdir extension-common"
mkdir extension-common
find ../../extension-common -print0 | while read -d $'\0' file
do
  curDir=`dirname ${file:6}`
  mkdir -p $curDir
  ln $file ${file:6}
done

